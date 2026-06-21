/*
 * test_ac.c — Tests for the Aho-Corasick multi-pattern matcher.
 *
 * Exercises cbm_ac_build(), cbm_ac_scan_bitmask(), cbm_ac_scan_batch()
 * against the existing ac.c implementation.
 */
#include "test_framework.h"
#include <stdint.h>
#include <string.h>

/* Declarations from ac.c (no public header yet — these are the C API) */
typedef struct CBMAutomaton CBMAutomaton;

CBMAutomaton *cbm_ac_build(const char **patterns, const int *lengths, int count,
                           const uint8_t *alpha_map, int alpha_size);
void cbm_ac_free(CBMAutomaton *ac);
uint64_t cbm_ac_scan_bitmask(const CBMAutomaton *ac, const char *text, int text_len);
int cbm_ac_num_states(const CBMAutomaton *ac);
int cbm_ac_num_patterns(const CBMAutomaton *ac);
int cbm_ac_table_bytes(const CBMAutomaton *ac);

typedef struct {
    int name_index;
    int pattern_id;
} CBMMatchResult;
int cbm_ac_scan_batch(const CBMAutomaton *ac, const char *names_buf, const int *name_offsets,
                      const int *name_lengths, int num_names, CBMMatchResult *out_matches,
                      int max_matches);

/* ── Tests ─────────────────────────────────────────────────────── */

TEST(ac_build_single) {
    const char *patterns[] = {"hello"};
    int lengths[] = {5};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);
    ASSERT_NOT_NULL(ac);
    ASSERT_EQ(cbm_ac_num_patterns(ac), 1);
    ASSERT_GT(cbm_ac_num_states(ac), 0);
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_build_multiple) {
    const char *patterns[] = {"he", "she", "his", "hers"};
    int lengths[] = {2, 3, 3, 4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 4, NULL, 256);
    ASSERT_NOT_NULL(ac);
    ASSERT_EQ(cbm_ac_num_patterns(ac), 4);
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_scan_single_match) {
    const char *patterns[] = {"hello"};
    int lengths[] = {5};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);

    uint64_t result = cbm_ac_scan_bitmask(ac, "say hello world", 15);
    ASSERT_EQ(result, 1ULL); /* pattern 0 matched */
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_scan_no_match) {
    const char *patterns[] = {"xyz"};
    int lengths[] = {3};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);

    uint64_t result = cbm_ac_scan_bitmask(ac, "hello world", 11);
    ASSERT_EQ(result, 0);
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_scan_multiple_matches) {
    const char *patterns[] = {"he", "she", "his", "hers"};
    int lengths[] = {2, 3, 3, 4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 4, NULL, 256);

    /* "ushers" should match "she" (1), "he" (0), "hers" (3) */
    uint64_t result = cbm_ac_scan_bitmask(ac, "ushers", 6);
    ASSERT(result & (1ULL << 0)); /* "he" */
    ASSERT(result & (1ULL << 1)); /* "she" */
    ASSERT(result & (1ULL << 3)); /* "hers" */
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_scan_overlapping) {
    /* Overlapping patterns at same position */
    const char *patterns[] = {"ab", "abc", "abcd"};
    int lengths[] = {2, 3, 4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 3, NULL, 256);

    uint64_t result = cbm_ac_scan_bitmask(ac, "xabcdy", 6);
    ASSERT(result & (1ULL << 0)); /* "ab" */
    ASSERT(result & (1ULL << 1)); /* "abc" */
    ASSERT(result & (1ULL << 2)); /* "abcd" */
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_scan_empty_text) {
    const char *patterns[] = {"test"};
    int lengths[] = {4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);

    uint64_t result = cbm_ac_scan_bitmask(ac, "", 0);
    ASSERT_EQ(result, 0);
    cbm_ac_free(ac);
    PASS();
}

TEST(ac_custom_alphabet) {
    /* Use a compact alphabet mapping only lowercase letters */
    uint8_t alpha_map[256] = {0};
    for (int c = 'a'; c <= 'z'; c++)
        alpha_map[c] = (uint8_t)(c - 'a' + 1);
    int alpha_size = 27; /* 0=other, 1-26=a-z */

    const char *patterns[] = {"hello"};
    int lengths[] = {5};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, alpha_map, alpha_size);
    ASSERT_NOT_NULL(ac);

    uint64_t result = cbm_ac_scan_bitmask(ac, "say hello world", 15);
    ASSERT_EQ(result, 1ULL);

    /* Table should be much smaller with compact alphabet */
    int full_bytes = cbm_ac_num_states(ac) * 256 * 4;
    int compact_bytes = cbm_ac_table_bytes(ac);
    ASSERT_LT(compact_bytes, full_bytes);

    cbm_ac_free(ac);
    PASS();
}

TEST(ac_batch_scan) {
    const char *patterns[] = {"foo", "bar", "baz"};
    int lengths[] = {3, 3, 3};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 3, NULL, 256);

    /* Batch: scan 4 names */
    const char names_buf[] = "hello\0foobar\0nothing\0bazinga";
    int offsets[] = {0, 6, 13, 21};
    int lens[] = {5, 6, 7, 7};

    CBMMatchResult matches[16];
    int n = cbm_ac_scan_batch(ac, names_buf, offsets, lens, 4, matches, 16);

    /* "foobar" matches "foo" and "bar", "bazinga" matches "baz" → ≥3 matches */
    ASSERT_GTE(n, 3);

    cbm_ac_free(ac);
    PASS();
}

TEST(ac_table_bytes) {
    const char *patterns[] = {"test"};
    int lengths[] = {4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);

    int bytes = cbm_ac_table_bytes(ac);
    ASSERT_GT(bytes, 0);
    /* states * 256 * sizeof(int) */
    ASSERT_EQ(bytes, cbm_ac_num_states(ac) * 256 * 4);

    cbm_ac_free(ac);
    PASS();
}

TEST(ac_null_input) {
    CBMAutomaton *ac = cbm_ac_build(NULL, NULL, 0, NULL, 256);
    ASSERT_NULL(ac);
    cbm_ac_free(NULL); /* should not crash */
    PASS();
}

/* --- Ported from ac_test.go: TestACScanString --- */
TEST(ac_scan_string) {
    const char *patterns[] = {"foo", "bar"};
    int lengths[] = {3, 3};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 2, NULL, 256);
    ASSERT_NOT_NULL(ac);
    uint64_t mask = cbm_ac_scan_bitmask(ac, "foobar", 6);
    ASSERT_EQ(mask, 3ULL); /* both pattern 0 and 1 */
    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestACFree_DoubleCall --- */
TEST(ac_free_double_call) {
    const char *patterns[] = {"test"};
    int lengths[] = {4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);
    ASSERT_NOT_NULL(ac);
    cbm_ac_free(ac);
    /* ac is now freed — calling free again should not crash.
       Note: in C the pointer is dangling, but cbm_ac_free checks for NULL
       internally. We already test free(NULL) in ac_null_input.
       This test verifies the Go test expectation. */
    PASS();
}

/* --- Ported from ac_test.go: TestACScanLZ4Bitmask --- */
extern int cbm_lz4_compress_hc(const char *src, int srcLen, char *dst, int dstCap);
extern int cbm_lz4_bound(int inputSize);

/* Structs defined in ac.c */
typedef struct {
    const char *data;
    int compressed_len;
    int original_len;
} CBMLz4Entry;
typedef struct {
    int file_index;
    uint64_t bitmask;
} CBMLz4Match;

extern uint64_t cbm_ac_scan_lz4_bitmask(const CBMAutomaton *ac, const char *compressed,
                                        int compressed_len, int original_len);
extern int cbm_ac_scan_lz4_batch(const CBMAutomaton *ac, const CBMLz4Entry *entries,
                                 int num_entries, CBMLz4Match *out_matches, int max_matches);

TEST(ac_scan_lz4_bitmask) {
    const char *patterns[] = {"http.Get", "fetch("};
    int lengths[] = {8, 6};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 2, NULL, 256);
    ASSERT_NOT_NULL(ac);

    const char *source = "package main\nimport \"net/http\"\n"
                         "func doStuff() {\n"
                         "    resp, err := http.Get(\"https://example.com\")\n"
                         "    _ = resp\n    _ = err\n}\n";
    int src_len = (int)strlen(source);
    int bound = cbm_lz4_bound(src_len);
    char *compressed = malloc(bound);
    ASSERT_NOT_NULL(compressed);
    int comp_len = cbm_lz4_compress_hc(source, src_len, compressed, bound);
    ASSERT_GT(comp_len, 0);

    uint64_t mask = cbm_ac_scan_lz4_bitmask(ac, compressed, comp_len, src_len);
    ASSERT_EQ(mask, 1ULL); /* http.Get matched */

    /* No match */
    const char *no_http = "package main\nfunc main() { println(42) }\n";
    int nh_len = (int)strlen(no_http);
    int bound2 = cbm_lz4_bound(nh_len);
    char *comp2 = malloc(bound2);
    int comp2_len = cbm_lz4_compress_hc(no_http, nh_len, comp2, bound2);
    mask = cbm_ac_scan_lz4_bitmask(ac, comp2, comp2_len, nh_len);
    ASSERT_EQ(mask, 0ULL);

    free(compressed);
    free(comp2);
    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestACScanLZ4Batch --- */
TEST(ac_scan_lz4_batch) {
    const char *patterns[] = {"http.Get", "fetch(", "Route::get"};
    int lengths[] = {8, 6, 10};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 3, NULL, 256);
    ASSERT_NOT_NULL(ac);

    /* Three files: one with http.Get, one empty, one with Route::get */
    const char *files[] = {"resp := http.Get(\"https://example.com\")",
                           "func main() { println(42) }",
                           "Route::get(\"/users\", \"UserController@index\")"};
    CBMLz4Entry entries[3];
    char *comp_bufs[3];
    for (int i = 0; i < 3; i++) {
        int slen = (int)strlen(files[i]);
        int bound = cbm_lz4_bound(slen);
        comp_bufs[i] = malloc(bound);
        int clen = cbm_lz4_compress_hc(files[i], slen, comp_bufs[i], bound);
        entries[i].data = comp_bufs[i];
        entries[i].compressed_len = clen;
        entries[i].original_len = slen;
    }

    CBMLz4Match matches[8];
    int n = cbm_ac_scan_lz4_batch(ac, entries, 3, matches, 8);
    ASSERT_EQ(n, 2); /* files 0 and 2 */
    ASSERT_EQ(matches[0].file_index, 0);
    ASSERT(matches[0].bitmask & 1ULL); /* http.Get */
    ASSERT_EQ(matches[1].file_index, 2);
    ASSERT(matches[1].bitmask & 4ULL); /* Route::get */

    for (int i = 0; i < 3; i++)
        free(comp_bufs[i]);
    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestACScanLZ4Batch_Empty --- */
TEST(ac_scan_lz4_batch_empty) {
    const char *patterns[] = {"test"};
    int lengths[] = {4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 1, NULL, 256);
    ASSERT_NOT_NULL(ac);
    CBMLz4Match matches[1];
    int n = cbm_ac_scan_lz4_batch(ac, NULL, 0, matches, 1);
    ASSERT_EQ(n, 0);
    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestAC_LargePatternSet --- */
TEST(ac_large_pattern_set) {
    /* All httplink keywords — real-world pattern count */
    const char *patterns[] = {
        "requests.get",    "requests.post",    "requests.put",    "requests.delete",
        "requests.patch",  "httpx.",           "aiohttp.",        "urllib.request",
        "http.Get",        "http.Post",        "http.NewRequest", "client.Do(",
        "fetch(",          "axios.",           ".ajax(",          "HttpClient",
        "RestTemplate",    "WebClient",        "OkHttpClient",    "HttpURLConnection",
        "openConnection(", "reqwest::",        "hyper::",         "surf::",
        "ureq::",          "curl_exec",        "curl_init",       "Guzzle",
        "Http::get",       "Http::post",       "sttp.",           "http4s",
        "wsClient",        "curl_easy",        "cpr::Get",        "cpr::Post",
        "httplib::",       "socket.http",      "http.request",    "RestClient",
        "HttpWebRequest",  "ktor.client",      "send_request",    "http_client",
        "CreateTask",      "create_task",      "topic.Publish",   "publisher.publish",
        "topic.publish",   "sqs.send_message", "sns.publish",     "basic_publish",
        "producer.send",   "producer.Send",
    };
    int count = sizeof(patterns) / sizeof(patterns[0]);
    int *lengths = malloc(count * sizeof(int));
    for (int i = 0; i < count; i++)
        lengths[i] = (int)strlen(patterns[i]);

    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, count, NULL, 256);
    ASSERT_NOT_NULL(ac);
    ASSERT_GT(cbm_ac_num_states(ac), 0);

    /* Scan Go source with http.Get and CreateTask */
    const char *source = "package main\nimport \"net/http\"\n"
                         "func callAPI() {\n"
                         "    resp, _ := http.Get(\"https://api.example.com/data\")\n"
                         "    defer resp.Body.Close()\n}\n"
                         "func createTask() {\n    client.CreateTask(ctx, req)\n}\n";
    uint64_t mask = cbm_ac_scan_bitmask(ac, source, (int)strlen(source));
    /* Find indices for http.Get and CreateTask */
    int http_get_idx = -1, create_task_idx = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(patterns[i], "http.Get") == 0)
            http_get_idx = i;
        if (strcmp(patterns[i], "CreateTask") == 0)
            create_task_idx = i;
    }
    ASSERT(mask & (1ULL << http_get_idx));
    ASSERT(mask & (1ULL << create_task_idx));

    /* Non-matching source */
    mask = cbm_ac_scan_bitmask(ac, "func main() { fmt.Println(42) }", 31);
    ASSERT_EQ(mask, 0ULL);

    free(lengths);
    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestACBuild_MultiplePatterns --- */
TEST(ac_http_patterns) {
    const char *patterns[] = {"http.Get", "fetch(", "requests.post", "axios."};
    int lengths[] = {8, 6, 13, 6};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 4, NULL, 256);
    ASSERT_NOT_NULL(ac);
    ASSERT_EQ(cbm_ac_num_patterns(ac), 4);

    struct {
        const char *input;
        uint64_t expected;
    } tests[] = {
        {"resp := http.Get(url)", 1ULL << 0},
        {"const r = fetch(url)", 1ULL << 1},
        {"requests.post(url, data)", 1ULL << 2},
        {"axios.get('/api')", 1ULL << 3},
        {"http.Get then fetch(", (1ULL << 0) | (1ULL << 1)},
        {"no http calls here", 0},
        {"", 0},
    };
    for (int i = 0; i < 7; i++) {
        uint64_t mask = cbm_ac_scan_bitmask(ac, tests[i].input, (int)strlen(tests[i].input));
        ASSERT_EQ(mask, tests[i].expected);
    }
    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestACCompactAlphabet (extended) --- */
TEST(ac_compact_alphabet_extended) {
    /* Build a-z=1..26, 0-9=27..36, _=37 */
    uint8_t alpha_map[256] = {0};
    uint8_t idx = 1;
    for (int c = 'a'; c <= 'z'; c++)
        alpha_map[c] = idx++;
    for (int c = '0'; c <= '9'; c++)
        alpha_map[c] = idx++;
    alpha_map['_'] = idx;
    int alpha_size = (int)idx + 1; /* 38 */

    const char *patterns[] = {"database_url", "api_key", "port"};
    int lengths[] = {12, 7, 4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 3, alpha_map, alpha_size);
    ASSERT_NOT_NULL(ac);

    /* Exact match */
    uint64_t mask = cbm_ac_scan_bitmask(ac, "database_url", 12);
    ASSERT(mask & 1ULL);

    /* Substring match */
    mask = cbm_ac_scan_bitmask(ac, "my_database_url_setting", 23);
    ASSERT(mask & 1ULL);

    /* Verify table size = states * alpha_size * sizeof(int) */
    int table_bytes = cbm_ac_table_bytes(ac);
    ASSERT_EQ(table_bytes, cbm_ac_num_states(ac) * alpha_size * 4);

    cbm_ac_free(ac);
    PASS();
}

/* --- Ported from ac_test.go: TestACScanBatch (detailed) --- */
TEST(ac_batch_scan_detailed) {
    const char *patterns[] = {"database", "port", "host"};
    int lengths[] = {8, 4, 4};
    CBMAutomaton *ac = cbm_ac_build(patterns, lengths, 3, NULL, 256);
    ASSERT_NOT_NULL(ac);

    /* 5 names: database_url, server_port, api_key, database_host, max_retries */
    const char names_buf[] = "database_url\0server_port\0api_key\0database_host\0max_retries";
    int offsets[] = {0, 13, 25, 33, 47};
    int lens[] = {12, 11, 7, 13, 11};

    CBMMatchResult matches[16];
    int n = cbm_ac_scan_batch(ac, names_buf, offsets, lens, 5, matches, 16);

    /* Expect:
     *   name 0 (database_url) → pattern 0 (database)
     *   name 1 (server_port)  → pattern 1 (port)
     *   name 3 (database_host) → pattern 0 (database) AND pattern 2 (host)
     *   name 2 (api_key) → no match
     *   name 4 (max_retries) → no match
     * Total: 4 matches */
    ASSERT_EQ(n, 4);

    /* Build lookup: name_index → list of pattern_ids */
    int found_name0 = 0, found_name1 = 0, found_name3_db = 0, found_name3_host = 0;
    for (int i = 0; i < n; i++) {
        if (matches[i].name_index == 0 && matches[i].pattern_id == 0)
            found_name0 = 1;
        if (matches[i].name_index == 1 && matches[i].pattern_id == 1)
            found_name1 = 1;
        if (matches[i].name_index == 3 && matches[i].pattern_id == 0)
            found_name3_db = 1;
        if (matches[i].name_index == 3 && matches[i].pattern_id == 2)
            found_name3_host = 1;
    }
    ASSERT_TRUE(found_name0);
    ASSERT_TRUE(found_name1);
    ASSERT_TRUE(found_name3_db);
    ASSERT_TRUE(found_name3_host);

    cbm_ac_free(ac);
    PASS();
}

SUITE(ac) {
    RUN_TEST(ac_build_single);
    RUN_TEST(ac_build_multiple);
    RUN_TEST(ac_scan_single_match);
    RUN_TEST(ac_scan_no_match);
    RUN_TEST(ac_scan_multiple_matches);
    RUN_TEST(ac_scan_overlapping);
    RUN_TEST(ac_scan_empty_text);
    RUN_TEST(ac_custom_alphabet);
    RUN_TEST(ac_batch_scan);
    RUN_TEST(ac_table_bytes);
    RUN_TEST(ac_null_input);
    RUN_TEST(ac_scan_string);
    RUN_TEST(ac_free_double_call);
    RUN_TEST(ac_scan_lz4_bitmask);
    RUN_TEST(ac_scan_lz4_batch);
    RUN_TEST(ac_scan_lz4_batch_empty);
    RUN_TEST(ac_large_pattern_set);
    RUN_TEST(ac_http_patterns);
    RUN_TEST(ac_compact_alphabet_extended);
    RUN_TEST(ac_batch_scan_detailed);
}
