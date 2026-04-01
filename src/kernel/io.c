#include "io.h"

// io.c: Port I/O routines for x86
// Provides inb/outb and related functions for hardware access

void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outw(uint16_t port, uint16_t value)
{
    __asm__ __volatile__("outw %0, %1" : : "a"(value), "Nd"(port));
}

uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void insw(uint16_t port, void* dst, uint32_t count)
{
    __asm__ __volatile__("cld; rep insw" : "+D"(dst), "+c"(count) : "d"(port) : "memory");
}

void outsw(uint16_t port, const void* src, uint32_t count)
{
    __asm__ __volatile__("cld; rep outsw" : "+S"(src), "+c"(count) : "d"(port) : "memory");
}

void io_wait(void)
{
    outb(0x80, 0);
}
