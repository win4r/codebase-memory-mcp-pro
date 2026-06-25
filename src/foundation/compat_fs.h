/*
 * compat_fs.h — Portable directory iteration, popen, and file operations.
 *
 * POSIX: thin wrappers around opendir/readdir, popen/pclose, mkdir, unlink.
 * Windows: FindFirstFile/FindNextFile, _popen/_pclose, _mkdir, _unlink.
 */
#ifndef CBM_COMPAT_FS_H
#define CBM_COMPAT_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* ── Directory iteration ──────────────────────────────────────── */

/* Max filename length (MAX_PATH on Windows, NAME_MAX on POSIX). */
#define CBM_DIRENT_NAME_MAX 260

typedef struct cbm_dir cbm_dir_t;

typedef struct {
    char name[CBM_DIRENT_NAME_MAX];
    bool is_dir;
    unsigned char d_type; /* DT_REG, DT_DIR, DT_LNK, etc. (POSIX only, 0 on Windows) */
} cbm_dirent_t;

/* Open a directory for iteration. Returns NULL on error. */
cbm_dir_t *cbm_opendir(const char *path);

/* Read next entry. Returns NULL when done. The returned pointer is
 * valid until the next cbm_readdir call on the same handle. */
cbm_dirent_t *cbm_readdir(cbm_dir_t *d);

/* Close directory handle. */
void cbm_closedir(cbm_dir_t *d);

/* ── Portable popen/pclose ──────────────────────────────────────
 *
 * WARNING: cbm_popen runs commands through cmd.exe on Windows and /bin/sh on
 * POSIX. On Windows, cmd.exe triggers HKLM/HKCU AutoRun on every spawn,
 * which can pollute stdout (e.g. `chcp 65001` prints "Active code page: 65001").
 * Prefer cbm_spawn_capture below for any caller that parses child stdout.
 */
FILE *cbm_popen(const char *cmd, const char *mode);
int cbm_pclose(FILE *f);

/* ── Shell-free spawn with stdout capture ───────────────────────
 *
 * Spawns a child process directly (no cmd.exe / no /bin/sh), so Windows AutoRun
 * is never triggered and shell-injection surface is eliminated. The child's
 * stdout is fully captured into a malloc'd buffer; stderr is left on the
 * parent's stderr (NOT merged — keeps parseable stdout clean).
 *
 * Returns the child exit code (0 on success), or CBM_NOT_FOUND on spawn/pipe
 * failure. On success `*out` receives a malloc'd NUL-terminated buffer (caller
 * frees); on failure `*out` is set to NULL.
 */
int cbm_spawn_capture(const char *exe, const char *const *argv,
                      char **out, size_t *out_len);

typedef bool (*cbm_line_validator_t)(const char *line, void *ctx);

/* ── Common validators for git/plumbing output ──────────────────
 *
 * Reusable across modules. Each accepts a single trimmed line and decides
 * whether it could be the real output of a known subcommand, rejecting
 * AutoRun banners, GCM hints, hook output, and other pollution. */

/* 40-char lowercase/uppercase hex string (git commit/object sha). */
bool cbm_validator_sha40_hex(const char *line, void *ctx);

/* Absolute path (POSIX /abs, Windows C:\..., \\server\share) or `.git`.
 * Matches rev-parse --show-toplevel / --git-dir / --git-common-dir output. */
bool cbm_validator_git_path(const char *line, void *ctx);

/* Git branch name: non-empty, no spaces/tabs/ref-meta chars. */
bool cbm_validator_branch_name(const char *line, void *ctx);

/* Scan a multi-line buffer and return the first line that passes `validator`.
 * Pure function — no process spawn. Returns 0 and stores a malloc'd copy of the
 * matched line (trimmed of trailing \r\n) in `*out`; returns CBM_NOT_FOUND if
 * no line matches (and sets `*out` to NULL). Used by cbm_spawn_capture_validated
 * and unit-testable in isolation. */
int cbm_find_validated_line(const char *text, cbm_line_validator_t validator,
                            void *ctx, char **out);

/* Spawn + capture + per-line validation. Each line of the child's stdout is
 * offered to `validator`; the first match is returned in `*out`. If no line
 * matches (output polluted beyond recognition, or genuine command failure),
 * returns CBM_NOT_FOUND and logs the first line + total line count at WARN —
 * never fuzzy-matches, never silently swallows.
 *
 * Pass NULL as `validator` to disable validation (returns the raw first line).
 */
int cbm_spawn_capture_validated(const char *exe, const char *const *argv,
                                char **out,
                                cbm_line_validator_t validator, void *ctx);


/* Create directory (and parents). mode is ignored on Windows. Returns true on success. */
bool cbm_mkdir_p(const char *path, int mode);

/* Delete a file. Returns 0 on success. */
int cbm_unlink(const char *path);

/* Delete an empty directory. Returns 0 on success. */
int cbm_rmdir(const char *path);

/* Execute a command without shell interpretation.
 * argv is a NULL-terminated array: {"cmd", "arg1", "arg2", NULL}.
 * Returns the process exit code, or -1 on fork/exec failure.
 * POSIX: fork() + execvp(). Windows: _spawnvp(). */
int cbm_exec_no_shell(const char *const *argv);

#endif /* CBM_COMPAT_FS_H */
