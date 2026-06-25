/*
 * test_spawn_capture.c — Tests for cbm_spawn_capture / cbm_find_validated_line.
 *
 * Validates the AutoRun-pollution defense: child stdout capture without
 * cmd.exe (so Windows AutoRun never fires), plus per-line validator that
 * skips chcp banners / GCM hints / git hook output / any future pollution
 * we haven't imagined yet. Pure-function tests are cross-platform;
 * integration tests require a spawnable echo/printf, so POSIX-only.
 */
#include "test_framework.h"
#include "../src/foundation/compat_fs.h"
#include "../src/foundation/constants.h"
#include <stdlib.h>
#include <string.h>

/* ── Validator helpers (mirror git_context.c validators) ─────────── */

static bool val_starts_with_target(const char *line, void *ctx) {
    (void)ctx;
    return strncmp(line, "TARGET", 6) == 0;
}

/* ── Pure function tests (cross-platform) ────────────────────────── */

TEST(find_validated_line_finds_first_match) {
    const char *text = "NOISE 1\nNOISE 2\nTARGET line here\nNOISE 3\n";
    char *out = NULL;
    int rc = cbm_find_validated_line(text, val_starts_with_target, NULL, &out);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "TARGET line here");
    free(out);
    PASS();
}

TEST(find_validated_line_no_match_returns_not_found) {
    const char *text = "NOISE 1\nNOISE 2\nNOISE 3\n";
    char *out = NULL;
    int rc = cbm_find_validated_line(text, val_starts_with_target, NULL, &out);
    ASSERT_EQ(rc, CBM_NOT_FOUND);
    ASSERT_NULL(out);
    PASS();
}

TEST(find_validated_line_handles_crlf) {
    const char *text = "NOISE\r\nTARGET hello\r\n";
    char *out = NULL;
    int rc = cbm_find_validated_line(text, val_starts_with_target, NULL, &out);
    ASSERT_EQ(rc, 0);
    ASSERT_STR_EQ(out, "TARGET hello");
    free(out);
    PASS();
}

TEST(find_validated_line_first_line_matches) {
    const char *text = "TARGET first\nNOISE\n";
    char *out = NULL;
    int rc = cbm_find_validated_line(text, val_starts_with_target, NULL, &out);
    ASSERT_EQ(rc, 0);
    ASSERT_STR_EQ(out, "TARGET first");
    free(out);
    PASS();
}

TEST(find_validated_line_last_line_matches) {
    /* No trailing newline — exercises the strlen() fallback path. */
    const char *text = "NOISE\nNOISE\nTARGET end";
    char *out = NULL;
    int rc = cbm_find_validated_line(text, val_starts_with_target, NULL, &out);
    ASSERT_EQ(rc, 0);
    ASSERT_STR_EQ(out, "TARGET end");
    free(out);
    PASS();
}

TEST(find_validated_line_truncates_long_line) {
    /* Build an 8 KiB line that exceeds the internal 4 KiB scratch buffer.
     * The first 6 bytes are "TARGET" so it must still match — proving the
     * truncation path doesn't break validator dispatch. */
    static char huge[8192 + 64];
    memset(huge, 'x', 8192);
    memcpy(huge, "TARGET", 6);
    huge[8192] = '\0';

    char *out = NULL;
    int rc = cbm_find_validated_line(huge, val_starts_with_target, NULL, &out);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(out);
    /* Returned copy is bounded by the scratch buffer (CBM_SZ_4K = 4096). */
    ASSERT_LT((long long)strlen(out), (long long)8192);
    free(out);
    PASS();
}

TEST(find_validated_line_null_safety) {
    char *out = NULL;
    ASSERT_EQ(cbm_find_validated_line(NULL, val_starts_with_target, NULL, &out), CBM_NOT_FOUND);
    ASSERT_EQ(cbm_find_validated_line("text", NULL, NULL, &out), CBM_NOT_FOUND);
    ASSERT_EQ(cbm_find_validated_line("text", val_starts_with_target, NULL, NULL), CBM_NOT_FOUND);
    PASS();
}

TEST(find_validated_line_empty_text) {
    char *out = NULL;
    /* Empty string is a single empty line — validator rejects it, returns -1. */
    ASSERT_EQ(cbm_find_validated_line("", val_starts_with_target, NULL, &out), CBM_NOT_FOUND);
    ASSERT_NULL(out);
    PASS();
}

/* ── Integration tests (POSIX only — Windows lacks a spawnable echo) ─
 *
 * Windows echo is a cmd.exe builtin, not a standalone .exe — invoking it
 * requires `cmd /c echo ...` which would itself trigger AutoRun and
 * invalidate the test's premise (the whole point is to NOT go through
 * cmd.exe). The cross-platform defense is the pure-function suite above;
 * these integration tests prove the wiring on POSIX where /bin/echo and
 * /usr/bin/printf are real executables. */

#ifndef _WIN32

TEST(spawn_capture_basic_echo) {
    const char *argv[] = {"/bin/echo", "hello-world", NULL};
    char *out = NULL;
    size_t len = 0;
    int rc = cbm_spawn_capture("/bin/echo", argv, &out, &len);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(out);
    ASSERT_GT((long long)len, 0LL);
    ASSERT_TRUE(strstr(out, "hello-world") != NULL);
    free(out);
    PASS();
}

TEST(spawn_capture_exit_code) {
    /* /bin/false always exits 1 on POSIX. */
    const char *argv[] = {"/bin/false", NULL};
    char *out = NULL;
    int rc = cbm_spawn_capture("/bin/false", argv, &out, NULL);
    ASSERT_EQ(rc, 1);
    free(out);
    PASS();
}

TEST(spawn_capture_missing_exe) {
    const char *argv[] = {"/nonexistent/exe/that/does/not/exist", NULL};
    char *out = NULL;
    int rc = cbm_spawn_capture("/nonexistent/exe/that/does/not/exist", argv, &out, NULL);
    ASSERT_EQ(rc, CBM_NOT_FOUND);
    ASSERT_NULL(out);
    PASS();
}

TEST(spawn_capture_validated_skips_pollution) {
    /* /usr/bin/printf emits two lines: simulate AutoRun banner + real output.
     * Validator must skip the banner and return only the TARGET line. */
    const char *argv[] = {"/usr/bin/printf",
                          "Active code page: 65001\nTARGET_real_output\n", NULL};
    char *out = NULL;
    int rc =
        cbm_spawn_capture_validated("/usr/bin/printf", argv, &out, val_starts_with_target, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT_STR_EQ(out, "TARGET_real_output");
    free(out);
    PASS();
}

TEST(spawn_capture_validated_fails_loud_when_no_match) {
    /* All output is garbage — validator must FAIL LOUD (return -1, *out = NULL),
     * never fuzzy-extract the "least bad" line. This is the core contract. */
    const char *argv[] = {"/usr/bin/printf", "GARBAGE1\nGARBAGE2\n", NULL};
    char *out = NULL;
    int rc = cbm_spawn_capture_validated("/usr/bin/printf", argv, &out, val_starts_with_target, NULL);
    ASSERT_EQ(rc, CBM_NOT_FOUND);
    ASSERT_NULL(out);
    PASS();
}

#endif /* _WIN32 */

SUITE(spawn_capture) {
    RUN_TEST(find_validated_line_finds_first_match);
    RUN_TEST(find_validated_line_no_match_returns_not_found);
    RUN_TEST(find_validated_line_handles_crlf);
    RUN_TEST(find_validated_line_first_line_matches);
    RUN_TEST(find_validated_line_last_line_matches);
    RUN_TEST(find_validated_line_truncates_long_line);
    RUN_TEST(find_validated_line_null_safety);
    RUN_TEST(find_validated_line_empty_text);
#ifndef _WIN32
    RUN_TEST(spawn_capture_basic_echo);
    RUN_TEST(spawn_capture_exit_code);
    RUN_TEST(spawn_capture_missing_exe);
    RUN_TEST(spawn_capture_validated_skips_pollution);
    RUN_TEST(spawn_capture_validated_fails_loud_when_no_match);
#endif
}
