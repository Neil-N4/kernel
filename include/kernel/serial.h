#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#include <stdint.h>

#define SERIAL_COM1 0x3F8u

void serial_init(void);
int serial_ready(void);
void serial_putc(char c);
void serial_write(const char *s);
void serial_write_hex32(uint32_t value);

#endif
