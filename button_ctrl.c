#include "button_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "led_ctrl.h"
#include "timer_ctrl.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "nrf_log.h"

static volatile uint32_t m_last_event_ms = 0;
static volatile uint32_t m_first_click_ms = 0;
static volatile bool     m_first_click_done = false;
static volatile bool     m_wait_release = false;
static volatile uint32_t m_press_ms = 0;
static volatile uint32_t m_release_ms = 0;
static volatile bool     m_have_event = false;

static void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (nrf_gpio_pin_read(BUTTON_PIN) != BUTTON_ACTIVE_LEVEL) return;

    uint32_t now = timer_ctrl_ms();

    if (m_wait_release) return;

    if (m_have_event && (uint32_t)(now - m_last_event_ms) < DEBOUNCE_MS) return;

    m_have_event    = true;
    m_last_event_ms = now;
    m_wait_release  = true;
    m_press_ms      = now;
    m_release_ms    = 0;

    NRF_LOG_INFO("Button pressed");

    if (m_first_click_done && (uint32_t)(now - m_first_click_ms) <= DOUBLE_CLICK_MS)
    {
        m_first_click_done = false;
        led_ctrl_toggle();
    }
    else
    {
        m_first_click_done = true;
        m_first_click_ms   = now;
    }
}

void button_ctrl_on_tick(void)
{
    if (!m_wait_release) return;

    uint32_t now = timer_ctrl_ms();

    if (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL)
    {
        m_release_ms = 0;
        return;
    }

    m_release_ms += TICK_MS;

    if (m_release_ms >= RELEASE_STABLE_MS || (uint32_t)(now - m_press_ms) >= MAX_HOLD_MS)
    {
        m_wait_release = false;
        m_release_ms   = 0;
    }
}

void button_ctrl_init(void)
{
    app_error_check(nrfx_gpiote_init());

    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    app_error_check(nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler));
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);

    m_last_event_ms = timer_ctrl_ms();
}