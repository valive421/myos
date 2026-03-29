#include "idt.h"
#include "boot.h"
#include "interrupts.h"

struct idt_entry
{
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry g_Idt[256];
static struct idt_ptr g_IdtPtr;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    g_Idt[num].base_lo = (uint16_t)(base & 0xFFFF);
    g_Idt[num].base_hi = (uint16_t)((base >> 16) & 0xFFFF);
    g_Idt[num].sel = sel;
    g_Idt[num].always0 = 0;
    g_Idt[num].flags = flags;
}

void idt_init(void)
{
    for (uint32_t i = 0; i < 256; i++)
        idt_set_gate((uint8_t)i, 0, 0, 0);

    interrupts_install_exceptions();
    interrupts_install_irqs();

    g_IdtPtr.limit = (uint16_t)(sizeof(g_Idt) - 1);
    g_IdtPtr.base = linear_addr(&g_Idt);

    asm volatile ("lidt %0" : : "m"(g_IdtPtr));
}
