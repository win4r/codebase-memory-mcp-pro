#include "cbm.h"
#include "arena.h"
#include "helpers.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    T100_LINE_MAX = 8192,
    T100_NAME_MAX = 160,
    T100_TABLE_INIT = 64,
};

typedef struct {
    const char **items;
    int count;
    int cap;
} T100TableSet;

typedef struct {
    const char *name;
    const char *qn;
    const char *label;
    const char *signature;
    int start_line;
    bool is_entry;
} T100ActiveDef;

static int t100_ci_cmp(char a, char b) {
    return tolower((unsigned char)a) - tolower((unsigned char)b);
}

static bool t100_istarts(const char *s, const char *prefix) {
    if (!s || !prefix) {
        return false;
    }
    while (*prefix) {
        if (!*s || t100_ci_cmp(*s, *prefix) != 0) {
            return false;
        }
        s++;
        prefix++;
    }
    return true;
}

static bool t100_is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool t100_is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '.';
}

static const char *t100_skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

static char *t100_clean_name(CBMArena *a, const char *name) {
    if (!name || !name[0]) {
        return cbm_arena_strdup(a, "unknown");
    }
    size_t n = strlen(name);
    char *out = (char *)cbm_arena_alloc(a, n + 1);
    if (!out) {
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        unsigned char ch = (unsigned char)name[i];
        out[i] = (isalnum(ch) || ch == '_') ? (char)ch : '_';
    }
    out[n] = '\0';
    return out;
}

static char *t100_file_stem(CBMArena *a, const char *rel_path) {
    const char *base = rel_path ? strrchr(rel_path, '/') : NULL;
    base = base ? base + 1 : (rel_path ? rel_path : "module");
    const char *dot = strrchr(base, '.');
    size_t len = (dot && dot > base) ? (size_t)(dot - base) : strlen(base);
    char *tmp = cbm_arena_strndup(a, base, len);
    return t100_clean_name(a, tmp);
}

static void t100_lower_name(char *dst, size_t dst_sz, const char *src, size_t src_len) {
    size_t n = src_len < dst_sz - 1 ? src_len : dst_sz - 1;
    for (size_t i = 0; i < n; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    dst[n] = '\0';
}

static const char *t100_table_qn(CBMExtractCtx *ctx, const char *table) {
    return cbm_arena_sprintf(ctx->arena, "%s.__t100_tables__.%s", ctx->project, table);
}

static const char *t100_table_lookup(const T100TableSet *set, const char *table) {
    for (int i = 0; i < set->count; i++) {
        if (strcmp(set->items[i], table) == 0) {
            return set->items[i];
        }
    }
    return NULL;
}

static const char *t100_add_table_def(CBMExtractCtx *ctx, T100TableSet *set, const char *table,
                                      int line_no) {
    const char *existing = t100_table_lookup(set, table);
    if (existing) {
        return existing;
    }
    if (set->count >= set->cap) {
        int new_cap = set->cap ? set->cap * 2 : T100_TABLE_INIT;
        const char **grown = (const char **)realloc(set->items, (size_t)new_cap * sizeof(char *));
        if (!grown) {
            return table;
        }
        set->items = grown;
        set->cap = new_cap;
    }
    const char *stored = cbm_arena_strdup(ctx->arena, table);
    set->items[set->count++] = stored;

    CBMDefinition def = {0};
    def.name = stored;
    def.qualified_name = t100_table_qn(ctx, stored);
    def.label = "Table";
    def.file_path = ctx->rel_path;
    def.start_line = (uint32_t)line_no;
    def.end_line = (uint32_t)line_no;
    def.lines = 1;
    cbm_defs_push(&ctx->result->defs, ctx->arena, def);
    return stored;
}

static void t100_push_def(CBMExtractCtx *ctx, const T100ActiveDef *active, int end_line) {
    if (!active || !active->name || !active->qn || active->start_line <= 0) {
        return;
    }
    CBMDefinition def = {0};
    def.name = active->name;
    def.qualified_name = active->qn;
    def.label = active->label ? active->label : "Function";
    def.file_path = ctx->rel_path;
    def.start_line = (uint32_t)active->start_line;
    def.end_line = (uint32_t)(end_line >= active->start_line ? end_line : active->start_line);
    def.signature = active->signature;
    def.lines = (int)def.end_line - (int)def.start_line + 1;
    def.is_entry_point = active->is_entry;
    cbm_defs_push(&ctx->result->defs, ctx->arena, def);
}

static void t100_start_def(CBMExtractCtx *ctx, T100ActiveDef *active, const char *name,
                           const char *label, const char *signature, int line_no,
                           bool is_entry) {
    char *clean = t100_clean_name(ctx->arena, name);
    active->name = clean;
    active->qn = cbm_fqn_compute(ctx->arena, ctx->project, ctx->rel_path, clean);
    active->label = label;
    active->signature = signature ? cbm_arena_strdup(ctx->arena, signature) : NULL;
    active->start_line = line_no;
    active->is_entry = is_entry;
}

static bool t100_keyword_boundary(char c) {
    return c == '\0' || isspace((unsigned char)c) || c == '(' || c == ';';
}

static bool t100_parse_named_block(const char *line, const char *keyword, char *name,
                                   size_t name_sz, const char **signature_out) {
    const char *p = t100_skip_ws(line);
    if (t100_istarts(p, "public ")) {
        p = t100_skip_ws(p + strlen("public"));
    } else if (t100_istarts(p, "private ")) {
        p = t100_skip_ws(p + strlen("private"));
    } else if (t100_istarts(p, "static ")) {
        p = t100_skip_ws(p + strlen("static"));
    }
    size_t kw_len = strlen(keyword);
    if (!t100_istarts(p, keyword) || !t100_keyword_boundary(p[kw_len])) {
        return false;
    }
    p = t100_skip_ws(p + kw_len);
    if (!t100_is_ident_start(*p)) {
        return false;
    }
    const char *start = p;
    while (t100_is_ident_char(*p)) {
        p++;
    }
    size_t len = (size_t)(p - start);
    if (len >= name_sz) {
        len = name_sz - 1;
    }
    memcpy(name, start, len);
    name[len] = '\0';
    if (signature_out) {
        *signature_out = t100_skip_ws(p);
    }
    return true;
}

static bool t100_is_main_start(const char *line) {
    const char *p = t100_skip_ws(line);
    return t100_istarts(p, "main") && t100_keyword_boundary(p[4]);
}

static bool t100_is_block_end(const char *line) {
    const char *p = t100_skip_ws(line);
    return t100_istarts(p, "end function") || t100_istarts(p, "end main") ||
           t100_istarts(p, "end report");
}

static void t100_extract_calls(CBMExtractCtx *ctx, const char *line, const char *enclosing_qn,
                               int line_no) {
    const char *p = line;
    while (*p) {
        if (!t100_istarts(p, "call")) {
            p++;
            continue;
        }
        bool left_ok = (p == line) || !t100_is_ident_char(*(p - 1));
        bool right_ok = isspace((unsigned char)p[4]);
        if (!left_ok || !right_ok) {
            p++;
            continue;
        }
        p = t100_skip_ws(p + 4);
        if (!t100_is_ident_start(*p)) {
            continue;
        }
        const char *start = p;
        while (t100_is_ident_char(*p)) {
            p++;
        }
        char raw[T100_NAME_MAX];
        size_t len = (size_t)(p - start);
        if (len >= sizeof(raw)) {
            len = sizeof(raw) - 1;
        }
        memcpy(raw, start, len);
        raw[len] = '\0';
        CBMCall call = {0};
        call.callee_name = t100_clean_name(ctx->arena, raw);
        call.enclosing_func_qn = enclosing_qn;
        call.start_line = line_no;
        cbm_calls_push(&ctx->result->calls, ctx->arena, call);
    }
}

static bool t100_line_suggests_write(const char *lower, const char *tok_start) {
    const char *verbs[] = {"update", "insert", "delete", "merge", "truncate", NULL};
    for (int i = 0; verbs[i]; i++) {
        const char *p = lower;
        size_t vlen = strlen(verbs[i]);
        while ((p = strstr(p, verbs[i])) != NULL) {
            if (p < tok_start && (size_t)(tok_start - p) < 80 &&
                (p == lower || !t100_is_ident_char(*(p - 1))) &&
                !t100_is_ident_char(p[vlen])) {
                return true;
            }
            p += vlen;
        }
    }
    return false;
}

static void t100_extract_tables(CBMExtractCtx *ctx, T100TableSet *tables, const char *lower,
                                const char *enclosing_qn, int line_no) {
    const char *p = lower;
    while (*p) {
        if (!t100_is_ident_start(*p)) {
            p++;
            continue;
        }
        const char *start = p;
        while (t100_is_ident_char(*p)) {
            p++;
        }
        const char *end = p;
        const char *base = start;
        for (const char *q = start; q < end; q++) {
            if (*q == '.') {
                base = q + 1;
            }
        }
        size_t len = (size_t)(end - base);
        bool is_t100_table = len > 2 && base[len - 2] == '_' && base[len - 1] == 't';
        bool is_tiptop_table = len > 5 && memcmp(base + len - 5, "_file", 5) == 0;
        if (len < T100_NAME_MAX && (is_t100_table || is_tiptop_table)) {
            char table[T100_NAME_MAX];
            memcpy(table, base, len);
            table[len] = '\0';
            const char *stored = t100_add_table_def(ctx, tables, table, line_no);
            CBMReadWrite rw = {0};
            rw.var_name = stored;
            rw.enclosing_func_qn = enclosing_qn;
            rw.is_write = t100_line_suggests_write(lower, start);
            cbm_rw_push(&ctx->result->rw, ctx->arena, rw);
        }
    }
}

void cbm_extract_t100_4gl(CBMExtractCtx *ctx) {
    if (!ctx || !ctx->result || !ctx->source || ctx->source_len <= 0) {
        return;
    }

    int line_count = 1;
    for (int i = 0; i < ctx->source_len; i++) {
        if (ctx->source[i] == '\n') {
            line_count++;
        }
    }

    const char *module_name = t100_file_stem(ctx->arena, ctx->rel_path);
    CBMDefinition module = {0};
    module.name = module_name;
    module.qualified_name = ctx->module_qn;
    module.label = "Module";
    module.file_path = ctx->rel_path;
    module.start_line = 1;
    module.end_line = (uint32_t)line_count;
    module.lines = line_count;
    module.is_entry_point = true;
    cbm_defs_push(&ctx->result->defs, ctx->arena, module);

    T100TableSet tables = {0};
    T100ActiveDef active = {0};
    const char *active_qn = ctx->module_qn;
    const char *cursor = ctx->source;
    const char *end = ctx->source + ctx->source_len;
    int line_no = 1;

    while (cursor < end) {
        const char *line_end = memchr(cursor, '\n', (size_t)(end - cursor));
        if (!line_end) {
            line_end = end;
        }
        size_t raw_len = (size_t)(line_end - cursor);
        if (raw_len > 0 && cursor[raw_len - 1] == '\r') {
            raw_len--;
        }
        size_t copy_len = raw_len < T100_LINE_MAX - 1 ? raw_len : T100_LINE_MAX - 1;
        char line[T100_LINE_MAX];
        memcpy(line, cursor, copy_len);
        line[copy_len] = '\0';

        char *hash = strchr(line, '#');
        if (hash) {
            *hash = '\0';
        }
        const char *trim = t100_skip_ws(line);

        char lower[T100_LINE_MAX];
        t100_lower_name(lower, sizeof(lower), line, strlen(line));

        if (trim[0] && trim[0] != '-' && trim[0] != '#') {
            char name[T100_NAME_MAX];
            const char *sig = NULL;
            if (t100_parse_named_block(trim, "function", name, sizeof(name), &sig)) {
                if (active.name) {
                    t100_push_def(ctx, &active, line_no - 1);
                }
                t100_start_def(ctx, &active, name, "Function", sig, line_no, false);
                active_qn = active.qn;
            } else if (t100_parse_named_block(trim, "report", name, sizeof(name), &sig)) {
                if (active.name) {
                    t100_push_def(ctx, &active, line_no - 1);
                }
                t100_start_def(ctx, &active, name, "Function", sig, line_no, true);
                active_qn = active.qn;
            } else if (t100_is_main_start(trim)) {
                if (active.name) {
                    t100_push_def(ctx, &active, line_no - 1);
                }
                char main_name[T100_NAME_MAX];
                snprintf(main_name, sizeof(main_name), "%s_main", module_name);
                t100_start_def(ctx, &active, main_name, "Function", NULL, line_no, true);
                active_qn = active.qn;
            } else if (t100_is_block_end(trim) && active.name) {
                t100_push_def(ctx, &active, line_no);
                memset(&active, 0, sizeof(active));
                active_qn = ctx->module_qn;
            }

            t100_extract_calls(ctx, trim, active_qn, line_no);
            t100_extract_tables(ctx, &tables, lower, active_qn, line_no);
        }

        if (line_end == end) {
            break;
        }
        cursor = line_end + 1;
        line_no++;
    }

    if (active.name) {
        t100_push_def(ctx, &active, line_count);
    }
    free(tables.items);
}
