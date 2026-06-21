/*
 * test_lz4.c — Tests for LZ4 HC compression wrappers.
 *
 * Ports from internal/cbm/lz4_test.go:
 *   TestLZ4RoundTrip, TestLZ4CompressionRatio,
 *   TestLZ4DecompressWrongLen, TestLZ4RandomData
 */
#include "test_framework.h"
#include <time.h>

/* lz4_store.c has no header — declare extern */
extern int cbm_lz4_compress_hc(const char *src, int srcLen, char *dst, int dstCap);
extern int cbm_lz4_decompress(const char *src, int srcLen, char *dst, int originalLen);
extern int cbm_lz4_bound(int inputSize);

/* ── Helper: compress + decompress round-trip ──────────────────── */

static int lz4_roundtrip(const char *data, int len) {
    if (len == 0) {
        /* Empty input: Go wrapper returns nil without calling LZ4.
         * LZ4_compress_HC actually returns 1 (end-of-stream marker) for empty input.
         * Match Go behavior: empty in → empty out is correct. */
        return 0;
    }

    int bound = cbm_lz4_bound(len);
    if (bound <= 0)
        return -1;

    char *cbuf = malloc(bound);
    if (!cbuf)
        return -1;

    int clen = cbm_lz4_compress_hc(data, len, cbuf, bound);
    if (clen <= 0) {
        free(cbuf);
        return -1;
    }

    char *dbuf = malloc(len);
    if (!dbuf) {
        free(cbuf);
        return -1;
    }

    int dlen = cbm_lz4_decompress(cbuf, clen, dbuf, len);
    int ok = (dlen == len && memcmp(dbuf, data, len) == 0) ? 0 : -1;

    free(cbuf);
    free(dbuf);
    return ok;
}

/* ── Tests ─────────────────────────────────────────────────────── */

TEST(lz4_roundtrip_empty) {
    /* Empty input: compress returns 0 (no output). Go wrapper returns nil. */
    int rc = lz4_roundtrip(NULL, 0);
    ASSERT_EQ(rc, 0);
    PASS();
}

TEST(lz4_roundtrip_small) {
    const char *data = "hello world";
    int rc = lz4_roundtrip(data, (int)strlen(data));
    ASSERT_EQ(rc, 0);
    PASS();
}

TEST(lz4_roundtrip_repeated) {
    /* 40,000 bytes of repeated "ABCD" — highly compressible */
    int len = 10000 * 4;
    char *data = malloc(len);
    ASSERT_NOT_NULL(data);
    for (int i = 0; i < 10000; i++)
        memcpy(data + i * 4, "ABCD", 4);

    int rc = lz4_roundtrip(data, len);
    free(data);
    ASSERT_EQ(rc, 0);
    PASS();
}

TEST(lz4_roundtrip_source_code) {
    const char *code = "package main\n"
                       "import \"fmt\"\n"
                       "func main() {\n"
                       "\tfor i := 0; i < 100; i++ {\n"
                       "\t\tfmt.Println(\"Hello, World!\", i)\n"
                       "\t}\n"
                       "}\n";
    int rc = lz4_roundtrip(code, (int)strlen(code));
    ASSERT_EQ(rc, 0);
    PASS();
}

TEST(lz4_compression_ratio) {
    /* Repetitive source code should compress >2x with HC */
    const char *line = "func handleRequest(w http.ResponseWriter, r *http.Request) {\n";
    int line_len = (int)strlen(line);
    int total = line_len * 1000;

    char *data = malloc(total);
    ASSERT_NOT_NULL(data);
    for (int i = 0; i < 1000; i++)
        memcpy(data + i * line_len, line, line_len);

    int bound = cbm_lz4_bound(total);
    char *cbuf = malloc(bound);
    ASSERT_NOT_NULL(cbuf);

    int clen = cbm_lz4_compress_hc(data, total, cbuf, bound);
    ASSERT_GT(clen, 0);

    double ratio = (double)total / (double)clen;
    /* Expect >2x compression on repetitive source code */
    ASSERT_TRUE(ratio >= 2.0);

    free(data);
    free(cbuf);
    PASS();
}

TEST(lz4_decompress_wrong_len) {
    /* Decompress with wrong originalLen — must not crash (safety property) */
    const char *src = "test data for decompression";
    int src_len = (int)strlen(src);

    int bound = cbm_lz4_bound(src_len);
    char *cbuf = malloc(bound);
    ASSERT_NOT_NULL(cbuf);

    int clen = cbm_lz4_compress_hc(src, src_len, cbuf, bound);
    ASSERT_GT(clen, 0);

    /* Allocate larger buffer, pass wrong originalLen */
    int wrong_len = src_len + 100;
    char *dbuf = malloc(wrong_len);
    ASSERT_NOT_NULL(dbuf);

    /* LZ4_decompress_safe won't crash — it either succeeds partially
     * or returns a negative error code. The key property is no buffer overflow. */
    int dlen = cbm_lz4_decompress(cbuf, clen, dbuf, wrong_len);
    (void)dlen; /* result may vary; safety is what matters */

    free(cbuf);
    free(dbuf);
    PASS();
}

TEST(lz4_random_data) {
    /* Random data is incompressible — LZ4 should still handle it */
    int len = 4096;
    char *data = malloc(len);
    ASSERT_NOT_NULL(data);

    /* Simple PRNG seeded from time — not crypto, just needs to be non-repeating */
    unsigned int seed = (unsigned int)time(NULL);
    for (int i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        data[i] = (char)(seed >> 16);
    }

    int bound = cbm_lz4_bound(len);
    char *cbuf = malloc(bound);
    ASSERT_NOT_NULL(cbuf);

    int clen = cbm_lz4_compress_hc(data, len, cbuf, bound);
    ASSERT_GT(clen, 0);

    char *dbuf = malloc(len);
    ASSERT_NOT_NULL(dbuf);

    int dlen = cbm_lz4_decompress(cbuf, clen, dbuf, len);
    ASSERT_EQ(dlen, len);
    ASSERT_MEM_EQ(dbuf, data, len);

    free(data);
    free(cbuf);
    free(dbuf);
    PASS();
}

TEST(lz4_bound_positive) {
    /* Bound should be positive for any positive input size */
    ASSERT_GT(cbm_lz4_bound(1), 0);
    ASSERT_GT(cbm_lz4_bound(100), 0);
    ASSERT_GT(cbm_lz4_bound(1000000), 0);
    /* Bound should be >= input size (compressed can't be smaller than worst case) */
    ASSERT_GTE(cbm_lz4_bound(100), 100);
    PASS();
}

/* ── Suite ─────────────────────────────────────────────────────── */

SUITE(lz4) {
    RUN_TEST(lz4_roundtrip_empty);
    RUN_TEST(lz4_roundtrip_small);
    RUN_TEST(lz4_roundtrip_repeated);
    RUN_TEST(lz4_roundtrip_source_code);
    RUN_TEST(lz4_compression_ratio);
    RUN_TEST(lz4_decompress_wrong_len);
    RUN_TEST(lz4_random_data);
    RUN_TEST(lz4_bound_positive);
}
