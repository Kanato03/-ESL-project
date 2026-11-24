#include "led_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "nrfx_pwm.h"

static nrfx_pwm_t m_pwm = NRFX_PWM_INSTANCE(PWM_INSTANCE_ID);
static nrf_pwm_values_individual_t m_pwm_values;

static const nrf_pwm_sequence_t m_pwm_sequence =
{
    .values.p_individual = &m_pwm_values,
    .length              = NRF_PWM_VALUES_LENGTH(m_pwm_values),
    .repeats             = 0,
    .end_delay           = 0
};

static void channel_set(uint32_t channel, uint16_t duty)
{
    volatile uint16_t * channels = (volatile uint16_t *)&m_pwm_values;

    if (duty > PWM_TOP_VALUE)
    {
        duty = PWM_TOP_VALUE;
    }
    channels[channel] = duty;
}

static uint16_t rgb_to_duty(uint8_t value)
{
    return (uint16_t)((uint32_t)value * PWM_TOP_VALUE / RGB_MAX);
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
}

void led_ctrl_set_rgb(rgb_t rgb)
{
    channel_set(PWM_CH_RED,   rgb_to_duty(rgb.r));
    channel_set(PWM_CH_GREEN, rgb_to_duty(rgb.g));
    channel_set(PWM_CH_BLUE,  rgb_to_duty(rgb.b));
}

void led_ctrl_set_led1(uint16_t duty)
{
    channel_set(PWM_CH_LED1, duty);
}
