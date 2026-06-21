// C++ stdlib type information for C/C++ LSP type resolver.

#include "../type_rep.h"
#include "../type_registry.h"
#include <string.h>

// Helper: register a method on a type
static void reg_method(CBMTypeRegistry* reg, CBMArena* arena,
    const char* recv_qn, const char* method_name, const CBMType* ret_type) {
    CBMRegisteredFunc rf;
    memset(&rf, 0, sizeof(rf));
    rf.min_params = -1;
    // Build QN: recv_qn.method_name
    size_t rlen = strlen(recv_qn);
    size_t mlen = strlen(method_name);
    char* qn = (char*)cbm_arena_alloc(arena, rlen + 1 + mlen + 1);
    if (!qn) return;
    memcpy(qn, recv_qn, rlen);
    qn[rlen] = '.';
    memcpy(qn + rlen + 1, method_name, mlen);
    qn[rlen + 1 + mlen] = '\0';

    rf.qualified_name = qn;
    rf.short_name = method_name;
    rf.receiver_type = recv_qn;
    rf.signature = cbm_type_func(arena, NULL, NULL,
        ret_type ? (const CBMType*[]){ret_type, NULL} : NULL);
    cbm_registry_add_func(reg, rf);
}

// Helper: register a type
static void reg_type(CBMTypeRegistry* reg, const char* qn, const char* short_name) {
    CBMRegisteredType rt;
    memset(&rt, 0, sizeof(rt));
    rt.qualified_name = qn;
    rt.short_name = short_name;
    cbm_registry_add_type(reg, rt);
}

// Helper: register a type with field names
static void reg_type_with_fields(CBMTypeRegistry* reg, CBMArena* arena,
    const char* qn, const char* short_name,
    const char** fnames, const CBMType** ftypes) {
    CBMRegisteredType rt;
    memset(&rt, 0, sizeof(rt));
    rt.qualified_name = qn;
    rt.short_name = short_name;
    rt.field_names = fnames;
    rt.field_types = ftypes;
    cbm_registry_add_type(reg, rt);
}

// Helper: register a free function
static void reg_func(CBMTypeRegistry* reg, CBMArena* arena,
    const char* qn, const char* short_name, const CBMType* ret_type) {
    CBMRegisteredFunc rf;
    memset(&rf, 0, sizeof(rf));
    rf.min_params = -1;
    rf.qualified_name = qn;
    rf.short_name = short_name;
    rf.signature = cbm_type_func(arena, NULL, NULL,
        ret_type ? (const CBMType*[]){ret_type, NULL} : NULL);
    cbm_registry_add_func(reg, rf);
}

void cbm_cpp_stdlib_register(CBMTypeRegistry* reg, CBMArena* arena) {
    const CBMType* t_void = cbm_type_builtin(arena, "void");
    const CBMType* t_void_ptr = cbm_type_pointer(arena, t_void);
    const CBMType* t_bool = cbm_type_builtin(arena, "bool");
    const CBMType* t_int = cbm_type_builtin(arena, "int");
    const CBMType* t_size_t = cbm_type_builtin(arena, "size_t");
    const CBMType* t_long = cbm_type_builtin(arena, "long");
    const CBMType* t_char = cbm_type_builtin(arena, "char");
    const CBMType* t_char_ptr = cbm_type_pointer(arena, cbm_type_builtin(arena, "const char"));

    // =========================================================================
    // std.string
    // =========================================================================
    const char* string_qn = "std.string";
    reg_type(reg, string_qn, "string");
    const CBMType* t_string = cbm_type_named(arena, string_qn);
    const CBMType* t_string_ref = cbm_type_reference(arena, t_string);

    reg_method(reg, arena, string_qn, "c_str", t_char_ptr);
    reg_method(reg, arena, string_qn, "data", t_char_ptr);
    reg_method(reg, arena, string_qn, "size", t_size_t);
    reg_method(reg, arena, string_qn, "length", t_size_t);
    reg_method(reg, arena, string_qn, "capacity", t_size_t);
    reg_method(reg, arena, string_qn, "max_size", t_size_t);
    reg_method(reg, arena, string_qn, "empty", t_bool);
    reg_method(reg, arena, string_qn, "clear", t_void);
    reg_method(reg, arena, string_qn, "substr", t_string);
    reg_method(reg, arena, string_qn, "find", t_size_t);
    reg_method(reg, arena, string_qn, "rfind", t_size_t);
    reg_method(reg, arena, string_qn, "compare", t_int);
    reg_method(reg, arena, string_qn, "append", t_string_ref);
    reg_method(reg, arena, string_qn, "insert", t_string_ref);
    reg_method(reg, arena, string_qn, "erase", t_string_ref);
    reg_method(reg, arena, string_qn, "replace", t_string_ref);
    reg_method(reg, arena, string_qn, "push_back", t_void);
    reg_method(reg, arena, string_qn, "pop_back", t_void);
    reg_method(reg, arena, string_qn, "front", cbm_type_reference(arena, t_char));
    reg_method(reg, arena, string_qn, "back", cbm_type_reference(arena, t_char));
    reg_method(reg, arena, string_qn, "at", cbm_type_reference(arena, t_char));
    reg_method(reg, arena, string_qn, "operator[]", cbm_type_reference(arena, t_char));
    reg_method(reg, arena, string_qn, "begin", cbm_type_named(arena, "std.string.iterator"));
    reg_method(reg, arena, string_qn, "end", cbm_type_named(arena, "std.string.iterator"));
    reg_method(reg, arena, string_qn, "resize", t_void);
    reg_method(reg, arena, string_qn, "reserve", t_void);
    reg_method(reg, arena, string_qn, "starts_with", t_bool);
    reg_method(reg, arena, string_qn, "ends_with", t_bool);
    reg_method(reg, arena, string_qn, "contains", t_bool);
    reg_method(reg, arena, string_qn, "operator+", t_string);
    reg_method(reg, arena, string_qn, "operator+=", t_string_ref);
    // Also register as std.basic_string alias
    {
        CBMRegisteredType rt;
        memset(&rt, 0, sizeof(rt));
        rt.qualified_name = "std.basic_string";
        rt.short_name = "basic_string";
        rt.alias_of = string_qn;
        cbm_registry_add_type(reg, rt);
    }

    // =========================================================================
    // std.string_view
    // =========================================================================
    const char* sv_qn = "std.string_view";
    reg_type(reg, sv_qn, "string_view");
    reg_method(reg, arena, sv_qn, "data", t_char_ptr);
    reg_method(reg, arena, sv_qn, "size", t_size_t);
    reg_method(reg, arena, sv_qn, "length", t_size_t);
    reg_method(reg, arena, sv_qn, "empty", t_bool);
    reg_method(reg, arena, sv_qn, "substr", cbm_type_named(arena, sv_qn));
    reg_method(reg, arena, sv_qn, "find", t_size_t);
    reg_method(reg, arena, sv_qn, "starts_with", t_bool);
    reg_method(reg, arena, sv_qn, "ends_with", t_bool);
    reg_method(reg, arena, sv_qn, "operator[]", t_char);

    // =========================================================================
    // std.vector<T> — T is first template arg
    // =========================================================================
    const char* vec_qn = "std.vector";
    reg_type(reg, vec_qn, "vector");
    const CBMType* t_T = cbm_type_type_param(arena, "T");
    const CBMType* t_T_ref = cbm_type_reference(arena, t_T);
    const CBMType* t_T_ptr = cbm_type_pointer(arena, t_T);

    reg_method(reg, arena, vec_qn, "push_back", t_void);
    reg_method(reg, arena, vec_qn, "emplace_back", t_void);
    reg_method(reg, arena, vec_qn, "pop_back", t_void);
    reg_method(reg, arena, vec_qn, "size", t_size_t);
    reg_method(reg, arena, vec_qn, "capacity", t_size_t);
    reg_method(reg, arena, vec_qn, "empty", t_bool);
    reg_method(reg, arena, vec_qn, "clear", t_void);
    reg_method(reg, arena, vec_qn, "resize", t_void);
    reg_method(reg, arena, vec_qn, "reserve", t_void);
    reg_method(reg, arena, vec_qn, "shrink_to_fit", t_void);
    reg_method(reg, arena, vec_qn, "front", t_T_ref);
    reg_method(reg, arena, vec_qn, "back", t_T_ref);
    reg_method(reg, arena, vec_qn, "at", t_T_ref);
    reg_method(reg, arena, vec_qn, "data", t_T_ptr);
    reg_method(reg, arena, vec_qn, "operator[]", t_T_ref);
    reg_method(reg, arena, vec_qn, "begin", cbm_type_named(arena, "std.vector.iterator"));
    reg_method(reg, arena, vec_qn, "end", cbm_type_named(arena, "std.vector.iterator"));
    reg_method(reg, arena, vec_qn, "erase", cbm_type_named(arena, "std.vector.iterator"));
    reg_method(reg, arena, vec_qn, "insert", cbm_type_named(arena, "std.vector.iterator"));

    // =========================================================================
    // std.map<K,V>
    // =========================================================================
    const char* map_qn = "std.map";
    reg_type(reg, map_qn, "map");
    const CBMType* t_V = cbm_type_type_param(arena, "V");
    const CBMType* t_V_ref = cbm_type_reference(arena, t_V);

    reg_method(reg, arena, map_qn, "operator[]", t_V_ref);
    reg_method(reg, arena, map_qn, "at", t_V_ref);
    reg_method(reg, arena, map_qn, "find", cbm_type_named(arena, "std.map.iterator"));
    reg_method(reg, arena, map_qn, "insert", cbm_type_named(arena, "std.pair"));
    reg_method(reg, arena, map_qn, "emplace", cbm_type_named(arena, "std.pair"));
    reg_method(reg, arena, map_qn, "erase", t_size_t);
    reg_method(reg, arena, map_qn, "count", t_size_t);
    reg_method(reg, arena, map_qn, "contains", t_bool);
    reg_method(reg, arena, map_qn, "size", t_size_t);
    reg_method(reg, arena, map_qn, "empty", t_bool);
    reg_method(reg, arena, map_qn, "clear", t_void);
    reg_method(reg, arena, map_qn, "begin", cbm_type_named(arena, "std.map.iterator"));
    reg_method(reg, arena, map_qn, "end", cbm_type_named(arena, "std.map.iterator"));

    // std.unordered_map<K,V> — same interface
    const char* umap_qn = "std.unordered_map";
    reg_type(reg, umap_qn, "unordered_map");
    reg_method(reg, arena, umap_qn, "operator[]", t_V_ref);
    reg_method(reg, arena, umap_qn, "at", t_V_ref);
    reg_method(reg, arena, umap_qn, "find", cbm_type_named(arena, "std.unordered_map.iterator"));
    reg_method(reg, arena, umap_qn, "insert", cbm_type_named(arena, "std.pair"));
    reg_method(reg, arena, umap_qn, "emplace", cbm_type_named(arena, "std.pair"));
    reg_method(reg, arena, umap_qn, "erase", t_size_t);
    reg_method(reg, arena, umap_qn, "count", t_size_t);
    reg_method(reg, arena, umap_qn, "contains", t_bool);
    reg_method(reg, arena, umap_qn, "size", t_size_t);
    reg_method(reg, arena, umap_qn, "empty", t_bool);
    reg_method(reg, arena, umap_qn, "clear", t_void);
    reg_method(reg, arena, umap_qn, "begin", cbm_type_named(arena, "std.unordered_map.iterator"));
    reg_method(reg, arena, umap_qn, "end", cbm_type_named(arena, "std.unordered_map.iterator"));

    // =========================================================================
    // std.set<T> / std.unordered_set<T>
    // =========================================================================
    const char* set_qn = "std.set";
    reg_type(reg, set_qn, "set");
    reg_method(reg, arena, set_qn, "insert", cbm_type_named(arena, "std.pair"));
    reg_method(reg, arena, set_qn, "find", cbm_type_named(arena, "std.set.iterator"));
    reg_method(reg, arena, set_qn, "erase", t_size_t);
    reg_method(reg, arena, set_qn, "count", t_size_t);
    reg_method(reg, arena, set_qn, "contains", t_bool);
    reg_method(reg, arena, set_qn, "size", t_size_t);
    reg_method(reg, arena, set_qn, "empty", t_bool);
    reg_method(reg, arena, set_qn, "clear", t_void);
    reg_method(reg, arena, set_qn, "begin", cbm_type_named(arena, "std.set.iterator"));
    reg_method(reg, arena, set_qn, "end", cbm_type_named(arena, "std.set.iterator"));

    const char* uset_qn = "std.unordered_set";
    reg_type(reg, uset_qn, "unordered_set");
    reg_method(reg, arena, uset_qn, "insert", cbm_type_named(arena, "std.pair"));
    reg_method(reg, arena, uset_qn, "find", cbm_type_named(arena, "std.unordered_set.iterator"));
    reg_method(reg, arena, uset_qn, "erase", t_size_t);
    reg_method(reg, arena, uset_qn, "count", t_size_t);
    reg_method(reg, arena, uset_qn, "contains", t_bool);
    reg_method(reg, arena, uset_qn, "size", t_size_t);
    reg_method(reg, arena, uset_qn, "empty", t_bool);
    reg_method(reg, arena, uset_qn, "clear", t_void);

    // =========================================================================
    // Smart pointers: std.unique_ptr<T>, std.shared_ptr<T>, std.weak_ptr<T>
    // =========================================================================
    const char* uptr_qn = "std.unique_ptr";
    reg_type(reg, uptr_qn, "unique_ptr");
    reg_method(reg, arena, uptr_qn, "get", t_T_ptr);
    reg_method(reg, arena, uptr_qn, "reset", t_void);
    reg_method(reg, arena, uptr_qn, "release", t_T_ptr);
    reg_method(reg, arena, uptr_qn, "operator->", t_T_ptr);
    reg_method(reg, arena, uptr_qn, "operator*", t_T_ref);
    reg_method(reg, arena, uptr_qn, "operator bool", t_bool);
    reg_method(reg, arena, uptr_qn, "swap", t_void);

    const char* sptr_qn = "std.shared_ptr";
    reg_type(reg, sptr_qn, "shared_ptr");
    reg_method(reg, arena, sptr_qn, "get", t_T_ptr);
    reg_method(reg, arena, sptr_qn, "reset", t_void);
    reg_method(reg, arena, sptr_qn, "use_count", t_long);
    reg_method(reg, arena, sptr_qn, "unique", t_bool);
    reg_method(reg, arena, sptr_qn, "operator->", t_T_ptr);
    reg_method(reg, arena, sptr_qn, "operator*", t_T_ref);
    reg_method(reg, arena, sptr_qn, "operator bool", t_bool);
    reg_method(reg, arena, sptr_qn, "swap", t_void);

    const char* wptr_qn = "std.weak_ptr";
    reg_type(reg, wptr_qn, "weak_ptr");
    reg_method(reg, arena, wptr_qn, "lock", cbm_type_named(arena, sptr_qn));
    reg_method(reg, arena, wptr_qn, "use_count", t_long);
    reg_method(reg, arena, wptr_qn, "expired", t_bool);
    reg_method(reg, arena, wptr_qn, "reset", t_void);

    // std.make_unique / std.make_shared — free functions
    // Return type depends on template arg, registered as returning unique_ptr/shared_ptr
    reg_func(reg, arena, "std.make_unique", "make_unique",
        cbm_type_named(arena, uptr_qn));
    reg_func(reg, arena, "std.make_shared", "make_shared",
        cbm_type_named(arena, sptr_qn));

    // =========================================================================
    // std.optional<T>
    // =========================================================================
    const char* opt_qn = "std.optional";
    reg_type(reg, opt_qn, "optional");
    reg_method(reg, arena, opt_qn, "value", t_T_ref);
    reg_method(reg, arena, opt_qn, "has_value", t_bool);
    reg_method(reg, arena, opt_qn, "value_or", t_T);
    reg_method(reg, arena, opt_qn, "operator*", t_T_ref);
    reg_method(reg, arena, opt_qn, "operator->", t_T_ptr);
    reg_method(reg, arena, opt_qn, "operator bool", t_bool);
    reg_method(reg, arena, opt_qn, "reset", t_void);
    reg_method(reg, arena, opt_qn, "emplace", t_T_ref);

    // =========================================================================
    // std.pair<T1, T2>
    // =========================================================================
    const char* pair_qn = "std.pair";
    {
        static const char* pair_field_names[] = {"first", "second", NULL};
        const CBMType** pair_field_types = (const CBMType**)cbm_arena_alloc(arena,
            3 * sizeof(const CBMType*));
        pair_field_types[0] = cbm_type_type_param(arena, "T1");
        pair_field_types[1] = cbm_type_type_param(arena, "T2");
        pair_field_types[2] = NULL;
        reg_type_with_fields(reg, arena, pair_qn, "pair", pair_field_names, pair_field_types);
    }

    // =========================================================================
    // std.tuple<...>
    // =========================================================================
    reg_type(reg, "std.tuple", "tuple");
    reg_func(reg, arena, "std.get", "get", cbm_type_unknown());

    // =========================================================================
    // std.array<T, N>
    // =========================================================================
    const char* arr_qn = "std.array";
    reg_type(reg, arr_qn, "array");
    reg_method(reg, arena, arr_qn, "size", t_size_t);
    reg_method(reg, arena, arr_qn, "data", t_T_ptr);
    reg_method(reg, arena, arr_qn, "at", t_T_ref);
    reg_method(reg, arena, arr_qn, "front", t_T_ref);
    reg_method(reg, arena, arr_qn, "back", t_T_ref);
    reg_method(reg, arena, arr_qn, "operator[]", t_T_ref);
    reg_method(reg, arena, arr_qn, "begin", t_T_ptr);
    reg_method(reg, arena, arr_qn, "end", t_T_ptr);
    reg_method(reg, arena, arr_qn, "empty", t_bool);
    reg_method(reg, arena, arr_qn, "fill", t_void);

    // =========================================================================
    // std.function<F>
    // =========================================================================
    reg_type(reg, "std.function", "function");
    reg_method(reg, arena, "std.function", "operator()", cbm_type_unknown());
    reg_method(reg, arena, "std.function", "operator bool", t_bool);
    reg_method(reg, arena, "std.function", "target", t_void_ptr);

    // =========================================================================
    // I/O streams
    // =========================================================================
    const char* os_qn = "std.ostream";
    reg_type(reg, os_qn, "ostream");
    const CBMType* t_ostream_ref = cbm_type_reference(arena, cbm_type_named(arena, os_qn));
    reg_method(reg, arena, os_qn, "operator<<", t_ostream_ref);
    reg_method(reg, arena, os_qn, "write", t_ostream_ref);
    reg_method(reg, arena, os_qn, "flush", t_ostream_ref);
    reg_method(reg, arena, os_qn, "put", t_ostream_ref);
    reg_method(reg, arena, os_qn, "good", t_bool);
    reg_method(reg, arena, os_qn, "fail", t_bool);
    reg_method(reg, arena, os_qn, "bad", t_bool);
    reg_method(reg, arena, os_qn, "eof", t_bool);

    const char* is_qn = "std.istream";
    reg_type(reg, is_qn, "istream");
    const CBMType* t_istream_ref = cbm_type_reference(arena, cbm_type_named(arena, is_qn));
    reg_method(reg, arena, is_qn, "operator>>", t_istream_ref);
    reg_method(reg, arena, is_qn, "read", t_istream_ref);
    reg_method(reg, arena, is_qn, "getline", t_istream_ref);
    reg_method(reg, arena, is_qn, "get", t_int);
    reg_method(reg, arena, is_qn, "peek", t_int);
    reg_method(reg, arena, is_qn, "good", t_bool);
    reg_method(reg, arena, is_qn, "fail", t_bool);

    // Global stream objects
    reg_func(reg, arena, "std.cout", "cout", cbm_type_named(arena, os_qn));
    reg_func(reg, arena, "std.cin", "cin", cbm_type_named(arena, is_qn));
    reg_func(reg, arena, "std.cerr", "cerr", cbm_type_named(arena, os_qn));
    reg_func(reg, arena, "std.clog", "clog", cbm_type_named(arena, os_qn));

    // std.endl, std.flush — manipulators (treat as returning ostream ref)
    reg_func(reg, arena, "std.endl", "endl", t_ostream_ref);
    reg_func(reg, arena, "std.flush", "flush", t_ostream_ref);

    // =========================================================================
    // std.filesystem.path
    // =========================================================================
    const char* fspath_qn = "std.filesystem.path";
    reg_type(reg, fspath_qn, "path");
    const CBMType* t_path = cbm_type_named(arena, fspath_qn);
    reg_method(reg, arena, fspath_qn, "string", t_string);
    reg_method(reg, arena, fspath_qn, "c_str", t_char_ptr);
    reg_method(reg, arena, fspath_qn, "extension", t_path);
    reg_method(reg, arena, fspath_qn, "filename", t_path);
    reg_method(reg, arena, fspath_qn, "parent_path", t_path);
    reg_method(reg, arena, fspath_qn, "stem", t_path);
    reg_method(reg, arena, fspath_qn, "root_path", t_path);
    reg_method(reg, arena, fspath_qn, "is_absolute", t_bool);
    reg_method(reg, arena, fspath_qn, "is_relative", t_bool);
    reg_method(reg, arena, fspath_qn, "empty", t_bool);
    reg_method(reg, arena, fspath_qn, "exists", t_bool);
    reg_method(reg, arena, fspath_qn, "operator/", t_path);
    reg_method(reg, arena, fspath_qn, "operator/=", cbm_type_reference(arena, t_path));

    // =========================================================================
    // std.thread
    // =========================================================================
    const char* thread_qn = "std.thread";
    reg_type(reg, thread_qn, "thread");
    reg_method(reg, arena, thread_qn, "join", t_void);
    reg_method(reg, arena, thread_qn, "detach", t_void);
    reg_method(reg, arena, thread_qn, "joinable", t_bool);
    reg_method(reg, arena, thread_qn, "get_id", cbm_type_named(arena, "std.thread.id"));

    // =========================================================================
    // std.mutex / std.lock_guard / std.unique_lock
    // =========================================================================
    const char* mutex_qn = "std.mutex";
    reg_type(reg, mutex_qn, "mutex");
    reg_method(reg, arena, mutex_qn, "lock", t_void);
    reg_method(reg, arena, mutex_qn, "unlock", t_void);
    reg_method(reg, arena, mutex_qn, "try_lock", t_bool);

    reg_type(reg, "std.lock_guard", "lock_guard");
    reg_type(reg, "std.unique_lock", "unique_lock");
    reg_method(reg, arena, "std.unique_lock", "lock", t_void);
    reg_method(reg, arena, "std.unique_lock", "unlock", t_void);
    reg_method(reg, arena, "std.unique_lock", "try_lock", t_bool);
    reg_method(reg, arena, "std.unique_lock", "owns_lock", t_bool);

    // =========================================================================
    // std.chrono basics
    // =========================================================================
    reg_type(reg, "std.chrono.seconds", "seconds");
    reg_type(reg, "std.chrono.milliseconds", "milliseconds");
    reg_type(reg, "std.chrono.microseconds", "microseconds");
    reg_type(reg, "std.chrono.nanoseconds", "nanoseconds");
    reg_type(reg, "std.chrono.system_clock", "system_clock");
    reg_type(reg, "std.chrono.steady_clock", "steady_clock");
    reg_func(reg, arena, "std.chrono.system_clock.now", "now",
        cbm_type_named(arena, "std.chrono.time_point"));
    reg_func(reg, arena, "std.chrono.steady_clock.now", "now",
        cbm_type_named(arena, "std.chrono.time_point"));

    // =========================================================================
    // std.algorithm free functions
    // =========================================================================
    reg_func(reg, arena, "std.sort", "sort", t_void);
    reg_func(reg, arena, "std.find", "find", cbm_type_unknown());
    reg_func(reg, arena, "std.find_if", "find_if", cbm_type_unknown());
    reg_func(reg, arena, "std.for_each", "for_each", cbm_type_unknown());
    reg_func(reg, arena, "std.transform", "transform", cbm_type_unknown());
    reg_func(reg, arena, "std.copy", "copy", cbm_type_unknown());
    reg_func(reg, arena, "std.move", "move", cbm_type_unknown());
    reg_func(reg, arena, "std.swap", "swap", t_void);
    reg_func(reg, arena, "std.min", "min", cbm_type_unknown());
    reg_func(reg, arena, "std.max", "max", cbm_type_unknown());
    reg_func(reg, arena, "std.accumulate", "accumulate", cbm_type_unknown());
    reg_func(reg, arena, "std.count", "count", t_size_t);
    reg_func(reg, arena, "std.count_if", "count_if", t_size_t);
    reg_func(reg, arena, "std.remove", "remove", cbm_type_unknown());
    reg_func(reg, arena, "std.remove_if", "remove_if", cbm_type_unknown());
    reg_func(reg, arena, "std.reverse", "reverse", t_void);
    reg_func(reg, arena, "std.unique", "unique", cbm_type_unknown());
    reg_func(reg, arena, "std.lower_bound", "lower_bound", cbm_type_unknown());
    reg_func(reg, arena, "std.upper_bound", "upper_bound", cbm_type_unknown());
    reg_func(reg, arena, "std.binary_search", "binary_search", t_bool);

    // std.to_string
    reg_func(reg, arena, "std.to_string", "to_string", t_string);
    reg_func(reg, arena, "std.stoi", "stoi", t_int);
    reg_func(reg, arena, "std.stol", "stol", t_long);
    reg_func(reg, arena, "std.stof", "stof", cbm_type_builtin(arena, "float"));
    reg_func(reg, arena, "std.stod", "stod", cbm_type_builtin(arena, "double"));

    // =========================================================================
    // =========================================================================
    // std.variant<T...>
    // =========================================================================
    reg_type(reg, "std.variant", "variant");
    reg_method(reg, arena, "std.variant", "index", t_size_t);
    reg_method(reg, arena, "std.variant", "valueless_by_exception", t_bool);
    reg_func(reg, arena, "std.holds_alternative", "holds_alternative", t_bool);
    reg_func(reg, arena, "std.get_if", "get_if", t_T_ptr);

    // =========================================================================
    // std.any
    // =========================================================================
    reg_type(reg, "std.any", "any");
    reg_method(reg, arena, "std.any", "has_value", t_bool);
    reg_method(reg, arena, "std.any", "type", cbm_type_unknown()); // type_info
    reg_method(reg, arena, "std.any", "reset", t_void);
    reg_func(reg, arena, "std.any_cast", "any_cast", t_T);
    reg_func(reg, arena, "std.make_any", "make_any", cbm_type_named(arena, "std.any"));

    // =========================================================================
    // std.regex / std.smatch
    // =========================================================================
    reg_type(reg, "std.regex", "regex");
    reg_type(reg, "std.smatch", "smatch");
    reg_method(reg, arena, "std.smatch", "size", t_size_t);
    reg_method(reg, arena, "std.smatch", "empty", t_bool);
    reg_method(reg, arena, "std.smatch", "str", t_string);
    reg_method(reg, arena, "std.smatch", "prefix", cbm_type_unknown());
    reg_method(reg, arena, "std.smatch", "suffix", cbm_type_unknown());
    reg_func(reg, arena, "std.regex_search", "regex_search", t_bool);
    reg_func(reg, arena, "std.regex_match", "regex_match", t_bool);
    reg_func(reg, arena, "std.regex_replace", "regex_replace", t_string);

    // =========================================================================
    // std.pair<K,V>
    // =========================================================================
    reg_type(reg, "std.pair", "pair");
    reg_func(reg, arena, "std.make_pair", "make_pair",
        cbm_type_named(arena, "std.pair"));

    // =========================================================================
    // <numeric> algorithms
    // =========================================================================
    reg_func(reg, arena, "std.accumulate", "accumulate", t_T);
    reg_func(reg, arena, "std.inner_product", "inner_product", t_T);
    reg_func(reg, arena, "std.iota", "iota", t_void);
    reg_func(reg, arena, "std.partial_sum", "partial_sum", cbm_type_unknown());
    reg_func(reg, arena, "std.adjacent_difference", "adjacent_difference", cbm_type_unknown());
    reg_func(reg, arena, "std.reduce", "reduce", t_T);
    reg_func(reg, arena, "std.transform_reduce", "transform_reduce", t_T);

    // =========================================================================
    // <iterator> functions
    // =========================================================================
    reg_func(reg, arena, "std.advance", "advance", t_void);
    reg_func(reg, arena, "std.distance", "distance", cbm_type_builtin(arena, "ptrdiff_t"));
    reg_func(reg, arena, "std.next", "next", cbm_type_unknown()); // iterator
    reg_func(reg, arena, "std.prev", "prev", cbm_type_unknown());
    reg_func(reg, arena, "std.begin", "begin", cbm_type_unknown());
    reg_func(reg, arena, "std.end", "end", cbm_type_unknown());
    reg_func(reg, arena, "std.rbegin", "rbegin", cbm_type_unknown());
    reg_func(reg, arena, "std.rend", "rend", cbm_type_unknown());

    // =========================================================================
    // <algorithm> — additional entries with better return types
    // =========================================================================
    reg_func(reg, arena, "std.for_each", "for_each", cbm_type_unknown());
    reg_func(reg, arena, "std.all_of", "all_of", t_bool);
    reg_func(reg, arena, "std.any_of", "any_of", t_bool);
    reg_func(reg, arena, "std.none_of", "none_of", t_bool);
    reg_func(reg, arena, "std.min", "min", t_T_ref);
    reg_func(reg, arena, "std.max", "max", t_T_ref);
    reg_func(reg, arena, "std.min_element", "min_element", cbm_type_unknown());
    reg_func(reg, arena, "std.max_element", "max_element", cbm_type_unknown());
    reg_func(reg, arena, "std.minmax", "minmax",
        cbm_type_named(arena, "std.pair"));
    reg_func(reg, arena, "std.clamp", "clamp", t_T_ref);
    reg_func(reg, arena, "std.swap", "swap", t_void);
    reg_func(reg, arena, "std.fill", "fill", t_void);
    reg_func(reg, arena, "std.replace", "replace", t_void);
    reg_func(reg, arena, "std.replace_if", "replace_if", t_void);
    reg_func(reg, arena, "std.equal", "equal", t_bool);
    reg_func(reg, arena, "std.mismatch", "mismatch",
        cbm_type_named(arena, "std.pair"));
    reg_func(reg, arena, "std.lexicographical_compare", "lexicographical_compare", t_bool);
    reg_func(reg, arena, "std.partition", "partition", cbm_type_unknown());
    reg_func(reg, arena, "std.stable_partition", "stable_partition", cbm_type_unknown());
    reg_func(reg, arena, "std.nth_element", "nth_element", t_void);
    reg_func(reg, arena, "std.partial_sort", "partial_sort", t_void);
    reg_func(reg, arena, "std.stable_sort", "stable_sort", t_void);
    reg_func(reg, arena, "std.is_sorted", "is_sorted", t_bool);
    reg_func(reg, arena, "std.merge", "merge", cbm_type_unknown());
    reg_func(reg, arena, "std.includes", "includes", t_bool);
    reg_func(reg, arena, "std.set_union", "set_union", cbm_type_unknown());
    reg_func(reg, arena, "std.set_intersection", "set_intersection", cbm_type_unknown());
    reg_func(reg, arena, "std.set_difference", "set_difference", cbm_type_unknown());
    reg_func(reg, arena, "std.generate", "generate", t_void);
    reg_func(reg, arena, "std.generate_n", "generate_n", cbm_type_unknown());

    // =========================================================================
    // <memory> — additional
    // =========================================================================
    reg_func(reg, arena, "std.addressof", "addressof", t_T_ptr);
    reg_func(reg, arena, "std.allocate_shared", "allocate_shared",
        cbm_type_named(arena, sptr_qn));

    // =========================================================================
    // <utility>
    // =========================================================================
    reg_func(reg, arena, "std.move", "move", t_T);
    reg_func(reg, arena, "std.forward", "forward", t_T);
    reg_func(reg, arena, "std.exchange", "exchange", t_T);
    reg_func(reg, arena, "std.declval", "declval", t_T);
    reg_func(reg, arena, "std.as_const", "as_const", t_T_ref);

    // =========================================================================
    // <type_traits> — common query functions
    // =========================================================================
    reg_func(reg, arena, "std.is_same_v", "is_same_v", t_bool);
    reg_func(reg, arena, "std.is_base_of_v", "is_base_of_v", t_bool);

    // =========================================================================
    // std.deque<T>
    // =========================================================================
    const char* deq_qn = "std.deque";
    reg_type(reg, deq_qn, "deque");
    reg_method(reg, arena, deq_qn, "push_back", t_void);
    reg_method(reg, arena, deq_qn, "push_front", t_void);
    reg_method(reg, arena, deq_qn, "pop_back", t_void);
    reg_method(reg, arena, deq_qn, "pop_front", t_void);
    reg_method(reg, arena, deq_qn, "front", t_T_ref);
    reg_method(reg, arena, deq_qn, "back", t_T_ref);
    reg_method(reg, arena, deq_qn, "size", t_size_t);
    reg_method(reg, arena, deq_qn, "empty", t_bool);
    reg_method(reg, arena, deq_qn, "clear", t_void);
    reg_method(reg, arena, deq_qn, "at", t_T_ref);

    // =========================================================================
    // std.list<T>
    // =========================================================================
    const char* list_qn = "std.list";
    reg_type(reg, list_qn, "list");
    reg_method(reg, arena, list_qn, "push_back", t_void);
    reg_method(reg, arena, list_qn, "push_front", t_void);
    reg_method(reg, arena, list_qn, "pop_back", t_void);
    reg_method(reg, arena, list_qn, "pop_front", t_void);
    reg_method(reg, arena, list_qn, "front", t_T_ref);
    reg_method(reg, arena, list_qn, "back", t_T_ref);
    reg_method(reg, arena, list_qn, "size", t_size_t);
    reg_method(reg, arena, list_qn, "empty", t_bool);
    reg_method(reg, arena, list_qn, "clear", t_void);
    reg_method(reg, arena, list_qn, "sort", t_void);
    reg_method(reg, arena, list_qn, "reverse", t_void);
    reg_method(reg, arena, list_qn, "merge", t_void);
    reg_method(reg, arena, list_qn, "unique", t_void);

    // =========================================================================
    // std.stack<T> / std.queue<T> / std.priority_queue<T>
    // =========================================================================
    const char* stack_qn = "std.stack";
    reg_type(reg, stack_qn, "stack");
    reg_method(reg, arena, stack_qn, "push", t_void);
    reg_method(reg, arena, stack_qn, "pop", t_void);
    reg_method(reg, arena, stack_qn, "top", t_T_ref);
    reg_method(reg, arena, stack_qn, "size", t_size_t);
    reg_method(reg, arena, stack_qn, "empty", t_bool);

    const char* queue_qn = "std.queue";
    reg_type(reg, queue_qn, "queue");
    reg_method(reg, arena, queue_qn, "push", t_void);
    reg_method(reg, arena, queue_qn, "pop", t_void);
    reg_method(reg, arena, queue_qn, "front", t_T_ref);
    reg_method(reg, arena, queue_qn, "back", t_T_ref);
    reg_method(reg, arena, queue_qn, "size", t_size_t);
    reg_method(reg, arena, queue_qn, "empty", t_bool);

    const char* pq_qn = "std.priority_queue";
    reg_type(reg, pq_qn, "priority_queue");
    reg_method(reg, arena, pq_qn, "push", t_void);
    reg_method(reg, arena, pq_qn, "pop", t_void);
    reg_method(reg, arena, pq_qn, "top", t_T_ref);
    reg_method(reg, arena, pq_qn, "size", t_size_t);
    reg_method(reg, arena, pq_qn, "empty", t_bool);

    // =========================================================================
    // std.bitset<N>
    // =========================================================================
    reg_type(reg, "std.bitset", "bitset");
    reg_method(reg, arena, "std.bitset", "set", cbm_type_named(arena, "std.bitset"));
    reg_method(reg, arena, "std.bitset", "reset", cbm_type_named(arena, "std.bitset"));
    reg_method(reg, arena, "std.bitset", "flip", cbm_type_named(arena, "std.bitset"));
    reg_method(reg, arena, "std.bitset", "test", t_bool);
    reg_method(reg, arena, "std.bitset", "count", t_size_t);
    reg_method(reg, arena, "std.bitset", "size", t_size_t);
    reg_method(reg, arena, "std.bitset", "any", t_bool);
    reg_method(reg, arena, "std.bitset", "none", t_bool);
    reg_method(reg, arena, "std.bitset", "all", t_bool);
    reg_method(reg, arena, "std.bitset", "to_string", t_string);
    reg_method(reg, arena, "std.bitset", "to_ulong", cbm_type_builtin(arena, "unsigned long"));

    // =========================================================================
    // std.stringstream / std.ostringstream / std.istringstream
    // =========================================================================
    const char* ss_qn = "std.stringstream";
    reg_type(reg, ss_qn, "stringstream");
    reg_method(reg, arena, ss_qn, "str", t_string);
    reg_method(reg, arena, ss_qn, "clear", t_void);

    const char* oss_qn = "std.ostringstream";
    reg_type(reg, oss_qn, "ostringstream");
    reg_method(reg, arena, oss_qn, "str", t_string);
    reg_method(reg, arena, oss_qn, "clear", t_void);

    const char* iss_qn = "std.istringstream";
    reg_type(reg, iss_qn, "istringstream");
    reg_method(reg, arena, iss_qn, "str", t_string);
    reg_method(reg, arena, iss_qn, "clear", t_void);

    // =========================================================================
    // std.filesystem — additional
    // =========================================================================
    reg_func(reg, arena, "std.filesystem.exists", "exists", t_bool);
    reg_func(reg, arena, "std.filesystem.is_directory", "is_directory", t_bool);
    reg_func(reg, arena, "std.filesystem.is_regular_file", "is_regular_file", t_bool);
    reg_func(reg, arena, "std.filesystem.file_size", "file_size", cbm_type_builtin(arena, "uintmax_t"));
    reg_func(reg, arena, "std.filesystem.create_directory", "create_directory", t_bool);
    reg_func(reg, arena, "std.filesystem.create_directories", "create_directories", t_bool);
    reg_func(reg, arena, "std.filesystem.remove", "remove", t_bool);
    reg_func(reg, arena, "std.filesystem.remove_all", "remove_all", cbm_type_builtin(arena, "uintmax_t"));
    reg_func(reg, arena, "std.filesystem.copy", "copy", t_void);
    reg_func(reg, arena, "std.filesystem.rename", "rename", t_void);
    reg_func(reg, arena, "std.filesystem.current_path", "current_path",
        cbm_type_named(arena, fspath_qn));
    reg_func(reg, arena, "std.filesystem.absolute", "absolute",
        cbm_type_named(arena, fspath_qn));
    reg_func(reg, arena, "std.filesystem.canonical", "canonical",
        cbm_type_named(arena, fspath_qn));
    reg_func(reg, arena, "std.filesystem.relative", "relative",
        cbm_type_named(arena, fspath_qn));
    reg_func(reg, arena, "std.filesystem.temp_directory_path", "temp_directory_path",
        cbm_type_named(arena, fspath_qn));

    const char* direntry_qn = "std.filesystem.directory_entry";
    reg_type(reg, direntry_qn, "directory_entry");
    reg_method(reg, arena, direntry_qn, "path", cbm_type_named(arena, fspath_qn));
    reg_method(reg, arena, direntry_qn, "exists", t_bool);
    reg_method(reg, arena, direntry_qn, "is_directory", t_bool);
    reg_method(reg, arena, direntry_qn, "is_regular_file", t_bool);
    reg_method(reg, arena, direntry_qn, "file_size", cbm_type_builtin(arena, "uintmax_t"));

    // =========================================================================
    // std.chrono — additional
    // =========================================================================
    reg_type(reg, "std.chrono.time_point", "time_point");
    reg_method(reg, arena, "std.chrono.time_point", "time_since_epoch", cbm_type_unknown());
    reg_method(reg, arena, "std.chrono.system_clock", "now",
        cbm_type_named(arena, "std.chrono.time_point"));
    reg_method(reg, arena, "std.chrono.steady_clock", "now",
        cbm_type_named(arena, "std.chrono.time_point"));
    reg_func(reg, arena, "std.chrono.duration_cast", "duration_cast", cbm_type_unknown());

    // =========================================================================
    // Boost smart pointers (common in older codebases)
    // =========================================================================
    const char* bsptr_qn = "boost.shared_ptr";
    reg_type(reg, bsptr_qn, "shared_ptr");
    reg_method(reg, arena, bsptr_qn, "get", t_T_ptr);
    reg_method(reg, arena, bsptr_qn, "reset", t_void);
    reg_method(reg, arena, bsptr_qn, "use_count", t_long);
    reg_method(reg, arena, bsptr_qn, "operator->", t_T_ptr);
    reg_method(reg, arena, bsptr_qn, "operator*", t_T_ref);

    const char* bscoped_qn = "boost.scoped_ptr";
    reg_type(reg, bscoped_qn, "scoped_ptr");
    reg_method(reg, arena, bscoped_qn, "get", t_T_ptr);
    reg_method(reg, arena, bscoped_qn, "reset", t_void);
    reg_method(reg, arena, bscoped_qn, "operator->", t_T_ptr);
    reg_method(reg, arena, bscoped_qn, "operator*", t_T_ref);

    // =========================================================================
    // Boost optional / filesystem
    // =========================================================================
    const char* bopt_qn = "boost.optional";
    reg_type(reg, bopt_qn, "optional");
    reg_method(reg, arena, bopt_qn, "value", t_T_ref);
    reg_method(reg, arena, bopt_qn, "get", t_T_ref);
    reg_method(reg, arena, bopt_qn, "value_or", t_T);
    reg_method(reg, arena, bopt_qn, "is_initialized", t_bool);
    reg_method(reg, arena, bopt_qn, "operator*", t_T_ref);
    reg_method(reg, arena, bopt_qn, "operator->", t_T_ptr);

    const char* bfspath_qn = "boost.filesystem.path";
    reg_type(reg, bfspath_qn, "path");
    reg_method(reg, arena, bfspath_qn, "string", t_string);
    reg_method(reg, arena, bfspath_qn, "parent_path", cbm_type_named(arena, bfspath_qn));
    reg_method(reg, arena, bfspath_qn, "filename", cbm_type_named(arena, bfspath_qn));
    reg_method(reg, arena, bfspath_qn, "extension", cbm_type_named(arena, bfspath_qn));
    reg_method(reg, arena, bfspath_qn, "stem", cbm_type_named(arena, bfspath_qn));
    reg_method(reg, arena, bfspath_qn, "empty", t_bool);
    reg_method(reg, arena, bfspath_qn, "exists", t_bool);

    // =========================================================================
    // Protobuf (google.protobuf.Message)
    // =========================================================================
    const char* pb_msg_qn = "google.protobuf.Message";
    reg_type(reg, pb_msg_qn, "Message");
    reg_method(reg, arena, pb_msg_qn, "SerializeToString", t_bool);
    reg_method(reg, arena, pb_msg_qn, "SerializeAsString", t_string);
    reg_method(reg, arena, pb_msg_qn, "ParseFromString", t_bool);
    reg_method(reg, arena, pb_msg_qn, "ParseFromArray", t_bool);
    reg_method(reg, arena, pb_msg_qn, "DebugString", t_string);
    reg_method(reg, arena, pb_msg_qn, "ShortDebugString", t_string);
    reg_method(reg, arena, pb_msg_qn, "ByteSizeLong", t_size_t);
    reg_method(reg, arena, pb_msg_qn, "IsInitialized", t_bool);
    reg_method(reg, arena, pb_msg_qn, "Clear", t_void);
    reg_method(reg, arena, pb_msg_qn, "CopyFrom", t_void);
    reg_method(reg, arena, pb_msg_qn, "MergeFrom", t_void);
    reg_method(reg, arena, pb_msg_qn, "GetTypeName", t_string);
    // MessageLite is base of Message
    reg_type(reg, "google.protobuf.MessageLite", "MessageLite");

    // =========================================================================
    // Abseil (absl.)
    // =========================================================================
    const char* absv_qn = "absl.string_view";
    reg_type(reg, absv_qn, "string_view");
    reg_method(reg, arena, absv_qn, "data", cbm_type_pointer(arena, cbm_type_builtin(arena, "char")));
    reg_method(reg, arena, absv_qn, "size", t_size_t);
    reg_method(reg, arena, absv_qn, "length", t_size_t);
    reg_method(reg, arena, absv_qn, "empty", t_bool);
    reg_method(reg, arena, absv_qn, "substr", cbm_type_named(arena, absv_qn));

    const char* abstat_qn = "absl.Status";
    reg_type(reg, abstat_qn, "Status");
    reg_method(reg, arena, abstat_qn, "ok", t_bool);
    reg_method(reg, arena, abstat_qn, "code", cbm_type_unknown());
    reg_method(reg, arena, abstat_qn, "message", cbm_type_named(arena, absv_qn));
    reg_method(reg, arena, abstat_qn, "ToString", t_string);
    reg_func(reg, arena, "absl.OkStatus", "OkStatus", cbm_type_named(arena, abstat_qn));

    const char* absor_qn = "absl.StatusOr";
    reg_type(reg, absor_qn, "StatusOr");
    reg_method(reg, arena, absor_qn, "ok", t_bool);
    reg_method(reg, arena, absor_qn, "status", cbm_type_named(arena, abstat_qn));
    reg_method(reg, arena, absor_qn, "value", t_T_ref);
    reg_method(reg, arena, absor_qn, "operator*", t_T_ref);
    reg_method(reg, arena, absor_qn, "operator->", t_T_ptr);

    const char* abfhm_qn = "absl.flat_hash_map";
    reg_type(reg, abfhm_qn, "flat_hash_map");
    reg_method(reg, arena, abfhm_qn, "find", cbm_type_unknown());
    reg_method(reg, arena, abfhm_qn, "contains", t_bool);
    reg_method(reg, arena, abfhm_qn, "size", t_size_t);
    reg_method(reg, arena, abfhm_qn, "empty", t_bool);
    reg_method(reg, arena, abfhm_qn, "clear", t_void);
    reg_method(reg, arena, abfhm_qn, "insert", cbm_type_unknown());
    reg_method(reg, arena, abfhm_qn, "erase", t_size_t);
    reg_method(reg, arena, abfhm_qn, "count", t_size_t);

    const char* abfhs_qn = "absl.flat_hash_set";
    reg_type(reg, abfhs_qn, "flat_hash_set");
    reg_method(reg, arena, abfhs_qn, "contains", t_bool);
    reg_method(reg, arena, abfhs_qn, "size", t_size_t);
    reg_method(reg, arena, abfhs_qn, "empty", t_bool);
    reg_method(reg, arena, abfhs_qn, "clear", t_void);
    reg_method(reg, arena, abfhs_qn, "insert", cbm_type_unknown());
    reg_method(reg, arena, abfhs_qn, "erase", t_size_t);
    reg_method(reg, arena, abfhs_qn, "count", t_size_t);

    const char* abspan_qn = "absl.Span";
    reg_type(reg, abspan_qn, "Span");
    reg_method(reg, arena, abspan_qn, "data", t_T_ptr);
    reg_method(reg, arena, abspan_qn, "size", t_size_t);
    reg_method(reg, arena, abspan_qn, "empty", t_bool);
    reg_method(reg, arena, abspan_qn, "front", t_T_ref);
    reg_method(reg, arena, abspan_qn, "back", t_T_ref);
    reg_method(reg, arena, abspan_qn, "at", t_T_ref);
    reg_method(reg, arena, abspan_qn, "subspan", cbm_type_named(arena, abspan_qn));

    // absl string functions
    reg_func(reg, arena, "absl.StrCat", "StrCat", t_string);
    reg_func(reg, arena, "absl.StrAppend", "StrAppend", t_void);
    reg_func(reg, arena, "absl.StrJoin", "StrJoin", t_string);
    reg_func(reg, arena, "absl.StrSplit", "StrSplit",
        cbm_type_named(arena, "std.vector"));
    reg_func(reg, arena, "absl.StrFormat", "StrFormat", t_string);
    reg_func(reg, arena, "absl.Substitute", "Substitute", t_string);

    // =========================================================================
    // gRPC
    // =========================================================================
    const char* grpc_status_qn = "grpc.Status";
    reg_type(reg, grpc_status_qn, "Status");
    reg_method(reg, arena, grpc_status_qn, "ok", t_bool);
    reg_method(reg, arena, grpc_status_qn, "error_code", cbm_type_unknown());
    reg_method(reg, arena, grpc_status_qn, "error_message", t_string);
    reg_func(reg, arena, "grpc.Status.OK", "OK", cbm_type_named(arena, grpc_status_qn));

    const char* grpc_sb_qn = "grpc.ServerBuilder";
    reg_type(reg, grpc_sb_qn, "ServerBuilder");
    reg_method(reg, arena, grpc_sb_qn, "AddListeningPort", cbm_type_named(arena, grpc_sb_qn));
    reg_method(reg, arena, grpc_sb_qn, "RegisterService", cbm_type_named(arena, grpc_sb_qn));
    reg_method(reg, arena, grpc_sb_qn, "BuildAndStart", cbm_type_unknown());

    const char* grpc_chan_qn = "grpc.Channel";
    reg_type(reg, grpc_chan_qn, "Channel");
    reg_func(reg, arena, "grpc.CreateChannel", "CreateChannel",
        cbm_type_named(arena, grpc_chan_qn));

    const char* grpc_ctx_qn = "grpc.ClientContext";
    reg_type(reg, grpc_ctx_qn, "ClientContext");
    reg_method(reg, arena, grpc_ctx_qn, "set_deadline", t_void);
    reg_method(reg, arena, grpc_ctx_qn, "AddMetadata", t_void);

    // =========================================================================
    // spdlog
    // =========================================================================
    const char* spdlog_qn = "spdlog.logger";
    reg_type(reg, spdlog_qn, "logger");
    reg_method(reg, arena, spdlog_qn, "info", t_void);
    reg_method(reg, arena, spdlog_qn, "warn", t_void);
    reg_method(reg, arena, spdlog_qn, "error", t_void);
    reg_method(reg, arena, spdlog_qn, "debug", t_void);
    reg_method(reg, arena, spdlog_qn, "trace", t_void);
    reg_method(reg, arena, spdlog_qn, "critical", t_void);
    reg_method(reg, arena, spdlog_qn, "set_level", t_void);
    reg_method(reg, arena, spdlog_qn, "flush", t_void);
    reg_func(reg, arena, "spdlog.info", "info", t_void);
    reg_func(reg, arena, "spdlog.warn", "warn", t_void);
    reg_func(reg, arena, "spdlog.error", "error", t_void);
    reg_func(reg, arena, "spdlog.debug", "debug", t_void);
    reg_func(reg, arena, "spdlog.trace", "trace", t_void);
    reg_func(reg, arena, "spdlog.critical", "critical", t_void);
    reg_func(reg, arena, "spdlog.set_level", "set_level", t_void);
    reg_func(reg, arena, "spdlog.get", "get",
        cbm_type_named(arena, spdlog_qn));

    // =========================================================================
    // Qt basics
    // =========================================================================
    const char* qobj_qn = "QObject";
    reg_type(reg, qobj_qn, "QObject");
    reg_method(reg, arena, qobj_qn, "parent", cbm_type_pointer(arena, cbm_type_named(arena, qobj_qn)));
    reg_method(reg, arena, qobj_qn, "children", cbm_type_unknown());
    reg_method(reg, arena, qobj_qn, "objectName", cbm_type_named(arena, "QString"));
    reg_method(reg, arena, qobj_qn, "setObjectName", t_void);
    reg_method(reg, arena, qobj_qn, "deleteLater", t_void);

    const char* qstr_qn = "QString";
    reg_type(reg, qstr_qn, "QString");
    reg_method(reg, arena, qstr_qn, "toStdString", t_string);
    reg_method(reg, arena, qstr_qn, "toUtf8", cbm_type_unknown());
    reg_method(reg, arena, qstr_qn, "toLatin1", cbm_type_unknown());
    reg_method(reg, arena, qstr_qn, "size", t_int);
    reg_method(reg, arena, qstr_qn, "length", t_int);
    reg_method(reg, arena, qstr_qn, "isEmpty", t_bool);
    reg_method(reg, arena, qstr_qn, "contains", t_bool);
    reg_method(reg, arena, qstr_qn, "startsWith", t_bool);
    reg_method(reg, arena, qstr_qn, "endsWith", t_bool);
    reg_method(reg, arena, qstr_qn, "trimmed", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "simplified", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "toLower", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "toUpper", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "arg", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "split", cbm_type_unknown());
    reg_method(reg, arena, qstr_qn, "replace", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "mid", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "left", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "right", cbm_type_named(arena, qstr_qn));
    reg_method(reg, arena, qstr_qn, "toInt", t_int);
    reg_method(reg, arena, qstr_qn, "toDouble", cbm_type_builtin(arena, "double"));
    reg_func(reg, arena, "QString.number", "number", cbm_type_named(arena, qstr_qn));
    reg_func(reg, arena, "QString.fromStdString", "fromStdString", cbm_type_named(arena, qstr_qn));
    reg_func(reg, arena, "QString.fromUtf8", "fromUtf8", cbm_type_named(arena, qstr_qn));

    const char* qwidget_qn = "QWidget";
    reg_type(reg, qwidget_qn, "QWidget");
    reg_method(reg, arena, qwidget_qn, "show", t_void);
    reg_method(reg, arena, qwidget_qn, "hide", t_void);
    reg_method(reg, arena, qwidget_qn, "close", t_bool);
    reg_method(reg, arena, qwidget_qn, "setVisible", t_void);
    reg_method(reg, arena, qwidget_qn, "isVisible", t_bool);
    reg_method(reg, arena, qwidget_qn, "resize", t_void);
    reg_method(reg, arena, qwidget_qn, "setWindowTitle", t_void);
    reg_method(reg, arena, qwidget_qn, "update", t_void);
    reg_method(reg, arena, qwidget_qn, "repaint", t_void);
    reg_method(reg, arena, qwidget_qn, "setLayout", t_void);

}
