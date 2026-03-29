
// keyboard.c: PS/2 keyboard driver
// Handles keyboard IRQs, scancode processing, and input buffer

#include "keyboard.h"
#include "io.h"

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64
#define KBD_BUF_SIZE  128

static char g_KeyBuf[KBD_BUF_SIZE];
static volatile uint8_t g_KeyHead = 0;
static volatile uint8_t g_KeyTail = 0;
static volatile uint8_t g_Shift = 0;

//
static void keybuf_push(char c)
{
    uint8_t next = (uint8_t)((g_KeyHead + 1u) % KBD_BUF_SIZE);
    if (next == g_KeyTail)
        return;

    g_KeyBuf[g_KeyHead] = c;
    g_KeyHead = next;
}

static int keybuf_pop(char* out_char)
{
    uint32_t flags;
    __asm__ __volatile__("pushf; pop %0; cli" : "=r"(flags) : : "memory");

    if (g_KeyTail == g_KeyHead)
    {
        __asm__ __volatile__("push %0; popf" : : "r"(flags) : "memory");
        return 0;
    }

    *out_char = g_KeyBuf[g_KeyTail];
    g_KeyTail = (uint8_t)((g_KeyTail + 1u) % KBD_BUF_SIZE);

    __asm__ __volatile__("push %0; popf" : : "r"(flags) : "memory");
    return 1;
}

static char scancode_to_char(uint8_t sc)
{
    switch (sc)
    {
        case 0x02: return g_Shift ? '!' : '1';
        case 0x03: return g_Shift ? '@' : '2';
        case 0x04: return g_Shift ? '#' : '3';
        case 0x05: return g_Shift ? '$' : '4';
        case 0x06: return g_Shift ? '%' : '5';
        case 0x07: return g_Shift ? '^' : '6';
        case 0x08: return g_Shift ? '&' : '7';
        case 0x09: return g_Shift ? '*' : '8';
        case 0x0A: return g_Shift ? '(' : '9';
        case 0x0B: return g_Shift ? ')' : '0';
        case 0x0C: return g_Shift ? '_' : '-';
        case 0x0D: return g_Shift ? '+' : '=';
        case 0x0E: return '\b';
        case 0x0F: return '\t';
        case 0x10: return g_Shift ? 'Q' : 'q';
        case 0x11: return g_Shift ? 'W' : 'w';
        case 0x12: return g_Shift ? 'E' : 'e';
        case 0x13: return g_Shift ? 'R' : 'r';
        case 0x14: return g_Shift ? 'T' : 't';
        case 0x15: return g_Shift ? 'Y' : 'y';
        case 0x16: return g_Shift ? 'U' : 'u';
        case 0x17: return g_Shift ? 'I' : 'i';
        case 0x18: return g_Shift ? 'O' : 'o';
        case 0x19: return g_Shift ? 'P' : 'p';
        case 0x1A: return g_Shift ? '{' : '[';
        case 0x1B: return g_Shift ? '}' : ']';
        case 0x1C: return '\n';
        case 0x1E: return g_Shift ? 'A' : 'a';
        case 0x1F: return g_Shift ? 'S' : 's';
        case 0x20: return g_Shift ? 'D' : 'd';
        case 0x21: return g_Shift ? 'F' : 'f';
        case 0x22: return g_Shift ? 'G' : 'g';
        case 0x23: return g_Shift ? 'H' : 'h';
        case 0x24: return g_Shift ? 'J' : 'j';
        case 0x25: return g_Shift ? 'K' : 'k';
        case 0x26: return g_Shift ? 'L' : 'l';
        case 0x27: return g_Shift ? ':' : ';';
        case 0x28: return g_Shift ? '"' : '\'';
        case 0x29: return g_Shift ? '~' : '`';
        case 0x2B: return g_Shift ? '|' : '\\';
        case 0x2C: return g_Shift ? 'Z' : 'z';
        case 0x2D: return g_Shift ? 'X' : 'x';
        case 0x2E: return g_Shift ? 'C' : 'c';
        case 0x2F: return g_Shift ? 'V' : 'v';
        case 0x30: return g_Shift ? 'B' : 'b';
        case 0x31: return g_Shift ? 'N' : 'n';
        case 0x32: return g_Shift ? 'M' : 'm';
        case 0x33: return g_Shift ? '<' : ',';
        case 0x34: return g_Shift ? '>' : '.';
        case 0x35: return g_Shift ? '?' : '/';
        case 0x39: return ' ';
        default:   return 0;
    }
}

static void keyboard_process_scancode(uint8_t sc)
{
    if (sc == 0x2A || sc == 0x36)
    {
        g_Shift = 1;
        return;
    }

    if (sc == 0xAA || sc == 0xB6)
    {
        g_Shift = 0;
        return;
    }

    if (sc & 0x80)
        return;

    char c = scancode_to_char(sc);
    if (c)
        keybuf_push(c);
}

void keyboard_init(void)
{
    g_KeyHead = 0;
    g_KeyTail = 0;
    g_Shift = 0;
}

void keyboard_on_irq(void)
{
    uint8_t sc = inb(KBD_DATA_PORT);
    keyboard_process_scancode(sc);
}

void keyboard_poll_hw(void)
{
    while (inb(KBD_STATUS_PORT) & 0x01)
    {
        uint8_t sc = inb(KBD_DATA_PORT);
        keyboard_process_scancode(sc);
    }
}

int keyboard_poll_char(char* out_char)
{
    return keybuf_pop(out_char);
}
