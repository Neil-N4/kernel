#ifndef ARCH_IDT_H
#define ARCH_IDT_H

#include <stdint.h>

#define IDT_GATE_INTERRUPT 0x8Eu

void idt_init(void);
void idt_set_gate(uint8_t index, uint32_t base, uint16_t selector, uint8_t flags);

#endif
