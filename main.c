/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except in the form of a compiled
 * executable, must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used on
 * hardware or software products from Nordic Semiconductor ASA.
 *
 * 5. Any software provided in binary form under this license must not be
 * reverse engineered, decompiled, disassembled or otherwise used to discover
 * the underlying algorithms, source code, or other intellectual property of
 * Nordic Semiconductor ASA or any other third party.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "nrf.h"
#include "app_board.h"
#include "pwm_ctrl.h"
#include "fade_ctrl.h"
#include "timer_ctrl.h"
#include "button_ctrl.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"

extern uint32_t __isr_vector;

static void vector_table_setup(void)
{
    SCB->VTOR = (uint32_t)&__isr_vector;
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

static void logs_init(void)
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void)
{
    vector_table_setup();
    gpio_output_voltage_setup();
    logs_init();

    fade_ctrl_init();

    NRF_LOG_INFO("Lab 3 started: device id %d, sequence length %d",
                 DEVICE_ID, fade_ctrl_sequence_len());

    pwm_ctrl_init();
    timer_ctrl_init();
    button_ctrl_init();

    while (true)
    {
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
        __WFI();
    }
}
