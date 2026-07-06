#include "kernel/kprintf.h"
#include "kernel/serial.h"
#include "arch/io.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

void kputc(char c)
{
    if (c == '\n') {
        serial_putc('\r');
    }
    serial_putc(c);
}

void kputs(const char *s)
{
    while (*s != '\0') {
        kputc(*s++);
    }
}

static void print_unsigned(uint32_t value, uint32_t base)
{
    char buf[33];
    const char digits[] = "0123456789ABCDEF";
    uint32_t i = 0;

    if (value == 0) {
        kputc('0');
        return;
    }

    while (value != 0 && i < sizeof(buf)) {
        buf[i++] = digits[value % base];
        value /= base;
    }

    while (i > 0) {
        kputc(buf[--i]);
    }
}

static void print_signed(int32_t value)
{
    if (value < 0) {
        kputc('-');
        print_unsigned((uint32_t)(-value), 10);
    } else {
        print_unsigned((uint32_t)value, 10);
    }
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p != '\0'; ++p) {
        if (*p != '%') {
            kputc(*p);
            continue;
        }

        ++p;
        switch (*p) {
        case '%':
            kputc('%');
            break;
        case 'c':
            kputc((char)va_arg(args, int));
            break;
        case 's': {
            const char *s = va_arg(args, const char *);
            kputs(s != 0 ? s : "(null)");
            break;
        }
        case 'd':
        case 'i':
            print_signed(va_arg(args, int32_t));
            break;
        case 'u':
            print_unsigned(va_arg(args, uint32_t), 10);
            break;
        case 'x':
            print_unsigned(va_arg(args, uint32_t), 16);
            break;
        case 'p':
            serial_write_hex32((uint32_t)va_arg(args, void *));
            break;
        default:
            kputc('%');
            kputc(*p);
            break;
        }
    }

    va_end(args);
}

void panic(const char *message)
{
    kprintf("\nPANIC: %s\n", message);
    __asm__ volatile("cli");
    for (;;) {
        halt_cpu();
    }
}
