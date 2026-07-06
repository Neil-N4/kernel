#ifndef SCHED_RBTREE_H
#define SCHED_RBTREE_H

#include <stddef.h>
#include "sched/task.h"

typedef struct rb_tree {
    task_t *root;
    task_t *leftmost;
    size_t size;
} rb_tree_t;

void rb_tree_init(rb_tree_t *tree);
void rb_insert_task(rb_tree_t *tree, task_t *task);
void rb_remove_task(rb_tree_t *tree, task_t *task);
task_t *rb_first_task(const rb_tree_t *tree);
task_t *rb_pop_first_task(rb_tree_t *tree);

#endif
