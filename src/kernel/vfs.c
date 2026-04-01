#include "vfs.h"

#include "memory.h"
#include "fat12.h"

typedef struct
{
    uint8_t used;
    char name[VFS_NAME_MAX];
    uint8_t* data;
    uint32_t size;
    uint32_t capacity;
} vfs_file_t;

typedef struct
{
    uint8_t used;
    uint32_t owner;
    uint32_t flags;
    vfs_file_t* file;
    uint32_t pos;
} vfs_fd_t;

static vfs_file_t g_Files[VFS_MAX_FILES];
static vfs_fd_t g_Fds[VFS_MAX_FDS];

static uint8_t g_PersistEnabled;
static uint8_t g_VfsDirty;

static int vfs_persist_load(void);
static int vfs_persist_flush(void);

static uint8_t to_upper(uint8_t c)
{
    if (c >= 'a' && c <= 'z')
        return (uint8_t)(c - ('a' - 'A'));
    return c;
}

static uint32_t str_len_max(const char* s, uint32_t max)
{
    if (!s)
        return 0;

    uint32_t n = 0;
    while (n < max && s[n])
        n++;

    return n;
}

static void str_copy_max(char* dst, uint32_t dst_max, const char* src)
{
    if (!dst || dst_max == 0)
        return;

    uint32_t i = 0;
    if (src)
    {
        while (i + 1 < dst_max && src[i])
        {
            dst[i] = src[i];
            i++;
        }
    }

    dst[i] = 0;
    while (++i < dst_max)
        dst[i] = 0;
}

static int str_eq(const char* a, const char* b)
{
    if (a == b)
        return 1;
    if (!a || !b)
        return 0;

    while (*a && *b)
    {
        if (to_upper((uint8_t)*a) != to_upper((uint8_t)*b))
            return 0;
        a++;
        b++;
    }

    return (*a == 0 && *b == 0);
}

static int file_find_by_name(const char* name)
{
    if (!name || !name[0])
        return -1;

    // Hard cap for sanity: we only support short names.
    if (str_len_max(name, VFS_NAME_MAX * 4u) >= (VFS_NAME_MAX * 4u))
        return -1;

    for (uint32_t i = 0; i < VFS_MAX_FILES; i++)
    {
        if (g_Files[i].used && str_eq(g_Files[i].name, name))
            return (int)i;
    }

    return -1;
}

static int file_alloc_slot(void)
{
    for (uint32_t i = 0; i < VFS_MAX_FILES; i++)
    {
        if (!g_Files[i].used)
            return (int)i;
    }

    return -1;
}

static int fd_alloc_slot(void)
{
    for (uint32_t i = 0; i < VFS_MAX_FDS; i++)
    {
        if (!g_Fds[i].used)
            return (int)i;
    }

    return -1;
}

static int fd_validate_owner(uint32_t owner, int fd)
{
    if (fd < 0 || fd >= (int)VFS_MAX_FDS)
        return 0;
    if (!g_Fds[fd].used)
        return 0;
    if (g_Fds[fd].owner != owner)
        return 0;
    if (!g_Fds[fd].file)
        return 0;

    return 1;
}

void vfs_init(void)
{
    g_PersistEnabled = 0;
    g_VfsDirty = 0;

    for (uint32_t i = 0; i < VFS_MAX_FILES; i++)
    {
        if (g_Files[i].data)
            kfree(g_Files[i].data);

        g_Files[i].used = 0;
        g_Files[i].name[0] = 0;
        g_Files[i].data = 0;
        g_Files[i].size = 0;
        g_Files[i].capacity = 0;
    }

    for (uint32_t i = 0; i < VFS_MAX_FDS; i++)
    {
        g_Fds[i].used = 0;
        g_Fds[i].owner = 0;
        g_Fds[i].flags = 0;
        g_Fds[i].file = 0;
        g_Fds[i].pos = 0;
    }

    // FAT12-backed persistence: if the attached disk contains a FAT12 volume,
    // load root-directory files into the in-memory VFS.
    if (fat12_init())
    {
        g_PersistEnabled = 1;
        (void)vfs_persist_load();
    }
}

int vfs_open(uint32_t owner, const char* name, uint32_t flags)
{
    if (!owner || !name || !name[0])
        return -1;

    // When persistence is enabled, enforce FAT12 8.3 filenames (no LFN).
    if (g_PersistEnabled && !fat12_is_valid_name(name))
        return -1;

    int idx = file_find_by_name(name);
    if (idx < 0)
    {
        if (!(flags & VFS_O_CREAT))
            return -1;

        idx = file_alloc_slot();
        if (idx < 0)
            return -1;

        g_Files[idx].used = 1;
        str_copy_max(g_Files[idx].name, VFS_NAME_MAX, name);
        g_Files[idx].data = 0;
        g_Files[idx].size = 0;
        g_Files[idx].capacity = 0;
        g_VfsDirty = 1;
    }

    vfs_file_t* f = &g_Files[idx];

    if (flags & VFS_O_TRUNC)
    {
        if (f->size != 0)
            g_VfsDirty = 1;
        f->size = 0;
    }

    int fd = fd_alloc_slot();
    if (fd < 0)
        return -1;

    g_Fds[fd].used = 1;
    g_Fds[fd].owner = owner;
    g_Fds[fd].flags = flags;
    g_Fds[fd].file = f;
    g_Fds[fd].pos = (flags & VFS_O_APPEND) ? f->size : 0;

    return fd;
}

int vfs_close(uint32_t owner, int fd)
{
    if (!fd_validate_owner(owner, fd))
        return -1;

    g_Fds[fd].used = 0;
    g_Fds[fd].owner = 0;
    g_Fds[fd].flags = 0;
    g_Fds[fd].file = 0;
    g_Fds[fd].pos = 0;

    if (g_PersistEnabled && g_VfsDirty)
    {
        if (vfs_persist_flush())
            g_VfsDirty = 0;
    }
    return 0;
}

int vfs_read(uint32_t owner, int fd, void* buf, uint32_t len)
{
    if (!buf)
        return -1;
    if (len == 0)
        return 0;
    if (!fd_validate_owner(owner, fd))
        return -1;

    uint32_t mode = (g_Fds[fd].flags & 0x3u);
    if (mode == VFS_O_WRONLY)
        return -1;

    vfs_file_t* f = g_Fds[fd].file;
    uint32_t pos = g_Fds[fd].pos;
    if (pos >= f->size)
        return 0;

    uint32_t remaining = f->size - pos;
    uint32_t to_read = (len < remaining) ? len : remaining;

    uint8_t* out = (uint8_t*)buf;
    for (uint32_t i = 0; i < to_read; i++)
        out[i] = f->data[pos + i];

    g_Fds[fd].pos = pos + to_read;
    return (int)to_read;
}

static int ensure_capacity(vfs_file_t* f, uint32_t needed)
{
    if (!f)
        return 0;
    if (needed <= f->capacity)
        return 1;

    uint32_t new_cap = (f->capacity == 0) ? 64u : f->capacity;
    while (new_cap < needed)
    {
        uint32_t next = new_cap * 2u;
        if (next <= new_cap)
            return 0;
        new_cap = next;
    }

    uint8_t* new_data = (uint8_t*)kmalloc(new_cap);
    if (!new_data)
        return 0;

    for (uint32_t i = 0; i < f->size; i++)
        new_data[i] = f->data ? f->data[i] : 0;

    if (f->data)
        kfree(f->data);

    f->data = new_data;
    f->capacity = new_cap;
    return 1;
}

int vfs_write(uint32_t owner, int fd, const void* buf, uint32_t len)
{
    if (!buf)
        return -1;
    if (len == 0)
        return 0;
    if (!fd_validate_owner(owner, fd))
        return -1;

    uint32_t mode = (g_Fds[fd].flags & 0x3u);
    if (mode == VFS_O_RDONLY)
        return -1;

    vfs_file_t* f = g_Fds[fd].file;
    uint32_t pos = g_Fds[fd].pos;

    uint32_t needed = pos + len;
    if (!ensure_capacity(f, needed))
        return -1;

    const uint8_t* in = (const uint8_t*)buf;
    for (uint32_t i = 0; i < len; i++)
        f->data[pos + i] = in[i];

    pos += len;
    g_Fds[fd].pos = pos;
    if (pos > f->size)
        f->size = pos;

    g_VfsDirty = 1;

    return (int)len;
}

int vfs_seek(uint32_t owner, int fd, int offset, uint32_t whence)
{
    if (!fd_validate_owner(owner, fd))
        return -1;

    vfs_file_t* f = g_Fds[fd].file;

    int base = 0;
    switch (whence)
    {
        case VFS_SEEK_SET:
            base = 0;
            break;
        case VFS_SEEK_CUR:
            base = (int)g_Fds[fd].pos;
            break;
        case VFS_SEEK_END:
            base = (int)f->size;
            break;
        default:
            return -1;
    }

    int new_pos = base + offset;
    if (new_pos < 0)
        return -1;

    if ((uint32_t)new_pos > f->size)
        new_pos = (int)f->size;

    g_Fds[fd].pos = (uint32_t)new_pos;
    return new_pos;
}
static int vfs_persist_load(void)
{
    if (!fat12_ready())
        return 0;

    fat12_file_info_t infos[VFS_MAX_FILES];
    uint32_t count = 0;
    if (!fat12_list_root(infos, VFS_MAX_FILES, &count))
        return 0;

    for (uint32_t i = 0; i < count; i++)
    {
        if (!infos[i].name[0])
            continue;

        int slot = file_alloc_slot();
        if (slot < 0)
            break;

        uint8_t* data = 0;
        uint32_t size = 0;
        if (!fat12_read_file(infos[i].name, &data, &size))
            continue;

        vfs_file_t* f = &g_Files[slot];
        f->used = 1;
        str_copy_max(f->name, VFS_NAME_MAX, infos[i].name);
        f->data = data;
        f->size = size;
        f->capacity = size;
    }

    // Loaded state is clean until first write.
    g_VfsDirty = 0;
    return 1;
}

static int vfs_persist_flush(void)
{
    if (!fat12_ready())
        return 0;

    for (uint32_t i = 0; i < VFS_MAX_FILES; i++)
    {
        if (!g_Files[i].used)
            continue;

        // Enforce 8.3 for persisted files.
        if (!fat12_is_valid_name(g_Files[i].name))
            return 0;

        if (!fat12_write_file(g_Files[i].name, g_Files[i].data, g_Files[i].size))
            return 0;
    }

    return 1;
}
