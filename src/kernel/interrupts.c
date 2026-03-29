// interrupts.c: C-level interrupt/exception handlers
// Handles ISRs, IRQs, and dispatches to device drivers

#include "interrupts.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "stdio.h"
#include "timer.h"
#include "vga.h"

static volatile uint32_t g_Irq0Count = 0;
static volatile uint32_t g_Irq1Count = 0;

void interrupts_install_exceptions(void)
{
    for (uint32_t i = 0; i < 32; i++)
        idt_set_gate((uint8_t)i, (uint32_t)isr_stub_table[i], 0x08, 0x8E);
}

void interrupts_install_irqs(void)
{
    for (uint32_t i = 0; i < 16; i++)
        idt_set_gate((uint8_t)(32 + i), (uint32_t)irq_stub_table[i], 0x08, 0x8E);
}

void isr_handler(int_frame_t* frame)
{
    printf("\n[FATAL] EXC #%x err=0x%x eip=0x%x\n",
           frame->int_no,
           frame->err_code,
           frame->eip);

    __asm__ __volatile__("cli");
    for (;;)
        __asm__ __volatile__("hlt");
}

void irq_handler(int_frame_t* frame)
{
    uint8_t irq = (uint8_t)(frame->int_no - 32);

    if (irq == 0)
    {
        g_Irq0Count++;
        timer_on_tick();
    }
    else if (irq == 1)
    {
        g_Irq1Count++;
        keyboard_on_irq();
    }

    pic_send_eoi(irq);
}

uint32_t interrupts_get_irq0_count(void)
{
    return g_Irq0Count;
}

uint32_t interrupts_get_irq1_count(void)
{
    return g_Irq1Count;
}
