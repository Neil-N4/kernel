#include "arch/idt.h"
#include "arch/gdt.h"
#include "kernel/string.h"

typedef struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

extern void idt_flush(uint32_t idt_ptr_addr);
extern void isr_default_stub(void);
extern void *interrupt_stub_table[48];

void idt_set_gate(uint8_t index, uint32_t base, uint16_t selector, uint8_t flags)
{
    idt[index].base_low = (uint16_t)(base & 0xFFFFu);
    idt[index].base_high = (uint16_t)((base >> 16) & 0xFFFFu);
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].flags = flags;
}

void idt_init(void)
{
    idt_ptr.limit = (uint16_t)(sizeof(idt) - 1);
    idt_ptr.base = (uint32_t)&idt;
    memset(&idt, 0, sizeof(idt));

    for (uint16_t i = 0; i < 256; ++i) {
        idt_set_gate((uint8_t)i, (uint32_t)isr_default_stub,
                     KERNEL_CODE_SELECTOR, IDT_GATE_INTERRUPT);
    }

    for (uint8_t i = 0; i < 48; ++i) {
        idt_set_gate(i, (uint32_t)interrupt_stub_table[i],
                     KERNEL_CODE_SELECTOR, IDT_GATE_INTERRUPT);
    }

    idt_flush((uint32_t)&idt_ptr);
}
