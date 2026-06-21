// Compile vendored tree-sitter runtime as a single compilation unit.
// Source: tree-sitter v0.26.0 (DeusData fork)
//
// lib.c internally #includes all other runtime .c files, so we only
// need this one entry point. The runtime headers are at vendored/ts_runtime/src/.
#include "vendored/ts_runtime/src/lib.c"
