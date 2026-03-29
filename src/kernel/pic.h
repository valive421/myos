#ifndef KERNEL_PIC_H
#define KERNEL_PIC_H

#include "types.h"

void pic_remap(uint8_t offset1, uint8_t offset2);
void pic_mask_all(void);
void pic_unmask_timer_only(void);
void pic_unmask_timer_keyboard(void);
void pic_send_eoi(uint8_t irq);

#endif
