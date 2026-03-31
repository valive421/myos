// shell.c: Minimal kernel shell with built-in diagnostics commands.
//
// Features:
// - line-based command input
// - command dispatch for memory/tasking/timer diagnostics
// - live ticks display mode

#include "shell.h"
#include "keyboard.h"
#include "memory.h"
#include "stdio.h"
#include "task.h"
#include "timer.h"
#include "vga.h"
#include "types.h"

#define SHELL_LINE_MAX 128

static char g_Line[SHELL_LINE_MAX];
static uint32_t g_LineLen = 0;
static uint8_t g_TicksLive = 0;
static uint32_t g_LastTicksDrawn = 0xFFFFFFFFu;

// Write a string at fixed VGA coordinates.
static void write_at(uint8_t row, uint8_t col, const char* s)
{
    while (*s && col < 80)
    {
        vga_write_at(row, col, *s);
        col++;
        s++;
    }
}

// Clear one VGA text line.
static void clear_line(uint8_t row)
{
    for (uint8_t col = 0; col < 80; col++)
        vga_write_at(row, col, ' ');
}

// Update live ticks monitor line if value changed.
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

// String equality helper (no libc).
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

// Prefix check helper (no libc).
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

// Skip leading spaces in command arguments.
static const char* skip_spaces(const char* s)
{
    while (*s == ' ')
        s++;
    return s;
}

// Parse unsigned decimal integer from command arguments.
static int parse_u32(const char* s, uint32_t* out)
{
    uint32_t value = 0;
    int has_digit = 0;

    s = skip_spaces(s);
    while (*s >= '0' && *s <= '9')
    {
        has_digit = 1;
        value = (value * 10u) + (uint32_t)(*s - '0');
        s++;
    }

    if (!has_digit)
        return 0;

    *out = value;
    return 1;
}

// Print shell prompt.
static void print_prompt(void)
{
    printf("Valive> ");
}

// Execute one parsed command line.
static void execute_command(const char* cmd)
{
    cmd = skip_spaces(cmd);

    if (*cmd == 0)
        return;

    if (str_eq(cmd, "help"))
    {
        printf("Commands: help clear ticks echo mem memtest tasks spawn kill tasktest\n");
        return;
    }

    if (str_eq(cmd, "tasks"))
    {
        printf("tasks: %u/%u\n", task_count(), task_max_slots());
        for (uint32_t i = 0; i < task_max_slots(); i++)
        {
            task_info_t info;
            if (task_get_info(i, &info))
                printf("id=%u state=%u runs=%u name=%s\n", info.id, info.state, info.runs, info.name);
        }
        return;
    }

    if (str_eq(cmd, "spawn"))
    {
        int id = task_spawn_counter();
        if (id > 0)
            printf("spawned counter task id=%d\n", id);
        else
            printf("spawn failed\n");
        return;
    }

    if (starts_with(cmd, "kill"))
    {
        uint32_t id = 0;
        if (!parse_u32(cmd + 4, &id))
        {
            printf("usage: kill <id>\n");
            return;
        }

        printf(task_kill(id) ? "killed task %u\n" : "task %u not found\n", id);
        return;
    }

    if (str_eq(cmd, "tasktest"))
    {
        printf("task self-test: %s\n", task_run_self_test() ? "PASS" : "FAIL");
        tasking_init();
        return;
    }

    if (str_eq(cmd, "mem"))
    {
        printf("heap total=%u used=%u free=%u\n",
            memory_heap_total(),
            memory_heap_used(),
            memory_heap_free());
        return;
    }

    if (str_eq(cmd, "memtest"))
    {
        printf("memory self-test: %s\n", memory_run_self_test() ? "PASS" : "FAIL");
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

// Handle one input character in line-edit mode.
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

// Initialize shell state and print first prompt.
void shell_init(void)
{
    g_LineLen = 0;
    g_TicksLive = 0;
    g_LastTicksDrawn = 0xFFFFFFFFu;
    print_prompt();
}

// Poll input and process shell modes.
// In ticks-live mode, only watch for quit key (q/Q).
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
