#ifndef LED_CTRL_H
#define LED_CTRL_H

#include <stdint.h>
#include "color.h"

#define PWM_CLOCK_HZ      1000000UL
#define PWM_FREQUENCY_HZ  1000UL
#define PWM_TOP_VALUE     (PWM_CLOCK_HZ / PWM_FREQUENCY_HZ)

void led_ctrl_init(void);
void led_ctrl_set_rgb(rgb_t rgb);
void led_ctrl_set_led1(uint16_t duty);

#endif
