#include "arch/isr.h"
#include "arch/pic.h"
#include "kernel/kprintf.h"

static isr_handler_t handlers[256];

static const char *exception_messages[32] = {
    "divide by zero",
    "debug",
    "non-maskable interrupt",
    "breakpoint",
    "overflow",
    "bound range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid TSS",
    "segment not present",
    "stack fault",
    "general protection fault",
    "page fault",
    "reserved",
    "x87 floating-point exception",
    "alignment check",
    "machine check",
    "SIMD floating-point exception",
    "virtualization exception",
    "control protection exception",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "hypervisor injection exception",
    "VMM communication exception",
    "security exception",
    "reserved"
};

void isr_init(void)
{
    for (uint16_t i = 0; i < 256; ++i) {
        handlers[i] = 0;
    }
}

void isr_register_handler(uint8_t vector, isr_handler_t handler)
{
    handlers[vector] = handler;
}

registers_t *isr_dispatch(registers_t *regs)
{
    registers_t *next_regs = regs;

    if (handlers[regs->int_no] != 0) {
        next_regs = handlers[regs->int_no](regs);
        if (next_regs == 0) {
            next_regs = regs;
        }
    } else if (regs->int_no < 32) {
        kprintf("Unhandled exception %u (%s), err=%x eip=%x\n",
                regs->int_no, exception_messages[regs->int_no],
                regs->err_code, regs->eip);
        panic("unhandled CPU exception");
    }

    if (regs->int_no >= PIC_MASTER_OFFSET && regs->int_no < PIC_MASTER_OFFSET + 16u) {
        pic_send_eoi((uint8_t)(regs->int_no - PIC_MASTER_OFFSET));
    }

    return next_regs;
}
