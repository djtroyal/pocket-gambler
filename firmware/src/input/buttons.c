#include "buttons.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

typedef enum {
    BS_UP,
    BS_DEBOUNCING_DOWN,
    BS_DOWN,
    BS_DEBOUNCING_UP
} BtnState;

static BtnState  g_state[BTN_COUNT];
static uint32_t  g_timestamp[BTN_COUNT];
static uint32_t  g_hold_start[BTN_COUNT];
static uint32_t  g_last_repeat[BTN_COUNT];
static bool      g_hold_fired[BTN_COUNT];

// Simple single-slot event queue
static ButtonEvent g_pending = {BTN_NONE, BTN_EVENT_NONE};

void buttons_init(void) {
    for (int i = 0; i < BTN_COUNT; i++) {
        uint pin = BTN_GPIO_BASE + i;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        g_state[i]      = BS_UP;
        g_timestamp[i]  = 0;
        g_hold_start[i] = 0;
        g_last_repeat[i]= 0;
        g_hold_fired[i] = false;
    }
}

ButtonEvent buttons_poll(void) {
    ButtonEvent ev = {BTN_NONE, BTN_EVENT_NONE};
    uint32_t now = to_ms_since_boot(get_absolute_time());
    // Active-low: pressed = GPIO reads 0
    uint32_t raw = (~gpio_get_all() >> BTN_GPIO_BASE) & 0xFF;

    for (int i = 0; i < BTN_COUNT; i++) {
        bool pressed = (raw >> i) & 1;

        switch (g_state[i]) {
            case BS_UP:
                if (pressed) {
                    g_state[i]     = BS_DEBOUNCING_DOWN;
                    g_timestamp[i] = now;
                }
                break;

            case BS_DEBOUNCING_DOWN:
                if (!pressed) {
                    g_state[i] = BS_UP;
                } else if ((now - g_timestamp[i]) >= BTN_DEBOUNCE_MS) {
                    g_state[i]      = BS_DOWN;
                    g_hold_start[i] = now;
                    g_last_repeat[i]= now;
                    g_hold_fired[i] = false;
                    ev.button = (Button)i;
                    ev.type   = BTN_EVENT_PRESS;
                    return ev;
                }
                break;

            case BS_DOWN:
                if (!pressed) {
                    g_state[i]     = BS_DEBOUNCING_UP;
                    g_timestamp[i] = now;
                } else {
                    // Key repeat
                    if (!g_hold_fired[i] && (now - g_hold_start[i]) >= BTN_HOLD_MS) {
                        g_hold_fired[i]  = true;
                        g_last_repeat[i] = now;
                        ev.button = (Button)i;
                        ev.type   = BTN_EVENT_REPEAT;
                        return ev;
                    } else if (g_hold_fired[i] && (now - g_last_repeat[i]) >= BTN_REPEAT_MS) {
                        g_last_repeat[i] = now;
                        ev.button = (Button)i;
                        ev.type   = BTN_EVENT_REPEAT;
                        return ev;
                    }
                }
                break;

            case BS_DEBOUNCING_UP:
                if (pressed) {
                    g_state[i] = BS_DOWN;
                } else if ((now - g_timestamp[i]) >= BTN_DEBOUNCE_MS) {
                    g_state[i] = BS_UP;
                    ev.button  = (Button)i;
                    ev.type    = BTN_EVENT_RELEASE;
                    return ev;
                }
                break;
        }
    }
    return ev;
}

ButtonEvent buttons_wait_any(void) {
    ButtonEvent ev;
    do {
        ev = buttons_poll();
        sleep_ms(10);
    } while (ev.type != BTN_EVENT_PRESS);
    return ev;
}

bool buttons_is_held(Button b) {
    if (b >= BTN_COUNT) return false;
    uint32_t raw = (~gpio_get_all() >> BTN_GPIO_BASE) & 0xFF;
    return (raw >> (int)b) & 1;
}

void buttons_clear_events(void) {
    for (int i = 0; i < BTN_COUNT; i++) {
        g_state[i] = BS_UP;
    }
}
