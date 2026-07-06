#ifndef MM_PMM_H
#define MM_PMM_H

#include <stddef.h>
#include <stdint.h>
#include "kernel/multiboot.h"

#define PMM_FRAME_SIZE 4096u
#define PMM_MAX_FRAMES 1048576u

void pmm_init(const multiboot_info_t *mbi);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys_addr);
void pmm_reserve_range(uint32_t base, uint32_t length);
void pmm_release_range(uint32_t base, uint32_t length);
uint32_t pmm_total_frames(void);
uint32_t pmm_free_frames(void);

#endif
