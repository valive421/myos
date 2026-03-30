#include "vga.h"
#include "boot.h"
#include "stdio.h"

// vga.c: VGA text mode output routines
// Provides functions for writing to VGA text buffer
static volatile uint16_t* g_Vga;
static uint8_t g_CursorRow = 0;
static uint8_t g_CursorCol = 0;

static void vga_scroll_if_needed(void)
{
    if (g_CursorRow >= 25)
    {
        for (uint32_t row = 1; row < 25; row++)
        {
            for (uint32_t col = 0; col < 80; col++)
                g_Vga[(row - 1) * 80 + col] = g_Vga[row * 80 + col];
        }

        for (uint32_t col = 0; col < 80; col++)
            g_Vga[24 * 80 + col] = 0x0F20;

        g_CursorRow = 24;
    }
}

void vga_init(void)
{
    g_Vga = (volatile uint16_t*)(0x000B8000u - kernel_base_phys);
    g_CursorRow = 20;
    g_CursorCol = 0;
}

void vga_clear(void)
{
    for (uint32_t row = 0; row < 25; row++)
    {
        for (uint32_t col = 0; col < 80; col++)
            g_Vga[row * 80 + col] = 0x0F20;
    }

    g_CursorRow = 0;
    g_CursorCol = 0;
}

void vga_backspace(void)
{
    if (g_CursorCol == 0)
    {
        if (g_CursorRow == 0)
            return;

        g_CursorRow--;
        g_CursorCol = 79;
    }
    else
    {
        g_CursorCol--;
    }

    g_Vga[g_CursorRow * 80 + g_CursorCol] = 0x0F20;
}

void vga_write_at(uint8_t row, uint8_t col, char c)
{
    if (row >= 25 || col >= 80)
        return;

    g_Vga[(uint32_t)row * 80u + (uint32_t)col] = (uint16_t)0x0F00 | (uint8_t)c;
}

void putc(char c)
{
    if (c == '\n')
    {
        g_CursorCol = 0;
        g_CursorRow++;
        goto maybe_scroll;
    }

    if (g_CursorCol >= 80)
    {
        g_CursorCol = 0;
        g_CursorRow++;
    }

    g_Vga[g_CursorRow * 80 + g_CursorCol] = (uint16_t)0x0F00 | (uint8_t)c;
    g_CursorCol++;

maybe_scroll:
    vga_scroll_if_needed();
}

void puts(const char* s)
{
    while (*s)
    {
        putc(*s);
        s++;
    }
}

void vga_puthex8(uint8_t value)
{
    printf("%X%X", (value >> 4) & 0x0F, value & 0x0F);
}

void vga_puthex32(uint32_t value)
{
    vga_puthex8((uint8_t)((value >> 24) & 0xFF));
    vga_puthex8((uint8_t)((value >> 16) & 0xFF));
    vga_puthex8((uint8_t)((value >> 8) & 0xFF));
    vga_puthex8((uint8_t)(value & 0xFF));
}
