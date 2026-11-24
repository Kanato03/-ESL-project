#ifndef BUTTON_CTRL_H
#define BUTTON_CTRL_H

#include <stdbool.h>

#define DEBOUNCE_MS          30
#define DOUBLE_CLICK_MS      300
#define HOLD_DELAY_MS        200

void button_ctrl_init(void);
void button_ctrl_on_tick(void);
bool button_ctrl_is_held(void);

extern volatile bool g_button_pressed;

#endif
