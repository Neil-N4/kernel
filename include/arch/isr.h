#ifndef ARCH_ISR_H
#define ARCH_ISR_H

#include <stdint.h>

typedef struct registers {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;
    uint32_t ss;
} registers_t;

typedef registers_t *(*isr_handler_t)(registers_t *regs);

void isr_init(void);
void isr_register_handler(uint8_t vector, isr_handler_t handler);
registers_t *isr_dispatch(registers_t *regs);

#endif
