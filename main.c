//Рабочая версия уже с логами
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "nrf.h"
#include "app_board.h"
#include "led_ctrl.h"
#include "color_picker.h"
#include "timer_ctrl.h"
#include "button_ctrl.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"

extern uint32_t __isr_vector;

static volatile bool m_button_pressed = false;

static void button_event_handler(button_event_t evt)
{
    if (evt == BUTTON_EVENT_PRESSED)
    {
        m_button_pressed = true;
    }
    color_picker_on_button(evt);
}

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

static void color_summary_log(void)
{
    hsv_t hsv = color_picker_hsv();
    rgb_t rgb = color_hsv_to_rgb(hsv);

    NRF_LOG_INFO("===== Color =====");
    NRF_LOG_INFO("  Saturation: %d%%", hsv.s);
    NRF_LOG_INFO("  Brightness: %d%%", hsv.v);
    NRF_LOG_INFO("  RGB:        (%d, %d, %d) (Hue: %d deg)", rgb.r, rgb.g, rgb.b, hsv.h);
}

static void setting_log(void)
{
    hsv_t hsv = color_picker_hsv();
    rgb_t rgb = color_hsv_to_rgb(hsv);

    switch (color_picker_mode())
    {
        case INPUT_MODE_HUE:        NRF_LOG_INFO("RGB: (%d, %d, %d) (Hue: %d deg)", rgb.r, rgb.g, rgb.b, hsv.h); break;
        case INPUT_MODE_SATURATION: NRF_LOG_INFO("Saturation: %d%%", hsv.s);   break;
        case INPUT_MODE_BRIGHTNESS: NRF_LOG_INFO("Brightness: %d%%", hsv.v);   break;
        default: break;
    }
}

int main(void)
{
    vector_table_setup();
    gpio_output_voltage_setup();
    logs_init();
    timer_ctrl_init();

    led_ctrl_init();
    color_picker_init();
    button_ctrl_init(button_event_handler);

    NRF_LOG_INFO("Workshop 5 started. Device ID: %d", DEVICE_ID);
    color_summary_log();

    while (true)
    {
        if (g_mode_changed)
        {
            g_mode_changed = false;
            NRF_LOG_INFO("Input mode: %s", color_picker_mode_name(color_picker_mode()));
        }

        if (m_button_pressed)
        {
            m_button_pressed = false;
            NRF_LOG_INFO("Button pressed");
        }

        if (g_color_changed)
        {
            g_color_changed = false;
            setting_log();
        }

        if (g_color_summary)
        {
            g_color_summary = false;
            color_summary_log();
        }

        LOG_BACKEND_USB_PROCESS();

        if (!NRF_LOG_PROCESS())
        {
            __WFE();
        }
    }
}
