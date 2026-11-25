#include "color_picker.h"
#include "app_board.h"
#include "led_ctrl.h"
#include "timer_ctrl.h"
#include "flash_storage.h"
#include "app_error.h"
#include "app_util_platform.h"

#define START_HUE   ((DEVICE_ID % 100) * HUE_MAX / 100)

static volatile input_mode_t m_mode = INPUT_MODE_NONE;
static hsv_t                 m_hsv;
static bool                  m_saturation_up = false;
static bool                  m_brightness_up = false;
static uint32_t              m_step_acc_ms = 0;
static volatile bool         m_adjusting = false;
static uint32_t              m_hold_log_ms = 0;
static uint32_t              m_blink_ms = 0;

APP_TIMER_DEF(m_tick_timer);

volatile bool g_mode_changed  = false;
volatile bool g_color_changed = false;
volatile bool g_color_summary = false;
volatile bool g_color_save    = false;
volatile bool g_color_loaded  = false;

static const char * const m_mode_names[INPUT_MODE_COUNT] =
{
    "NO INPUT",
    "HUE",
    "SATURATION",
    "BRIGHTNESS"
};

static void color_apply(void)
{
    led_ctrl_set_rgb(color_hsv_to_rgb(m_hsv));
}

static uint8_t ping_pong_step(uint8_t value, uint8_t max, bool * p_up)
{
    if (*p_up)
    {
        if (value >= max) *p_up = false;
        else              return value + 1;
    }

    if (value == 0)
    {
        *p_up = true;
        return 1;
    }
    return value - 1;
}

static void value_step(void)
{
    switch (m_mode)
    {
        case INPUT_MODE_HUE:
            m_hsv.h = (m_hsv.h + 1) % HUE_MAX;
            break;

        case INPUT_MODE_SATURATION:
            m_hsv.s = ping_pong_step(m_hsv.s, SATURATION_MAX, &m_saturation_up);
            break;

        case INPUT_MODE_BRIGHTNESS:
            m_hsv.v = ping_pong_step(m_hsv.v, BRIGHTNESS_MAX, &m_brightness_up);
            break;

        default:
            return;
    }

    color_apply();
}

static uint32_t step_period_ms(void)
{
    return (m_mode == INPUT_MODE_HUE) ? HUE_STEP_MS : SAT_BRIGHT_STEP_MS;
}

static uint16_t blink_duty(uint32_t period_ms)
{
    uint32_t half  = period_ms / 2;
    uint32_t phase = m_blink_ms % period_ms;

    if (phase >= half)
    {
        phase = period_ms - phase;
    }
    return (uint16_t)(phase * PWM_TOP_VALUE / half);
}

static void led1_update(void)
{
    switch (m_mode)
    {
        case INPUT_MODE_HUE:        led_ctrl_set_led1(blink_duty(LED1_SLOW_PERIOD_MS)); break;
        case INPUT_MODE_SATURATION: led_ctrl_set_led1(blink_duty(LED1_FAST_PERIOD_MS)); break;
        case INPUT_MODE_BRIGHTNESS: led_ctrl_set_led1(PWM_TOP_VALUE);                   break;
        default:                    led_ctrl_set_led1(0);                               break;
    }
}

static void tick_timer_handler(void * p_context)
{
    m_blink_ms += TICK_MS;

    if (m_adjusting && (m_mode != INPUT_MODE_NONE))
    {
        m_step_acc_ms += TICK_MS;
        while (m_step_acc_ms >= step_period_ms())
        {
            m_step_acc_ms -= step_period_ms();
            value_step();
        }

        m_hold_log_ms += TICK_MS;
        if (m_hold_log_ms >= HOLD_LOG_PERIOD_MS)
        {
            m_hold_log_ms = 0;
            g_color_changed = true;
        }
    }

    led1_update();
}

void color_picker_init(void)
{
    g_color_loaded = flash_storage_load(&m_hsv);
    if (!g_color_loaded)
    {
        m_hsv.h = START_HUE;
        m_hsv.s = SATURATION_MAX;
        m_hsv.v = BRIGHTNESS_MAX;
    }
    m_mode = INPUT_MODE_NONE;

    APP_ERROR_CHECK(app_timer_create(&m_tick_timer, APP_TIMER_MODE_REPEATED, tick_timer_handler));

    color_apply();
    led1_update();
}

void color_picker_next_mode(void)
{
    m_mode = (input_mode_t)((m_mode + 1) % INPUT_MODE_COUNT);
    m_step_acc_ms = 0;
    g_mode_changed = true;

    if (m_mode == INPUT_MODE_NONE)
    {
        app_timer_stop(m_tick_timer);
        m_adjusting   = false;
        m_hold_log_ms = 0;
        led1_update();

        g_color_summary = true;
        g_color_save    = true;
    }
    else if (m_mode == INPUT_MODE_HUE)
    {
        m_blink_ms = 0;
        APP_ERROR_CHECK(app_timer_start(m_tick_timer, APP_TIMER_TICKS(TICK_MS), NULL));
    }
}

void color_picker_on_button(button_event_t evt)
{
    switch (evt)
    {
        case BUTTON_EVENT_DOUBLE_CLICK:
            color_picker_next_mode();
            break;

        case BUTTON_EVENT_HOLD_START:
            m_step_acc_ms = 0;
            m_hold_log_ms = 0;
            m_adjusting   = true;
            break;

        case BUTTON_EVENT_HOLD_END:
            if (m_adjusting && (m_mode != INPUT_MODE_NONE))
            {
                g_color_changed = true;
        g_color_save    = true;
            }
            m_adjusting   = false;
            m_step_acc_ms = 0;
            m_hold_log_ms = 0;
            break;

        default:
            break;
    }
}

input_mode_t color_picker_mode(void)
{
    return m_mode;
}

hsv_t color_picker_hsv(void)
{
    hsv_t hsv;

    // m_hsv is modified from the app_timer interrupt; copy it atomically.
    CRITICAL_REGION_ENTER();
    hsv = m_hsv;
    CRITICAL_REGION_EXIT();

    return hsv;
}

const char * color_picker_mode_name(input_mode_t mode)
{
    return (mode < INPUT_MODE_COUNT) ? m_mode_names[mode] : "?";
}
