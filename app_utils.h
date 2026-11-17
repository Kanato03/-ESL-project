#ifndef APP_UTILS_H__
#define APP_UTILS_H__

#include <stdbool.h>
#include "nrfx.h"

static inline void app_error_check(nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
    {
        while (true) {}
    }
}

#endif
