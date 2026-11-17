#ifndef PWM_CTRL_H__
#define PWM_CTRL_H__

#include <stdint.h>

#define PWM_CLOCK_HZ       1000000UL
#define PWM_FREQUENCY_HZ   1000UL
#define PWM_TOP_VALUE      (PWM_CLOCK_HZ / PWM_FREQUENCY_HZ)

void pwm_ctrl_init(void);
void pwm_ctrl_set_duty(uint32_t led_idx, uint16_t duty);

#endif
