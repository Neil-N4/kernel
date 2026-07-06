#include "arch/gdt.h"
#include "kernel/string.h"

typedef struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

static gdt_entry_t gdt[6];
static gdt_ptr_t gdt_ptr;
static tss_entry_t tss;

extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void tss_flush(void);

static void gdt_set_gate(uint32_t index, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t granularity)
{
    gdt[index].base_low = (uint16_t)(base & 0xFFFFu);
    gdt[index].base_middle = (uint8_t)((base >> 16) & 0xFFu);
    gdt[index].base_high = (uint8_t)((base >> 24) & 0xFFu);
    gdt[index].limit_low = (uint16_t)(limit & 0xFFFFu);
    gdt[index].granularity = (uint8_t)((limit >> 16) & 0x0Fu);
    gdt[index].granularity |= granularity & 0xF0u;
    gdt[index].access = access;
}

void tss_set_kernel_stack(uint32_t esp0)
{
    tss.esp0 = esp0;
}

void gdt_init(void)
{
    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_ptr.base = (uint32_t)&gdt;

    memset(&gdt, 0, sizeof(gdt));
    memset(&tss, 0, sizeof(tss));

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFFu, 0x9Au, 0xCFu);
    gdt_set_gate(2, 0, 0xFFFFFFFFu, 0x92u, 0xCFu);
    gdt_set_gate(3, 0, 0xFFFFFFFFu, 0xFAu, 0xCFu);
    gdt_set_gate(4, 0, 0xFFFFFFFFu, 0xF2u, 0xCFu);

    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iomap_base = sizeof(tss_entry_t);
    tss.cs = USER_CODE_SELECTOR | 0x3u;
    tss.ss = USER_DATA_SELECTOR | 0x3u;
    tss.ds = USER_DATA_SELECTOR | 0x3u;
    tss.es = USER_DATA_SELECTOR | 0x3u;
    tss.fs = USER_DATA_SELECTOR | 0x3u;
    tss.gs = USER_DATA_SELECTOR | 0x3u;
    gdt_set_gate(5, (uint32_t)&tss, sizeof(tss_entry_t) - 1, 0x89u, 0x40u);

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}
