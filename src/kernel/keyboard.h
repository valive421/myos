#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
void keyboard_on_irq(void);
void keyboard_poll_hw(void);
int keyboard_poll_char(char* out_char);

#endif
