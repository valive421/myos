#include "boot.h"
#include "io.h"

uint32_t linear_addr(const void* p)
{
    return kernel_base_phys + (uint32_t)p;
}

uint32_t kernel_linear_addr(const void* p)
{
    return linear_addr(p);
}

void boot_enable_a20_fast(void)
{
    uint8_t v = inb(0x92);
    v |= 0x02;
    outb(0x92, v);
}
