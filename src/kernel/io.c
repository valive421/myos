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

void io_wait(void)
{
    outb(0x80, 0);
}
