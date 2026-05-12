#include "cards.h"
#include "pico/rand.h"
#include <string.h>

void deck_init(Deck *d) {
    int idx = 0;
    for (int s = 0; s < 4; s++) {
        for (int r = 1; r <= 13; r++) {
            d->cards[idx].rank    = (Rank)r;
            d->cards[idx].suit    = (Suit)s;
            d->cards[idx].face_up = true;
            idx++;
        }
    }
    d->top = 0;
}

void deck_shuffle(Deck *d) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = (int)(get_rand_32() % (uint32_t)(i + 1));
        Card tmp   = d->cards[i];
        d->cards[i] = d->cards[j];
        d->cards[j] = tmp;
    }
    d->top = 0;
}

Card deck_deal(Deck *d) {
    if (d->top >= DECK_SIZE) {
        deck_shuffle(d);
    }
    Card c = d->cards[d->top++];
    c.face_up = true;
    return c;
}

Card deck_deal_face_down(Deck *d) {
    Card c = deck_deal(d);
    c.face_up = false;
    return c;
}

void hand_clear(Hand *h) {
    h->count = 0;
}

void hand_add(Hand *h, Card c) {
    if (h->count < MAX_HAND) {
        h->cards[h->count++] = c;
    }
}

void hand_remove(Hand *h, int index) {
    if (index < 0 || index >= h->count) return;
    for (int i = index; i < h->count - 1; i++) {
        h->cards[i] = h->cards[i + 1];
    }
    h->count--;
}

const char *rank_to_str(Rank r) {
    switch (r) {
        case RANK_ACE:   return "A";
        case RANK_2:     return "2";
        case RANK_3:     return "3";
        case RANK_4:     return "4";
        case RANK_5:     return "5";
        case RANK_6:     return "6";
        case RANK_7:     return "7";
        case RANK_8:     return "8";
        case RANK_9:     return "9";
        case RANK_10:    return "10";
        case RANK_JACK:  return "J";
        case RANK_QUEEN: return "Q";
        case RANK_KING:  return "K";
        default:         return "?";
    }
}

const char *suit_to_char(Suit s) {
    switch (s) {
        case SUIT_CLUBS:    return "C";
        case SUIT_DIAMONDS: return "D";
        case SUIT_HEARTS:   return "H";
        case SUIT_SPADES:   return "S";
        default:            return "?";
    }
}
