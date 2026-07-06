#include "kernel/string.h"

void *memset(void *dest, int value, size_t count)
{
    uint8_t *out = (uint8_t *)dest;
    for (size_t i = 0; i < count; ++i) {
        out[i] = (uint8_t)value;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    uint8_t *out = (uint8_t *)dest;
    const uint8_t *in = (const uint8_t *)src;
    for (size_t i = 0; i < count; ++i) {
        out[i] = in[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t count)
{
    uint8_t *out = (uint8_t *)dest;
    const uint8_t *in = (const uint8_t *)src;

    if (out < in) {
        for (size_t i = 0; i < count; ++i) {
            out[i] = in[i];
        }
    } else if (out > in) {
        for (size_t i = count; i > 0; --i) {
            out[i - 1] = in[i - 1];
        }
    }

    return dest;
}

int memcmp(const void *a, const void *b, size_t count)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;

    for (size_t i = 0; i < count; ++i) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
    }

    return 0;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];

        if (ca != cb || ca == '\0' || cb == '\0') {
            return (int)ca - (int)cb;
        }
    }
    return 0;
}
