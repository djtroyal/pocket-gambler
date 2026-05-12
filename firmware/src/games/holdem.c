#include "holdem.h"
#include "display/ssd1306.h"
#include "display/ui.h"
#include "audio/buzzer.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// All C(7,5)=21 five-card index combinations from 7 cards
static const int COMBOS_7C5[21][5] = {
    {0,1,2,3,4},{0,1,2,3,5},{0,1,2,3,6},
    {0,1,2,4,5},{0,1,2,4,6},{0,1,2,5,6},
    {0,1,3,4,5},{0,1,3,4,6},{0,1,3,5,6},
    {0,1,4,5,6},{0,2,3,4,5},{0,2,3,4,6},
    {0,2,3,5,6},{0,2,4,5,6},{0,3,4,5,6},
    {1,2,3,4,5},{1,2,3,4,6},{1,2,3,5,6},
    {1,2,4,5,6},{1,3,4,5,6},{2,3,4,5,6}
};

HandEval holdem_best_hand(const Hand *hole, const Card *community, int comm_count) {
    // Assemble 7-card pool (may be 5-7 depending on street)
    Card all[7];
    int total = 0;
    for (int i = 0; i < hole->count && total < 7; i++) all[total++] = hole->cards[i];
    for (int i = 0; i < comm_count && total < 7; i++) all[total++] = community[i];

    HandEval best = {HAND_HIGH_CARD, {0,0,0,0,0}};
    // C(total, 5) combinations
    for (int c = 0; c < 21; c++) {
        // Check combo indices are within bounds
        if (COMBOS_7C5[c][4] >= total) continue;
        Hand h;
        hand_clear(&h);
        for (int k = 0; k < 5; k++) hand_add(&h, all[COMBOS_7C5[c][k]]);
        HandEval ev = poker_evaluate_hand(&h);
        if (poker_compare(ev, best) > 0) best = ev;
    }
    return best;
}

void holdem_init(HoldemGame *g, uint16_t starting_credits) {
    deck_init(&g->deck);
    deck_shuffle(&g->deck);
    g->player_stack      = starting_credits;
    g->ai_stack          = starting_credits;
    g->hand_number       = 0;
    g->player_is_dealer  = true;
    g->result_sound_played = false;
    g->raise_amount      = HOLDEM_MIN_RAISE;
    hand_clear(&g->player_hole);
    hand_clear(&g->ai_hole);
    memset(g->community, 0, sizeof(g->community));
    g->community_count   = 0;
}

void holdem_start_hand(HoldemGame *g) {
    if (g->deck.top > DECK_SIZE - 10) deck_shuffle(&g->deck);

    hand_clear(&g->player_hole);
    hand_clear(&g->ai_hole);
    memset(g->community, 0, sizeof(g->community));
    g->community_count     = 0;
    g->pot                 = 0;
    g->player_bet_street   = 0;
    g->ai_bet_street       = 0;
    g->player_folded       = false;
    g->ai_folded           = false;
    g->street              = HOLDEM_STREET_PREFLOP;
    g->result_sound_played = false;
    g->raise_amount        = HOLDEM_MIN_RAISE;
    g->hand_number++;

    // Post blinds (dealer posts SB, other posts BB)
    uint16_t sb = HOLDEM_SMALL_BLIND;
    uint16_t bb = HOLDEM_BIG_BLIND;
    if (sb > g->player_stack) sb = g->player_stack;
    if (bb > g->ai_stack) bb = g->ai_stack;

    if (g->player_is_dealer) {
        g->player_stack -= sb; g->player_bet_street = sb;
        g->ai_stack     -= bb; g->ai_bet_street     = bb;
        g->current_to_call = bb - sb;
        g->state = HOLDEM_STATE_PLAYER_ACT;
    } else {
        g->ai_stack     -= sb; g->ai_bet_street     = sb;
        g->player_stack -= bb; g->player_bet_street = bb;
        g->current_to_call = 0;
        g->state = HOLDEM_STATE_AI_ACT;
    }
    g->pot = sb + bb;

    // Deal hole cards
    hand_add(&g->player_hole, deck_deal(&g->deck));
    hand_add(&g->ai_hole,     deck_deal_face_down(&g->deck));
    hand_add(&g->player_hole, deck_deal(&g->deck));
    hand_add(&g->ai_hole,     deck_deal_face_down(&g->deck));

    if (g->state == HOLDEM_STATE_AI_ACT) holdem_ai_action(g);

    buzzer_play_deal();
    g->player_is_dealer = !g->player_is_dealer;
}

void holdem_next_street(HoldemGame *g) {
    g->player_bet_street = 0;
    g->ai_bet_street     = 0;
    g->current_to_call   = 0;
    g->street            = (HoldemStreet)((int)g->street + 1);

    switch (g->street) {
        case HOLDEM_STREET_FLOP:
            for (int i = 0; i < 3; i++) {
                g->community[g->community_count++] = deck_deal(&g->deck);
            }
            break;
        case HOLDEM_STREET_TURN:
        case HOLDEM_STREET_RIVER:
            g->community[g->community_count++] = deck_deal(&g->deck);
            break;
        case HOLDEM_STREET_SHOWDOWN:
            holdem_next_street(g); // fall through to showdown
            return;
        default:
            break;
    }

    if (g->street <= HOLDEM_STREET_RIVER) {
        // Post-flop: non-dealer acts first
        g->state = g->player_is_dealer ? HOLDEM_STATE_AI_ACT : HOLDEM_STATE_PLAYER_ACT;
        if (g->state == HOLDEM_STATE_AI_ACT) holdem_ai_action(g);
    }
}

static void holdem_showdown(HoldemGame *g) {
    // Reveal AI hole cards
    for (int i = 0; i < g->ai_hole.count; i++) g->ai_hole.cards[i].face_up = true;

    HandEval player_ev = holdem_best_hand(&g->player_hole, g->community, g->community_count);
    HandEval ai_ev     = holdem_best_hand(&g->ai_hole,     g->community, g->community_count);

    int cmp = poker_compare(player_ev, ai_ev);
    if (cmp > 0) {
        g->net_result    = (int)g->pot;
        g->player_stack += g->pot;
        buzzer_play_win();
    } else if (cmp < 0) {
        g->net_result = -(int)g->pot;
        g->ai_stack  += g->pot;
        buzzer_play_lose();
    } else {
        g->net_result    = 0;
        g->player_stack += g->pot / 2;
        g->ai_stack     += g->pot / 2;
    }
    g->state = HOLDEM_STATE_RESULT;
}

// ── AI logic ─────────────────────────────────────────────────────────────────

typedef enum { AI_STRENGTH_TRASH=0, AI_STRENGTH_WEAK, AI_STRENGTH_MEDIUM, AI_STRENGTH_STRONG } AIStrength;

static AIStrength preflop_strength(const Hand *hole) {
    int r1 = (int)hole->cards[0].rank;
    int r2 = (int)hole->cards[1].rank;
    bool suited = hole->cards[0].suit == hole->cards[1].suit;
    if (r1 < r2) { int tmp=r1; r1=r2; r2=tmp; } // r1 >= r2

    if (r1 == r2 && r1 >= 8)  return AI_STRENGTH_STRONG;
    if (r1 == r2)              return AI_STRENGTH_MEDIUM;
    if (r1 == 1  && r2 >= 10) return AI_STRENGTH_STRONG;  // AK, AQ, AJ, AT
    if (r1 == 1)               return suited ? AI_STRENGTH_MEDIUM : AI_STRENGTH_WEAK;
    if (r1 >= 11 && r2 >= 10 && suited) return AI_STRENGTH_STRONG; // KQ, KJ, QJ suited
    if (r1 >= 10 && r2 >= 9)   return AI_STRENGTH_MEDIUM;
    if (suited && r1 - r2 <= 3) return AI_STRENGTH_WEAK;
    return AI_STRENGTH_TRASH;
}

void holdem_ai_action(HoldemGame *g) {
    bool bluff = (get_rand_32() % 10 == 0);
    uint16_t call_needed = g->current_to_call;
    if (call_needed > g->ai_stack) call_needed = g->ai_stack;

    AIStrength strength;
    if (g->street == HOLDEM_STREET_PREFLOP) {
        strength = preflop_strength(&g->ai_hole);
    } else {
        HandEval ev = holdem_best_hand(&g->ai_hole, g->community, g->community_count);
        if      (ev.rank >= HAND_TWO_PAIR)  strength = AI_STRENGTH_STRONG;
        else if (ev.rank == HAND_ONE_PAIR)  strength = AI_STRENGTH_MEDIUM;
        else                                strength = AI_STRENGTH_WEAK;
    }

    if (bluff) strength = AI_STRENGTH_STRONG; // 10% bluff

    if (strength == AI_STRENGTH_TRASH && !bluff) {
        if (call_needed > 0) {
            // Fold
            g->ai_folded = true;
            g->player_stack += g->pot;
            g->net_result    = (int)g->pot;
            g->state         = HOLDEM_STATE_RESULT;
            buzzer_play_win();
            return;
        }
        // Check
        g->ai_bet_street += 0;
    } else if (strength == AI_STRENGTH_WEAK) {
        if (call_needed > 0) {
            // Call
            g->ai_stack      -= call_needed;
            g->ai_bet_street += call_needed;
            g->pot           += call_needed;
        }
        // else check
    } else if (strength == AI_STRENGTH_MEDIUM || strength == AI_STRENGTH_STRONG) {
        // Bet/raise: random 50-100% of pot
        uint16_t raise_sz = (uint16_t)(g->pot / 2 + (get_rand_32() % (g->pot / 2 + 1)));
        if (raise_sz < HOLDEM_MIN_RAISE) raise_sz = HOLDEM_MIN_RAISE;
        if (raise_sz > g->ai_stack) raise_sz = g->ai_stack;

        if (call_needed > 0) raise_sz += call_needed;

        if (raise_sz > g->ai_stack) raise_sz = g->ai_stack;
        g->ai_stack      -= raise_sz;
        g->ai_bet_street += raise_sz;
        g->pot           += raise_sz;
        g->current_to_call = raise_sz - call_needed;
    }

    g->state = HOLDEM_STATE_PLAYER_ACT;
}

// ── Player actions ───────────────────────────────────────────────────────────

void holdem_player_fold(HoldemGame *g) {
    g->player_folded  = true;
    g->ai_stack      += g->pot;
    g->net_result     = -(int)g->player_bet_street;
    g->state          = HOLDEM_STATE_RESULT;
    buzzer_play_lose();
}

void holdem_player_check_call(HoldemGame *g) {
    if (g->state != HOLDEM_STATE_PLAYER_ACT) return;
    uint16_t call_needed = g->current_to_call;
    if (call_needed > g->player_stack) call_needed = g->player_stack;

    g->player_stack      -= call_needed;
    g->player_bet_street += call_needed;
    g->pot               += call_needed;
    g->current_to_call    = 0;

    // Advance street if both players have matched
    if (g->player_bet_street == g->ai_bet_street || call_needed == 0) {
        if (g->street == HOLDEM_STREET_RIVER) {
            holdem_showdown(g);
        } else {
            holdem_next_street(g);
        }
    } else {
        g->state = HOLDEM_STATE_AI_ACT;
        holdem_ai_action(g);
    }
}

void holdem_player_raise(HoldemGame *g) {
    if (g->state != HOLDEM_STATE_PLAYER_ACT) return;
    uint16_t total = g->raise_amount + g->current_to_call;
    if (total > g->player_stack) total = g->player_stack;

    g->player_stack      -= total;
    g->player_bet_street += total;
    g->pot               += total;
    g->current_to_call    = g->raise_amount;

    g->state = HOLDEM_STATE_AI_ACT;
    holdem_ai_action(g);
}

void holdem_player_adjust_raise(HoldemGame *g, int dir) {
    int new_raise = (int)g->raise_amount + dir * HOLDEM_RAISE_STEP;
    if (new_raise < HOLDEM_MIN_RAISE) new_raise = HOLDEM_MIN_RAISE;
    if (new_raise > (int)g->player_stack) new_raise = (int)g->player_stack;
    g->raise_amount = (uint16_t)new_raise;
}

// ── Render ────────────────────────────────────────────────────────────────────

static const char *STREET_LABELS[] = {"PREFLOP","FLOP","TURN","RIVER","SHOWDOWN"};

void holdem_render(const HoldemGame *g, uint8_t battery_pct) {
    ssd1306_clear();

    // Header: stacks + pot
    char hbuf[32];
    snprintf(hbuf, sizeof(hbuf), "YOU:$%u", g->player_stack);
    ssd1306_draw_string(0, 0, hbuf);
    ui_draw_battery(battery_pct);

    snprintf(hbuf, sizeof(hbuf), "POT:$%u", g->pot);
    ssd1306_draw_string_centered(0, hbuf);

    snprintf(hbuf, sizeof(hbuf), "CPU:$%u", g->ai_stack);
    int w = ssd1306_string_width(hbuf);
    ssd1306_draw_string(SSD1306_WIDTH - w - 17, 0, hbuf);
    ssd1306_draw_hline(0, 8, SSD1306_WIDTH);

    // Street label + community cards
    ssd1306_draw_string(0, 10, STREET_LABELS[(int)g->street]);

    if (g->community_count > 0) {
        int total_w = g->community_count * (CARD_SMALL_W + 2) - 2;
        int start_x = (SSD1306_WIDTH - total_w) / 2;
        for (int i = 0; i < g->community_count; i++) {
            ui_draw_card_small(start_x + i * (CARD_SMALL_W + 2), 10, &g->community[i]);
        }
    }

    ssd1306_draw_hline(0, 29, SSD1306_WIDTH);

    // AI hole cards (face down, revealed on showdown)
    ssd1306_draw_string(0, 31, "CPU:");
    int ax = 28;
    for (int i = 0; i < g->ai_hole.count; i++) {
        ui_draw_card_small(ax, 31, &g->ai_hole.cards[i]);
        ax += CARD_SMALL_W + 2;
    }

    ssd1306_draw_hline(0, 43, SSD1306_WIDTH);

    // Player hole cards
    ssd1306_draw_string(0, 45, "YOU:");
    int px = 28;
    for (int i = 0; i < g->player_hole.count; i++) {
        ui_draw_card_large(px, 37, &g->player_hole.cards[i]);
        px += CARD_LARGE_W + 2;
    }

    // Action bar
    if (g->state == HOLDEM_STATE_PLAYER_ACT) {
        char abuf[32];
        if (g->current_to_call == 0)
            snprintf(abuf, sizeof(abuf), "A:Chk U/D:$%u B:Fold", g->raise_amount);
        else
            snprintf(abuf, sizeof(abuf), "A:Call$%u B:Fold", g->current_to_call);
        ssd1306_draw_string(0, 57, abuf);
    } else if (g->state == HOLDEM_STATE_AI_ACT) {
        ssd1306_draw_string_centered(57, "CPU thinking...");
    } else if (g->state == HOLDEM_STATE_RESULT) {
        char rbuf[20];
        if (g->player_folded) snprintf(rbuf, sizeof(rbuf), "You folded");
        else if (g->ai_folded) snprintf(rbuf, sizeof(rbuf), "CPU folded-YOU WIN");
        else if (g->net_result > 0) snprintf(rbuf, sizeof(rbuf), "YOU WIN +$%d!", g->net_result);
        else if (g->net_result < 0) snprintf(rbuf, sizeof(rbuf), "CPU wins -$%d", -g->net_result);
        else snprintf(rbuf, sizeof(rbuf), "TIE - split pot");
        ssd1306_draw_string_centered(57, rbuf);
    }

    ssd1306_display();
}
