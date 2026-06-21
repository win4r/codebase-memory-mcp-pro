// C stdlib type information for C/C++ LSP type resolver.

#include "../type_rep.h"
#include "../type_registry.h"
#include <string.h>

// Helper macros for concise registration
#define REG_FUNC(qn, short, ret_type) do { \
    memset(&rf, 0, sizeof(rf)); \
    rf.min_params = -1; \
    rf.qualified_name = (qn); \
    rf.short_name = (short); \
    rf.signature = cbm_type_func(arena, NULL, NULL, (const CBMType*[]){(ret_type), NULL}); \
    cbm_registry_add_func(reg, rf); \
} while(0)

#define REG_TYPE(qn, short) do { \
    memset(&rt, 0, sizeof(rt)); \
    rt.qualified_name = (qn); \
    rt.short_name = (short); \
    cbm_registry_add_type(reg, rt); \
} while(0)

void cbm_c_stdlib_register(CBMTypeRegistry* reg, CBMArena* arena) {
    CBMRegisteredFunc rf;
    CBMRegisteredType rt;

    const CBMType* t_int = cbm_type_builtin(arena, "int");
    const CBMType* t_size_t = cbm_type_builtin(arena, "size_t");
    const CBMType* t_double = cbm_type_builtin(arena, "double");
    const CBMType* t_void = cbm_type_builtin(arena, "void");
    const CBMType* t_char_ptr = cbm_type_pointer(arena, cbm_type_builtin(arena, "char"));
    const CBMType* t_void_ptr = cbm_type_pointer(arena, t_void);

    // FILE type
    REG_TYPE("FILE", "FILE");
    const CBMType* t_file_ptr = cbm_type_pointer(arena, cbm_type_named(arena, "FILE"));

    // stdio.h
    REG_FUNC("fopen", "fopen", t_file_ptr);
    REG_FUNC("fclose", "fclose", t_int);
    REG_FUNC("fprintf", "fprintf", t_int);
    REG_FUNC("fread", "fread", t_size_t);
    REG_FUNC("fwrite", "fwrite", t_size_t);
    REG_FUNC("printf", "printf", t_int);
    REG_FUNC("scanf", "scanf", t_int);
    REG_FUNC("fgets", "fgets", t_char_ptr);
    REG_FUNC("fputs", "fputs", t_int);
    REG_FUNC("fseek", "fseek", t_int);
    REG_FUNC("ftell", "ftell", cbm_type_builtin(arena, "long"));
    REG_FUNC("rewind", "rewind", t_void);
    REG_FUNC("feof", "feof", t_int);
    REG_FUNC("ferror", "ferror", t_int);
    REG_FUNC("snprintf", "snprintf", t_int);
    REG_FUNC("sprintf", "sprintf", t_int);
    REG_FUNC("sscanf", "sscanf", t_int);
    REG_FUNC("getchar", "getchar", t_int);
    REG_FUNC("putchar", "putchar", t_int);
    REG_FUNC("puts", "puts", t_int);
    REG_FUNC("perror", "perror", t_void);

    // stdlib.h
    REG_FUNC("malloc", "malloc", t_void_ptr);
    REG_FUNC("calloc", "calloc", t_void_ptr);
    REG_FUNC("realloc", "realloc", t_void_ptr);
    REG_FUNC("free", "free", t_void);
    REG_FUNC("atoi", "atoi", t_int);
    REG_FUNC("atof", "atof", t_double);
    REG_FUNC("atol", "atol", cbm_type_builtin(arena, "long"));
    REG_FUNC("strtol", "strtol", cbm_type_builtin(arena, "long"));
    REG_FUNC("strtod", "strtod", t_double);
    REG_FUNC("exit", "exit", t_void);
    REG_FUNC("abort", "abort", t_void);
    REG_FUNC("abs", "abs", t_int);
    REG_FUNC("rand", "rand", t_int);
    REG_FUNC("srand", "srand", t_void);
    REG_FUNC("qsort", "qsort", t_void);
    REG_FUNC("bsearch", "bsearch", t_void_ptr);
    REG_FUNC("getenv", "getenv", t_char_ptr);
    REG_FUNC("system", "system", t_int);

    // string.h
    REG_FUNC("strlen", "strlen", t_size_t);
    REG_FUNC("strcmp", "strcmp", t_int);
    REG_FUNC("strncmp", "strncmp", t_int);
    REG_FUNC("strcpy", "strcpy", t_char_ptr);
    REG_FUNC("strncpy", "strncpy", t_char_ptr);
    REG_FUNC("strcat", "strcat", t_char_ptr);
    REG_FUNC("strncat", "strncat", t_char_ptr);
    REG_FUNC("strchr", "strchr", t_char_ptr);
    REG_FUNC("strrchr", "strrchr", t_char_ptr);
    REG_FUNC("strstr", "strstr", t_char_ptr);
    REG_FUNC("strtok", "strtok", t_char_ptr);
    REG_FUNC("memcpy", "memcpy", t_void_ptr);
    REG_FUNC("memmove", "memmove", t_void_ptr);
    REG_FUNC("memset", "memset", t_void_ptr);
    REG_FUNC("memcmp", "memcmp", t_int);

    // ctype.h
    REG_FUNC("isalpha", "isalpha", t_int);
    REG_FUNC("isdigit", "isdigit", t_int);
    REG_FUNC("isalnum", "isalnum", t_int);
    REG_FUNC("isspace", "isspace", t_int);
    REG_FUNC("isupper", "isupper", t_int);
    REG_FUNC("islower", "islower", t_int);
    REG_FUNC("toupper", "toupper", t_int);
    REG_FUNC("tolower", "tolower", t_int);

    // math.h
    REG_FUNC("sqrt", "sqrt", t_double);
    REG_FUNC("pow", "pow", t_double);
    REG_FUNC("sin", "sin", t_double);
    REG_FUNC("cos", "cos", t_double);
    REG_FUNC("tan", "tan", t_double);
    REG_FUNC("exp", "exp", t_double);
    REG_FUNC("log", "log", t_double);
    REG_FUNC("log10", "log10", t_double);
    REG_FUNC("fabs", "fabs", t_double);
    REG_FUNC("ceil", "ceil", t_double);
    REG_FUNC("floor", "floor", t_double);
    REG_FUNC("round", "round", t_double);

    // assert.h / signal.h / time.h basics
    REG_FUNC("assert", "assert", t_void);
    REG_FUNC("signal", "signal", t_void);
    REG_FUNC("time", "time", cbm_type_builtin(arena, "time_t"));
    REG_FUNC("clock", "clock", cbm_type_builtin(arena, "clock_t"));
    REG_FUNC("difftime", "difftime", t_double);
}

#undef REG_FUNC
#undef REG_TYPE
