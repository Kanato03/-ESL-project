#ifndef BUTTON_CTRL_H
#define BUTTON_CTRL_H

#define DEBOUNCE_MS          30
#define DOUBLE_CLICK_MS      300
#define RELEASE_STABLE_MS    20
#define MAX_HOLD_MS          1000

void button_ctrl_init(void);
void button_ctrl_on_tick(void);

#endif