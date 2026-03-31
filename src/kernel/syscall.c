#include "syscall.h"

#include "memory.h"
#include "serial.h"
#include "task.h"
#include "vga.h"

static uint32_t syscall_write_str(const char* s)
{
    if (!s)
        return 0;

    uint32_t n = 0;
    while (n < 4096u)
    {
        char c = s[n];
        if (c == 0)
            break;

        putc(c);
        if (c == '\n')
            serial_write_char('\r');
        serial_write_char(c);
        n++;
    }

    return n;
}

void syscall_handle(int_frame_t* frame)
{
    if (!frame)
        return;

    task_t* current = task_current();
    uint32_t num = frame->eax;

    switch (num)
    {
        case SYSCALL_YIELD:
            if (current)
            {
                __asm__ __volatile__("sti" : : : "memory");
                task_yield(current);
            }
            frame->eax = 0;
            return;

        case SYSCALL_SLEEP:
            if (current)
            {
                uint32_t ticks = frame->ebx;
                __asm__ __volatile__("sti" : : : "memory");
                task_sleep(current, ticks);
            }
            frame->eax = 0;
            return;

        case SYSCALL_EXIT:
            if (current)
            {
                __asm__ __volatile__("sti" : : : "memory");
                task_exit(current);
            }
            frame->eax = 0;
            return;

        case SYSCALL_WRITE:
            frame->eax = syscall_write_str((const char*)frame->ebx);
            return;

        case SYSCALL_PUTC:
        {
            char c = (char)(frame->ebx & 0xFFu);
            putc(c);
            if (c == '\n')
                serial_write_char('\r');
            serial_write_char(c);
            frame->eax = 0;
            return;
        }

        case SYSCALL_ALLOC:
            frame->eax = (uint32_t)kmalloc(frame->ebx);
            return;

        case SYSCALL_FREE:
            kfree((void*)frame->ebx);
            frame->eax = 0;
            return;

        default:
            frame->eax = 0xFFFFFFFFu;
            return;
    }
}
