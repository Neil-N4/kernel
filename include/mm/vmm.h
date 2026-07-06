#ifndef MM_VMM_H
#define MM_VMM_H

#include <stdint.h>
#include "arch/isr.h"

#define VMM_PAGE_SIZE       4096u
#define VMM_PAGE_PRESENT    0x001u
#define VMM_PAGE_WRITE      0x002u
#define VMM_PAGE_USER       0x004u
#define VMM_PAGE_WRITETHRU  0x008u
#define VMM_PAGE_NOCACHE    0x010u

#define KERNEL_HIGH_HALF    0xC0000000u
#define KERNEL_HEAP_START   0xD0000000u
#define KERNEL_HEAP_LIMIT   0xD1000000u

void vmm_init(void);
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void vmm_unmap_page(uint32_t virtual_addr);
uint32_t vmm_get_physical(uint32_t virtual_addr);
void *vmm_heap_reserve_page(void);
registers_t *vmm_page_fault(registers_t *regs);

#endif
