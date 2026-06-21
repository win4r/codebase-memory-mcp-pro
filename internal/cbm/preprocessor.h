#ifndef CBM_PREPROCESSOR_H
#define CBM_PREPROCESSOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Preprocess C/C++ source: expand macros, evaluate #ifdef, resolve #include.
// Returns malloc-allocated expanded source, or NULL if no expansion needed/on failure.
// extra_defines: NULL-terminated array of "NAME=VALUE" strings (can be NULL).
// include_paths: NULL-terminated array of directory paths for #include resolution (can be NULL).
// The returned string must be freed with cbm_preprocess_free().
char *cbm_preprocess(const char *source, int source_len, const char *filename,
                     const char **extra_defines, const char **include_paths, int cpp_mode);

// Free preprocessed source returned by cbm_preprocess.
void cbm_preprocess_free(char *expanded);

#ifdef __cplusplus
}
#endif

#endif // CBM_PREPROCESSOR_H
