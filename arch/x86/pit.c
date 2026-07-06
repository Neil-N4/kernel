#include "arch/pit.h"
#include "arch/io.h"
#include "arch/isr.h"
#include "arch/pic.h"
#include "sched/scheduler.h"

#define PIT_COMMAND 0x43u
#define PIT_CHANNEL0 0x40u
#define PIT_INPUT_HZ 1193182u

static volatile uint64_t ticks;

registers_t *pit_interrupt(registers_t *regs)
{
    ++ticks;
    return scheduler_on_tick(regs);
}

void pit_init(uint32_t hz)
{
    if (hz == 0) {
        hz = 100;
    }

    uint32_t divisor = PIT_INPUT_HZ / hz;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));
    isr_register_handler(PIC_MASTER_OFFSET, pit_interrupt);
    pic_clear_mask(0);
}

uint64_t pit_ticks(void)
{
    return ticks;
}
