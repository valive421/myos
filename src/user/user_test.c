#include "valive/syscall.h"

static void u32_to_hex(char out[11], uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    out[0] = '0';
    out[1] = 'x';
    for (uint32_t i = 0; i < 8; i++)
    {
        uint32_t shift = 28u - (i * 4u);
        out[2 + i] = hex[(v >> shift) & 0x0Fu];
    }
    out[10] = 0;
}

static void write_u32_hex(uint32_t v)
{
    char buf[11];
    u32_to_hex(buf, v);
    sys_write(buf);
}

__attribute__((noreturn))
void user_test_entry(void)
{
    sys_write("[U] user_test: hello from Ring3\n");

    for (uint32_t iter = 0; iter < 8; iter++)
    {
        uint32_t size = 64u + (iter * 16u);

        uint8_t* buf = (uint8_t*)sys_alloc(size);
        if (!buf)
        {
            sys_write("[U] alloc failed\n");
            sys_exit(1);
        }

        uint32_t sum = 0;
        for (uint32_t i = 0; i < size; i++)
        {
            uint8_t v = (uint8_t)(i ^ (iter * 13u));
            buf[i] = v;
            sum += (uint32_t)v;
        }

        sys_write("[U] iter=");
        write_u32_hex(iter);
        sys_write(" size=");
        write_u32_hex(size);
        sys_write(" sum=");
        write_u32_hex(sum);
        sys_write("\n");

        sys_free(buf);

        sys_sleep(10);
        sys_yield();
    }

    sys_write("[U] user_test: exiting via syscall\n");
    sys_exit(0);
}
