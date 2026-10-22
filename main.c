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
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

/* ID of the device (the number on the label after '#'). Change it to yours. */
#define DEVICE_ID 4321

/* Number of decimal digits in DEVICE_ID, and thus the number of LEDs used. */
#define ID_DIGITS 4

#define BLINK_ON_MS     300   /* LED is lit */
#define BLINK_OFF_MS    300   /* pause between two blinks of the same LED */
#define LED_PAUSE_MS    1000  /* pause between two different LEDs */
#define CYCLE_PAUSE_MS  2500  /* pause before the whole sequence repeats */

/**
 * @brief Blink one LED the given number of times.
 *
 * @param led_idx Board LED index (0 .. LEDS_NUMBER - 1).
 * @param times   How many times to blink. 0 means the LED stays dark.
 */
static void blink_led(uint32_t led_idx, uint32_t times)
{
    for (uint32_t i = 0; i < times; i++)
    {
        bsp_board_led_on(led_idx);
        nrf_delay_ms(BLINK_ON_MS);
        bsp_board_led_off(led_idx);
        nrf_delay_ms(BLINK_OFF_MS);
    }
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    /* Configure board. */
    bsp_board_init(BSP_INIT_LEDS);

    /* Split DEVICE_ID into digits: digits[0] is the first (thousands) digit. */
    uint32_t digits[ID_DIGITS];
    uint32_t id = DEVICE_ID;
    for (int i = ID_DIGITS - 1; i >= 0; i--)
    {
        digits[i] = id % 10;
        id /= 10;
    }

    /* Digit N is the blink count of LED N: 1st - yellow/green, 2nd - red,
     * 3rd - green, 4th - blue. */
    while (true)
    {
        for (uint32_t led = 0; led < ID_DIGITS; led++)
        {
            blink_led(led, digits[led]);
            nrf_delay_ms(LED_PAUSE_MS);
        }
        nrf_delay_ms(CYCLE_PAUSE_MS);
    }
}

/**
 *@}
 **/
