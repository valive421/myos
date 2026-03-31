#pragma once

#include "types.h"

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

static inline uint32_t sys_yield(void)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_YIELD) : "cc", "memory");
    return ret;
}

static inline uint32_t sys_sleep(uint32_t ticks)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_SLEEP), "b"(ticks) : "cc", "memory");
    return ret;
}

__attribute__((noreturn)) static inline void sys_exit(uint32_t code)
{
    __asm__ __volatile__("int $0x80" : : "a"(SYSCALL_EXIT), "b"(code) : "cc", "memory");
    for (;;)
        ;
}

static inline uint32_t sys_write(const char* s)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_WRITE), "b"(s) : "cc", "memory");
    return ret;
}

static inline uint32_t sys_putc(char c)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_PUTC), "b"((uint32_t)(uint8_t)c) : "cc", "memory");
    return ret;
}

static inline void* sys_alloc(uint32_t size)
{
    void* ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_ALLOC), "b"(size) : "cc", "memory");
    return ret;
}

static inline uint32_t sys_free(void* ptr)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_FREE), "b"(ptr) : "cc", "memory");
    return ret;
}
