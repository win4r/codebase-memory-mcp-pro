/*
 * Test the contract emitted by the source-owned MCP tool registry without
 * needing the complete extraction/grammar test runner.
 */
#include "mcp/mcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int require_fragment(const char *json, const char *fragment) {
    if (strstr(json, fragment)) {
        return 0;
    }
    fprintf(stderr, "missing capability fragment: %s\n", fragment);
    return 1;
}

int main(void) {
    char *json = cbm_mcp_tools_list();
    if (!json) {
        fprintf(stderr, "tools/list returned NULL\n");
        return 1;
    }

    int failed = require_fragment(json, "\"capabilities\"") ||
                 require_fragment(json, "\"schema_version\":1") ||
                 require_fragment(json, "\"tool_count\":17") ||
                 require_fragment(json, "\"ui_assets\":false") ||
                 require_fragment(json, "\"explore\"") ||
                 require_fragment(json, "\"cancel_index\"") ||
                 require_fragment(json, "\"prune_index\"") ||
                 require_fragment(json, "\"background\"") ||
                 require_fragment(json, "\"job_id\"") ||
                 require_fragment(json, "\"dry_run\"") ||
                 require_fragment(json, "stale_source") ||
                 require_fragment(json, "stale_extractor") ||
                 require_fragment(json, "missing_root") ||
                 require_fragment(json, "partial");
    if (strstr(json, "\"repo_map\"")) {
        fprintf(stderr, "source contract must not advertise repo_map\n");
        failed = 1;
    }
    free(json);
    return failed;
}
