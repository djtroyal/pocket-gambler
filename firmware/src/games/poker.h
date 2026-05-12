#ifndef POKER_H
#define POKER_H

#include "cards/cards.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POKER_STATE_BET_ANTE,
    POKER_STATE_DEAL,
    POKER_STATE_DISCARD,
    POKER_STATE_DRAW,
    POKER_STATE_SHOWDOWN,
    POKER_STATE_RESULT
} PokerState;

typedef enum {
    HAND_HIGH_CARD = 0,
    HAND_ONE_PAIR,
    HAND_TWO_PAIR,
    HAND_THREE_OF_A_KIND,
    HAND_STRAIGHT,
    HAND_FLUSH,
    HAND_FULL_HOUSE,
    HAND_FOUR_OF_A_KIND,
    HAND_STRAIGHT_FLUSH,
    HAND_ROYAL_FLUSH
} HandRank;

typedef struct {
    HandRank rank;
    int      tiebreaker[5];
} HandEval;

#define POKER_MIN_BET   5
#define POKER_MAX_BET  50
#define POKER_BET_STEP  5

typedef struct {
    Deck       deck;
    Hand       player_hand;
    Hand       ai_hand;
    bool       discard_flags[5];
    int        cursor;
    uint16_t   bet;
    uint16_t   pot;
    uint16_t   credits;
    PokerState state;
    HandEval   player_eval;
    HandEval   ai_eval;
    int        net_result;
} PokerGame;

void     poker_init(PokerGame *g, uint16_t starting_credits);
void     poker_start_hand(PokerGame *g);
HandEval poker_evaluate_hand(const Hand *h);
int      poker_compare(HandEval a, HandEval b);

void     poker_ai_discard_and_draw(PokerGame *g);
void     poker_action_toggle_discard(PokerGame *g);
void     poker_action_move_cursor(PokerGame *g, int dir);
void     poker_action_confirm_discard(PokerGame *g);
void     poker_action_fold(PokerGame *g);
void     poker_action_increase_bet(PokerGame *g);
void     poker_action_decrease_bet(PokerGame *g);

void     poker_render(const PokerGame *g, uint8_t battery_pct);

const char *hand_rank_name(HandRank r);

#endif
