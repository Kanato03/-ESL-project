#ifndef TIMER_CTRL_H
#define TIMER_CTRL_H

#include <stdint.h>
#include "nrfx_timer.h"

#define TICK_MS 10

void timer_ctrl_init(void);
uint32_t timer_ctrl_ms(void);

#endif