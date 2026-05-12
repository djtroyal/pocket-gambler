#!/usr/bin/env python3
"""
Pocket Gambler — Terminal Simulator
Blackjack, 5-Card Draw Poker, Texas Hold'em

Controls:
  UP / DOWN        Adjust bet or raise amount
  LEFT / RIGHT     Move cursor (Poker discard phase)
  Z or Enter       A button  (Hit / Call / Check / Mark discard)
  X or Backspace   B button  (Stand / Fold)
  Space            START     (Deal / New round)
  Tab              SELECT    (Fold in Poker)
  Q                Quit to menu

Requires Python 3.7+  |  Terminal ≥ 62×22
"""

import curses, random, sys
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Tuple, Optional
from itertools import combinations


# ══════════════════════════════════════════════════════════════════════════════
# CARD ENGINE
# ══════════════════════════════════════════════════════════════════════════════

class Suit(IntEnum):
    CLUBS=0; DIAMONDS=1; HEARTS=2; SPADES=3

class Rank(IntEnum):
    ACE=1; TWO=2; THREE=3; FOUR=4; FIVE=5; SIX=6; SEVEN=7
    EIGHT=8; NINE=9; TEN=10; JACK=11; QUEEN=12; KING=13

_SUIT_SYM = {Suit.CLUBS:'c', Suit.DIAMONDS:'d', Suit.HEARTS:'h', Suit.SPADES:'s'}
_RANK_STR = {Rank.ACE:'A', Rank.TWO:'2', Rank.THREE:'3', Rank.FOUR:'4',
             Rank.FIVE:'5', Rank.SIX:'6', Rank.SEVEN:'7', Rank.EIGHT:'8',
             Rank.NINE:'9', Rank.TEN:'T', Rank.JACK:'J', Rank.QUEEN:'Q', Rank.KING:'K'}


@dataclass
class Card:
    rank: Rank
    suit: Suit
    face_up: bool = True

    def label(self) -> str:
        return f"{_RANK_STR[self.rank]}{_SUIT_SYM[self.suit]}" if self.face_up else '??'

    def color(self) -> int:
        # Returns curses color pair index (red for H/D, white for C/S)
        return 2 if self.suit in (Suit.HEARTS, Suit.DIAMONDS) and self.face_up else 1


class Deck:
    def __init__(self):
        self._reset()

    def _reset(self):
        self._cards: List[Card] = [Card(r, s) for s in Suit for r in Rank]
        random.shuffle(self._cards)

    def deal(self, face_up=True) -> Card:
        if len(self._cards) < 10:
            self._reset()
        c = self._cards.pop()
        c.face_up = face_up
        return c


# ══════════════════════════════════════════════════════════════════════════════
# HAND EVALUATOR
# ══════════════════════════════════════════════════════════════════════════════

class HR(IntEnum):                         # Hand Rank
    HIGH_CARD=0; ONE_PAIR=1; TWO_PAIR=2; THREE_OF_A_KIND=3
    STRAIGHT=4; FLUSH=5; FULL_HOUSE=6; FOUR_OF_A_KIND=7
    STRAIGHT_FLUSH=8; ROYAL_FLUSH=9

_HR_NAME = ['HIGH CARD','ONE PAIR','TWO PAIR','THREE OF A KIND',
            'STRAIGHT','FLUSH','FULL HOUSE','FOUR OF A KIND',
            'STRAIGHT FLUSH','ROYAL FLUSH']


def _eval5(cards: List[Card]) -> Tuple[HR, List[int]]:
    rv = [c.rank.value for c in cards]
    rc: dict = {}
    for r in rv: rc[r] = rc.get(r, 0) + 1
    flush   = len({c.suit for c in cards}) == 1
    sr      = sorted(set(rv))
    straight = len(sr) == 5 and sr[-1] - sr[0] == 4
    if not straight and set(rv) == {1, 2, 3, 4, 5}:
        straight = True                            # wheel A-2-3-4-5
    if not straight and set(rv) == {1, 10, 11, 12, 13}:
        straight = True                            # broadway T-J-Q-K-A
    # For tiebreakers treat Ace as 14 when used high (broadway / no wheel context)
    ace_high = set(rv) == {1, 10, 11, 12, 13}
    tb_rv = [14 if (r == 1 and ace_high) else r for r in rv]
    tb = sorted(tb_rv, key=lambda r: (rc.get(r, rc.get(1 if r==14 else r, 0)), r), reverse=True)
    p = sum(1 for v in rc.values() if v == 2)
    t = sum(1 for v in rc.values() if v == 3)
    q = sum(1 for v in rc.values() if v == 4)
    if   flush and straight: hr = HR.ROYAL_FLUSH if set(rv)=={1,10,11,12,13} else HR.STRAIGHT_FLUSH
    elif q:                  hr = HR.FOUR_OF_A_KIND
    elif t and p:            hr = HR.FULL_HOUSE
    elif flush:              hr = HR.FLUSH
    elif straight:           hr = HR.STRAIGHT
    elif t:                  hr = HR.THREE_OF_A_KIND
    elif p == 2:             hr = HR.TWO_PAIR
    elif p == 1:             hr = HR.ONE_PAIR
    else:                    hr = HR.HIGH_CARD
    return hr, tb


def best_hand(cards: List[Card]) -> Tuple[HR, List[int]]:
    return max((_eval5(list(c)) for c in combinations(cards, 5)),
               key=lambda x: (x[0].value, x[1]))


# ══════════════════════════════════════════════════════════════════════════════
# BLACKJACK
# ══════════════════════════════════════════════════════════════════════════════

class Blackjack:
    MIN_BET = 5; MAX_BET = 50; STEP = 5

    def __init__(self, credits: int):
        self.deck    = Deck()
        self.credits = credits
        self.bet     = 10
        self.phase   = 'bet'          # bet | player | dealer | result
        self.player: List[Card] = []
        self.dealer: List[Card] = []
        self.split:  List[Card] = []
        self.active  = 'main'         # main | split
        self.doubled = False
        self.msg     = ''
        self.net     = 0

    def _val(self, hand: List[Card]) -> int:
        v, aces = 0, 0
        for c in hand:
            if   c.rank == Rank.ACE:  v += 11; aces += 1
            elif c.rank >= Rank.JACK: v += 10
            else:                     v += c.rank.value
        while v > 21 and aces: v -= 10; aces -= 1
        return v

    def _active(self): return self.player if self.active == 'main' else self.split
    def _is_bj(self, h): return len(h) == 2 and self._val(h) == 21
    def can_split(self):  return (len(self.player) == 2 and not self.split
                                  and self.player[0].rank == self.player[1].rank
                                  and self.credits >= self.bet)
    def can_double(self): return len(self._active()) == 2 and self.credits >= self.bet

    def deal(self):
        self.credits -= self.bet
        self.player  = [self.deck.deal(), self.deck.deal()]
        self.dealer  = [self.deck.deal(), self.deck.deal(face_up=False)]
        self.split   = []
        self.active  = 'main'
        self.doubled = False
        self.net     = 0
        self.phase   = 'player'
        if self._is_bj(self.player):
            self._dealer_play()

    def hit(self):
        self._active().append(self.deck.deal())
        if self._val(self._active()) > 21:
            if self.active == 'main' and self.split:
                self.active = 'split'
            else:
                self._dealer_play()
        elif self.doubled:
            self.stand()

    def stand(self):
        if self.active == 'main' and self.split:
            self.active = 'split'
        else:
            self._dealer_play()

    def double_down(self):
        if not self.can_double(): return
        self.credits -= self.bet
        self.bet     *= 2
        self.doubled  = True
        self.hit()

    def do_split(self):
        if not self.can_split(): return
        self.credits -= self.bet
        self.split    = [self.player.pop(), self.deck.deal()]
        self.player.append(self.deck.deal())

    def _dealer_play(self):
        self.dealer[1].face_up = True
        while self._val(self.dealer) < 17:
            self.dealer.append(self.deck.deal())
        self._settle()

    def _settle(self):
        dv, dbj = self._val(self.dealer), self._is_bj(self.dealer)
        self.net = 0
        bet_half = self.bet // (2 if self.split else 1)
        for hand, b in ([(self.player, bet_half), (self.split, bet_half)]
                        if self.split else [(self.player, self.bet)]):
            if not hand: continue
            pv, pbj = self._val(hand), self._is_bj(hand)
            if pv > 21: pass
            elif pbj and not dbj: self.net += int(b*1.5); self.credits += b + int(b*1.5)
            elif pbj and dbj:     self.credits += b
            elif dv > 21 or pv > dv: self.net += b; self.credits += b * 2
            elif pv == dv:        self.credits += b
        self.phase = 'result'
        if self._is_bj(self.player) and not dbj:
            self.msg = f'BLACKJACK!  +${int(self.bet*0.5)} bonus'
        elif self.net > 0:  self.msg = f'YOU WIN  +${self.net}'
        elif self.net == 0: self.msg = 'PUSH — TIE'
        else:               self.msg = f'DEALER WINS  lose ${self.bet}'

    def inc_bet(self): self.bet = min(self.bet + self.STEP, min(self.MAX_BET, self.credits))
    def dec_bet(self): self.bet = max(self.bet - self.STEP, self.MIN_BET)


# ══════════════════════════════════════════════════════════════════════════════
# 5-CARD DRAW POKER
# ══════════════════════════════════════════════════════════════════════════════

class Poker:
    MIN_BET = 5; MAX_BET = 50; STEP = 5

    def __init__(self, credits: int):
        self.deck    = Deck()
        self.credits = credits
        self.bet     = 10
        self.phase   = 'bet'          # bet | discard | result
        self.player: List[Card] = []
        self.ai:     List[Card] = []
        self.flags   = [False] * 5    # discard selection
        self.cursor  = 0
        self.pot     = 0
        self.msg     = ''
        self.net     = 0
        self.player_name = ''
        self.ai_name     = ''

    def deal(self):
        self.credits -= self.bet
        self.pot      = self.bet * 2
        self.player   = [self.deck.deal() for _ in range(5)]
        self.ai       = [self.deck.deal(face_up=False) for _ in range(5)]
        self.flags    = [False] * 5
        self.cursor   = 0
        self.phase    = 'discard'

    def toggle(self): self.flags[self.cursor] = not self.flags[self.cursor]
    def move(self, d): self.cursor = (self.cursor + d) % 5

    def draw(self):
        # Player replaces flagged cards
        for i in range(5):
            if self.flags[i]:
                self.player[i] = self.deck.deal()

        # AI strategy: keep pairs+, otherwise try to improve
        ai_hr, _ = _eval5(self.ai)
        rc: dict = {}
        for c in self.ai: rc[c.rank] = rc.get(c.rank, 0) + 1
        if ai_hr >= HR.ONE_PAIR:
            self.ai = [c if rc[c.rank] >= 2 else self.deck.deal() for c in self.ai]
        else:
            sc: dict = {}
            for c in self.ai: sc[c.suit] = sc.get(c.suit, 0) + 1
            dom_suit = max(sc, key=lambda s: sc[s])
            if sc[dom_suit] == 4:    # 4-flush draw
                self.ai = [c if c.suit == dom_suit else self.deck.deal() for c in self.ai]
            else:                    # redraw all but best card
                best_r = max(c.rank.value for c in self.ai)
                kept   = False
                new_ai = []
                for c in self.ai:
                    if c.rank.value == best_r and not kept:
                        new_ai.append(c); kept = True
                    else:
                        new_ai.append(self.deck.deal())
                self.ai = new_ai

        for c in self.ai: c.face_up = True
        p_hr, p_tb = _eval5(self.player)
        a_hr, a_tb = _eval5(self.ai)
        self.player_name = _HR_NAME[p_hr]
        self.ai_name     = _HR_NAME[a_hr]

        if p_hr > a_hr or (p_hr == a_hr and p_tb > a_tb):
            self.net = self.pot; self.credits += self.pot
            self.msg = f'YOU WIN  +${self.pot}'
        elif p_hr < a_hr or (p_hr == a_hr and p_tb < a_tb):
            self.net = -self.bet
            self.msg = f'CPU WINS  lose ${self.bet}'
        else:
            self.net = 0; self.credits += self.bet
            self.msg = 'TIE — ante returned'
        self.phase = 'result'

    def fold(self):
        self.net = -self.bet; self.msg = 'You folded'
        self.phase = 'result'

    def inc_bet(self): self.bet = min(self.bet + self.STEP, min(self.MAX_BET, self.credits))
    def dec_bet(self): self.bet = max(self.bet - self.STEP, self.MIN_BET)


# ══════════════════════════════════════════════════════════════════════════════
# TEXAS HOLD'EM
# ══════════════════════════════════════════════════════════════════════════════

class HoldEm:
    SB=5; BB=10

    def __init__(self, credits: int):
        self.deck         = Deck()
        self.player_stack = credits
        self.ai_stack     = credits
        self.phase        = 'menu'    # menu | player_act | ai_act | result
        self.street       = 0         # 0=preflop 1=flop 2=turn 3=river
        self.player_hole: List[Card] = []
        self.ai_hole:     List[Card] = []
        self.community:   List[Card] = []
        self.pot          = 0
        self.player_bet   = 0
        self.ai_bet       = 0
        self.to_call      = 0
        self.raise_amt    = self.BB
        self.player_folded = False
        self.ai_folded     = False
        self.msg           = ''
        self.net           = 0
        self.is_dealer     = True     # True = player is dealer/SB
        self.hand_num      = 0
        self.ai_hand_name  = ''

    def start_hand(self):
        self.deck         = Deck()
        self.player_hole  = [self.deck.deal(), self.deck.deal()]
        self.ai_hole      = [self.deck.deal(face_up=False), self.deck.deal(face_up=False)]
        self.community    = []
        self.pot          = 0
        self.player_bet   = 0
        self.ai_bet       = 0
        self.player_folded= False
        self.ai_folded    = False
        self.street       = 0
        self.raise_amt    = self.BB
        self.net          = 0
        self.msg          = ''
        self.ai_hand_name = ''
        self.hand_num    += 1

        sb, bb = self.SB, self.BB
        if self.is_dealer:
            self.player_stack -= sb; self.player_bet = sb
            self.ai_stack     -= bb; self.ai_bet     = bb
            self.to_call       = bb - sb
            self.phase         = 'player_act'
        else:
            self.ai_stack     -= sb; self.ai_bet     = sb
            self.player_stack -= bb; self.player_bet = bb
            self.to_call       = 0
            self.phase         = 'ai_act'
        self.pot = sb + bb
        self.is_dealer = not self.is_dealer
        if self.phase == 'ai_act':
            self._ai_act()

    def _ai_strength(self) -> int:
        cards = self.ai_hole + self.community
        if len(cards) < 5:                    # preflop
            rv = [c.rank.value for c in self.ai_hole]
            if max(rv) >= 11 or rv[0]==rv[1]: return 3
            if max(rv) >= 9:                  return 2
            return 1
        return best_hand(cards)[0].value       # 0-9

    def _ai_act(self):
        bluff    = random.randint(0, 9) == 0
        strength = self._ai_strength()
        if bluff: strength = 5
        call_need = min(self.to_call, self.ai_stack)

        if strength <= 1 and not bluff and call_need > 0:
            self.ai_folded    = True
            self.player_stack += self.pot
            self.net           = self.pot
            self.msg           = 'CPU folded — YOU WIN!'
            self.phase         = 'result'
        elif strength <= 2:
            self.ai_stack  -= call_need
            self.ai_bet    += call_need
            self.pot       += call_need
            self.to_call    = 0
            self.phase      = 'player_act'
        else:
            raise_sz = max(self.BB, self.pot // 2)
            raise_sz = min(raise_sz, self.ai_stack)
            total    = min(call_need + raise_sz, self.ai_stack)
            self.ai_stack  -= total
            self.ai_bet    += total
            self.pot       += total
            self.to_call    = max(0, total - call_need)
            self.phase      = 'player_act'

    def _advance(self):
        self.player_bet = 0; self.ai_bet = 0; self.to_call = 0
        self.street += 1
        if   self.street == 1: self.community  = [self.deck.deal() for _ in range(3)]
        elif self.street in (2,3): self.community.append(self.deck.deal())
        else:
            self._showdown(); return
        self.phase = 'player_act'

    def fold(self):
        self.player_folded = True
        self.ai_stack     += self.pot
        self.net           = -self.player_bet
        self.msg           = 'You folded'
        self.phase         = 'result'

    def check_call(self):
        call = min(self.to_call, self.player_stack)
        self.player_stack -= call; self.player_bet += call
        self.pot          += call; self.to_call     = 0
        matched = (self.player_bet == self.ai_bet) or call == 0
        if matched:
            if self.street >= 3: self._showdown()
            else:                self._advance()
        else:
            self.phase = 'ai_act'; self._ai_act()

    def raise_bet(self):
        total = min(self.to_call + self.raise_amt, self.player_stack)
        self.player_stack -= total; self.player_bet += total
        self.pot          += total; self.to_call     = self.raise_amt
        self.phase = 'ai_act'; self._ai_act()

    def adj_raise(self, d):
        self.raise_amt = max(self.BB, min(self.raise_amt + d * self.BB, self.player_stack))

    def _showdown(self):
        for c in self.ai_hole: c.face_up = True
        p_ev = best_hand(self.player_hole + self.community)
        a_ev = best_hand(self.ai_hole     + self.community)
        self.ai_hand_name = _HR_NAME[a_ev[0]]
        if p_ev[0] > a_ev[0] or (p_ev[0]==a_ev[0] and p_ev[1] > a_ev[1]):
            self.player_stack += self.pot; self.net = self.pot
            self.msg = f'YOU WIN +${self.pot}  ({_HR_NAME[p_ev[0]]})'
        elif p_ev[0] < a_ev[0] or (p_ev[0]==a_ev[0] and p_ev[1] < a_ev[1]):
            self.ai_stack += self.pot; self.net = -self.player_bet
            self.msg = f'CPU wins  ({_HR_NAME[a_ev[0]]})'
        else:
            self.player_stack += self.pot//2; self.ai_stack += self.pot//2; self.net = 0
            self.msg = 'TIE — split pot'
        self.phase = 'result'


# ══════════════════════════════════════════════════════════════════════════════
# TERMINAL UI
# ══════════════════════════════════════════════════════════════════════════════

STREET_NAMES = ['PREFLOP', 'FLOP', 'TURN', 'RIVER', 'SHOWDOWN']


def _card_str(c: Card) -> str:
    return f"[{c.label():>2}]"


def _draw_cards(win, y: int, x: int, cards: List[Card], highlight: int = -1,
                flags: Optional[List[bool]] = None):
    """Draw a row of cards starting at (y, x). Returns x after last card."""
    cx = x
    for i, c in enumerate(cards):
        label = _card_str(c)
        attr = curses.color_pair(c.color())
        if highlight == i:
            attr |= curses.A_REVERSE
        try:
            win.addstr(y, cx, label, attr)
            if flags and flags[i]:
                win.addstr(y + 1, cx + 1, 'XX', curses.color_pair(3))
        except curses.error:
            pass
        cx += len(label) + 1
    return cx


def _center(win, y: int, text: str, attr=0):
    _, w = win.getmaxyx()
    x = max(0, (w - len(text)) // 2)
    try: win.addstr(y, x, text, attr)
    except curses.error: pass


def _hline(win, y: int):
    _, w = win.getmaxyx()
    try: win.hline(y, 0, curses.ACS_HLINE, w)
    except curses.error: pass


def _safe(win, y, x, text, attr=0):
    try: win.addstr(y, x, text, attr)
    except curses.error: pass


# ── Blackjack renderer ────────────────────────────────────────────────────────

def render_blackjack(win, g: Blackjack):
    win.erase()
    h, w = win.getmaxyx()
    _safe(win, 0, 0, f" BLACKJACK  Credits:${g.credits}  Bet:${g.bet} ",
          curses.color_pair(4) | curses.A_BOLD)
    _hline(win, 1)

    if g.phase == 'bet':
        _center(win, 4,  'PLACE YOUR BET', curses.A_BOLD)
        _center(win, 6,  f'Bet: ${g.bet}')
        bar_w = max(1, (g.bet - g.MIN_BET) * 30 // max(1, g.MAX_BET - g.MIN_BET))
        _center(win, 8,  '[' + '=' * bar_w + ' ' * (30 - bar_w) + ']')
        _center(win, 10, f'Credits: ${g.credits}')
        _center(win, 13, 'UP/DOWN: adjust bet   Space/Enter: deal', curses.color_pair(3))
        win.refresh(); return

    # Dealer
    dv = g._val(g.dealer)
    dealer_str = f'DEALER ({dv}):' if g.dealer[1].face_up else 'DEALER (??):'
    _safe(win, 2, 1, dealer_str)
    _draw_cards(win, 3, 1, g.dealer)

    # Separator
    _hline(win, 7)

    # Player
    pv = g._val(g.player)
    _safe(win, 8, 1, f'YOU ({pv}):')
    _draw_cards(win, 9, 1, g.player)
    if g.split:
        sv = g._val(g.split)
        marker = '>> ' if g.active == 'split' else '   '
        _safe(win, 11, 1, f'{marker}SPLIT ({sv}):')
        _draw_cards(win, 12, 4, g.split)
    if g.active == 'main' and g.phase == 'player':
        _safe(win, 8, 0, '>', curses.color_pair(4) | curses.A_BOLD)

    # Result / hints
    if g.phase == 'result':
        _center(win, h-4, g.msg, curses.color_pair(4) | curses.A_BOLD)
        _center(win, h-2, 'Space: play again   Q: menu', curses.color_pair(3))
    elif g.phase == 'player':
        hints = ['Z/Enter:Hit', 'X:Stand']
        if g.can_double(): hints.append('D:Double')
        if g.can_split():  hints.append('S:Split')
        _center(win, h-2, '  '.join(hints), curses.color_pair(3))

    win.refresh()


# ── Poker renderer ────────────────────────────────────────────────────────────

def render_poker(win, g: Poker):
    win.erase()
    h, w = win.getmaxyx()
    _safe(win, 0, 0, f" 5-CARD DRAW  Credits:${g.credits}  Pot:${g.pot} ",
          curses.color_pair(4) | curses.A_BOLD)
    _hline(win, 1)

    if g.phase == 'bet':
        _center(win, 4,  'ANTE UP', curses.A_BOLD)
        _center(win, 6,  f'Ante: ${g.bet}')
        bar_w = max(1, (g.bet - g.MIN_BET) * 30 // max(1, g.MAX_BET - g.MIN_BET))
        _center(win, 8,  '[' + '=' * bar_w + ' ' * (30 - bar_w) + ']')
        _center(win, 10, f'Credits: ${g.credits}')
        _center(win, 13, 'UP/DOWN: adjust   Space/Enter: deal', curses.color_pair(3))
        win.refresh(); return

    _safe(win, 2, 1, 'CPU: ')
    _draw_cards(win, 3, 1, g.ai)
    if g.phase == 'result':
        _safe(win, 5, 1, g.ai_name, curses.color_pair(3))
    _hline(win, 7)
    _safe(win, 8, 1, 'YOU: ')
    _draw_cards(win, 9, 1, g.player,
                highlight=g.cursor if g.phase == 'discard' else -1,
                flags=g.flags if g.phase == 'discard' else None)

    if g.phase == 'discard':
        _center(win, 12, 'Mark cards to discard, then confirm', curses.color_pair(3))
        _center(win, h-3, 'LEFT/RIGHT: move   Z/Enter: mark   X: confirm draw   Tab: fold',
                curses.color_pair(3))
    elif g.phase == 'result':
        _safe(win, 11, 1, g.player_name, curses.color_pair(3))
        _center(win, h-4, g.msg, curses.color_pair(4) | curses.A_BOLD)
        _center(win, h-2, 'Space: play again   Q: menu', curses.color_pair(3))

    win.refresh()


# ── Hold'em renderer ──────────────────────────────────────────────────────────

def render_holdem(win, g: HoldEm):
    win.erase()
    h, w = win.getmaxyx()
    header = f" TEXAS HOLD'EM  You:${g.player_stack}  CPU:${g.ai_stack}  Pot:${g.pot} "
    _safe(win, 0, 0, header[:w-1], curses.color_pair(4) | curses.A_BOLD)
    _hline(win, 1)

    if g.phase == 'menu':
        _center(win, 5,  "TEXAS HOLD'EM", curses.A_BOLD)
        _center(win, 7,  f'You: ${g.player_stack}   CPU: ${g.ai_stack}')
        _center(win, 9,  'No-limit heads-up  Blinds: 5/10')
        _center(win, 12, 'Space: deal   Q: back to menu', curses.color_pair(3))
        win.refresh(); return

    # Street + community
    street_label = STREET_NAMES[min(g.street, 4)]
    _safe(win, 2, 1, f'{street_label}:')
    if g.community:
        _draw_cards(win, 2, 12, g.community)

    _hline(win, 5)

    # CPU hole cards
    _safe(win, 6, 1, 'CPU:  ')
    _draw_cards(win, 6, 7, g.ai_hole)
    if g.phase == 'result' and g.ai_hand_name:
        _safe(win, 7, 7, g.ai_hand_name, curses.color_pair(3))

    _hline(win, 9)

    # Player hole cards
    _safe(win, 10, 1, 'YOU:  ')
    _draw_cards(win, 10, 7, g.player_hole)

    # Action bar
    if g.phase == 'player_act':
        if g.to_call == 0:
            hint = f'Z:Check  Start:Raise(${g.raise_amt}) UP/DN:adj  X:Fold'
        else:
            hint = f'Z:Call(${g.to_call})  Start:Raise(${g.raise_amt}) UP/DN:adj  X:Fold'
        _center(win, h-3, hint, curses.color_pair(3))
    elif g.phase == 'ai_act':
        _center(win, h-3, 'CPU is thinking...', curses.color_pair(3))
    elif g.phase == 'result':
        _center(win, h-4, g.msg, curses.color_pair(4) | curses.A_BOLD)
        _center(win, h-2, 'Space: next hand   Q: menu', curses.color_pair(3))

    win.refresh()


# ── Main menu ─────────────────────────────────────────────────────────────────

def render_menu(win, sel: int, credits: int):
    win.erase()
    h, w = win.getmaxyx()
    _center(win, 1,  'P O C K E T   G A M B L E R', curses.color_pair(4) | curses.A_BOLD)
    _hline(win, 2)
    items = ['Blackjack', '5-Card Draw Poker', 'Texas Hold\'em']
    for i, name in enumerate(items):
        y   = 5 + i * 3
        attr = curses.color_pair(4) | curses.A_BOLD if i == sel else 0
        pfx  = '> ' if i == sel else '  '
        _center(win, y, f'{pfx}{name}{pfx[::-1]}', attr)
    _hline(win, h-4)
    _center(win, h-3, f'Credits: ${credits}', curses.A_BOLD)
    _center(win, h-2, 'UP/DOWN: select   Space/Enter: play   Q: quit', curses.color_pair(3))
    win.refresh()


# ══════════════════════════════════════════════════════════════════════════════
# MAIN GAME LOOP
# ══════════════════════════════════════════════════════════════════════════════

def _get_key(win) -> str:
    """Read key, return semantic action string."""
    try:
        k = win.getch()
    except KeyboardInterrupt:
        return 'quit'
    if k == -1: return ''
    if k in (curses.KEY_UP,    ord('w'), ord('W')): return 'up'
    if k in (curses.KEY_DOWN,  ord('s'), ord('S')): return 'down'
    if k in (curses.KEY_LEFT,  ord('a'), ord('A')): return 'left'
    if k in (curses.KEY_RIGHT, ord('d'), ord('D')): return 'right'
    if k in (ord('z'), ord('Z'), 10, 13):            return 'a'        # Z or Enter
    if k in (ord('x'), ord('X'), 127, curses.KEY_BACKSPACE): return 'b'  # X or BS
    if k in (ord(' '),):                             return 'start'
    if k in (ord('\t'), curses.KEY_BTAB):            return 'select'
    if k in (ord('q'), ord('Q')):                    return 'quit'
    if k in (ord('D'),): return 'double'
    if k in (ord('p'), ord('P')): return 'split'
    return ''


def _check_size(stdscr):
    h, w = stdscr.getmaxyx()
    if h < 22 or w < 62:
        stdscr.erase()
        try:
            stdscr.addstr(0, 0, f'Terminal too small ({w}x{h}). Need 62x22 minimum.')
        except curses.error: pass
        stdscr.refresh()
        return False
    return True


def main(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.timeout(50)

    # Colors
    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_WHITE,  -1)   # normal
    curses.init_pair(2, curses.COLOR_RED,    -1)   # hearts/diamonds
    curses.init_pair(3, curses.COLOR_CYAN,   -1)   # hints
    curses.init_pair(4, curses.COLOR_YELLOW, -1)   # highlights

    credits = 100
    sel     = 0
    mode    = 'menu'   # menu | bj | poker | holdem
    bj:     Optional[Blackjack] = None
    poker:  Optional[Poker]     = None
    holdem: Optional[HoldEm]    = None

    while True:
        if not _check_size(stdscr):
            import time; time.sleep(0.5); continue

        key = _get_key(stdscr)

        # ── MENU ──────────────────────────────────────────────────────────────
        if mode == 'menu':
            if key == 'up':    sel = (sel - 1) % 3
            if key == 'down':  sel = (sel + 1) % 3
            if key in ('a', 'start'):
                if sel == 0:
                    bj   = Blackjack(credits); mode = 'bj'
                elif sel == 1:
                    poker = Poker(credits);    mode = 'poker'
                else:
                    holdem = HoldEm(credits);  mode = 'holdem'
            if key == 'quit':
                break
            render_menu(stdscr, sel, credits)

        # ── BLACKJACK ─────────────────────────────────────────────────────────
        elif mode == 'bj':
            g = bj
            if g.phase == 'bet':
                if key == 'up':            g.inc_bet()
                if key == 'down':          g.dec_bet()
                if key in ('a','start'):   g.deal()
                if key == 'quit':          credits = g.credits; mode = 'menu'
            elif g.phase == 'player':
                if key == 'a':             g.hit()
                if key == 'b':             g.stand()
                if key == 'double':        g.double_down()
                if key == 'split':         g.do_split()
                if key == 'quit':          credits = g.credits; mode = 'menu'
            elif g.phase == 'result':
                if key in ('start',):
                    credits = g.credits
                    g.bet   = min(g.bet, g.credits)
                    g.phase = 'bet'
                if key == 'quit':          credits = g.credits; mode = 'menu'
            render_blackjack(stdscr, g)

        # ── POKER ─────────────────────────────────────────────────────────────
        elif mode == 'poker':
            g = poker
            if g.phase == 'bet':
                if key == 'up':            g.inc_bet()
                if key == 'down':          g.dec_bet()
                if key in ('a','start'):   g.deal()
                if key == 'quit':          credits = g.credits; mode = 'menu'
            elif g.phase == 'discard':
                if key == 'left':          g.move(-1)
                if key == 'right':         g.move(1)
                if key == 'a':             g.toggle()
                if key == 'b':             g.draw()
                if key in ('select','quit'): g.fold()
            elif g.phase == 'result':
                if key in ('start',):
                    credits = g.credits
                    g.bet   = min(g.bet, g.credits)
                    g.phase = 'bet'
                if key == 'quit':          credits = g.credits; mode = 'menu'
            render_poker(stdscr, g)

        # ── HOLD'EM ───────────────────────────────────────────────────────────
        elif mode == 'holdem':
            g = holdem
            if g.phase == 'menu':
                if key in ('start',):      g.start_hand()
                if key == 'quit':          credits = g.player_stack; mode = 'menu'
            elif g.phase == 'player_act':
                if key == 'up':            g.adj_raise(1)
                if key == 'down':          g.adj_raise(-1)
                if key == 'a':             g.check_call()
                if key == 'b':             g.fold()
                if key == 'start':         g.raise_bet()
                if key == 'quit':          credits = g.player_stack; mode = 'menu'
            elif g.phase == 'result':
                if key in ('start',):      g.start_hand()
                if key == 'quit':          credits = g.player_stack; mode = 'menu'
            render_holdem(stdscr, g)

    # Restore terminal
    curses.endwin()
    print(f'\nGame over. Final credits: ${credits}')


if __name__ == '__main__':
    try:
        curses.wrapper(main)
    except KeyboardInterrupt:
        pass
