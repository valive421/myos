#pragma once

#include "types.h"

// Minimal VFS.
// Provides a flat namespace of files by name and a small fd table.
// When a QEMU persistent disk is attached (see run.sh) and contains a FAT12 volume,
// VFS state is loaded from FAT12 root directory at boot and flushed back on close.

#define VFS_MAX_FILES 32
#define VFS_MAX_FDS 32
#define VFS_NAME_MAX 32

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

void vfs_init(void);

int vfs_open(uint32_t owner, const char* name, uint32_t flags);
int vfs_close(uint32_t owner, int fd);
int vfs_read(uint32_t owner, int fd, void* buf, uint32_t len);
int vfs_write(uint32_t owner, int fd, const void* buf, uint32_t len);
int vfs_seek(uint32_t owner, int fd, int offset, uint32_t whence);
