#include "led_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "nrfx_pwm.h"
#include "nrf_log.h"

static nrfx_pwm_t m_pwm = NRFX_PWM_INSTANCE(PWM_INSTANCE_ID);
static nrf_pwm_values_individual_t m_pwm_values;

static const nrf_pwm_sequence_t m_pwm_sequence =
{
    .values.p_individual = &m_pwm_values,
    .length              = NRF_PWM_VALUES_LENGTH(m_pwm_values),
    .repeats             = 0,
    .end_delay           = 0
};

static volatile uint32_t m_sequence_pos;
static volatile uint16_t m_duty;
static volatile bool     m_fade_rising;
static volatile bool     m_blinking;
static uint8_t           m_sequence[64];
static uint32_t          m_sequence_len;
volatile bool     g_led_changed = false;
volatile uint8_t  g_active_led = 0;

volatile bool g_fade_state_changed = false; 

static void sequence_build(void)
{
    uint32_t digits[LEDS_NUMBER];
    uint32_t id = DEVICE_ID;
    
    for (int i = LEDS_NUMBER - 1; i >= 0; i--)
    {
        digits[i] = id % 10;
        id /= 10;
    }

    m_sequence_len = 0;
    for (uint32_t led = 0; led < LEDS_NUMBER; led++)
    {
        for (uint32_t k = 0; k < digits[led]; k++)
        {
            m_sequence[m_sequence_len++] = (uint8_t)led;
        }
    }
}

void led_ctrl_init(void)
{
    nrfx_pwm_config_t const config =
    {
        .output_pins =
        {
            LED1_G_PIN | NRFX_PWM_PIN_INVERTED,
            LED2_R_PIN | NRFX_PWM_PIN_INVERTED,
            LED2_G_PIN | NRFX_PWM_PIN_INVERTED,
            LED2_B_PIN | NRFX_PWM_PIN_INVERTED
        },
        .irq_priority = IRQ_PRIORITY,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = PWM_TOP_VALUE,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    app_error_check(nrfx_pwm_init(&m_pwm, &config, NULL));
    (void)nrfx_pwm_simple_playback(&m_pwm, &m_pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);

    sequence_build();
    m_duty = 0;
    m_fade_rising = true;
    m_blinking = false;
}

void led_ctrl_step(void)
{
    if (!m_blinking || m_sequence_len == 0) return;

    uint32_t active_led = m_sequence[m_sequence_pos];
    volatile uint16_t * channels = (volatile uint16_t *)&m_pwm_values;

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
        channels[active_led] = 0;
        m_duty = 0;
        m_fade_rising = true;
        m_sequence_pos = (m_sequence_pos + 1) % m_sequence_len;
        active_led = m_sequence[m_sequence_pos];
        g_active_led = active_led; 
        g_led_changed = true;
    }

    channels[active_led] = m_duty;
}

void led_ctrl_toggle(void)
{
    if (m_sequence_len == 0) return;

    m_blinking = !m_blinking;
    g_fade_state_changed = true;
}

bool led_ctrl_is_blinking(void)
{
    return m_blinking;
}