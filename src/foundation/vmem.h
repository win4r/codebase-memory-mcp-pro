/*
 * vmem.h — Budget-tracked virtual memory allocator.
 *
 * Allocates via mmap/VirtualAlloc instead of malloc to avoid ptmalloc2's
 * per-thread arena fragmentation (the cause of 321GB VSZ on Linux kernel
 * indexing with 12 workers).
 *
 * Budget: configurable fraction of physical RAM (default 50%). When
 * allocation exceeds the budget, a pressure event is logged. Workers
 * never block — the budget is advisory for monitoring/alerting only.
 *
 * All allocations are page-aligned and zeroed by the OS.
 */
#ifndef CBM_VMEM_H
#define CBM_VMEM_H

#include <stdbool.h>
#include <stddef.h>

/* Initialize vmem with a budget = ram_fraction * total_physical_ram.
 * Thread-safe: only the first call takes effect.
 * Must be called before any cbm_vmem_alloc() calls. */
void cbm_vmem_init(double ram_fraction);

/* Allocate size bytes via mmap/VirtualAlloc.
 * Returns page-aligned, zeroed memory. NULL on failure.
 * Tracks allocation against the budget and logs pressure events. */
void *cbm_vmem_alloc(size_t size);

/* Free memory previously allocated by cbm_vmem_alloc.
 * size must match the original allocation size. */
void cbm_vmem_free(void *ptr, size_t size);

/* Current total allocated bytes (atomic). */
size_t cbm_vmem_allocated(void);

/* Peak allocated bytes seen so far. */
size_t cbm_vmem_peak(void);

/* Total budget in bytes. */
size_t cbm_vmem_budget(void);

/* Returns true if current allocation exceeds the budget. */
bool cbm_vmem_over_budget(void);

/* Per-worker budget hint: budget / num_workers.
 * For monitoring/logging only — workers never block. */
size_t cbm_vmem_worker_budget(int num_workers);

#endif /* CBM_VMEM_H */
