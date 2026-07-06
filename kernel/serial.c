#include "kernel/serial.h"
#include "arch/io.h"

void serial_init(void)
{
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x80);
    outb(SERIAL_COM1 + 0, 0x03);
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x03);
    outb(SERIAL_COM1 + 2, 0xC7);
    outb(SERIAL_COM1 + 4, 0x0B);
}

int serial_ready(void)
{
    return (inb(SERIAL_COM1 + 5) & 0x20) != 0;
}

void serial_putc(char c)
{
    while (!serial_ready()) {
    }
    outb(SERIAL_COM1, (uint8_t)c);
}

void serial_write(const char *s)
{
    while (*s != '\0') {
        serial_putc(*s++);
    }
}

void serial_write_hex32(uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    serial_write("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        serial_putc(digits[(value >> shift) & 0xFu]);
    }
}
