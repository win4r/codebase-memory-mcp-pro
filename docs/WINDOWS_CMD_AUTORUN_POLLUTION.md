# Windows: cmd.exe AutoRun 污染子进程 stdout

## 症状

`index_repository` 带 `persistence=true` 在 Windows 上返回 `artifact_present: false`，不生成 `.codebase-memory/graph.db.zst`。日志只输出一行：

```
level=info msg=incremental.dump rc=-1 elapsed_ms=7
```

`index_status` 返回的 git 字段（`git_dir`、`worktree_root`、`branch`、`head_sha` 等）全是字符串 `"Active code page: 65001"` —— 这是 cmd.exe 切换 UTF-8 代码页时往 stdout 打印的提示行。

`dump rc=-1` 的内部子错误（二进制内嵌字符串）有四种：
`dump: backup init failed` / `dump: backup step failed` / `dump: cannot open temp file` / `dump: rename failed`。本案例实际命中的是路径解析失败那一类——dump 拿到的 repo 根路径是 `"Active code page: 65001"` 字符串垃圾，写文件找不到目录。

## 根因

Windows 注册表 `HKLM\Software\Microsoft\Command Processor\AutoRun`（或 HKCU 同路径）设了 `chcp 65001`（常见手动加的 UTF-8 切换）。任何进程通过 `_popen()` spawn 子进程时，Windows 内部经由 cmd.exe 解释命令，cmd.exe 启动时先执行 AutoRun，输出 `"Active code page: 65001"` 进 stdout，污染后续真实命令的输出。

代码路径（本仓库）：

1. **`src/foundation/compat_fs.c:125-126`**
   ```c
   FILE *cbm_popen(const char *cmd, const char *mode) {
       return _popen(cmd, mode);   // Windows 上 _popen 经由 cmd.exe 解释
   }
   ```

2. **`src/git/git_context.c:78`** —— `git_capture()` 跑 `git -C "<path>" rev-parse --show-toplevel` 等：
   ```c
   int n = snprintf(cmd, sizeof(cmd), "git -C \"%s\" %s 2>%s", repo_path, git_args, null_dev);
   ...
   FILE *fp = cbm_popen(cmd, "r");
   ...
   if (!fgets(buf, sizeof(buf), fp)) { ... }   // 第一行读到的是 chcp 输出
   trim_newlines(buf);
   int rc = cbm_pclose(fp);
   if (rc != 0 || buf[0] == '\0') { return CBM_NOT_FOUND; }   // buf 非空，误判成功
   *out = git_strdup(buf);   // 把 "Active code page: 65001" 当 git 输出返回
   ```
   `git_capture` 是所有 git 字段获取的底层调用点（branch、head_sha、worktree root、git_dir 等都从这里拿值）。一旦这里 stdout 被污染，后续所有依赖 git 信息的代码（包括 dump 写文件时定位 repo 根）全部拿到垃圾路径。

3. **`src/pipeline/pass_githistory.c:242`** —— 同样的 popen 模式（popen fallback 路径，仅当未编译 libgit2 时启用）：
   ```c
   snprintf(cmd, sizeof(cmd),
            "git -C \"%s\" log --name-only --pretty=format:COMMIT:%%H:%%ct "
            "--since=\"1 year ago\" --max-count=10000 2>%s",
            repo_path, null_dev);
   FILE *fp = cbm_popen(cmd, "r");
   ```

4. **`src/pipeline/artifact.c:112`** 和 **`src/pipeline/artifact.c:259`** —— 同样 popen 模式，artifact 导出/导入路径也会受影响。

`pipeline.c::dump_and_persist_hashes()` 调用 `cbm_artifact_export()` 失败时**静默忽略**（只返回 rc=-1 到日志的 info 级别，不上抛 pipeline error），所以从外部看就是 `artifact_present: false` 但 `status: "indexed"`。

## 诊断

PowerShell 跑：

```powershell
Get-ItemProperty 'HKLM:\Software\Microsoft\Command Processor' AutoRun
# 若返回 "chcp 65001"（不带 >nul）即中招

cmd /c echo hello
# 输出含 "Active code page: 65001" 行 = 污染确认
```

## 修复方案（已实施）

### 方案 A + validator（已合并，根治）

**底层**：新增 `cbm_spawn_capture()` 到 `src/foundation/compat_fs.{h,c}`。Windows 走 `CreateProcessW` + 匿名 pipe，POSIX 走 `fork` + `pipe` + `execvp`。**子进程永远不经 cmd.exe / /bin/sh**——AutoRun 是 cmd.exe 的启动钩子，cmd 不启动则钩子不触发。同时彻底消除 shell 注入面（argv 数组传递，不再拼字符串）。

**防御层**：新增 `cbm_spawn_capture_validated()` + `cbm_line_validator_t` + 3 个公开 validator（`cbm_validator_sha40_hex` / `cbm_validator_git_path` / `cbm_validator_branch_name`）。每个调用方声明“我期望的输出长什么样”；输出逐行验证，**找不到匹配 → fail loud**（返回 `CBM_NOT_FOUND` + log warn 上报首行 + 总行数），绝不模糊提取。

为什么需要 validator：即使不经 cmd.exe，git 自身偶尔也会往 stdout 打印 hint（"It took X seconds to refresh index..."）、GCM 凭证提示、git hook 输出、未来代码回归。Validator 是与方案 A 正交的防御层。

**已改造的 4 个 bug 路径**：

| 文件 | 函数 | 原调用 | 新调用 |
|---|---|---|---|
| `src/git/git_context.c` | `git_capture()` | `cbm_popen("git -C …")` | `cbm_spawn_capture_validated` + 4 路 validator 派发（`rev-parse --verify`→sha40, `rev-parse *`→git_path, `symbolic-ref`→branch, `merge-base`→sha40） |
| `src/pipeline/artifact.c` | `git_head_hash()` | `cbm_popen("git -C '...' rev-parse HEAD")` ⚠️**附带修复**：原代码用单引号在 Windows cmd.exe 上根本不生效 | `cbm_spawn_capture_validated` + `cbm_validator_sha40_hex` |
| `src/pipeline/pass_githistory.c` | `parse_git_log()` | `cbm_popen("git -C … log …")` | `cbm_spawn_capture` + `memchr` 逐行扫描（git log 输出自描述，不需 validator） |
| `src/foundation/compat_fs.c` | （新函数本身） | — | Windows `CreateProcessW` / POSIX `fork+execvp` + 共享 `cbm_find_validated_line` + `cbm_spawn_capture_validated` |

**调用方 API 示例**：

```c
/* rev-parse --show-toplevel → 绝对路径 */
const char *argv[] = {"git", "-C", repo_path, "--no-pager",
                     "rev-parse", "--show-toplevel", NULL};
char *out = NULL;
int rc = cbm_spawn_capture_validated("git", argv, &out,
                                    cbm_validator_git_path, NULL);
/* rc == 0 && out != NULL → 绝对路径, 安全使用 */
/* rc != 0 → 真的失败 OR 输出被污染到没一行像路径 — 都按 "取不到" 处理 */
```

### 反模式（不要这样做）

- ❌ **黑名单过滤 `"Active code page:"`**——只挡 chcp 一种，doskey / prompt / 自定义 .bat / Cmder 初始化 / multi-line AutoRun 都挡不住。
- ❌ **模糊匹配 "最像绝对路径的行"**——掩盖真 bug，下次污染发生时拿不到错误信号。
- ❌ **要求用户改注册表 `@chcp 65001>nul`**——客服成本永久存在，且其他 AutoRun 配置仍会踩坑。
- ❌ **按 “父进程是 PowerShell / cmd” 分支**——问题是 `_popen`，不是 shell；PowerShell 用户同样中招，方案 A 一次性解决。

### 遗留 popen 点（follow-up，不在 bug 路径）

全仓库 `cbm_popen` 调用共 15 处，bug 路径 4 处已全部改造。剩余调用分两类：

| 文件:行 | 用途 | 解析 stdout? | 状态 |
|---|---|---|---|
| `src/pipeline/artifact.c:264` | `git config merge.ours.driver true` | ❌ fire-and-forget | 保留（无污染面） |
| `src/mcp/mcp.c:3986, 4252, 5109` | git 调用 | ✅ | TODO follow-up |
| `src/mcp/mcp.c:5138` | curl GitHub API | ✅ | TODO follow-up |
| `src/watcher/watcher.c:105, 120, 146, 179, 201` | git polling | ✅ | TODO follow-up |
| `src/cli/cli.c:2712, 2772, 3873` | pgrep / 进程查询 | 部分 | TODO follow-up |

Follow-up 原则：哪个 popen 解析 stdout（拿第一行作为结构化数据），就改成 `cbm_spawn_capture_validated` + 适配的 validator。fire-and-forget 调用（如 `git config`）不需要改。

### 环境侧 workaround（仅诊断用，fork 不再需要）

如果 fork 二进制已部署但想确认现场 AutoRun 配置，可运行：

```powershell
Get-ItemProperty 'HKLM:\Software\Microsoft\Command Processor' AutoRun
Get-ItemProperty 'HKCU:\Software\Microsoft\Command Processor' AutoRun
# 值不含 >nul = 该用户过去会被旧版 fork 踩坑
```

**fork 现在不需要用户改注册表**——保留此段仅供现场诊断。

## 验证步骤

### 单元测试

```bash
make -f Makefile.cbm test-foundation   # 跑 test_spawn_capture.c 13 个测试
# 期望: 8 个跨平台纯函数测试 + 5 个 POSIX-only 集成测试 全绿
# Windows CI: 只跑 8 个纯函数测试（SKIP_PLATFORM 不适用，用 #ifndef _WIN32 编译门）
```

覆盖场景：
- `cbm_find_validated_line` 多行污染 / 无匹配 / CRLF / 长行截断 / NULL 安全
- `cbm_spawn_capture` 基本 echo / 退出码传递 / 找不到 exe
- `cbm_spawn_capture_validated` 跳过污染行找到匹配 / 无匹配 fail loud

### 集成测试（现场验收）

用最恶心的 AutoRun 验证（不只 chcp）：

```powershell
# 备份
reg export "HKCU\Software\Microsoft\Command Processor" "$env:TEMP\ar-backup.reg" /y
# 设个明显会污染的 AutoRun（多行 + doskey + 多 echo）
Set-ItemProperty 'HKCU:\Software\Microsoft\Command Processor' -Name AutoRun `
    -Value '@echo AUTORUN_LINE_1 & chcp 65001 & echo AUTORUN_LINE_2 & doskey macro=echo hi'

# 清状态 + 跑索引
Remove-Item "$env:USERPROFILE\.cache\codebase-memory-mcp" -Recurse -Force -EA SilentlyContinue
Remove-Item "<repo>\.codebase-memory" -Recurse -Force -EA SilentlyContinue
& "<path>\codebase-memory-mcp.exe" cli index_repository '{"repo_path":"<repo>","persistence":true}'

# 期望:
#   - index_status 的 git_dir / branch / head_sha 都是真实 git 输出（不是 AUTORUN_LINE_*）
#   - artifact_present: true
#   - 日志无 "spawn.validation_failed"

# 还原
reg import "$env:TEMP\ar-backup.reg"
```

如果这个测试通过，任何现实里的 AutoRun 配置（chcp / doskey / prompt / Cmder / 自定义 .bat）都过得了。

## 已知现场案例

- 2026-06-24：Be90nia fork 二进制在 Windows 11 上 `dump rc=-1`，根因即 AutoRun = `chcp 65001`（不带 `>nul`）。临时用方案 D（改注册表）让 artifact 正常生成 6.18 MB。本仓库用户应优先实施方案 A/B，让 fork 容错，不再要求用户改注册表。
