#ifndef CBM_AC_H
#define CBM_AC_H

#include <stdint.h>

// Forward declaration — full struct in ac.c
typedef struct CBMAutomaton CBMAutomaton;

// Input for batch LZ4 scanning.
typedef struct {
    const char *data;
    int compressed_len;
    int original_len;
} CBMLz4Entry;

// Output for batch LZ4 scanning.
typedef struct {
    int file_index;
    uint64_t bitmask;
} CBMLz4Match;

// Output for batch name scanning.
typedef struct {
    int name_index;
    int pattern_id;
} CBMMatchResult;

// Build an Aho-Corasick automaton from patterns.
CBMAutomaton *cbm_ac_build(const char **patterns, const int *lengths, int count,
                           const uint8_t *alpha_map, int alpha_size);
void cbm_ac_free(CBMAutomaton *ac);

// Single-text scanning (returns bitmask of matched pattern IDs).
uint64_t cbm_ac_scan_bitmask(const CBMAutomaton *ac, const char *text, int text_len);

// LZ4-compressed scanning.
uint64_t cbm_ac_scan_lz4_bitmask(const CBMAutomaton *ac, const char *compressed, int compressed_len,
                                 int original_len);
int cbm_ac_scan_lz4_batch(const CBMAutomaton *ac, const CBMLz4Entry *entries, int num_entries,
                          CBMLz4Match *out_matches, int max_matches);

// Batch name scanning.
int cbm_ac_scan_batch(const CBMAutomaton *ac, const char *names_buf, const int *name_offsets,
                      const int *name_lengths, int num_names, CBMMatchResult *out_matches,
                      int max_matches);

// Introspection.
int cbm_ac_num_states(const CBMAutomaton *ac);
int cbm_ac_num_patterns(const CBMAutomaton *ac);
int cbm_ac_table_bytes(const CBMAutomaton *ac);

#endif // CBM_AC_H
