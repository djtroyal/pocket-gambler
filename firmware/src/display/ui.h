#ifndef UI_H
#define UI_H

#include "cards/cards.h"
#include <stdint.h>
#include <stdbool.h>

// Card rendering sizes (px)
#define CARD_LARGE_W  18
#define CARD_LARGE_H  26
#define CARD_SMALL_W  12
#define CARD_SMALL_H  18
#define CARD_MINI_W    8
#define CARD_MINI_H   12

void ui_draw_battery(uint8_t percent);

void ui_draw_card_large(int x, int y, const Card *c);
void ui_draw_card_small(int x, int y, const Card *c);
void ui_draw_card_back(int x, int y, int w, int h);
void ui_draw_suit_glyph(int x, int y, Suit s);

void ui_draw_main_menu(int selection, uint16_t credits, uint8_t battery_pct);
void ui_draw_credits_display(uint16_t amount, int x, int y);
void ui_draw_pot(uint16_t pot, int x, int y);
void ui_draw_hand_label(const char *label, int x, int y);

void ui_draw_bet_screen(uint16_t bet, uint16_t credits, uint16_t min_bet, uint16_t max_bet,
                        const char *game_name);
void ui_draw_result_banner(const char *message, int net_credits);
void ui_draw_message_center(const char *line1, const char *line2);

void ui_draw_community_cards(const Card *community, int count);
void ui_draw_action_bar_holdem(bool can_check, uint16_t call_amt, uint16_t raise_amt);
void ui_draw_street_label(int street);

void ui_draw_low_battery_screen(void);
void ui_draw_splash(void);
void ui_draw_header(const char *title, uint16_t credits, uint8_t battery_pct);

#endif
