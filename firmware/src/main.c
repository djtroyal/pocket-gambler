#include "pico/stdlib.h"
#include "cards/cards.h"
#include "games/blackjack.h"
#include "games/poker.h"
#include "games/holdem.h"
#include "display/ssd1306.h"
#include "display/ui.h"
#include "input/buttons.h"
#include "audio/buzzer.h"
#include "power/power.h"

typedef enum {
    STATE_MENU,
    STATE_BLACKJACK,
    STATE_POKER,
    STATE_HOLDEM,
    STATE_LOW_BATTERY
} AppState;

static AppState      g_state          = STATE_MENU;
static int           g_menu_sel       = 0;
static uint32_t      g_last_input_ms  = 0;
static uint16_t      g_credits        = 100;

// Game contexts (one active at a time to save RAM)
static BlackjackGame g_bj;
static PokerGame     g_poker;
static HoldemGame    g_holdem;

static uint8_t battery_pct(void) {
    return power_battery_percent();
}

static void update_idle(void) {
    g_last_input_ms = to_ms_since_boot(get_absolute_time());
}

// ── Menu ──────────────────────────────────────────────────────────────────────

static void handle_menu(ButtonEvent ev) {
    if (ev.type == BTN_EVENT_NONE) return;
    update_idle();

    switch (ev.button) {
        case BTN_UP:
            g_menu_sel = (g_menu_sel - 1 + 3) % 3;
            buzzer_play_button_click();
            break;
        case BTN_DOWN:
            g_menu_sel = (g_menu_sel + 1) % 3;
            buzzer_play_button_click();
            break;
        case BTN_A:
        case BTN_START:
            switch (g_menu_sel) {
                case 0:
                    bj_init(&g_bj, g_credits);
                    g_state = STATE_BLACKJACK;
                    break;
                case 1:
                    poker_init(&g_poker, g_credits);
                    g_state = STATE_POKER;
                    break;
                case 2:
                    holdem_init(&g_holdem, g_credits);
                    g_state = STATE_HOLDEM;
                    break;
            }
            buzzer_play_deal();
            break;
        default:
            break;
    }
    ui_draw_main_menu(g_menu_sel, g_credits, battery_pct());
    ssd1306_display();
}

// ── Blackjack ─────────────────────────────────────────────────────────────────

static void handle_blackjack(ButtonEvent ev) {
    if (ev.type == BTN_EVENT_NONE) {
        bj_render(&g_bj, battery_pct());
        return;
    }
    update_idle();

    switch (g_bj.state) {
        case BJ_STATE_BET:
            if (ev.button == BTN_UP   && ev.type != BTN_EVENT_RELEASE) bj_action_increase_bet(&g_bj);
            if (ev.button == BTN_DOWN && ev.type != BTN_EVENT_RELEASE) bj_action_decrease_bet(&g_bj);
            if ((ev.button == BTN_A || ev.button == BTN_START) && ev.type == BTN_EVENT_PRESS) {
                g_credits -= g_bj.bet;
                bj_start_round(&g_bj);
            }
            break;

        case BJ_STATE_PLAYER_TURN:
            if (ev.type != BTN_EVENT_PRESS) break;
            if (ev.button == BTN_A)                          bj_action_hit(&g_bj);
            if (ev.button == BTN_B)                          bj_action_stand(&g_bj);
            if (ev.button == BTN_UP   && bj_can_double(&g_bj)) bj_action_double(&g_bj);
            if (ev.button == BTN_RIGHT && bj_can_split(&g_bj)) bj_action_split(&g_bj);
            break;

        case BJ_STATE_RESULT:
            if (ev.type != BTN_EVENT_PRESS) break;
            if (!g_bj.result_sound_played) {
                if (g_bj.result == BJ_RESULT_PLAYER_BLACKJACK) buzzer_play_blackjack();
                else if (g_bj.result == BJ_RESULT_PLAYER_WIN)  buzzer_play_win();
                else if (g_bj.result != BJ_RESULT_PUSH)        buzzer_play_lose();
                g_bj.result_sound_played = true;
            }
            g_credits = g_bj.credits;
            if (ev.button == BTN_A) {
                g_bj.state = BJ_STATE_BET; // play again
            }
            if (ev.button == BTN_START) {
                g_state = STATE_MENU;
                ui_draw_main_menu(g_menu_sel, g_credits, battery_pct());
                ssd1306_display();
                return;
            }
            break;

        default:
            break;
    }
    bj_render(&g_bj, battery_pct());
}

// ── 5-Card Draw Poker ─────────────────────────────────────────────────────────

static void handle_poker(ButtonEvent ev) {
    if (ev.type == BTN_EVENT_NONE) {
        poker_render(&g_poker, battery_pct());
        return;
    }
    update_idle();

    switch (g_poker.state) {
        case POKER_STATE_BET_ANTE:
            if (ev.button == BTN_UP   && ev.type != BTN_EVENT_RELEASE) poker_action_increase_bet(&g_poker);
            if (ev.button == BTN_DOWN && ev.type != BTN_EVENT_RELEASE) poker_action_decrease_bet(&g_poker);
            if ((ev.button == BTN_A || ev.button == BTN_START) && ev.type == BTN_EVENT_PRESS) {
                poker_start_hand(&g_poker);
            }
            break;

        case POKER_STATE_DISCARD:
            if (ev.type != BTN_EVENT_PRESS && ev.type != BTN_EVENT_REPEAT) break;
            if (ev.button == BTN_LEFT)  poker_action_move_cursor(&g_poker, -1);
            if (ev.button == BTN_RIGHT) poker_action_move_cursor(&g_poker,  1);
            if (ev.button == BTN_A)     poker_action_toggle_discard(&g_poker);
            if (ev.button == BTN_B)     poker_action_confirm_discard(&g_poker);
            if (ev.button == BTN_SELECT) poker_action_fold(&g_poker);
            break;

        case POKER_STATE_RESULT:
            if (ev.type != BTN_EVENT_PRESS) break;
            g_credits = g_poker.credits;
            if (ev.button == BTN_A) {
                g_poker.state = POKER_STATE_BET_ANTE;
            }
            if (ev.button == BTN_START) {
                g_state = STATE_MENU;
                ui_draw_main_menu(g_menu_sel, g_credits, battery_pct());
                ssd1306_display();
                return;
            }
            break;

        default:
            break;
    }
    poker_render(&g_poker, battery_pct());
}

// ── Texas Hold'em ─────────────────────────────────────────────────────────────

static void handle_holdem(ButtonEvent ev) {
    if (g_holdem.hand_number == 0) {
        holdem_start_hand(&g_holdem);
        holdem_render(&g_holdem, battery_pct());
        return;
    }

    if (ev.type == BTN_EVENT_NONE) {
        holdem_render(&g_holdem, battery_pct());
        return;
    }
    update_idle();

    if (g_holdem.state == HOLDEM_STATE_PLAYER_ACT) {
        if (ev.type == BTN_EVENT_PRESS) {
            if (ev.button == BTN_A)    holdem_player_check_call(&g_holdem);
            if (ev.button == BTN_B)    holdem_player_fold(&g_holdem);
            if (ev.button == BTN_START) holdem_player_raise(&g_holdem);
        }
        if (ev.type == BTN_EVENT_PRESS || ev.type == BTN_EVENT_REPEAT) {
            if (ev.button == BTN_UP)   holdem_player_adjust_raise(&g_holdem, 1);
            if (ev.button == BTN_DOWN) holdem_player_adjust_raise(&g_holdem, -1);
        }
    } else if (g_holdem.state == HOLDEM_STATE_RESULT) {
        if (ev.type == BTN_EVENT_PRESS) {
            g_credits = g_holdem.player_stack;
            if (ev.button == BTN_A) {
                holdem_start_hand(&g_holdem);
            }
            if (ev.button == BTN_START) {
                g_state = STATE_MENU;
                ui_draw_main_menu(g_menu_sel, g_credits, battery_pct());
                ssd1306_display();
                return;
            }
        }
    }
    holdem_render(&g_holdem, battery_pct());
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    power_init();
    buttons_init();
    ssd1306_init();
    buzzer_init();

    ui_draw_splash();
    ssd1306_display();
    buttons_wait_any();

    ui_draw_main_menu(g_menu_sel, g_credits, battery_pct());
    ssd1306_display();
    update_idle();

    while (true) {
        ButtonEvent ev = buttons_poll();

        // Sleep on idle timeout
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - g_last_input_ms) > SLEEP_TIMEOUT_MS) {
            power_enter_dormant();
            update_idle();
            // Redraw current screen after wake
        }

        // Low battery check (sample every ~10s approximately via idle counter)
        if (power_is_critical()) {
            g_state = STATE_LOW_BATTERY;
        }

        switch (g_state) {
            case STATE_MENU:       handle_menu(ev);      break;
            case STATE_BLACKJACK:  handle_blackjack(ev); break;
            case STATE_POKER:      handle_poker(ev);     break;
            case STATE_HOLDEM:     handle_holdem(ev);    break;
            case STATE_LOW_BATTERY:
                ui_draw_low_battery_screen();
                ssd1306_display();
                sleep_ms(5000);
                // Return to menu once level recovers (fresh batteries)
                if (!power_is_critical()) g_state = STATE_MENU;
                break;
        }

        sleep_ms(10); // ~100 Hz poll rate
    }
    return 0;
}
