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
    SYSCALL_FREE = 6
};

void syscall_handle(int_frame_t* frame);
