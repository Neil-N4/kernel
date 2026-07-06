#ifndef ARCH_GDT_H
#define ARCH_GDT_H

#include <stdint.h>

#define KERNEL_CODE_SELECTOR 0x08u
#define KERNEL_DATA_SELECTOR 0x10u
#define USER_CODE_SELECTOR   0x18u
#define USER_DATA_SELECTOR   0x20u
#define TSS_SELECTOR         0x28u

void gdt_init(void);
void tss_set_kernel_stack(uint32_t esp0);

#endif
