#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

#include <stdbool.h>
#include "color.h"
#include "button_ctrl.h"

#define HUE_STEP_MS          20
#define SAT_BRIGHT_STEP_MS   30
#define HOLD_LOG_PERIOD_MS   250

#define LED1_SLOW_PERIOD_MS  2000
#define LED1_FAST_PERIOD_MS  500

typedef enum
{
    INPUT_MODE_NONE = 0,
    INPUT_MODE_HUE,
    INPUT_MODE_SATURATION,
    INPUT_MODE_BRIGHTNESS,
    INPUT_MODE_COUNT
} input_mode_t;

void color_picker_init(void);
void color_picker_next_mode(void);
void color_picker_on_button(button_event_t evt);

input_mode_t color_picker_mode(void);
hsv_t color_picker_hsv(void);
const char * color_picker_mode_name(input_mode_t mode);

extern volatile bool g_mode_changed;
extern volatile bool g_color_changed;
extern volatile bool g_color_summary;
extern volatile bool g_color_save;
extern volatile bool g_color_loaded;

#endif
