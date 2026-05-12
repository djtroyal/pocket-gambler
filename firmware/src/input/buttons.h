#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BTN_UP     = 0,
    BTN_DOWN   = 1,
    BTN_LEFT   = 2,
    BTN_RIGHT  = 3,
    BTN_A      = 4,  // Hit / Call / Check / Confirm
    BTN_B      = 5,  // Stand / Fold / Cancel
    BTN_START  = 6,
    BTN_SELECT = 7,
    BTN_NONE   = 8
} Button;

typedef enum {
    BTN_EVENT_NONE,
    BTN_EVENT_PRESS,
    BTN_EVENT_REPEAT,
    BTN_EVENT_RELEASE
} ButtonEventType;

typedef struct {
    Button          button;
    ButtonEventType type;
} ButtonEvent;

#define BTN_DEBOUNCE_MS   5
#define BTN_HOLD_MS     500
#define BTN_REPEAT_MS   150
#define BTN_GPIO_BASE     6   // GPIO6 = BTN_UP ... GPIO13 = BTN_SELECT
#define BTN_COUNT         8

void        buttons_init(void);
ButtonEvent buttons_poll(void);
ButtonEvent buttons_wait_any(void);
bool        buttons_is_held(Button b);
void        buttons_clear_events(void);

#endif
