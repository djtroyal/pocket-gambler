#include "poker.h"
#include "display/ssd1306.h"
#include "display/ui.h"
#include "audio/buzzer.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

void poker_init(PokerGame *g, uint16_t starting_credits) {
    deck_init(&g->deck);
    deck_shuffle(&g->deck);
    g->credits    = starting_credits;
    g->bet        = POKER_MIN_BET;
    g->pot        = 0;
    g->state      = POKER_STATE_BET_ANTE;
    g->cursor     = 0;
    g->net_result = 0;
    hand_clear(&g->player_hand);
    hand_clear(&g->ai_hand);
    memset(g->discard_flags, 0, sizeof(g->discard_flags));
}

void poker_start_hand(PokerGame *g) {
    if (g->deck.top > DECK_SIZE - 12) {
        deck_shuffle(&g->deck);
    }
    hand_clear(&g->player_hand);
    hand_clear(&g->ai_hand);
    memset(g->discard_flags, 0, sizeof(g->discard_flags));
    g->cursor = 0;
    g->pot    = g->bet * 2; // ante from both sides
    g->credits -= g->bet;

    for (int i = 0; i < 5; i++) {
        hand_add(&g->player_hand, deck_deal(&g->deck));
        hand_add(&g->ai_hand, deck_deal_face_down(&g->deck));
    }
    g->state = POKER_STATE_DISCARD;
    buzzer_play_deal();
}

// ── Hand evaluator ──────────────────────────────────────────────────────────

HandEval poker_evaluate_hand(const Hand *h) {
    HandEval result = {0};
    int rank_count[14] = {0};
    int suit_count[4]  = {0};

    for (int i = 0; i < 5; i++) {
        rank_count[(int)h->cards[i].rank]++;
        suit_count[(int)h->cards[i].suit]++;
    }

    bool is_flush = false;
    for (int s = 0; s < 4; s++) {
        if (suit_count[s] == 5) { is_flush = true; break; }
    }

    // Collect ranks present and check for straight
    int ranks_present[5];
    int rp_count = 0;
    for (int r = 1; r <= 13; r++) {
        if (rank_count[r] > 0) ranks_present[rp_count++] = r;
    }
    bool is_straight = false;
    if (rp_count == 5) {
        is_straight = (ranks_present[4] - ranks_present[0] == 4);
        // Wheel: A-2-3-4-5 (stored as 1,2,3,4,5)
        if (!is_straight && rank_count[1] && rank_count[2] && rank_count[3]
                         && rank_count[4] && rank_count[5]) {
            is_straight = true;
            ranks_present[0] = 1; // Ace low
        }
    }

    int pairs = 0, trips = 0, quads = 0;
    for (int r = 1; r <= 13; r++) {
        if (rank_count[r] == 2) pairs++;
        if (rank_count[r] == 3) trips++;
        if (rank_count[r] == 4) quads++;
    }

    // Classify
    if (is_flush && is_straight) {
        bool royal = rank_count[1] && rank_count[10] && rank_count[11]
                  && rank_count[12] && rank_count[13];
        result.rank = royal ? HAND_ROYAL_FLUSH : HAND_STRAIGHT_FLUSH;
    } else if (quads == 1) {
        result.rank = HAND_FOUR_OF_A_KIND;
    } else if (trips == 1 && pairs == 1) {
        result.rank = HAND_FULL_HOUSE;
    } else if (is_flush) {
        result.rank = HAND_FLUSH;
    } else if (is_straight) {
        result.rank = HAND_STRAIGHT;
    } else if (trips == 1) {
        result.rank = HAND_THREE_OF_A_KIND;
    } else if (pairs == 2) {
        result.rank = HAND_TWO_PAIR;
    } else if (pairs == 1) {
        result.rank = HAND_ONE_PAIR;
    } else {
        result.rank = HAND_HIGH_CARD;
    }

    // Build tiebreaker: sort cards by count desc, then rank desc
    // Use a simple insertion sort on (count, rank) tuples
    int tb_ranks[5];
    int tb_idx = 0;
    // First: quads, then trips, then pairs, then singles — all desc by rank
    for (int freq = 4; freq >= 1; freq--) {
        for (int r = 13; r >= 1; r--) {
            for (int f = 0; f < rank_count[r] && rank_count[r] == freq; f++) {
                if (tb_idx < 5) tb_ranks[tb_idx++] = r;
            }
        }
    }
    for (int i = 0; i < 5; i++) result.tiebreaker[i] = (i < tb_idx) ? tb_ranks[i] : 0;

    return result;
}

int poker_compare(HandEval a, HandEval b) {
    if (a.rank != b.rank) return (int)a.rank - (int)b.rank;
    for (int i = 0; i < 5; i++) {
        if (a.tiebreaker[i] != b.tiebreaker[i])
            return a.tiebreaker[i] - b.tiebreaker[i];
    }
    return 0;
}

// ── AI discard logic ─────────────────────────────────────────────────────────

void poker_ai_discard_and_draw(PokerGame *g) {
    HandEval ev = poker_evaluate_hand(&g->ai_hand);

    bool keep[5] = {false, false, false, false, false};

    // Keep anything pair or better
    if (ev.rank >= HAND_ONE_PAIR) {
        // Mark the cards belonging to the valued combination
        int rank_count[14] = {0};
        for (int i = 0; i < 5; i++) rank_count[(int)g->ai_hand.cards[i].rank]++;

        for (int i = 0; i < 5; i++) {
            int r = (int)g->ai_hand.cards[i].rank;
            if (rank_count[r] >= 2) keep[i] = true;  // part of a pair/trips/quads
        }
        if (ev.rank >= HAND_THREE_OF_A_KIND) {
            // Also keep the kicker for trips or better
            for (int i = 0; i < 5; i++) keep[i] = true;
        }
    } else {
        // Check for 4-to-a-flush
        int suit_count[4] = {0};
        for (int i = 0; i < 5; i++) suit_count[(int)g->ai_hand.cards[i].suit]++;
        for (int s = 0; s < 4; s++) {
            if (suit_count[s] == 4) {
                for (int i = 0; i < 5; i++) {
                    if ((int)g->ai_hand.cards[i].suit == s) keep[i] = true;
                }
                goto draw_ai;
            }
        }
        // Check for 4-to-a-straight (4 consecutive ranks)
        int sorted[5];
        for (int i = 0; i < 5; i++) sorted[i] = (int)g->ai_hand.cards[i].rank;
        // Insertion sort
        for (int i = 1; i < 5; i++) {
            int key = sorted[i], j = i - 1;
            while (j >= 0 && sorted[j] > key) { sorted[j+1] = sorted[j]; j--; }
            sorted[j+1] = key;
        }
        for (int start = 0; start < 2; start++) {
            if (sorted[start+3] - sorted[start] == 3) {
                // Found 4-straight — keep those ranks
                for (int i = 0; i < 5; i++) {
                    int r = (int)g->ai_hand.cards[i].rank;
                    if (r >= sorted[start] && r <= sorted[start+3]) keep[i] = true;
                }
                goto draw_ai;
            }
        }
        // Keep highest card only
        int best_rank = 0, best_idx = 0;
        for (int i = 0; i < 5; i++) {
            if ((int)g->ai_hand.cards[i].rank > best_rank) {
                best_rank = (int)g->ai_hand.cards[i].rank; best_idx = i;
            }
        }
        keep[best_idx] = true;
    }

draw_ai:;
    // Discard and redraw cards not kept
    int new_count = 0;
    Card kept[5];
    for (int i = 0; i < 5; i++) {
        if (keep[i]) kept[new_count++] = g->ai_hand.cards[i];
    }
    hand_clear(&g->ai_hand);
    for (int i = 0; i < new_count; i++) hand_add(&g->ai_hand, kept[i]);
    while (g->ai_hand.count < 5) hand_add(&g->ai_hand, deck_deal(&g->deck));
    // Reveal AI cards for showdown
    for (int i = 0; i < g->ai_hand.count; i++) g->ai_hand.cards[i].face_up = true;
}

// ── Player actions ───────────────────────────────────────────────────────────

void poker_action_move_cursor(PokerGame *g, int dir) {
    if (g->state != POKER_STATE_DISCARD) return;
    g->cursor = (g->cursor + dir + 5) % 5;
}

void poker_action_toggle_discard(PokerGame *g) {
    if (g->state != POKER_STATE_DISCARD) return;
    g->discard_flags[g->cursor] = !g->discard_flags[g->cursor];
    buzzer_play_button_click();
}

void poker_action_confirm_discard(PokerGame *g) {
    if (g->state != POKER_STATE_DISCARD) return;
    // Remove discarded cards, draw replacements
    for (int i = 4; i >= 0; i--) {
        if (g->discard_flags[i]) hand_remove(&g->player_hand, i);
    }
    while (g->player_hand.count < 5) {
        hand_add(&g->player_hand, deck_deal(&g->deck));
    }
    buzzer_play_deal();

    poker_ai_discard_and_draw(g);

    g->player_eval = poker_evaluate_hand(&g->player_hand);
    g->ai_eval     = poker_evaluate_hand(&g->ai_hand);

    int cmp = poker_compare(g->player_eval, g->ai_eval);
    if (cmp > 0) {
        g->net_result = (int)g->pot;
        g->credits   += g->pot;
        buzzer_play_win();
    } else if (cmp < 0) {
        g->net_result = -(int)g->bet;
        buzzer_play_lose();
    } else {
        g->net_result = 0;
        g->credits   += g->bet; // return ante
    }
    g->state = POKER_STATE_RESULT;
}

void poker_action_fold(PokerGame *g) {
    g->net_result = -(int)g->bet;
    g->state      = POKER_STATE_RESULT;
    buzzer_play_lose();
}

void poker_action_increase_bet(PokerGame *g) {
    if (g->state != POKER_STATE_BET_ANTE) return;
    uint16_t new_bet = g->bet + POKER_BET_STEP;
    uint16_t cap     = g->credits < POKER_MAX_BET ? g->credits : POKER_MAX_BET;
    if (new_bet > cap) new_bet = cap;
    g->bet = new_bet;
}

void poker_action_decrease_bet(PokerGame *g) {
    if (g->state != POKER_STATE_BET_ANTE) return;
    if (g->bet > POKER_MIN_BET) g->bet -= POKER_BET_STEP;
}

// ── Render ────────────────────────────────────────────────────────────────────

const char *hand_rank_name(HandRank r) {
    switch (r) {
        case HAND_HIGH_CARD:       return "HIGH CARD";
        case HAND_ONE_PAIR:        return "ONE PAIR";
        case HAND_TWO_PAIR:        return "TWO PAIR";
        case HAND_THREE_OF_A_KIND: return "THREE OF A KIND";
        case HAND_STRAIGHT:        return "STRAIGHT";
        case HAND_FLUSH:           return "FLUSH";
        case HAND_FULL_HOUSE:      return "FULL HOUSE";
        case HAND_FOUR_OF_A_KIND:  return "FOUR OF A KIND";
        case HAND_STRAIGHT_FLUSH:  return "STR FLUSH";
        case HAND_ROYAL_FLUSH:     return "ROYAL FLUSH!";
        default:                   return "?";
    }
}

void poker_render(const PokerGame *g, uint8_t battery_pct) {
    ssd1306_clear();

    if (g->state == POKER_STATE_BET_ANTE) {
        uint16_t cap = g->credits < POKER_MAX_BET ? g->credits : POKER_MAX_BET;
        ui_draw_bet_screen(g->bet, g->credits, POKER_MIN_BET, cap, "5-CARD DRAW");
        ssd1306_display();
        return;
    }

    ui_draw_header("5-CARD DRAW", g->credits, battery_pct);

    // AI hand row (face-down unless showdown/result)
    ssd1306_draw_string(0, 10, "CPU:");
    int ax = 28;
    for (int i = 0; i < g->ai_hand.count; i++) {
        ui_draw_card_small(ax, 10, &g->ai_hand.cards[i]);
        ax += CARD_SMALL_W + 2;
    }

    if (g->state == POKER_STATE_RESULT) {
        ssd1306_draw_string(0, 29, hand_rank_name(g->ai_eval.rank));
    }

    ssd1306_draw_hline(0, 36, SSD1306_WIDTH);

    // Player hand row
    ssd1306_draw_string(0, 37, "YOU:");
    int px = 28;
    for (int i = 0; i < g->player_hand.count; i++) {
        ui_draw_card_small(px, 37, &g->player_hand.cards[i]);

        // Highlight cursor and mark discards
        if (g->state == POKER_STATE_DISCARD) {
            if (i == g->cursor) ssd1306_draw_rect(px - 1, 36, CARD_SMALL_W + 2, CARD_SMALL_H + 2);
            if (g->discard_flags[i]) {
                // X mark over card
                ssd1306_draw_pixel(px, 37, true);
                ssd1306_draw_pixel(px + CARD_SMALL_W - 1, 37, true);
                ssd1306_draw_pixel(px, 37 + CARD_SMALL_H - 1, true);
                ssd1306_draw_pixel(px + CARD_SMALL_W - 1, 37 + CARD_SMALL_H - 1, true);
            }
        }
        px += CARD_SMALL_W + 2;
    }

    if (g->state == POKER_STATE_DISCARD) {
        ssd1306_draw_string(0, 57, "L/R:Move A:Mark B:Draw");
    } else if (g->state == POKER_STATE_RESULT) {
        ssd1306_draw_string(0, 56, hand_rank_name(g->player_eval.rank));
        char buf[16];
        if (g->net_result > 0) snprintf(buf, sizeof(buf), "+$%d WIN!", g->net_result);
        else if (g->net_result < 0) snprintf(buf, sizeof(buf), "LOST $%d", -g->net_result);
        else snprintf(buf, sizeof(buf), "TIE");
        ssd1306_draw_string_centered(57, buf);
    }

    ssd1306_display();
}
