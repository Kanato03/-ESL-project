#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "nrf.h"
#include "app_board.h"
#include "led_ctrl.h"
#include "timer_ctrl.h"
#include "button_ctrl.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "nrf_delay.h"
#include "app_usbd_serial_num.h"

extern uint32_t __isr_vector;
extern volatile bool g_fade_state_changed; 
extern volatile bool g_fade_state_changed;
extern volatile bool g_led_changed;
extern volatile uint8_t g_active_led;

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

    led_ctrl_init();
    timer_ctrl_init();
    button_ctrl_init();

    NRF_LOG_INFO("Lab 3 started. Device ID: %d", DEVICE_ID);
    while (true)
    {

        if (g_fade_state_changed)
        {
            g_fade_state_changed = false;
            
            if (led_ctrl_is_blinking())
            {
                NRF_LOG_INFO("Double click: STARTED");
            }
            else
            {
                NRF_LOG_INFO("Double click: FROZEN");
            }
        }

        if (g_led_changed)
        {
            g_led_changed = false;
            NRF_LOG_INFO("Next LED: %d", g_active_led); 
        }

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();

        for(volatile int i=0; i<10000; i++); 
    }
}