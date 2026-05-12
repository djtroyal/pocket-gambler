#include "blackjack.h"
#include "display/ssd1306.h"
#include "display/ui.h"
#include "audio/buzzer.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

void bj_init(BlackjackGame *g, uint16_t starting_credits) {
    deck_init(&g->deck);
    deck_shuffle(&g->deck);
    g->credits             = starting_credits;
    g->bet                 = BJ_MIN_BET;
    g->state               = BJ_STATE_BET;
    g->result              = BJ_RESULT_NONE;
    g->doubled_down        = false;
    g->player_split_active = false;
    g->current_split       = 0;
    g->result_sound_played = false;
    hand_clear(&g->player_hand);
    hand_clear(&g->dealer_hand);
    hand_clear(&g->split_hand);
}

void bj_start_round(BlackjackGame *g) {
    // Reshuffle if fewer than 15 cards remain
    if (g->deck.top > DECK_SIZE - 15) {
        deck_shuffle(&g->deck);
    }
    hand_clear(&g->player_hand);
    hand_clear(&g->dealer_hand);
    hand_clear(&g->split_hand);
    g->player_split_active = false;
    g->current_split       = 0;
    g->doubled_down        = false;
    g->result              = BJ_RESULT_NONE;
    g->result_sound_played = false;

    // Deal: player, dealer, player, dealer-hole
    hand_add(&g->player_hand, deck_deal(&g->deck));
    hand_add(&g->dealer_hand, deck_deal(&g->deck));
    hand_add(&g->player_hand, deck_deal(&g->deck));
    hand_add(&g->dealer_hand, deck_deal_face_down(&g->deck));

    g->state = BJ_STATE_PLAYER_TURN;

    // Check for immediate player blackjack
    if (bj_is_blackjack(&g->player_hand)) {
        bj_run_dealer(g);
    }
}

int bj_hand_value(const Hand *h) {
    int value = 0;
    int aces  = 0;
    for (int i = 0; i < h->count; i++) {
        int r = (int)h->cards[i].rank;
        if (r == RANK_ACE) {
            aces++;
            value += 11;
        } else if (r >= RANK_JACK) {
            value += 10;
        } else {
            value += r;
        }
    }
    // Reduce aces from 11 to 1 while busted
    while (value > 21 && aces > 0) {
        value -= 10;
        aces--;
    }
    return value;
}

bool bj_is_blackjack(const Hand *h) {
    return h->count == 2 && bj_hand_value(h) == 21;
}

bool bj_can_split(const BlackjackGame *g) {
    return !g->player_split_active
        && g->player_hand.count == 2
        && g->player_hand.cards[0].rank == g->player_hand.cards[1].rank
        && g->credits >= g->bet;
}

bool bj_can_double(const BlackjackGame *g) {
    return !g->doubled_down
        && g->player_hand.count == 2
        && g->credits >= g->bet;
}

void bj_action_hit(BlackjackGame *g) {
    if (g->state != BJ_STATE_PLAYER_TURN) return;

    Hand *active = g->player_split_active && g->current_split == 1
                   ? &g->split_hand : &g->player_hand;

    hand_add(active, deck_deal(&g->deck));
    buzzer_play_deal();

    if (bj_hand_value(active) > 21) {
        if (g->player_split_active && g->current_split == 0) {
            // First split hand busted — move to second
            g->current_split = 1;
        } else {
            g->result = BJ_RESULT_BUST;
            bj_run_dealer(g);
        }
    }
    if (g->doubled_down) {
        bj_action_stand(g);
    }
}

void bj_action_stand(BlackjackGame *g) {
    if (g->state != BJ_STATE_PLAYER_TURN) return;

    if (g->player_split_active && g->current_split == 0) {
        g->current_split = 1;
        return;
    }
    bj_run_dealer(g);
}

void bj_action_double(BlackjackGame *g) {
    if (g->state != BJ_STATE_PLAYER_TURN || !bj_can_double(g)) return;
    g->credits     -= g->bet;
    g->bet         *= 2;
    g->doubled_down = true;
    bj_action_hit(g);
}

void bj_action_split(BlackjackGame *g) {
    if (g->state != BJ_STATE_PLAYER_TURN || !bj_can_split(g)) return;
    g->credits -= g->bet;
    g->split_bet = g->bet;
    hand_clear(&g->split_hand);
    hand_add(&g->split_hand, g->player_hand.cards[1]);
    g->player_hand.count = 1;
    hand_add(&g->player_hand, deck_deal(&g->deck));
    hand_add(&g->split_hand,  deck_deal(&g->deck));
    g->player_split_active = true;
    g->current_split       = 0;
    buzzer_play_deal();
}

void bj_action_increase_bet(BlackjackGame *g) {
    if (g->state != BJ_STATE_BET) return;
    uint16_t new_bet = g->bet + BJ_BET_STEP;
    uint16_t cap     = g->credits < BJ_MAX_BET ? g->credits : BJ_MAX_BET;
    if (new_bet > cap) new_bet = cap;
    g->bet = new_bet;
}

void bj_action_decrease_bet(BlackjackGame *g) {
    if (g->state != BJ_STATE_BET) return;
    if (g->bet > BJ_MIN_BET) g->bet -= BJ_BET_STEP;
    if (g->bet < BJ_MIN_BET) g->bet  = BJ_MIN_BET;
}

void bj_run_dealer(BlackjackGame *g) {
    g->state = BJ_STATE_DEALER_TURN;
    // Reveal hole card
    g->dealer_hand.cards[1].face_up = true;

    // Dealer hits until >= 17 (stands on soft 17)
    while (bj_hand_value(&g->dealer_hand) < 17) {
        hand_add(&g->dealer_hand, deck_deal(&g->deck));
    }
    bj_evaluate(g);
}

static int bj_settle(BlackjackGame *g, const Hand *player_h, uint16_t bet, bool is_bj) {
    int player_val = bj_hand_value(player_h);
    int dealer_val = bj_hand_value(&g->dealer_hand);
    bool dealer_bj = bj_is_blackjack(&g->dealer_hand);

    if (player_val > 21) {
        return -(int)bet;
    }
    if (is_bj && !dealer_bj) {
        return (int)(bet + bet / 2); // 3:2 payout
    }
    if (is_bj && dealer_bj) {
        return 0; // push
    }
    if (dealer_val > 21 || player_val > dealer_val) {
        return (int)bet;
    }
    if (player_val == dealer_val) {
        return 0; // push
    }
    return -(int)bet;
}

BJResult bj_evaluate(BlackjackGame *g) {
    int net = bj_settle(g, &g->player_hand, g->bet, bj_is_blackjack(&g->player_hand));
    if (g->player_split_active) {
        net += bj_settle(g, &g->split_hand, g->split_bet, false);
    }

    g->credits = (uint16_t)((int)g->credits + net);

    if (net > 0) {
        g->result = bj_is_blackjack(&g->player_hand)
                    ? BJ_RESULT_PLAYER_BLACKJACK : BJ_RESULT_PLAYER_WIN;
    } else if (net < 0) {
        g->result = bj_hand_value(&g->player_hand) > 21
                    ? BJ_RESULT_BUST : BJ_RESULT_DEALER_WIN;
    } else {
        g->result = BJ_RESULT_PUSH;
    }
    g->state = BJ_STATE_RESULT;
    return g->result;
}

void bj_render(const BlackjackGame *g, uint8_t battery_pct) {
    ssd1306_clear();

    if (g->state == BJ_STATE_BET) {
        uint16_t cap = g->credits < BJ_MAX_BET ? g->credits : BJ_MAX_BET;
        ui_draw_bet_screen(g->bet, g->credits, BJ_MIN_BET, cap, "BLACKJACK");
        ssd1306_display();
        return;
    }

    ui_draw_header("BLACKJACK", g->credits, battery_pct);

    // Dealer hand (y=10)
    int dealer_val = g->dealer_hand.cards[1].face_up
                     ? bj_hand_value(&g->dealer_hand) : -1;
    char dbuf[12];
    if (dealer_val < 0)
        snprintf(dbuf, sizeof(dbuf), "DEALER: ?");
    else
        snprintf(dbuf, sizeof(dbuf), "DEALER: %d", dealer_val);
    ssd1306_draw_string(0, 10, dbuf);

    // Draw dealer cards (up to 6 small)
    int dx = 0;
    for (int i = 0; i < g->dealer_hand.count && i < 6; i++) {
        ui_draw_card_small(dx, 18, &g->dealer_hand.cards[i]);
        dx += CARD_SMALL_W + 2;
    }

    // Separator
    ssd1306_draw_hline(0, 37, SSD1306_WIDTH);

    // Player hand
    char pbuf[12];
    int pval = bj_hand_value(&g->player_hand);
    snprintf(pbuf, sizeof(pbuf), "YOU: %d", pval);
    ssd1306_draw_string(0, 39, pbuf);

    int px = 0;
    for (int i = 0; i < g->player_hand.count && i < 6; i++) {
        ui_draw_card_small(px, 47, &g->player_hand.cards[i]);
        px += CARD_SMALL_W + 2;
    }

    // Action hints
    if (g->state == BJ_STATE_PLAYER_TURN) {
        char hint[32];
        if (bj_can_double(g) && bj_can_split(g))
            snprintf(hint, sizeof(hint), "A:Hit B:Stand U:Dbl R:Spl");
        else if (bj_can_double(g))
            snprintf(hint, sizeof(hint), "A:Hit B:Stand U:Dbl");
        else
            snprintf(hint, sizeof(hint), "A:Hit  B:Stand");
        ssd1306_draw_string(0, 57, hint);
    } else if (g->state == BJ_STATE_RESULT) {
        const char *msg;
        switch (g->result) {
            case BJ_RESULT_PLAYER_BLACKJACK: msg = "BLACKJACK! 3:2"; break;
            case BJ_RESULT_PLAYER_WIN:       msg = "YOU WIN!";       break;
            case BJ_RESULT_DEALER_WIN:       msg = "DEALER WINS";    break;
            case BJ_RESULT_PUSH:             msg = "PUSH - TIE";     break;
            case BJ_RESULT_BUST:             msg = "BUST!";          break;
            default:                         msg = "";               break;
        }
        ssd1306_draw_string_centered(57, msg);
    }

    ssd1306_display();
}
