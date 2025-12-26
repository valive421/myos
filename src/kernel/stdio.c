#include "stdio.h"
#include "x86.h"

void putc(char c)
{
    x86_Video_WriteCharTeletype(c, 0);
}

void puts(const char* str)
{
    while(*str)
    {
        putc(*str);
        str++;
    }
}

// Simple print function - just outputs string
// Note: This is not a full printf implementation
void printf(const char* fmt, ...)
{
    puts(fmt);
}
