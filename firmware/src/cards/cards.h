#ifndef CARDS_H
#define CARDS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SUIT_CLUBS    = 0,
    SUIT_DIAMONDS = 1,
    SUIT_HEARTS   = 2,
    SUIT_SPADES   = 3
} Suit;

typedef enum {
    RANK_ACE   = 1,
    RANK_2     = 2,
    RANK_3     = 3,
    RANK_4     = 4,
    RANK_5     = 5,
    RANK_6     = 6,
    RANK_7     = 7,
    RANK_8     = 8,
    RANK_9     = 9,
    RANK_10    = 10,
    RANK_JACK  = 11,
    RANK_QUEEN = 12,
    RANK_KING  = 13
} Rank;

typedef struct {
    Rank rank;
    Suit suit;
    bool face_up;
} Card;

#define DECK_SIZE 52
#define MAX_HAND  7

typedef struct {
    Card cards[DECK_SIZE];
    int  top;
} Deck;

typedef struct {
    Card cards[MAX_HAND];
    int  count;
} Hand;

void       deck_init(Deck *d);
void       deck_shuffle(Deck *d);
Card       deck_deal(Deck *d);
Card       deck_deal_face_down(Deck *d);

void       hand_clear(Hand *h);
void       hand_add(Hand *h, Card c);
void       hand_remove(Hand *h, int index);

const char *rank_to_str(Rank r);
const char *suit_to_char(Suit s);

#endif
