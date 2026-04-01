#pragma once

#include "types.h"
#include "syscall.h"

// User-visible flags for the minimal RAM VFS.
// Matches kernel values in src/kernel/vfs.h.

enum
{
    VFS_O_RDONLY = 0x1u,
    VFS_O_WRONLY = 0x2u,
    VFS_O_RDWR = 0x3u,

    VFS_O_CREAT = 0x100u,
    VFS_O_TRUNC = 0x200u,
    VFS_O_APPEND = 0x400u
};

enum
{
    VFS_SEEK_SET = 0,
    VFS_SEEK_CUR = 1,
    VFS_SEEK_END = 2
};

static inline int sys_fs_open(const char* name, uint32_t flags)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80"
                         : "=a"(ret)
                         : "a"(SYSCALL_FS_OPEN), "b"(name), "c"(flags)
                         : "cc", "memory");
    return (int)ret;
}

static inline int sys_fs_close(int fd)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(SYSCALL_FS_CLOSE), "b"(fd) : "cc", "memory");
    return (int)ret;
}

static inline int sys_fs_read(int fd, void* buf, uint32_t len)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80"
                         : "=a"(ret)
                         : "a"(SYSCALL_FS_READ), "b"(fd), "c"(buf), "d"(len)
                         : "cc", "memory");
    return (int)ret;
}

static inline int sys_fs_write(int fd, const void* buf, uint32_t len)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80"
                         : "=a"(ret)
                         : "a"(SYSCALL_FS_WRITE), "b"(fd), "c"(buf), "d"(len)
                         : "cc", "memory");
    return (int)ret;
}

static inline int sys_fs_seek(int fd, int offset, uint32_t whence)
{
    uint32_t ret;
    __asm__ __volatile__("int $0x80"
                         : "=a"(ret)
                         : "a"(SYSCALL_FS_SEEK), "b"(fd), "c"(offset), "d"(whence)
                         : "cc", "memory");
    return (int)ret;
}
