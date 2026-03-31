// interrupts.h: Interrupt/exception frame and handler declarations
// Defines int_frame_t and handler prototypes
#ifndef KERNEL_INTERRUPTS_H
#define KERNEL_INTERRUPTS_H

#include "types.h"

typedef struct
{
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} int_frame_t;

extern void* isr_stub_table[32];
extern void* irq_stub_table[16];

void interrupts_install_exceptions(void);
void interrupts_install_irqs(void);
void interrupts_install_syscalls(void);

void isr_handler(int_frame_t* frame);
void irq_handler(int_frame_t* frame);

uint32_t interrupts_get_irq0_count(void);
uint32_t interrupts_get_irq1_count(void);

#endif
