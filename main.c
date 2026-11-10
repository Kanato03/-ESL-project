/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** @file
 * @brief Blinky example for nRF52840 Dongle.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "nrfx_pwm.h"
#include "nrfx_timer.h"

#ifndef DEVICE_ID
#define DEVICE_ID 6598
#endif

#define LEDS_NUMBER     4

#define LED1_G_PORT     0   
#define LED1_G_NUM      6
#define LED2_R_PORT     0
#define LED2_R_NUM      8
#define LED2_G_PORT     1
#define LED2_G_NUM      9
#define LED2_B_PORT     0 
#define LED2_B_NUM      12
#define BUTTON_PORT     1
#define BUTTON_NUM      6

#define LED1_G_PIN      NRF_GPIO_PIN_MAP(LED1_G_PORT, LED1_G_NUM)
#define LED2_R_PIN      NRF_GPIO_PIN_MAP(LED2_R_PORT, LED2_R_NUM)
#define LED2_G_PIN      NRF_GPIO_PIN_MAP(LED2_G_PORT, LED2_G_NUM)
#define LED2_B_PIN      NRF_GPIO_PIN_MAP(LED2_B_PORT, LED2_B_NUM)
#define BUTTON_PIN      NRF_GPIO_PIN_MAP(BUTTON_PORT, BUTTON_NUM)

#define BUTTON_ACTIVE_LEVEL  0

#define PWM_INSTANCE_ID    0  
#define TIMER_INSTANCE_ID  1 
#define IRQ_PRIORITY       6 

#define PWM_CLOCK_HZ       1000000UL 
#define PWM_FREQUENCY_HZ   1000UL
#define PWM_TOP_VALUE      (PWM_CLOCK_HZ / PWM_FREQUENCY_HZ) 

#define FADE_HALF_PERIOD_MS  1000 
#define FADE_STEP_MS         10
#define FADE_STEPS           (FADE_HALF_PERIOD_MS / FADE_STEP_MS)
#define DUTY_STEP            (PWM_TOP_VALUE / FADE_STEPS)

#define DEBOUNCE_MS          20
#define DOUBLE_CLICK_MS      500 

#define TICK_MS              1

#define ID_DIGITS     LEDS_NUMBER 
#define DECIMAL_BASE  10
#define MAX_DIGIT     (DECIMAL_BASE - 1) 
#define MAX_SEQ_LEN   (MAX_DIGIT * ID_DIGITS)

static nrfx_pwm_t   m_pwm   = NRFX_PWM_INSTANCE(PWM_INSTANCE_ID);
static nrfx_timer_t m_timer = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_ID);

static nrf_pwm_values_individual_t m_pwm_values;

static const nrf_pwm_sequence_t m_pwm_sequence =
{
    .values.p_individual = &m_pwm_values,
    .length              = NRF_PWM_VALUES_LENGTH(m_pwm_values),
    .repeats             = 0,
    .end_delay           = 0
};

static uint8_t  m_sequence[MAX_SEQ_LEN];
static uint32_t m_sequence_len;

static volatile uint32_t m_time_ms; 
static volatile bool     m_blinking;
static volatile uint32_t m_fade_timer_ms; 
static volatile uint32_t m_sequence_pos; 
static volatile uint16_t m_duty;
static volatile bool     m_fade_rising;

static volatile uint32_t m_last_edge_ms;
static volatile uint32_t m_first_click_ms;
static volatile bool     m_first_click_done;
static volatile bool     m_button_pressed;

static void error_check(nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
    {
        while (true) {}
    }
}

static void gpio_output_voltage_setup(void)
{
    if ((NRF_POWER->MAINREGSTATUS &
         (POWER_MAINREGSTATUS_MAINREGSTATUS_High << POWER_MAINREGSTATUS_MAINREGSTATUS_Pos)) == 0)
    {
        return;
    }

    if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
        (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))
    {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}

        NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
                            (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos);

        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}

        NVIC_SystemReset();
    }
}

extern uint32_t __isr_vector;

static void vector_table_setup(void)
{
    SCB->VTOR = (uint32_t)&__isr_vector;
}

static uint32_t sequence_build(uint8_t * seq)
{
    uint32_t digits[ID_DIGITS];
    uint32_t id = DEVICE_ID;
    for (int i = ID_DIGITS - 1; i >= 0; i--)
    {
        digits[i] = id % DECIMAL_BASE;
        id /= DECIMAL_BASE;
    }

    uint32_t len = 0;
    for (uint32_t led = 0; led < ID_DIGITS; led++)
    {
        for (uint32_t k = 0; k < digits[led]; k++)
        {
            seq[len++] = (uint8_t)led;
        }
    }
    return len;
}

static void pwm_set_duty(uint32_t led_idx, uint16_t duty)
{
    volatile uint16_t * channels = (volatile uint16_t *)&m_pwm_values;
    channels[led_idx] = duty;
}

static void fade_step(void)
{
    uint32_t led = m_sequence[m_sequence_pos];

    if (m_fade_rising)
    {
        m_duty += DUTY_STEP;
        if (m_duty >= PWM_TOP_VALUE)
        {
            m_duty = PWM_TOP_VALUE;
            m_fade_rising = false;
        }
    }
    else if (m_duty > DUTY_STEP)
    {
        m_duty -= DUTY_STEP;
    }
    else
    {
        m_duty = 0;
        m_fade_rising = true;
        m_sequence_pos = (m_sequence_pos + 1) % m_sequence_len;
    }

    pwm_set_duty(led, m_duty);
}

static void timer_event_handler(nrf_timer_event_t event_type, void * p_context)
{
    (void)p_context;

    if (event_type != NRF_TIMER_EVENT_COMPARE0)
    {
        return;
    }

    m_time_ms += TICK_MS;

    if (m_blinking)
    {
        m_fade_timer_ms += TICK_MS;
        if (m_fade_timer_ms >= FADE_STEP_MS)
        {
            m_fade_timer_ms = 0;
            fade_step();
        }
    }
}

static void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;

    uint32_t now = m_time_ms;
    bool quiet_before = ((uint32_t)(now - m_last_edge_ms) >= DEBOUNCE_MS);
    m_last_edge_ms = now;

    if (!quiet_before)
    {
        return;
    }

    m_button_pressed = !m_button_pressed;
    if (!m_button_pressed)
    {
        return;
    }

    if (m_first_click_done && (uint32_t)(now - m_first_click_ms) <= DOUBLE_CLICK_MS)
    {
        m_first_click_done = false;
        if (m_sequence_len != 0)
        {
            m_blinking = !m_blinking;
        }
    }
    else
    {
        m_first_click_done = true;
        m_first_click_ms = now;
    }
}

static void pwm_init(void)
{
    nrfx_pwm_config_t const config =
    {
        .output_pins =
        {
            LED1_G_PIN | NRFX_PWM_PIN_INVERTED,   /* channel 0 - LED 1 */
            LED2_R_PIN | NRFX_PWM_PIN_INVERTED,   /* channel 1 - LED 2 */
            LED2_G_PIN | NRFX_PWM_PIN_INVERTED,   /* channel 2 - LED 3 */
            LED2_B_PIN | NRFX_PWM_PIN_INVERTED    /* channel 3 - LED 4 */
        },
        .irq_priority = IRQ_PRIORITY,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = PWM_TOP_VALUE,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    error_check(nrfx_pwm_init(&m_pwm, &config, NULL));

    (void)nrfx_pwm_simple_playback(&m_pwm, &m_pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
}

static void timer_init(void)
{
    nrfx_timer_config_t const config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_TIMER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = IRQ_PRIORITY,
        .p_context          = NULL
    };

    error_check(nrfx_timer_init(&m_timer, &config, timer_event_handler));

    nrfx_timer_extended_compare(&m_timer,
                                NRF_TIMER_CC_CHANNEL0,
                                nrfx_timer_ms_to_ticks(&m_timer, TICK_MS),
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                true);
    nrfx_timer_enable(&m_timer);
}

static void button_init(void)
{
    error_check(nrfx_gpiote_init());

    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    error_check(nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler));
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);
    m_button_pressed = (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL);
}

int main(void)
{
    vector_table_setup();
    gpio_output_voltage_setup();

    m_sequence_len = sequence_build(m_sequence);
    m_fade_rising  = true;

    pwm_init();
    timer_init();
    button_init();

    while (true)
    {
        __WFI();
    }
}