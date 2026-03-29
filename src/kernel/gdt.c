#include "gdt.h"
#include "boot.h"

#define GDT_PRESENT 0x80
#define GDT_RING0   0x00
#define GDT_CODE    0x18
#define GDT_DATA    0x10
#define GDT_GRAN4K  0x80
#define GDT_32BIT   0x40

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags_limit_hi;
    uint8_t base_hi;
} __attribute__((packed));

struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss_entry
{
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry g_Gdt[6];
static struct gdt_ptr g_GdtPtr;
static struct tss_entry g_Tss;

static void gdt_set_entry(uint32_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    g_Gdt[index].limit_low = (uint16_t)(limit & 0xFFFF);
    g_Gdt[index].base_low = (uint16_t)(base & 0xFFFF);
    g_Gdt[index].base_mid = (uint8_t)((base >> 16) & 0xFF);
    g_Gdt[index].access = access;
    g_Gdt[index].flags_limit_hi = (uint8_t)(((limit >> 16) & 0x0F) | (flags & 0xF0));
    g_Gdt[index].base_hi = (uint8_t)((base >> 24) & 0xFF);
}

static void tss_clear(void)
{
    uint8_t* p = (uint8_t*)&g_Tss;
    for (uint32_t i = 0; i < sizeof(g_Tss); i++)
        p[i] = 0;
}

void gdt_init(void)
{
    uint32_t seg_base = kernel_base_phys;

    gdt_set_entry(0, 0x00000000, 0x00000000, 0x00, 0x00);

    gdt_set_entry(1, seg_base, 0x000FFFFF,
        GDT_PRESENT | GDT_RING0 | GDT_CODE | 0x02,
        GDT_GRAN4K | GDT_32BIT);

    gdt_set_entry(2, seg_base, 0x000FFFFF,
        GDT_PRESENT | GDT_RING0 | GDT_DATA | 0x02,
        GDT_GRAN4K | GDT_32BIT);

    gdt_set_entry(3, seg_base, 0x000FFFFF,
        GDT_PRESENT | 0x60 | GDT_CODE | 0x02,
        GDT_GRAN4K | GDT_32BIT);

    gdt_set_entry(4, seg_base, 0x000FFFFF,
        GDT_PRESENT | 0x60 | GDT_DATA | 0x02,
        GDT_GRAN4K | GDT_32BIT);

    tss_clear();
    g_Tss.ss0 = 0x10;
    g_Tss.esp0 = 0x0000F000;
    g_Tss.iomap_base = (uint16_t)sizeof(g_Tss);

    gdt_set_entry(5, linear_addr(&g_Tss), sizeof(g_Tss) - 1,
        GDT_PRESENT | GDT_RING0 | 0x09, 0x00);

    g_GdtPtr.limit = (uint16_t)(sizeof(g_Gdt) - 1);
    g_GdtPtr.base = linear_addr(&g_Gdt);

    asm volatile ("lgdt %0" : : "m"(g_GdtPtr));

    asm volatile (
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        // gdt.c: Global Descriptor Table setup for protected mode
        // Defines GDT entries and loads GDT for x86 segmentation
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        "ljmp $0x08, $.gdt_flush_done\n"
        ".gdt_flush_done:\n"
        :
        :
        : "ax", "memory"
    );

    asm volatile (
        "movw $0x28, %%ax\n"
        "ltr %%ax\n"
        :
        :
        : "ax", "memory"
    );
}
