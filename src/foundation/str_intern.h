/*
 * str_intern.h — String interning pool.
 *
 * Deduplicates strings: identical strings share a single allocation.
 * Returns stable pointers — safe to compare by pointer equality after interning.
 *
 * Uses an arena for string storage (bulk free) + hash table for dedup lookup.
 */
#ifndef CBM_STR_INTERN_H
#define CBM_STR_INTERN_H

#include <stddef.h>
#include <stdint.h>

typedef struct CBMInternPool CBMInternPool;

/* Create a new intern pool. */
CBMInternPool *cbm_intern_create(void);

/* Free the pool and all interned strings. */
void cbm_intern_free(CBMInternPool *pool);

/* Intern a NUL-terminated string. Returns a stable pointer.
 * The same input always returns the same pointer. */
const char *cbm_intern(CBMInternPool *pool, const char *s);

/* Intern a string of known length. */
const char *cbm_intern_n(CBMInternPool *pool, const char *s, size_t len);

/* Number of unique strings in the pool. */
uint32_t cbm_intern_count(const CBMInternPool *pool);

/* Total bytes stored (unique strings only). */
size_t cbm_intern_bytes(const CBMInternPool *pool);

#endif /* CBM_STR_INTERN_H */
