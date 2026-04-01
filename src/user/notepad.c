#include "valive/syscall.h"
#include "valive/vfs.h"

// Ring3 interactive notepad.
// Launched from the kernel shell via: unote <file>
// Minimal UX:
// - displays current file content
// - you type to append/edit-at-end (backspace supported)
// - ESC saves + exits

typedef struct
{
    char* data;
    uint32_t len;
    uint32_t cap;
} text_buf_t;

static int buf_reserve(text_buf_t* b, uint32_t needed)
{
    if (!b)
        return 0;

    if (needed <= b->cap)
        return 1;

    uint32_t new_cap = (b->cap == 0) ? 256u : b->cap;
    while (new_cap < needed)
    {
        uint32_t next = new_cap * 2u;
        if (next <= new_cap)
            return 0;
        new_cap = next;
    }

    char* new_data = (char*)sys_alloc(new_cap);
    if (!new_data)
        return 0;

    for (uint32_t i = 0; i < b->len; i++)
        new_data[i] = b->data ? b->data[i] : 0;

    if (b->data)
        sys_free(b->data);

    b->data = new_data;
    b->cap = new_cap;
    b->data[b->len] = 0;
    return 1;
}

static int buf_append(text_buf_t* b, const char* s, uint32_t n)
{
    if (!b)
        return 0;
    if (!s && n)
        return 0;

    if (!buf_reserve(b, b->len + n + 1u))
        return 0;

    for (uint32_t i = 0; i < n; i++)
        b->data[b->len + i] = s[i];

    b->len += n;
    b->data[b->len] = 0;
    return 1;
}

static int load_file(const char* name, text_buf_t* b)
{
    int fd = sys_fs_open(name, VFS_O_CREAT | VFS_O_RDWR);
    if (fd < 0)
        return -1;

    sys_fs_seek(fd, 0, VFS_SEEK_SET);

    char tmp[64];
    for (;;)
    {
        int r = sys_fs_read(fd, tmp, sizeof(tmp));
        if (r < 0)
        {
            sys_fs_close(fd);
            return -1;
        }

        if (r == 0)
            break;

        if (!buf_append(b, tmp, (uint32_t)r))
        {
            sys_fs_close(fd);
            return -1;
        }
    }

    sys_fs_close(fd);
    return 0;
}

static int save_file(const char* name, const text_buf_t* b)
{
    int fd = sys_fs_open(name, VFS_O_CREAT | VFS_O_TRUNC | VFS_O_WRONLY);
    if (fd < 0)
        return -1;

    if (b && b->len)
    {
        int w = sys_fs_write(fd, b->data, b->len);
        if (w < 0 || (uint32_t)w != b->len)
        {
            sys_fs_close(fd);
            return -1;
        }
    }

    sys_fs_close(fd);
    return 0;
}

__attribute__((noreturn))
void user_notepad_entry(const char* path)
{
    const char* name = (path && path[0]) ? path : "notes.txt";

    sys_write("Notepad (Ring3)\n");
    sys_write("File: ");
    sys_write(name);
    sys_write("\n");
    sys_write("Press ESC to save and quit\n");
    sys_write("-------------------\n");

    text_buf_t buf;
    buf.data = 0;
    buf.len = 0;
    buf.cap = 0;

    if (load_file(name, &buf) < 0)
    {
        sys_write("unote: open/load failed\n");
        sys_exit(1);
    }

    // Display file contents (no prompts/commands).
    if (buf.data && buf.len)
    {
        for (uint32_t i = 0; i < buf.len; i++)
            sys_putc(buf.data[i]);
    }

    // Editor loop (append/edit-at-end).
    for (;;)
    {
        uint32_t ch = sys_getc();
        if (ch == 0)
        {
            sys_yield();
            continue;
        }

        if (ch == 27u)
        {
            // ESC: save and exit.
            if (save_file(name, &buf) < 0)
                sys_write("unote: save failed\n");

            if (buf.data)
                sys_free(buf.data);

            sys_exit(0);
        }

        char c = (char)ch;
        if (c == '\r')
            c = '\n';

        if (c == '\b')
        {
            if (buf.len > 0)
            {
                buf.len--;
                if (buf.data)
                    buf.data[buf.len] = 0;
                sys_putc('\b');
            }
            continue;
        }

        if (c == '\n')
        {
            if (!buf_append(&buf, "\n", 1))
                break;
            sys_putc('\n');
            continue;
        }

        if ((uint8_t)c < 32u || (uint8_t)c > 126u)
            continue;

        if (!buf_append(&buf, &c, 1))
            break;
        sys_putc(c);
    }

    // Out of memory: best-effort save, then exit.
    sys_write("unote: out of memory\n");
    save_file(name, &buf);

    if (buf.data)
        sys_free(buf.data);

    sys_exit(1);
}
