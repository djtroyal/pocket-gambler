#include "ui.h"
#include "ssd1306.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// 7×7 suit glyphs, column-major (7 bytes, each byte = column, LSB = top row)
static const uint8_t GLYPH_CLUBS[7] = {
    0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08  // rounded trefoil
};
static const uint8_t GLYPH_DIAMONDS[7] = {
    0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08  // rotated square (same shape, context differs)
};
static const uint8_t GLYPH_HEARTS[7] = {
    0x0C, 0x1E, 0x3C, 0x78, 0x3C, 0x1E, 0x0C  // two bumps top, point bottom
};
static const uint8_t GLYPH_SPADES[7] = {
    0x10, 0x38, 0x7C, 0x7E, 0x7C, 0x38, 0x10  // upside-down heart + stem
};

// Distinct filled diamond for DIAMONDS
static const uint8_t GLYPH_DIAMOND_REAL[7] = {
    0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08
};

// Heart glyph (two bumps + point)
static const uint8_t GLYPH_HEART_REAL[7] = {
    0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x00
};

// Spade glyph (upside-down heart + stem notch)
static const uint8_t GLYPH_SPADE_REAL[7] = {
    0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x36, 0x08
};

// Club glyph (three circles + stem)
static const uint8_t GLYPH_CLUB_REAL[7] = {
    0x1C, 0x3E, 0x7F, 0x1C, 0x7F, 0x3E, 0x1C
};

void ui_draw_suit_glyph(int x, int y, Suit s) {
    const uint8_t *g;
    switch (s) {
        case SUIT_CLUBS:    g = GLYPH_CLUB_REAL;    break;
        case SUIT_DIAMONDS: g = GLYPH_DIAMOND_REAL; break;
        case SUIT_HEARTS:   g = GLYPH_HEART_REAL;   break;
        case SUIT_SPADES:   g = GLYPH_SPADE_REAL;   break;
        default:            g = GLYPH_CLUB_REAL;    break;
    }
    ssd1306_draw_bitmap(x, y, g, 7, 7);
}

void ui_draw_card_back(int x, int y, int w, int h) {
    ssd1306_draw_rect(x, y, w, h);
    // Crosshatch fill
    for (int r = y + 2; r < y + h - 2; r += 2) {
        ssd1306_draw_hline(x + 1, r, w - 2);
    }
}

void ui_draw_card_large(int x, int y, const Card *c) {
    if (!c->face_up) {
        ui_draw_card_back(x, y, CARD_LARGE_W, CARD_LARGE_H);
        return;
    }
    ssd1306_fill_rect(x, y, CARD_LARGE_W, CARD_LARGE_H);    // white background
    ssd1306_invert_rect(x, y, CARD_LARGE_W, CARD_LARGE_H);  // now black background
    ssd1306_fill_rect(x, y, CARD_LARGE_W, CARD_LARGE_H);    // actually: draw white card
    // Draw white-filled rect then border
    ssd1306_draw_rect(x, y, CARD_LARGE_W, CARD_LARGE_H);
    // Rank top-left
    ssd1306_draw_string(x + 1, y + 1, rank_to_str(c->rank));
    // Suit glyph centered
    ui_draw_suit_glyph(x + (CARD_LARGE_W - 7) / 2, y + (CARD_LARGE_H - 7) / 2, c->suit);
}

void ui_draw_card_small(int x, int y, const Card *c) {
    if (!c->face_up) {
        ui_draw_card_back(x, y, CARD_SMALL_W, CARD_SMALL_H);
        return;
    }
    ssd1306_draw_rect(x, y, CARD_SMALL_W, CARD_SMALL_H);
    ssd1306_draw_char(x + 1, y + 1, rank_to_str(c->rank)[0]);
    // Mini 5×5 suit indicator at bottom center
    ui_draw_suit_glyph(x + (CARD_SMALL_W - 5) / 2, y + CARD_SMALL_H - 7, c->suit);
}

void ui_draw_battery(uint8_t percent) {
    int x = SSD1306_WIDTH - 16;
    int y = 0;
    // Outer battery outline 14×7
    ssd1306_draw_rect(x, y, 13, 7);
    // Positive nub
    ssd1306_draw_vline(x + 13, y + 2, 3);
    // Fill proportional to charge
    int fill = (percent * 11) / 100;
    if (fill > 0) ssd1306_fill_rect(x + 1, y + 1, fill, 5);
}

void ui_draw_header(const char *title, uint16_t credits, uint8_t battery_pct) {
    ssd1306_draw_hline(0, 8, SSD1306_WIDTH);
    ssd1306_draw_string(0, 0, title);
    char buf[12];
    snprintf(buf, sizeof(buf), "$%u", credits);
    int w = ssd1306_string_width(buf);
    ssd1306_draw_string(SSD1306_WIDTH - w - 17, 0, buf);
    ui_draw_battery(battery_pct);
}

void ui_draw_main_menu(int selection, uint16_t credits, uint8_t battery_pct) {
    ssd1306_clear();
    ssd1306_draw_string_centered(0, "POCKET GAMBLER");
    ssd1306_draw_hline(0, 9, SSD1306_WIDTH);
    ui_draw_battery(battery_pct);

    const char *games[] = {"Blackjack", "5-Card Draw", "Texas Holdem"};
    for (int i = 0; i < 3; i++) {
        int y = 14 + i * 14;
        if (i == selection) {
            ssd1306_fill_rect(0, y - 1, SSD1306_WIDTH, 11);
            ssd1306_draw_string(8, y, games[i]);
            // Invert text for highlight
            ssd1306_invert_rect(0, y - 1, SSD1306_WIDTH, 11);
        } else {
            ssd1306_draw_string(8, y, games[i]);
        }
        ssd1306_draw_char(1, y, i == selection ? '>' : ' ');
    }

    char cbuf[16];
    snprintf(cbuf, sizeof(cbuf), "Credits: $%u", credits);
    ssd1306_draw_string_centered(57, cbuf);
}

void ui_draw_credits_display(uint16_t amount, int x, int y) {
    char buf[12];
    snprintf(buf, sizeof(buf), "$%u", amount);
    ssd1306_draw_string(x, y, buf);
}

void ui_draw_pot(uint16_t pot, int x, int y) {
    char buf[16];
    snprintf(buf, sizeof(buf), "POT:$%u", pot);
    ssd1306_draw_string(x, y, buf);
}

void ui_draw_hand_label(const char *label, int x, int y) {
    ssd1306_draw_string(x, y, label);
}

void ui_draw_bet_screen(uint16_t bet, uint16_t credits, uint16_t min_bet, uint16_t max_bet,
                        const char *game_name) {
    ssd1306_clear();
    ssd1306_draw_string_centered(0, game_name);
    ssd1306_draw_hline(0, 9, SSD1306_WIDTH);

    ssd1306_draw_string_centered(16, "PLACE YOUR BET");

    char buf[20];
    snprintf(buf, sizeof(buf), "BET: $%u", bet);
    ssd1306_draw_string_centered(30, buf);

    // Bet bar visualization
    int bar_w = (int)(((uint32_t)(bet - min_bet) * 80) / (max_bet - min_bet + 1));
    ssd1306_draw_rect(24, 42, 82, 7);
    if (bar_w > 0) ssd1306_fill_rect(25, 43, bar_w, 5);

    snprintf(buf, sizeof(buf), "Credits: $%u", credits);
    ssd1306_draw_string_centered(52, buf);

    ssd1306_draw_string(0, 56, "U/D:Adj  A:Confirm");
}

void ui_draw_result_banner(const char *message, int net_credits) {
    ssd1306_clear();
    ssd1306_draw_rect(10, 18, 108, 28);
    ssd1306_draw_string_centered(22, message);
    char buf[16];
    if (net_credits >= 0)
        snprintf(buf, sizeof(buf), "+$%d", net_credits);
    else
        snprintf(buf, sizeof(buf), "-$%d", -net_credits);
    ssd1306_draw_string_centered(32, buf);
    ssd1306_draw_string_centered(54, "A:Again  START:Menu");
}

void ui_draw_message_center(const char *line1, const char *line2) {
    ssd1306_clear();
    ssd1306_draw_string_centered(24, line1);
    if (line2) ssd1306_draw_string_centered(36, line2);
}

void ui_draw_community_cards(const Card *community, int count) {
    // 5 small cards centered, y=12
    int total_w = count * (CARD_SMALL_W + 2) - 2;
    int start_x = (SSD1306_WIDTH - total_w) / 2;
    for (int i = 0; i < count; i++) {
        ui_draw_card_small(start_x + i * (CARD_SMALL_W + 2), 10, &community[i]);
    }
}

static const char *STREET_NAMES[] = {"PREFLOP", "FLOP", "TURN", "RIVER", "SHOWDOWN"};

void ui_draw_street_label(int street) {
    if (street < 0 || street > 4) return;
    ssd1306_draw_string(0, 30, STREET_NAMES[street]);
}

void ui_draw_action_bar_holdem(bool can_check, uint16_t call_amt, uint16_t raise_amt) {
    char buf[32];
    if (can_check)
        snprintf(buf, sizeof(buf), "A:Check B:Fold R:$%u", raise_amt);
    else
        snprintf(buf, sizeof(buf), "A:Call$%u B:Fold", call_amt);
    ssd1306_draw_string(0, 57, buf);
}

void ui_draw_low_battery_screen(void) {
    ssd1306_clear();
    ssd1306_draw_string_centered(20, "LOW BATTERY");
    ssd1306_draw_string_centered(34, "Replace AAA cells");
    ssd1306_draw_string_centered(48, "Game will pause.");
}

void ui_draw_splash(void) {
    ssd1306_clear();

    // Title block
    ssd1306_draw_rect(4, 4, 120, 36);
    ssd1306_draw_string_centered(10, "POCKET");
    ssd1306_draw_string_centered(22, "GAMBLER");

    // Decorative card suits row
    int suits_y = 43;
    int suits_x = (SSD1306_WIDTH - 4 * 9) / 2;
    for (int i = 0; i < 4; i++) {
        ui_draw_suit_glyph(suits_x + i * 9, suits_y, (Suit)i);
    }

    ssd1306_draw_string_centered(56, "PRESS START");
}
