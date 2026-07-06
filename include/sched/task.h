#ifndef SCHED_TASK_H
#define SCHED_TASK_H

#include <stdint.h>

typedef enum task_state {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

typedef enum rb_color {
    RB_RED = 0,
    RB_BLACK = 1
} rb_color_t;

typedef void (*task_entry_t)(void *arg);

typedef struct task {
    uint32_t tid;
    task_state_t state;
    uint32_t priority_weight;
    uint64_t vruntime_ns;
    uint64_t runtime_slice_ns;
    uint32_t context_esp;
    task_entry_t entry;
    void *arg;

    struct task *rb_left;
    struct task *rb_right;
    struct task *rb_parent;
    rb_color_t rb_color;
    uint8_t in_runqueue;
    uint8_t stack_index;
    uint16_t reserved;
} task_t;

#endif
