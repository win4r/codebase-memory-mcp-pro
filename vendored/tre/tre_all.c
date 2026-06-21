/*
 * tre_all.c — Single compilation unit for vendored TRE regex library.
 * Only compiled on Windows (POSIX systems use system <regex.h>).
 */
#ifdef _WIN32

#include "tre-config.h"

/* Core library sources */
#include "tre-ast.c"
#include "tre-compile.c"
#include "tre-mem.c"
#include "tre-parse.c"
#include "tre-stack.c"

/* Matchers (tre-match-utils.h has include guard to handle being
 * included by both matcher .c files in same translation unit) */
#include "tre-match-parallel.c"
#include "tre-match-backtrack.c"

/* Approximate matching + filter */
#include "tre-match-approx.c"
#include "tre-filter.c"

/* POSIX API */
#include "regcomp.c"
#include "regexec.c"
#include "regerror.c"

#endif /* _WIN32 */
