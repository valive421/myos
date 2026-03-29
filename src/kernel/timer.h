// timer.h: Timer function declarations
// Declares timer init and tick handler prototypes

#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include "types.h"

void timer_init(uint32_t hz);
void timer_on_tick(void);
uint32_t timer_get_ticks(void);

#endif
