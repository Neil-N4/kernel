#include <stdint.h>
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/isr.h"
#include "arch/pic.h"
#include "arch/pit.h"
#include "arch/io.h"
#include "kernel/kprintf.h"
#include "kernel/multiboot.h"
#include "kernel/serial.h"
#include "mm/page_cache.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "sched/scheduler.h"

static void idle_task(void *arg)
{
    (void)arg;
    for (;;) {
        halt_cpu();
    }
}

static void heartbeat_task(void *arg)
{
    const char *name = (const char *)arg;
    uint32_t count = 0;

    for (;;) {
        if ((count++ % 1000000u) == 0) {
            kprintf("task %s alive, vruntime=%u\n",
                    name, (uint32_t)scheduler_current()->vruntime_ns);
            scheduler_yield();
        }
    }
}

void kernel_main(uint32_t magic, uint32_t multiboot_info_addr)
{
    serial_init();
    kprintf("\nDeterministic x86 monolithic kernel booting\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("Unexpected Multiboot magic: %x\n", magic);
        panic("invalid boot protocol");
    }

    const multiboot_info_t *mbi = (const multiboot_info_t *)multiboot_info_addr;

    gdt_init();
    isr_init();
    idt_init();
    pic_init();

    pmm_init(mbi);
    vmm_init();
    page_cache_init();

    scheduler_init();
    scheduler_create_task(idle_task, 0, SCHED_DEFAULT_WEIGHT);
    scheduler_create_task(heartbeat_task, "A", SCHED_DEFAULT_WEIGHT);
    scheduler_create_task(heartbeat_task, "B", SCHED_DEFAULT_WEIGHT / 2u);

    pit_init(100);

    kprintf("Kernel initialized; enabling interrupts\n");
    __asm__ volatile("sti");

    for (;;) {
        halt_cpu();
    }
}
