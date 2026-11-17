#include "timer_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "fade_ctrl.h"
#include "nrfx_timer.h"

static nrfx_timer_t m_timer = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_ID);

static volatile uint32_t m_time_ms;

static void timer_event_handler(nrf_timer_event_t event_type, void * p_context)
{
    (void)p_context;

    if (event_type != NRF_TIMER_EVENT_COMPARE0)
    {
        return;
    }

    m_time_ms += TICK_MS;
    fade_ctrl_on_tick();
}

void timer_ctrl_init(void)
{
    nrfx_timer_config_t const config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_TIMER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = IRQ_PRIORITY,
        .p_context          = NULL
    };

    app_error_check(nrfx_timer_init(&m_timer, &config, timer_event_handler));

    nrfx_timer_extended_compare(&m_timer,
                                NRF_TIMER_CC_CHANNEL0,
                                nrfx_timer_ms_to_ticks(&m_timer, TICK_MS),
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                true);
    nrfx_timer_enable(&m_timer);
}

uint32_t timer_ctrl_ms(void)
{
    return m_time_ms;
}
