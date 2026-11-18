#ifndef LED_CTRL_H
#define LED_CTRL_H

#include <stdint.h>
#include <stdbool.h>

#define PWM_CLOCK_HZ      1000000UL
#define PWM_FREQUENCY_HZ  1000UL
#define PWM_TOP_VALUE     (PWM_CLOCK_HZ / PWM_FREQUENCY_HZ)

#define FADE_STEP_MS      10
#define DUTY_STEP         (PWM_TOP_VALUE / 100)

void led_ctrl_init(void);
void led_ctrl_step(void);
void led_ctrl_toggle(void);
bool led_ctrl_is_blinking(void);

extern volatile bool     g_led_changed;
extern volatile uint8_t  g_active_led;

#endif