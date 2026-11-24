#include "button_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "color_picker.h"
#include "timer_ctrl.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

static volatile bool     m_pressed = false;
static volatile uint32_t m_edge_ms = 0;
static volatile uint32_t m_press_ms = 0;
static volatile uint32_t m_release_ms = 0;
static volatile bool     m_click_pending = false;
static volatile bool     m_hold_blocked = false;
static volatile bool     m_recheck = false;

volatile bool g_button_pressed = false;

static void button_on_press(uint32_t now)
{
    m_press_ms = now;
    g_button_pressed = true;

    if (m_click_pending && (uint32_t)(now - m_release_ms) <= DOUBLE_CLICK_MS)
    {
        m_click_pending = false;
        m_hold_blocked  = true;
        color_picker_next_mode();
    }
    else
    {
        m_click_pending = false;
        m_hold_blocked  = false;
    }
}

static void button_on_release(uint32_t now)
{
    bool short_click = !m_hold_blocked && (uint32_t)(now - m_press_ms) < HOLD_DELAY_MS;

    m_click_pending = short_click;
    m_release_ms    = now;
    m_hold_blocked  = false;
}

static void button_update(void)
{
    bool pressed = (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL);
    if (pressed == m_pressed) return;

    uint32_t now = timer_ctrl_ms();

    if ((uint32_t)(now - m_edge_ms) < DEBOUNCE_MS)
    {
        m_recheck = true;
        return;
    }

    m_edge_ms = now;
    m_pressed = pressed;

    if (pressed) button_on_press(now);
    else         button_on_release(now);
}

static void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    button_update();
}

void button_ctrl_on_tick(void)
{
    if (!m_recheck) return;
    if ((uint32_t)(timer_ctrl_ms() - m_edge_ms) < DEBOUNCE_MS) return;

    m_recheck = false;
    button_update();
}

bool button_ctrl_is_held(void)
{
    if (!m_pressed || m_hold_blocked) return false;

    return (uint32_t)(timer_ctrl_ms() - m_press_ms) >= HOLD_DELAY_MS;
}

void button_ctrl_init(void)
{
    app_error_check(nrfx_gpiote_init());

    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    app_error_check(nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler));
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);

    m_pressed = (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL);
}
