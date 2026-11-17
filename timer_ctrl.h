#ifndef TIMER_CTRL_H__
#define TIMER_CTRL_H__

#include <stdint.h>

#define TICK_MS              1

void timer_ctrl_init(void);
uint32_t timer_ctrl_ms(void);

#endif
