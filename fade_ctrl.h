#ifndef FADE_CTRL_H__
#define FADE_CTRL_H__

#include <stdint.h>
#include "pwm_ctrl.h"

#define FADE_HALF_PERIOD_MS  1000
#define FADE_STEP_MS         10
#define FADE_STEPS           (FADE_HALF_PERIOD_MS / FADE_STEP_MS)
#define DUTY_STEP            (PWM_TOP_VALUE / FADE_STEPS)

void fade_ctrl_init(void);
void fade_ctrl_on_tick(void);
void fade_ctrl_toggle(void);
uint32_t fade_ctrl_sequence_len(void);

#endif
