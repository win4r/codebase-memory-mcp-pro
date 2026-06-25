/*
 * compat_fs.c — Portable file system operations.
 *
 * POSIX: direct wrappers around opendir/readdir/closedir, popen/pclose, mkdir, unlink.
 * Windows: FindFirstFile/FindNextFile, _popen/_pclose, _mkdir, _unlink.
 */
#include "foundation/constants.h"
#include "foundation/compat_fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

/* ── Windows implementation ────────────────────────────────── */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <direct.h> /* _wmkdir */
#include <io.h>     /* _wunlink */
#include "foundation/win_utf8.h"

struct cbm_dir {
    HANDLE find_handle;
    WIN32_FIND_DATAW find_data;
    wchar_t wide_pattern[CBM_PATH_MAX];
    cbm_dirent_t entry;
    bool first;
    bool done;
};

cbm_dir_t *cbm_opendir(const char *path) {
    if (!path) {
        return NULL;
    }
    wchar_t *wpath = cbm_utf8_to_wide(path);
    if (!wpath) {
        return NULL;
    }

    size_t wlen = wcslen(wpath);
    if (wlen == 0 || wlen + 2 >= CBM_PATH_MAX) {
        free(wpath);
        return NULL;
    }

    cbm_dir_t *d = (cbm_dir_t *)calloc(CBM_ALLOC_ONE, sizeof(cbm_dir_t));
    if (!d) {
        free(wpath);
        return NULL;
    }

    wmemcpy(d->wide_pattern, wpath, wlen + 1);
    wchar_t *p = d->wide_pattern + wlen - SKIP_ONE;
    if (*p != L'\\' && *p != L'/') {
        ++p;
        *p++ = L'\\';
    } else {
        ++p;
    }
    *p++ = L'*';
    *p = L'\0';
    free(wpath);

    d->find_handle = FindFirstFileW(d->wide_pattern, &d->find_data);
    if (d->find_handle == INVALID_HANDLE_VALUE) {
        free(d);
        return NULL;
    }
    d->first = true;
    d->done = false;
    return d;
}

cbm_dirent_t *cbm_readdir(cbm_dir_t *d) {
    if (!d || d->done) {
        return NULL;
    }
    if (!d->first) {
        if (!FindNextFileW(d->find_handle, &d->find_data)) {
            d->done = true;
            return NULL;
        }
    }
    d->first = false;

    while (d->find_data.cFileName[0] == L'.' &&
           (d->find_data.cFileName[1] == L'\0' ||
            (d->find_data.cFileName[1] == L'.' && d->find_data.cFileName[2] == L'\0'))) {
        if (!FindNextFileW(d->find_handle, &d->find_data)) {
            d->done = true;
            return NULL;
        }
    }

    char *u8 = cbm_wide_to_utf8(d->find_data.cFileName);
    if (!u8) {
        d->done = true;
        return NULL;
    }
    size_t nlen = strlen(u8);
    if (nlen >= CBM_DIRENT_NAME_MAX) {
        nlen = CBM_DIRENT_NAME_MAX - SKIP_ONE;
    }
    memcpy(d->entry.name, u8, nlen);
    d->entry.name[nlen] = '\0';
    free(u8);
    d->entry.is_dir = (d->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    d->entry.d_type = 0;
    return &d->entry;
}

void cbm_closedir(cbm_dir_t *d) {
    if (d) {
        if (d->find_handle != INVALID_HANDLE_VALUE) {
            FindClose(d->find_handle);
        }
        free(d);
    }
}

FILE *cbm_popen(const char *cmd, const char *mode) {
    return _popen(cmd, mode);
}

int cbm_pclose(FILE *f) {
    return _pclose(f);
}

bool cbm_mkdir_p(const char *path, int mode) {
    (void)mode;
    wchar_t *wpath = cbm_utf8_to_wide(path);
    if (!wpath) {
        return false;
    }

    if (_wmkdir(wpath) == 0) {
        free(wpath);
        return true;
    }
    size_t wlen = wcslen(wpath);
    wchar_t *tmp = (wchar_t *)malloc((wlen + 1) * sizeof(wchar_t));
    if (!tmp) {
        free(wpath);
        return false;
    }
    wmemcpy(tmp, wpath, wlen + 1);
    for (wchar_t *p = tmp + SKIP_ONE; *p; p++) {
        if (*p == L'/' || *p == L'\\') {
            *p = L'\0';
            _wmkdir(tmp);
            *p = L'\\';
        }
    }
    bool ok = _wmkdir(tmp) == 0 || GetLastError() == ERROR_ALREADY_EXISTS;
    free(tmp);
    free(wpath);
    return ok;
}

int cbm_unlink(const char *path) {
    wchar_t *wpath = cbm_utf8_to_wide(path);
    if (!wpath) {
        return CBM_NOT_FOUND;
    }
    int ret = _wunlink(wpath);
    free(wpath);
    return ret;
}

int cbm_rmdir(const char *path) {
    wchar_t *wpath = cbm_utf8_to_wide(path);
    if (!wpath) {
        return CBM_NOT_FOUND;
    }
    int ret = _wrmdir(wpath);
    free(wpath);
    return ret;
}

int cbm_exec_no_shell(const char *const *argv) {
    if (!argv || !argv[0]) {
        return CBM_NOT_FOUND;
    }
    return (int)_spawnvp(_P_WAIT, argv[0], argv);
}

int cbm_spawn_capture(const char *exe, const char *const *argv,
                      char **out, size_t *out_len) {
    if (out) *out = NULL;
    if (out_len) *out_len = 0;
    if (!exe || !argv || !argv[0] || !out) {
        return CBM_NOT_FOUND;
    }

    /* CreatePipe: stdout pipe with inheritable write end. */
    HANDLE pipe_r, pipe_w;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    if (!CreatePipe(&pipe_r, &pipe_w, &sa, 0)) {
        return CBM_NOT_FOUND;
    }
    if (!SetHandleInformation(pipe_r, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(pipe_r);
        CloseHandle(pipe_w);
        return CBM_NOT_FOUND;
    }

    /* lpApplicationName = explicit exe path (no shell interpretation, no AutoRun).
     * lpCommandLine = argv joined with strict double-quote escaping so the
     * target program parses argv correctly without cmd.exe ever seeing it. */
    wchar_t wexe[CBM_PATH_MAX];
    if (MultiByteToWideChar(CP_UTF8, 0, exe, -1, wexe, CBM_PATH_MAX) <= 0) {
        CloseHandle(pipe_r);
        CloseHandle(pipe_w);
        return CBM_NOT_FOUND;
    }

    wchar_t cmdline[CBM_SZ_8K];
    int pos = 0;
    for (int i = 0; argv[i]; i++) {
        if (pos >= CBM_SZ_8K - 1) goto fail;
        if (i > 0) cmdline[pos++] = L' ';
        if (pos >= CBM_SZ_8K - 1) goto fail;
        cmdline[pos++] = L'"';
        for (const char *p = argv[i]; *p; p++) {
            wchar_t wc_buf[2];
            int n = MultiByteToWideChar(CP_UTF8, 0, p, 1, wc_buf, 2);
            if (n <= 0) continue;
            wchar_t wc = wc_buf[0];
            if (wc == L'"' || wc == L'\\') {
                if (pos >= CBM_SZ_8K - 2) goto fail;
                cmdline[pos++] = L'\\';
            }
            if (pos >= CBM_SZ_8K - 1) goto fail;
            cmdline[pos++] = wc;
        }
        if (pos >= CBM_SZ_8K - 1) goto fail;
        cmdline[pos++] = L'"';
    }
    cmdline[pos] = L'\0';

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = pipe_w;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessW(wexe, cmdline, NULL, NULL, TRUE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pipe_w);
    if (!ok) {
        CloseHandle(pipe_r);
        return CBM_NOT_FOUND;
    }

    char buf[CBM_SZ_4K];
    size_t total = 0;
    char *result = NULL;
    for (;;) {
        DWORD n = 0;
        BOOL rok = ReadFile(pipe_r, buf, sizeof(buf), &n, NULL);
        if (n == 0) break;
        char *np = (char *)realloc(result, total + n + 1);
        if (!np) {
            free(result);
            result = NULL;
            total = 0;
            break;
        }
        result = np;
        memcpy(result + total, buf, n);
        total += n;
        result[total] = '\0';
        if (!rok) break;
    }
    CloseHandle(pipe_r);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!result) {
        return CBM_NOT_FOUND;
    }
    *out = result;
    if (out_len) *out_len = total;
    return (int)exit_code;

fail:
    CloseHandle(pipe_r);
    CloseHandle(pipe_w);
    return CBM_NOT_FOUND;
}

#else /* POSIX */

/* ── POSIX implementation ────────────────────────────────── */

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

struct cbm_dir {
    DIR *dir;
    cbm_dirent_t entry;
};

cbm_dir_t *cbm_opendir(const char *path) {
    if (!path) {
        return NULL;
    }
    DIR *dir = opendir(path);
    if (!dir) {
        return NULL;
    }
    cbm_dir_t *d = (cbm_dir_t *)calloc(CBM_ALLOC_ONE, sizeof(cbm_dir_t));
    if (!d) {
        closedir(dir);
        return NULL;
    }
    d->dir = dir;
    return d;
}

cbm_dirent_t *cbm_readdir(cbm_dir_t *d) {
    if (!d || !d->dir) {
        return NULL;
    }
    struct dirent *de;
    while ((de = readdir(d->dir)) != NULL) {
        /* Skip "." and ".." */
        if (de->d_name[0] == '.' &&
            (de->d_name[SKIP_ONE] == '\0' ||
             (de->d_name[SKIP_ONE] == '.' && de->d_name[PAIR_LEN] == '\0'))) {
            continue;
        }
        size_t nlen = strlen(de->d_name);
        if (nlen >= CBM_DIRENT_NAME_MAX) {
            nlen = CBM_DIRENT_NAME_MAX - SKIP_ONE;
        }
        memcpy(d->entry.name, de->d_name, nlen);
        d->entry.name[nlen] = '\0';
        d->entry.is_dir = (de->d_type == DT_DIR);
        d->entry.d_type = de->d_type;
        return &d->entry;
    }
    return NULL;
}

void cbm_closedir(cbm_dir_t *d) {
    if (d) {
        if (d->dir) {
            closedir(d->dir);
        }
        free(d);
    }
}

FILE *cbm_popen(const char *cmd, const char *mode) {
    return popen(cmd, mode);
}

int cbm_pclose(FILE *f) {
    return pclose(f);
}

bool cbm_mkdir_p(const char *path, int mode) {
    /* Try direct mkdir first */
    if (mkdir(path, (mode_t)mode) == 0) {
        return true;
    }
    /* Walk path and create each component */
    char *tmp = strdup(path);
    if (!tmp) {
        return false;
    }
    for (char *p = tmp + SKIP_ONE; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, (mode_t)mode); /* ignore intermediate errors */
            *p = '/';
        }
    }
    bool ok = (mkdir(tmp, (mode_t)mode) == 0 || errno == EEXIST) != 0;
    free(tmp);
    return ok;
}

int cbm_unlink(const char *path) {
    return unlink(path);
}

int cbm_rmdir(const char *path) {
    return rmdir(path);
}

int cbm_exec_no_shell(const char *const *argv) {
    if (!argv || !argv[0]) {
        return CBM_NOT_FOUND;
    }
    pid_t pid = fork();
    if (pid < 0) {
        return CBM_NOT_FOUND;
    }
    if (pid == 0) {
        /* Child: exec directly — no shell interpretation */
        /* 127 = standard "command not found" exit code (POSIX convention) */
        enum { EXEC_NOT_FOUND = 127 };
        execvp(argv[0], (char *const *)argv);
        _exit(EXEC_NOT_FOUND);
    }
    /* Parent: wait for child */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return CBM_NOT_FOUND;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return CBM_NOT_FOUND; /* killed by signal */
}

int cbm_spawn_capture(const char *exe, const char *const *argv,
                      char **out, size_t *out_len) {
    if (out) *out = NULL;
    if (out_len) *out_len = 0;
    if (!exe || !argv || !argv[0] || !out) {
        return CBM_NOT_FOUND;
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        return CBM_NOT_FOUND;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return CBM_NOT_FOUND;
    }
    if (pid == 0) {
        /* Child: wire stdout to pipe write end, then execvp directly.
         * No shell, no AutoRun-equivalent on POSIX. */
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            _exit(127);
        }
        close(pipefd[1]);
        execvp(exe, (char *const *)argv);
        _exit(127); /* exec failed */
    }

    /* Parent */
    close(pipefd[1]);

    char buf[CBM_SZ_4K];
    size_t total = 0;
    char *result = NULL;
    for (;;) {
        ssize_t n = read(pipefd[0], buf, sizeof(buf));
        if (n <= 0) break;
        char *np = (char *)realloc(result, total + (size_t)n + 1);
        if (!np) {
            free(result);
            result = NULL;
            total = 0;
            break;
        }
        result = np;
        memcpy(result + total, buf, (size_t)n);
        total += (size_t)n;
        result[total] = '\0';
    }
    close(pipefd[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        free(result);
        return CBM_NOT_FOUND;
    }

    if (!result) {
        return CBM_NOT_FOUND;
    }
    *out = result;
    if (out_len) *out_len = total;

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return CBM_NOT_FOUND; /* killed by signal */
}

#endif /* _WIN32 */

/* ── Cross-platform helpers (validated output extraction) ───────
 *
 * Pure functions: no spawn, no IPC. Unit-testable in isolation. */

#include <ctype.h>
#include "foundation/log.h"

/* ── Common validators (used by git_capture, artifact.c, etc.) ───
 *
 * Pure functions: take one trimmed stdout line, decide whether it could be
 * the real output of a known subcommand. Reject chcp banners, GCM hints,
 * hook output, and anything else that doesn't match the expected schema. */

bool cbm_validator_sha40_hex(const char *line, void *ctx) {
    (void)ctx;
    if (!line || strlen(line) != 40) {
        return false;
    }
    for (int i = 0; i < 40; i++) {
        if (!isxdigit((unsigned char)line[i])) {
            return false;
        }
    }
    return true;
}

bool cbm_validator_git_path(const char *line, void *ctx) {
    (void)ctx;
    if (!line || !line[0]) {
        return false;
    }
    /* Accept `.git` (worktree-relative), /abs/path, C:\...\, \\server\share. */
    if (strcmp(line, ".git") == 0) {
        return true;
    }
#ifdef _WIN32
    if (isalpha((unsigned char)line[0]) && line[1] == ':' &&
        (line[2] == '\\' || line[2] == '/')) {
        return true;
    }
    if (line[0] == '\\' || line[0] == '/') {
        return true;
    }
#else
    if (line[0] == '/') {
        return true;
    }
#endif
    return false;
}

bool cbm_validator_branch_name(const char *line, void *ctx) {
    (void)ctx;
    if (!line || !line[0]) {
        return false;
    }
    /* Branch names never contain spaces, tabs, or git ref meta-characters. */
    for (const char *p = line; *p; p++) {
        if (*p == ' ' || *p == '\t' || *p == ':' || *p == '?' || *p == '*' ||
            *p == '[' || *p == '\\' || *p == '~' || *p == '^') {
            return false;
        }
    }
    return true;
}


int cbm_find_validated_line(const char *text, cbm_line_validator_t validator,
                            void *ctx, char **out) {
    if (out) *out = NULL;
    if (!text || !validator || !out) {
        return CBM_NOT_FOUND;
    }

    const char *line_start = text;
    while (1) {
        const char *line_end = strchr(line_start, '\n');
        size_t len = line_end ? (size_t)(line_end - line_start) : strlen(line_start);

        /* Copy into a scratch buffer with trailing \r/\n trimmed. */
        char tmp[CBM_SZ_4K];
        size_t copy_len = len;
        if (copy_len >= sizeof(tmp)) {
            copy_len = sizeof(tmp) - 1;
        }
        memcpy(tmp, line_start, copy_len);
        tmp[copy_len] = '\0';
        size_t tlen = strlen(tmp);
        while (tlen > 0 && (tmp[tlen - 1] == '\r' || tmp[tlen - 1] == '\n')) {
            tmp[--tlen] = '\0';
        }

        if (validator(tmp, ctx)) {
            char *dup = (char *)malloc(strlen(tmp) + 1);
            if (!dup) return CBM_NOT_FOUND;
            strcpy(dup, tmp);
            *out = dup;
            return 0;
        }

        if (!line_end) break;
        line_start = line_end + 1;
    }

    return CBM_NOT_FOUND;
}

int cbm_spawn_capture_validated(const char *exe, const char *const *argv,
                                char **out,
                                cbm_line_validator_t validator, void *ctx) {
    if (out) *out = NULL;
    if (!exe || !argv || !out) {
        return CBM_NOT_FOUND;
    }

    char *raw = NULL;
    size_t raw_len = 0;
    int rc = cbm_spawn_capture(exe, argv, &raw, &raw_len);
    if (rc != 0) {
        free(raw);
        return CBM_NOT_FOUND;
    }
    if (!raw) {
        return CBM_NOT_FOUND;
    }

    /* No validator → return raw output as-is. */
    if (!validator) {
        *out = raw;
        return 0;
    }

    char *match = NULL;
    int vrc = cbm_find_validated_line(raw, validator, ctx, &match);
    if (vrc != 0) {
        /* Fail loud: no line passed validation. Surface the first line +
         * total line count so the user can diagnose the pollution source
         * (chcp banner, GCM hint, git hook output, etc.). Never fuzzy-match. */
        char first_line[CBM_SZ_256];
        size_t i = 0;
        for (; i < raw_len && i < sizeof(first_line) - 1 && raw[i] != '\n'; i++) {
            first_line[i] = raw[i];
        }
        first_line[i] = '\0';

        size_t lines = 1;
        for (size_t j = 0; j < raw_len; j++) {
            if (raw[j] == '\n') lines++;
        }
        cbm_log_warn("spawn.validation_failed",
                     "exe", exe,
                     "first_line", first_line,
                     "total_lines", (int)lines);
        free(raw);
        return CBM_NOT_FOUND;
    }

    free(raw);
    *out = match;
    return 0;
}
