#ifndef SCHED_SCHEDULER_H
#define SCHED_SCHEDULER_H

#include <stdint.h>
#include "arch/isr.h"
#include "sched/task.h"

#define SCHED_MAX_TASKS       32u
#define SCHED_STACK_SIZE      16384u
#define SCHED_DEFAULT_WEIGHT  1024u
#define SCHED_TICK_NS         10000000ull
#define SCHED_QUANTUM_NS      10000000ull

void scheduler_init(void);
int scheduler_create_task(task_entry_t entry, void *arg, uint32_t priority_weight);
registers_t *scheduler_on_tick(registers_t *regs);
task_t *scheduler_current(void);
void scheduler_yield(void);
void scheduler_task_entry(task_t *task) __attribute__((noreturn));
void scheduler_exit_current(void) __attribute__((noreturn));

#endif
