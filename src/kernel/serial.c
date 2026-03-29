// serial.c: COM1 serial port routines
// Initializes and writes to serial port for diagnostics
#include "serial.h"
#include "io.h"

#define COM1 0x3F8

static int serial_ready(void)
{
    return (inb(COM1 + 5) & 0x20) != 0;
}

void serial_init(void)
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void serial_write_char(char c)
{
    while (!serial_ready())
        ;

    outb(COM1, (uint8_t)c);
}

void serial_write_str(const char* s)
{
    while (*s)
    {
        if (*s == '\n')
            serial_write_char('\r');
        serial_write_char(*s++);
    }
}

void serial_write_hex32(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    for (int shift = 28; shift >= 0; shift -= 4)
        serial_write_char(hex[(value >> shift) & 0x0F]);
}
