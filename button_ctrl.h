#ifndef BUTTON_CTRL_H
#define BUTTON_CTRL_H

#define DEBOUNCE_MS          30
#define DOUBLE_CLICK_MS      300
#define HOLD_DELAY_MS        200

typedef enum
{
    BUTTON_EVENT_PRESSED,       // any debounced press
    BUTTON_EVENT_DOUBLE_CLICK,  // second press within DOUBLE_CLICK_MS after a short click
    BUTTON_EVENT_HOLD_START,    // button held longer than HOLD_DELAY_MS
    BUTTON_EVENT_HOLD_END       // button released after HOLD_START
} button_event_t;

typedef void (*button_event_handler_t)(button_event_t evt);

void button_ctrl_init(button_event_handler_t handler);

#endif
