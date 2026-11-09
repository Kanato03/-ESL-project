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
 *
 * Blinky for nRF52840 Dongle (PCA10059): the LED sequence is derived from the
 * device ID and is played only while the user button is pressed.
 *
 * Only the GPIO HAL (nrf_gpio.h) is used, "boards.h" is not included.
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

/* ID of the device (the number on the label after '#'). Change it to yours. */
#ifndef DEVICE_ID
#define DEVICE_ID 6598
#endif

/* ---- Board description (PCA10059), taken from components/boards/pca10059.h */

#define LEDS_NUMBER     4

#define LED1_G_PIN      NRF_GPIO_PIN_MAP(0, 6)   /* LED 1, green              */
#define LED2_R_PIN      NRF_GPIO_PIN_MAP(0, 8)   /* LED 2, red   (RGB)        */
#define LED2_G_PIN      NRF_GPIO_PIN_MAP(1, 9)   /* LED 2, green (RGB)        */
#define LED2_B_PIN      NRF_GPIO_PIN_MAP(0, 12)  /* LED 2, blue  (RGB)        */
#define BUTTON_PIN      NRF_GPIO_PIN_MAP(1, 6)   /* SW1                       */

#define LED_ACTIVE_LEVEL     0   /* LED lights up when the pin is low         */
#define BUTTON_ACTIVE_LEVEL  0   /* button reads low when pressed             */

/* ---- Timing --------------------------------------------------------------- */

#define SLOT_MS      1000  /* one element of the sequence lasts this long     */
#define LED_ON_MS     500  /* ... and the LED is lit during the first part    */
#define TICK_MS         1  /* how often the main loop looks at the button     */

/* Optional task: 1 - if the button is released while an LED is lit, the LED
 * stays lit until the button is pressed again; 0 - all LEDs go dark. */
#define KEEP_LED_ON_WHEN_RELEASED  1

/* ---- Sequence ------------------------------------------------------------- */

#define ID_DIGITS     4
#define MAX_SEQ_LEN  (9 * ID_DIGITS)

static const uint32_t m_led_pins[LEDS_NUMBER] =
    { LED1_G_PIN, LED2_R_PIN, LED2_G_PIN, LED2_B_PIN };

/* Own LED functions on top of the GPIO HAL ---------------------------------- */

/* On a dongle powered from USB the GPIO output voltage is 1.8 V by default,
 * which is not enough for the green and blue LEDs. The value is stored in UICR
 * (survives power loss), so this only does something once per dongle.
 * Same code as in boards.c from Nordic SDK. */
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

        /* System reset is needed to update UICR registers. */
        NVIC_SystemReset();
    }
}

static void led_set(uint32_t led_idx, bool on)
{
    nrf_gpio_pin_write(m_led_pins[led_idx], on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
}

static void leds_all_off(void)
{
    for (uint32_t i = 0; i < LEDS_NUMBER; i++)
    {
        led_set(i, false);
    }
}

static void leds_init(void)
{
    gpio_output_voltage_setup();

    for (uint32_t i = 0; i < LEDS_NUMBER; i++)
    {
        nrf_gpio_cfg_output(m_led_pins[i]);
    }
    leds_all_off();
}

static void button_init(void)
{
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLUP);
}

static bool button_is_pressed(void)
{
    return nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL;
}

/* Build the sequence: digit N of DEVICE_ID is how many times LED N appears.
 * Example: 4321 -> 0 0 0 0 1 1 1 2 2 3. Returns the length of the sequence. */
static uint32_t sequence_build(uint8_t * seq)
{
    uint32_t digits[ID_DIGITS];
    uint32_t id = DEVICE_ID;
    for (int i = ID_DIGITS - 1; i >= 0; i--)
    {
        digits[i] = id % 10;
        id /= 10;
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

int main(void)
{
    leds_init();
    button_init();

    uint8_t  seq[MAX_SEQ_LEN];
    uint32_t seq_len = sequence_build(seq);
    if (seq_len == 0)
    {
        while (true) {}   /* nothing to play (DEVICE_ID has only zeros) */
    }

    uint32_t pos      = 0;  /* index of the current element of the sequence   */
    uint32_t slot_ms  = 0;  /* how long the current element has been played   */

    while (true)
    {
        if (button_is_pressed())
        {
            /* Lit in the first part of the slot, dark in the second part. This
             * is what separates two equal neighbours (R R) from one long R. */
            led_set(seq[pos], slot_ms < LED_ON_MS);

            if (++slot_ms >= SLOT_MS)
            {
                slot_ms = 0;
                pos = (pos + 1) % seq_len;   /* wrap around the sequence */
            }
        }
        else if (!KEEP_LED_ON_WHEN_RELEASED)
        {
            leds_all_off();
        }
        /* else: do nothing. The LED stays as it was and slot_ms is kept, so
         * the next press continues exactly from this moment. */

        nrf_delay_ms(TICK_MS);
    }
}