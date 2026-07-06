#ifndef ARCH_PIT_H
#define ARCH_PIT_H

#include <stdint.h>
#include "arch/isr.h"

void pit_init(uint32_t hz);
uint64_t pit_ticks(void);
registers_t *pit_interrupt(registers_t *regs);

#endif
