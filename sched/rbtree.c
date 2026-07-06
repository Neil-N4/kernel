#include "sched/rbtree.h"

static rb_color_t color_of(task_t *node)
{
    return node == 0 ? RB_BLACK : node->rb_color;
}

static int task_less(const task_t *a, const task_t *b)
{
    if (a->vruntime_ns < b->vruntime_ns) {
        return 1;
    }
    if (a->vruntime_ns > b->vruntime_ns) {
        return 0;
    }
    return a->tid < b->tid;
}

static task_t *minimum(task_t *node)
{
    if (node == 0) {
        return 0;
    }
    while (node->rb_left != 0) {
        node = node->rb_left;
    }
    return node;
}

static void refresh_leftmost(rb_tree_t *tree)
{
    tree->leftmost = minimum(tree->root);
}

static void rotate_left(rb_tree_t *tree, task_t *node)
{
    task_t *right = node->rb_right;
    node->rb_right = right->rb_left;
    if (right->rb_left != 0) {
        right->rb_left->rb_parent = node;
    }

    right->rb_parent = node->rb_parent;
    if (node->rb_parent == 0) {
        tree->root = right;
    } else if (node == node->rb_parent->rb_left) {
        node->rb_parent->rb_left = right;
    } else {
        node->rb_parent->rb_right = right;
    }

    right->rb_left = node;
    node->rb_parent = right;
}

static void rotate_right(rb_tree_t *tree, task_t *node)
{
    task_t *left = node->rb_left;
    node->rb_left = left->rb_right;
    if (left->rb_right != 0) {
        left->rb_right->rb_parent = node;
    }

    left->rb_parent = node->rb_parent;
    if (node->rb_parent == 0) {
        tree->root = left;
    } else if (node == node->rb_parent->rb_right) {
        node->rb_parent->rb_right = left;
    } else {
        node->rb_parent->rb_left = left;
    }

    left->rb_right = node;
    node->rb_parent = left;
}

void rb_tree_init(rb_tree_t *tree)
{
    tree->root = 0;
    tree->leftmost = 0;
    tree->size = 0;
}

void rb_insert_task(rb_tree_t *tree, task_t *task)
{
    task_t *parent = 0;
    task_t *cursor = tree->root;

    while (cursor != 0) {
        parent = cursor;
        cursor = task_less(task, cursor) ? cursor->rb_left : cursor->rb_right;
    }

    task->rb_parent = parent;
    task->rb_left = 0;
    task->rb_right = 0;
    task->rb_color = RB_RED;
    task->in_runqueue = 1;

    if (parent == 0) {
        tree->root = task;
    } else if (task_less(task, parent)) {
        parent->rb_left = task;
    } else {
        parent->rb_right = task;
    }

    while (task != tree->root && color_of(task->rb_parent) == RB_RED) {
        task_t *parent_node = task->rb_parent;
        task_t *grandparent = parent_node->rb_parent;

        if (parent_node == grandparent->rb_left) {
            task_t *uncle = grandparent->rb_right;
            if (color_of(uncle) == RB_RED) {
                parent_node->rb_color = RB_BLACK;
                uncle->rb_color = RB_BLACK;
                grandparent->rb_color = RB_RED;
                task = grandparent;
            } else {
                if (task == parent_node->rb_right) {
                    task = parent_node;
                    rotate_left(tree, task);
                    parent_node = task->rb_parent;
                    grandparent = parent_node->rb_parent;
                }
                parent_node->rb_color = RB_BLACK;
                grandparent->rb_color = RB_RED;
                rotate_right(tree, grandparent);
            }
        } else {
            task_t *uncle = grandparent->rb_left;
            if (color_of(uncle) == RB_RED) {
                parent_node->rb_color = RB_BLACK;
                uncle->rb_color = RB_BLACK;
                grandparent->rb_color = RB_RED;
                task = grandparent;
            } else {
                if (task == parent_node->rb_left) {
                    task = parent_node;
                    rotate_right(tree, task);
                    parent_node = task->rb_parent;
                    grandparent = parent_node->rb_parent;
                }
                parent_node->rb_color = RB_BLACK;
                grandparent->rb_color = RB_RED;
                rotate_left(tree, grandparent);
            }
        }
    }

    tree->root->rb_color = RB_BLACK;
    ++tree->size;
    refresh_leftmost(tree);
}

static void transplant(rb_tree_t *tree, task_t *old_node, task_t *new_node)
{
    if (old_node->rb_parent == 0) {
        tree->root = new_node;
    } else if (old_node == old_node->rb_parent->rb_left) {
        old_node->rb_parent->rb_left = new_node;
    } else {
        old_node->rb_parent->rb_right = new_node;
    }

    if (new_node != 0) {
        new_node->rb_parent = old_node->rb_parent;
    }
}

static void delete_fixup(rb_tree_t *tree, task_t *node, task_t *parent)
{
    while (node != tree->root && color_of(node) == RB_BLACK) {
        if (parent == 0) {
            break;
        }

        if (node == parent->rb_left) {
            task_t *sibling = parent->rb_right;
            if (sibling == 0) {
                node = parent;
                parent = node->rb_parent;
                continue;
            }

            if (color_of(sibling) == RB_RED) {
                sibling->rb_color = RB_BLACK;
                parent->rb_color = RB_RED;
                rotate_left(tree, parent);
                sibling = parent->rb_right;
            }

            if (color_of(sibling->rb_left) == RB_BLACK &&
                color_of(sibling->rb_right) == RB_BLACK) {
                sibling->rb_color = RB_RED;
                node = parent;
                parent = node->rb_parent;
            } else {
                if (color_of(sibling->rb_right) == RB_BLACK) {
                    if (sibling->rb_left != 0) {
                        sibling->rb_left->rb_color = RB_BLACK;
                    }
                    sibling->rb_color = RB_RED;
                    rotate_right(tree, sibling);
                    sibling = parent->rb_right;
                }
                sibling->rb_color = parent->rb_color;
                parent->rb_color = RB_BLACK;
                if (sibling->rb_right != 0) {
                    sibling->rb_right->rb_color = RB_BLACK;
                }
                rotate_left(tree, parent);
                node = tree->root;
                parent = 0;
            }
        } else {
            task_t *sibling = parent->rb_left;
            if (sibling == 0) {
                node = parent;
                parent = node->rb_parent;
                continue;
            }

            if (color_of(sibling) == RB_RED) {
                sibling->rb_color = RB_BLACK;
                parent->rb_color = RB_RED;
                rotate_right(tree, parent);
                sibling = parent->rb_left;
            }

            if (color_of(sibling->rb_right) == RB_BLACK &&
                color_of(sibling->rb_left) == RB_BLACK) {
                sibling->rb_color = RB_RED;
                node = parent;
                parent = node->rb_parent;
            } else {
                if (color_of(sibling->rb_left) == RB_BLACK) {
                    if (sibling->rb_right != 0) {
                        sibling->rb_right->rb_color = RB_BLACK;
                    }
                    sibling->rb_color = RB_RED;
                    rotate_left(tree, sibling);
                    sibling = parent->rb_left;
                }
                sibling->rb_color = parent->rb_color;
                parent->rb_color = RB_BLACK;
                if (sibling->rb_left != 0) {
                    sibling->rb_left->rb_color = RB_BLACK;
                }
                rotate_right(tree, parent);
                node = tree->root;
                parent = 0;
            }
        }
    }

    if (node != 0) {
        node->rb_color = RB_BLACK;
    }
}

void rb_remove_task(rb_tree_t *tree, task_t *task)
{
    task_t *moved = task;
    task_t *fix_node;
    task_t *fix_parent;
    rb_color_t moved_original_color = moved->rb_color;

    if (task->rb_left == 0) {
        fix_node = task->rb_right;
        fix_parent = task->rb_parent;
        transplant(tree, task, task->rb_right);
    } else if (task->rb_right == 0) {
        fix_node = task->rb_left;
        fix_parent = task->rb_parent;
        transplant(tree, task, task->rb_left);
    } else {
        moved = minimum(task->rb_right);
        moved_original_color = moved->rb_color;
        fix_node = moved->rb_right;

        if (moved->rb_parent == task) {
            fix_parent = moved;
            if (fix_node != 0) {
                fix_node->rb_parent = moved;
            }
        } else {
            fix_parent = moved->rb_parent;
            transplant(tree, moved, moved->rb_right);
            moved->rb_right = task->rb_right;
            moved->rb_right->rb_parent = moved;
        }

        transplant(tree, task, moved);
        moved->rb_left = task->rb_left;
        moved->rb_left->rb_parent = moved;
        moved->rb_color = task->rb_color;
    }

    task->rb_parent = 0;
    task->rb_left = 0;
    task->rb_right = 0;
    task->in_runqueue = 0;
    if (tree->size > 0) {
        --tree->size;
    }

    if (moved_original_color == RB_BLACK) {
        delete_fixup(tree, fix_node, fix_parent);
    }
    refresh_leftmost(tree);
}

task_t *rb_first_task(const rb_tree_t *tree)
{
    return tree->leftmost;
}

task_t *rb_pop_first_task(rb_tree_t *tree)
{
    task_t *first = tree->leftmost;
    if (first != 0) {
        rb_remove_task(tree, first);
    }
    return first;
}
