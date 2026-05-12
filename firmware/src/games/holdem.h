#ifndef HOLDEM_H
#define HOLDEM_H

#include "cards/cards.h"
#include "poker.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HOLDEM_STREET_PREFLOP = 0,
    HOLDEM_STREET_FLOP,
    HOLDEM_STREET_TURN,
    HOLDEM_STREET_RIVER,
    HOLDEM_STREET_SHOWDOWN
} HoldemStreet;

typedef enum {
    HOLDEM_STATE_PLAYER_ACT,
    HOLDEM_STATE_AI_ACT,
    HOLDEM_STATE_DEALING,
    HOLDEM_STATE_SHOWDOWN,
    HOLDEM_STATE_RESULT
} HoldemState;

#define HOLDEM_SMALL_BLIND   5
#define HOLDEM_BIG_BLIND    10
#define HOLDEM_MIN_RAISE    10
#define HOLDEM_RAISE_STEP    5

typedef struct {
    Deck         deck;
    Hand         player_hole;
    Hand         ai_hole;
    Card         community[5];
    int          community_count;
    uint16_t     pot;
    uint16_t     player_stack;
    uint16_t     ai_stack;
    uint16_t     player_bet_street;
    uint16_t     ai_bet_street;
    uint16_t     current_to_call;
    bool         player_is_dealer;
    bool         player_folded;
    bool         ai_folded;
    HoldemStreet street;
    HoldemState  state;
    int          hand_number;
    uint16_t     raise_amount;
    bool         result_sound_played;
    int          net_result;
} HoldemGame;

void     holdem_init(HoldemGame *g, uint16_t starting_credits);
void     holdem_start_hand(HoldemGame *g);
void     holdem_next_street(HoldemGame *g);
void     holdem_ai_action(HoldemGame *g);

void     holdem_player_fold(HoldemGame *g);
void     holdem_player_check_call(HoldemGame *g);
void     holdem_player_raise(HoldemGame *g);
void     holdem_player_adjust_raise(HoldemGame *g, int dir);

HandEval holdem_best_hand(const Hand *hole, const Card *community, int comm_count);

void     holdem_render(const HoldemGame *g, uint8_t battery_pct);

#endif
