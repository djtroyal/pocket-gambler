#ifndef BLACKJACK_H
#define BLACKJACK_H

#include "cards/cards.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BJ_STATE_BET,
    BJ_STATE_PLAYER_TURN,
    BJ_STATE_DEALER_TURN,
    BJ_STATE_RESULT
} BJState;

typedef enum {
    BJ_RESULT_NONE,
    BJ_RESULT_PLAYER_WIN,
    BJ_RESULT_PLAYER_BLACKJACK,
    BJ_RESULT_DEALER_WIN,
    BJ_RESULT_PUSH,
    BJ_RESULT_BUST
} BJResult;

#define BJ_MIN_BET   5
#define BJ_MAX_BET  50
#define BJ_BET_STEP  5

typedef struct {
    Deck     deck;
    Hand     player_hand;
    Hand     dealer_hand;
    bool     player_split_active;
    Hand     split_hand;
    int      current_split;
    uint16_t bet;
    uint16_t split_bet;
    uint16_t credits;
    BJState  state;
    BJResult result;
    bool     doubled_down;
    bool     result_sound_played;
} BlackjackGame;

void     bj_init(BlackjackGame *g, uint16_t starting_credits);
void     bj_start_round(BlackjackGame *g);
int      bj_hand_value(const Hand *h);
bool     bj_is_blackjack(const Hand *h);
bool     bj_can_split(const BlackjackGame *g);
bool     bj_can_double(const BlackjackGame *g);

void     bj_action_hit(BlackjackGame *g);
void     bj_action_stand(BlackjackGame *g);
void     bj_action_double(BlackjackGame *g);
void     bj_action_split(BlackjackGame *g);
void     bj_action_increase_bet(BlackjackGame *g);
void     bj_action_decrease_bet(BlackjackGame *g);

void     bj_run_dealer(BlackjackGame *g);
BJResult bj_evaluate(BlackjackGame *g);
void     bj_render(const BlackjackGame *g, uint8_t battery_pct);

#endif
