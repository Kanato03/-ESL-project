#include "button_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "fade_ctrl.h"
#include "timer_ctrl.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "nrf_log.h"

static volatile uint32_t m_last_edge_ms;
static volatile uint32_t m_first_click_ms;
static volatile bool     m_first_click_done;
static volatile bool     m_button_pressed;

static void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;

    uint32_t now = timer_ctrl_ms();
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
    NRF_LOG_INFO("Button pressed");

    if (m_first_click_done && (uint32_t)(now - m_first_click_ms) <= DOUBLE_CLICK_MS)
    {
        m_first_click_done = false;
        fade_ctrl_toggle();
    }
    else
    {
        m_first_click_done = true;
        m_first_click_ms = now;
    }
}

void button_ctrl_init(void)
{
    app_error_check(nrfx_gpiote_init());

    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    app_error_check(nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler));
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);
    m_button_pressed = (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL);
}
