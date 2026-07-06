#include "sched/scheduler.h"
#include "sched/rbtree.h"
#include "arch/gdt.h"
#include "kernel/kprintf.h"
#include "kernel/string.h"

extern void task_entry_trampoline(void);

static task_t tasks[SCHED_MAX_TASKS];
static uint8_t task_stacks[SCHED_MAX_TASKS][SCHED_STACK_SIZE] __attribute__((aligned(16)));
static rb_tree_t runqueue;
static task_t *current_task;
static uint32_t next_tid;

static task_t *alloc_task(void)
{
    for (uint32_t i = 0; i < SCHED_MAX_TASKS; ++i) {
        if (tasks[i].state == TASK_UNUSED || tasks[i].state == TASK_ZOMBIE) {
            memset(&tasks[i], 0, sizeof(tasks[i]));
            tasks[i].stack_index = (uint8_t)i;
            return &tasks[i];
        }
    }
    return 0;
}

static uint64_t weighted_tick_ns(const task_t *task)
{
    uint32_t weight = task->priority_weight == 0 ? SCHED_DEFAULT_WEIGHT
                                                 : task->priority_weight;
    return (SCHED_TICK_NS * SCHED_DEFAULT_WEIGHT) / weight;
}

static registers_t *task_saved_regs(task_t *task)
{
    return (registers_t *)task->context_esp;
}

void scheduler_init(void)
{
    memset(tasks, 0, sizeof(tasks));
    rb_tree_init(&runqueue);
    next_tid = 1;

    current_task = &tasks[0];
    current_task->tid = next_tid++;
    current_task->state = TASK_RUNNING;
    current_task->priority_weight = SCHED_DEFAULT_WEIGHT;
    current_task->stack_index = 0;

    kprintf("SCHED: initialized fixed pool with %u tasks\n", SCHED_MAX_TASKS);
}

int scheduler_create_task(task_entry_t entry, void *arg, uint32_t priority_weight)
{
    if (entry == 0) {
        return -1;
    }

    task_t *task = alloc_task();
    if (task == 0) {
        return -1;
    }

    task->tid = next_tid++;
    task->state = TASK_READY;
    task->priority_weight = priority_weight == 0 ? SCHED_DEFAULT_WEIGHT
                                                 : priority_weight;
    task->entry = entry;
    task->arg = arg;

    uint32_t stack_top = (uint32_t)&task_stacks[task->stack_index][SCHED_STACK_SIZE];
    stack_top &= ~0xFu;
    stack_top -= sizeof(registers_t);

    registers_t *frame = (registers_t *)stack_top;
    memset(frame, 0, sizeof(*frame));
    frame->gs = KERNEL_DATA_SELECTOR;
    frame->fs = KERNEL_DATA_SELECTOR;
    frame->es = KERNEL_DATA_SELECTOR;
    frame->ds = KERNEL_DATA_SELECTOR;
    frame->eax = (uint32_t)task;
    frame->eip = (uint32_t)task_entry_trampoline;
    frame->cs = KERNEL_CODE_SELECTOR;
    frame->eflags = 0x202u;
    frame->ss = KERNEL_DATA_SELECTOR;
    task->context_esp = (uint32_t)frame;

    rb_insert_task(&runqueue, task);
    return (int)task->tid;
}

task_t *scheduler_current(void)
{
    return current_task;
}

static registers_t *switch_to_next(registers_t *regs, int current_is_exiting)
{
    if (runqueue.size == 0) {
        if (current_is_exiting) {
            panic("last runnable task exited");
        }
        return regs;
    }

    task_t *previous = current_task;
    if (!current_is_exiting) {
        previous->state = TASK_READY;
        previous->context_esp = (uint32_t)regs;
        rb_insert_task(&runqueue, previous);
    }

    task_t *next = rb_pop_first_task(&runqueue);
    next->state = TASK_RUNNING;
    next->runtime_slice_ns = 0;
    current_task = next;

    registers_t *next_regs = task_saved_regs(next);
    tss_set_kernel_stack((uint32_t)next_regs);
    return next_regs;
}

registers_t *scheduler_on_tick(registers_t *regs)
{
    if (current_task == 0) {
        return regs;
    }

    if (current_task->state == TASK_ZOMBIE) {
        return switch_to_next(regs, 1);
    }

    current_task->context_esp = (uint32_t)regs;
    current_task->vruntime_ns += weighted_tick_ns(current_task);
    current_task->runtime_slice_ns += SCHED_TICK_NS;

    task_t *min = rb_first_task(&runqueue);
    if (min == 0) {
        return regs;
    }

    if (current_task->runtime_slice_ns >= SCHED_QUANTUM_NS ||
        current_task->vruntime_ns > min->vruntime_ns) {
        return switch_to_next(regs, 0);
    }

    return regs;
}

void scheduler_yield(void)
{
    __asm__ volatile("int $32");
}

void scheduler_task_entry(task_t *task)
{
    if (task != 0 && task->entry != 0) {
        task->entry(task->arg);
    }
    scheduler_exit_current();
}

void scheduler_exit_current(void)
{
    if (current_task != 0) {
        current_task->state = TASK_ZOMBIE;
    }

    for (;;) {
        scheduler_yield();
        __asm__ volatile("hlt");
    }
}
