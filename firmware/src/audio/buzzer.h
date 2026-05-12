#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#define BUZZER_GPIO       14
#define BUZZER_PWM_SLICE   7  // GPIO14 = PWM7A

void buzzer_init(void);
void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms);
void buzzer_stop(void);

void buzzer_play_deal(void);
void buzzer_play_win(void);
void buzzer_play_lose(void);
void buzzer_play_shuffle(void);
void buzzer_play_blackjack(void);
void buzzer_play_button_click(void);

#endif
