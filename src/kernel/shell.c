#include "shell.h"
#include "keyboard.h"
#include "stdio.h"
#include "timer.h"
#include "vga.h"
#include "types.h"

#define SHELL_LINE_MAX 128

static char g_Line[SHELL_LINE_MAX];
static uint32_t g_LineLen = 0;
static uint8_t g_TicksLive = 0;
static uint32_t g_LastTicksDrawn = 0xFFFFFFFFu;

static void write_at(uint8_t row, uint8_t col, const char* s)
{
    while (*s && col < 80)
    {
        vga_write_at(row, col, *s);
        col++;
        s++;
    }
}

static void clear_line(uint8_t row)
{
    for (uint8_t col = 0; col < 80; col++)
        vga_write_at(row, col, ' ');
}

static void draw_ticks_live(void)
{
    static const char hex[] = "0123456789ABCDEF";
    uint32_t t = timer_get_ticks();

    if (t == g_LastTicksDrawn)
        return;

    g_LastTicksDrawn = t;
    write_at(2, 0, "ticks(live)=0x");

    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t shift = (uint8_t)(28 - i * 4);
        char c = hex[(t >> shift) & 0x0F];
        vga_write_at(2, (uint8_t)(13 + i), c);
    }
}

static int str_eq(const char* a, const char* b)
{
    while (*a && *b)
    {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }

    return (*a == 0 && *b == 0);
}

static int starts_with(const char* s, const char* prefix)
{
    while (*prefix)
    {
        if (*s != *prefix)
            return 0;
        s++;
        prefix++;
    }

    return 1;
}

static const char* skip_spaces(const char* s)
{
    while (*s == ' ')
        s++;
    return s;
}

static void print_prompt(void)
{
    printf("Valive> ");
}

static void execute_command(const char* cmd)
{
    cmd = skip_spaces(cmd);

    if (*cmd == 0)
        return;

    if (str_eq(cmd, "help"))
    {
        printf("Commands: help clear ticks echo\n");
        return;
    }

    if (str_eq(cmd, "clear"))
    {
        vga_clear();
        return;
    }

    if (str_eq(cmd, "ticks"))
    {
        g_TicksLive = 1;
        g_LastTicksDrawn = 0xFFFFFFFFu;
        printf("ticks live mode (press q to quit)\n");
        draw_ticks_live();
        return;
    }

    if (starts_with(cmd, "echo"))
    {
        const char* p = cmd + 4;
        p = skip_spaces(p);
        printf("%s\n", p);
        return;
    }

    printf("Unknown command: %s\n", cmd);
}

static void shell_on_char(char c)
{
    if (c == '\r')
        c = '\n';

    if (c == '\n')
    {
        putc('\n');
        g_Line[g_LineLen] = 0;
        execute_command(g_Line);
        g_LineLen = 0;
        print_prompt();
        return;
    }

    if (c == '\b')
    {
        if (g_LineLen > 0)
        {
            g_LineLen--;
            vga_backspace();
        }
        return;
    }

    if (c < 32 || c > 126)
        return;

    if (g_LineLen + 1 < SHELL_LINE_MAX)
    {
        g_Line[g_LineLen++] = c;
        putc(c);
    }
}

void shell_init(void)
{
    g_LineLen = 0;
    g_TicksLive = 0;
    g_LastTicksDrawn = 0xFFFFFFFFu;
    print_prompt();
}

void shell_poll(void)
{
    char c;

    if (g_TicksLive)
    {
        draw_ticks_live();

        while (keyboard_poll_char(&c))
        {
            if (c == 'q' || c == 'Q')
            {
                g_TicksLive = 0;
                clear_line(2);
				printf("\nleft ticks live mode\n");
                print_prompt();
                break;
            }
        }

        return;
    }

    while (keyboard_poll_char(&c))
        shell_on_char(c);
}
