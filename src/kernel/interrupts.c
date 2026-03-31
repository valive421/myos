// interrupts.c: C-level interrupt/exception handlers
// Handles ISRs, IRQs, and dispatches to device drivers

#include "interrupts.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "syscall.h"
#include "stdio.h"
#include "task.h"
#include "timer.h"
#include "vga.h"

static volatile uint32_t g_Irq0Count = 0;
static volatile uint32_t g_Irq1Count = 0;

extern void isr128(void);

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

void interrupts_install_syscalls(void)
{
    // Present + 32-bit interrupt gate + DPL=3 so Ring3 can invoke int 0x80.
    idt_set_gate(0x80, (uint32_t)isr128, 0x08, 0xEE);
}

void isr_handler(int_frame_t* frame)
{
    if (frame && frame->int_no == 0x80)
    {
        syscall_handle(frame);
        return;
    }

    // If a user task faults (e.g., privileged instruction), kill it but keep the kernel alive.
    if (frame && ((frame->cs & 3u) == 3u))
    {
        printf("\n[USER-FAULT] EXC #%x err=0x%x eip=0x%x\n",
               frame->int_no,
               frame->err_code,
               frame->eip);

        task_t* cur = task_current();
        if (cur)
        {
            __asm__ __volatile__("sti" : : : "memory");
            task_exit(cur);
        }
    }

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
