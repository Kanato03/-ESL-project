#include "timer_ctrl.h"
#include "app_error.h"
#include "nrf_drv_clock.h"

void timer_ctrl_init(void)
{
    ret_code_t err = nrf_drv_clock_init();
    if (err != NRF_ERROR_MODULE_ALREADY_INITIALIZED)
    {
        APP_ERROR_CHECK(err);
    }
    nrf_drv_clock_lfclk_request(NULL);

    APP_ERROR_CHECK(app_timer_init());
}
