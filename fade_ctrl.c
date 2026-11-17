#include "fade_ctrl.h"
#include "app_board.h"
#include "pwm_ctrl.h"
#include "timer_ctrl.h"
#include "nrf_log.h"

#define ID_DIGITS     LEDS_NUMBER
#define DECIMAL_BASE  10
#define MAX_DIGIT     (DECIMAL_BASE - 1)
#define MAX_SEQ_LEN   (MAX_DIGIT * ID_DIGITS)

static uint8_t  m_sequence[MAX_SEQ_LEN];
static uint32_t m_sequence_len;

static volatile uint32_t m_sequence_pos;
static volatile uint16_t m_duty;
static volatile bool     m_fade_rising;
static volatile bool     m_blinking;
static volatile uint32_t m_fade_timer_ms;

static uint32_t sequence_build(uint8_t * seq)
{
    uint32_t digits[ID_DIGITS];
    uint32_t id = DEVICE_ID;
    for (int i = ID_DIGITS - 1; i >= 0; i--)
    {
        digits[i] = id % DECIMAL_BASE;
        id /= DECIMAL_BASE;
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

static void fade_step(void)
{
    uint32_t led = m_sequence[m_sequence_pos];

    if (m_fade_rising)
    {
        m_duty += DUTY_STEP;
        if (m_duty >= PWM_TOP_VALUE)
        {
            m_duty = PWM_TOP_VALUE;
            m_fade_rising = false;
        }
    }
    else if (m_duty > DUTY_STEP)
    {
        m_duty -= DUTY_STEP;
    }
    else
    {
        m_duty = 0;
        m_fade_rising = true;
        m_sequence_pos = (m_sequence_pos + 1) % m_sequence_len;
        NRF_LOG_INFO("LED %d finished, next element %d of %d",
                     led, m_sequence_pos + 1, m_sequence_len);
    }

    pwm_ctrl_set_duty(led, m_duty);
}

void fade_ctrl_init(void)
{
    m_sequence_len = sequence_build(m_sequence);
    m_fade_rising  = true;
}

void fade_ctrl_on_tick(void)
{
    if (!m_blinking)
    {
        return;
    }

    m_fade_timer_ms += TICK_MS;
    if (m_fade_timer_ms >= FADE_STEP_MS)
    {
        m_fade_timer_ms = 0;
        fade_step();
    }
}

void fade_ctrl_toggle(void)
{
    if (m_sequence_len == 0)
    {
        return;
    }

    m_blinking = !m_blinking;
    if (m_blinking)
    {
        NRF_LOG_INFO("Double click: smooth blinking started");
    }
    else
    {
        NRF_LOG_INFO("Double click: blinking frozen, duty cycle %d of %d",
                     m_duty, PWM_TOP_VALUE);
    }
}

uint32_t fade_ctrl_sequence_len(void)
{
    return m_sequence_len;
}
