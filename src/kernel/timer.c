#include "timer.h"
#include "io.h"

#define PIT_INPUT_HZ 1193182u
#define PIT_CMD      0x43
#define PIT_CH0      0x40

static volatile uint32_t g_Ticks = 0;

void timer_init(uint32_t hz)
{
    if (hz == 0)
        hz = 100;

    uint32_t divisor = PIT_INPUT_HZ / hz;
    if (divisor == 0)
        divisor = 1;
    if (divisor > 65535u)
        divisor = 65535u;

    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_on_tick(void)
{
    g_Ticks++;
}

uint32_t timer_get_ticks(void)
{
    return g_Ticks;
}
