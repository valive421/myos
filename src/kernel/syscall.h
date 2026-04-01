#pragma once

#include "interrupts.h"

enum
{
    SYSCALL_YIELD = 0,
    SYSCALL_SLEEP = 1,
    SYSCALL_EXIT = 2,
    SYSCALL_WRITE = 3,
    SYSCALL_PUTC = 4,
    SYSCALL_ALLOC = 5,
    SYSCALL_FREE = 6,

    // Minimal VFS syscalls (RAM-backed VFS for now)
    SYSCALL_FS_OPEN = 7,
    SYSCALL_FS_CLOSE = 8,
    SYSCALL_FS_READ = 9,
    SYSCALL_FS_WRITE = 10,
    SYSCALL_FS_SEEK = 11,

    // Console/keyboard input (non-blocking)
    SYSCALL_GETC = 12
};

void syscall_handle(int_frame_t* frame);
