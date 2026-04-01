#include "syscall.h"

#include "memory.h"
#include "serial.h"
#include "keyboard.h"
#include "task.h"
#include "vfs.h"
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

        // VGA only: serial output can block under host stdio backpressure.
        putc(c);
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
            // VGA only: serial output can block under host stdio backpressure.
            putc(c);
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

        case SYSCALL_FS_OPEN:
            frame->eax = (uint32_t)vfs_open((uint32_t)current, (const char*)frame->ebx, frame->ecx);
            return;

        case SYSCALL_FS_CLOSE:
            frame->eax = (uint32_t)vfs_close((uint32_t)current, (int)frame->ebx);
            return;

        case SYSCALL_FS_READ:
            frame->eax = (uint32_t)vfs_read((uint32_t)current,
                                            (int)frame->ebx,
                                            (void*)frame->ecx,
                                            frame->edx);
            return;

        case SYSCALL_FS_WRITE:
            frame->eax = (uint32_t)vfs_write((uint32_t)current,
                                             (int)frame->ebx,
                                             (const void*)frame->ecx,
                                             frame->edx);
            return;

        case SYSCALL_FS_SEEK:
            frame->eax = (uint32_t)vfs_seek((uint32_t)current,
                                            (int)frame->ebx,
                                            (int)frame->ecx,
                                            frame->edx);
            return;

        case SYSCALL_GETC:
        {
            char c;
            if (keyboard_poll_char(&c))
                frame->eax = (uint32_t)(uint8_t)c;
            else
                frame->eax = 0xFFFFFFFFu;
            return;
        }

        default:
            frame->eax = 0xFFFFFFFFu;
            return;
    }
}
