#ifndef KERNEL_KPRINTF_H
#define KERNEL_KPRINTF_H

#include <stdint.h>

void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
void panic(const char *message) __attribute__((noreturn));

#endif
