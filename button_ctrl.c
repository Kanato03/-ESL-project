#include "button_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "app_error.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

typedef enum
{
    BTN_STATE_IDLE,               // released, nothing pending
    BTN_STATE_PRESSED,            // pressed, waiting for hold timeout
    BTN_STATE_HELD,               // pressed longer than HOLD_DELAY_MS
    BTN_STATE_WAIT_SECOND_CLICK,  // short click done, waiting for second press
    BTN_STATE_SECOND_PRESS        // second press of a double click, no hold allowed
} btn_state_t;

typedef enum
{
    BTN_INPUT_PRESS,
    BTN_INPUT_RELEASE,
    BTN_INPUT_HOLD_TIMEOUT,
    BTN_INPUT_DOUBLE_CLICK_TIMEOUT
} btn_input_t;

APP_TIMER_DEF(m_debounce_timer);
APP_TIMER_DEF(m_hold_timer);
APP_TIMER_DEF(m_double_click_timer);

static button_event_handler_t m_handler = NULL;
static btn_state_t            m_state   = BTN_STATE_IDLE;
static bool                   m_pressed = false;

static void emit(button_event_t evt)
{
    if (m_handler != NULL)
    {
        m_handler(evt);
    }
}

static void timer_start(app_timer_id_t timer, uint32_t ms)
{
    APP_ERROR_CHECK(app_timer_start(timer, APP_TIMER_TICKS(ms), NULL));
}

// Single place where all state transitions happen.
// Called only from app_timer handlers, i.e. always from the same interrupt context.
static void button_fsm(btn_input_t input)
{
    switch (m_state)
    {
        case BTN_STATE_IDLE:
            if (input == BTN_INPUT_PRESS)
            {
                emit(BUTTON_EVENT_PRESSED);
                timer_start(m_hold_timer, HOLD_DELAY_MS);
                m_state = BTN_STATE_PRESSED;
            }
            break;

        case BTN_STATE_PRESSED:
            if (input == BTN_INPUT_HOLD_TIMEOUT)
            {
                emit(BUTTON_EVENT_HOLD_START);
                m_state = BTN_STATE_HELD;
            }
            else if (input == BTN_INPUT_RELEASE)
            {
                app_timer_stop(m_hold_timer);
                timer_start(m_double_click_timer, DOUBLE_CLICK_MS);
                m_state = BTN_STATE_WAIT_SECOND_CLICK;
            }
            break;

        case BTN_STATE_HELD:
            if (input == BTN_INPUT_RELEASE)
            {
                emit(BUTTON_EVENT_HOLD_END);
                m_state = BTN_STATE_IDLE;
            }
            break;

        case BTN_STATE_WAIT_SECOND_CLICK:
            if (input == BTN_INPUT_PRESS)
            {
                app_timer_stop(m_double_click_timer);
                emit(BUTTON_EVENT_PRESSED);
                emit(BUTTON_EVENT_DOUBLE_CLICK);
                m_state = BTN_STATE_SECOND_PRESS;
            }
            else if (input == BTN_INPUT_DOUBLE_CLICK_TIMEOUT)
            {
                m_state = BTN_STATE_IDLE;
            }
            break;

        case BTN_STATE_SECOND_PRESS:
            if (input == BTN_INPUT_RELEASE)
            {
                m_state = BTN_STATE_IDLE;
            }
            break;

        default:
            m_state = BTN_STATE_IDLE;
            break;
    }
}

static void debounce_timer_handler(void * p_context)
{
    bool pressed = (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL);
    if (pressed == m_pressed) return;

    m_pressed = pressed;
    button_fsm(pressed ? BTN_INPUT_PRESS : BTN_INPUT_RELEASE);
}

static void hold_timer_handler(void * p_context)
{
    button_fsm(BTN_INPUT_HOLD_TIMEOUT);
}

static void double_click_timer_handler(void * p_context)
{
    button_fsm(BTN_INPUT_DOUBLE_CLICK_TIMEOUT);
}

static void gpiote_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // Restart debounce window on every edge; pin is sampled when it expires.
    app_timer_stop(m_debounce_timer);
    timer_start(m_debounce_timer, DEBOUNCE_MS);
}

void button_ctrl_init(button_event_handler_t handler)
{
    m_handler = handler;

    APP_ERROR_CHECK(app_timer_create(&m_debounce_timer,     APP_TIMER_MODE_SINGLE_SHOT, debounce_timer_handler));
    APP_ERROR_CHECK(app_timer_create(&m_hold_timer,         APP_TIMER_MODE_SINGLE_SHOT, hold_timer_handler));
    APP_ERROR_CHECK(app_timer_create(&m_double_click_timer, APP_TIMER_MODE_SINGLE_SHOT, double_click_timer_handler));

    app_error_check(nrfx_gpiote_init());

    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    app_error_check(nrfx_gpiote_in_init(BUTTON_PIN, &config, gpiote_event_handler));
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);

    m_pressed = (nrf_gpio_pin_read(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL);
}
