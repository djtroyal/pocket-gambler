#include "buzzer.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

void buzzer_init(void) {
    gpio_set_function(BUZZER_GPIO, GPIO_FUNC_PWM);
    pwm_set_enabled(BUZZER_PWM_SLICE, false);
}

void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0) {
        sleep_ms(duration_ms);
        return;
    }
    uint32_t wrap = 125000000 / freq_hz - 1;
    if (wrap > 65535) wrap = 65535;
    pwm_set_wrap(BUZZER_PWM_SLICE, (uint16_t)wrap);
    pwm_set_chan_level(BUZZER_PWM_SLICE, PWM_CHAN_A, (uint16_t)(wrap / 2));
    pwm_set_enabled(BUZZER_PWM_SLICE, true);
    sleep_ms(duration_ms);
    pwm_set_enabled(BUZZER_PWM_SLICE, false);
}

void buzzer_stop(void) {
    pwm_set_enabled(BUZZER_PWM_SLICE, false);
}

void buzzer_play_button_click(void) {
    buzzer_tone(2000, 3);
}

void buzzer_play_deal(void) {
    buzzer_tone(1800, 5);
    sleep_ms(15);
    buzzer_tone(2200, 5);
}

void buzzer_play_shuffle(void) {
    for (int i = 0; i < 6; i++) {
        buzzer_tone(1200 + i * 200, 8);
        sleep_ms(5);
    }
}

void buzzer_play_win(void) {
    // C4 - E4 - G4 ascending jingle
    buzzer_tone(262, 80);
    sleep_ms(20);
    buzzer_tone(330, 80);
    sleep_ms(20);
    buzzer_tone(392, 160);
}

void buzzer_play_lose(void) {
    // G4 - E4 - C4 descending
    buzzer_tone(392, 80);
    sleep_ms(20);
    buzzer_tone(330, 80);
    sleep_ms(20);
    buzzer_tone(196, 200);
}

void buzzer_play_blackjack(void) {
    // C4 - E4 - G4 - C5 fanfare arpeggio
    buzzer_tone(262, 70);
    sleep_ms(15);
    buzzer_tone(330, 70);
    sleep_ms(15);
    buzzer_tone(392, 70);
    sleep_ms(15);
    buzzer_tone(523, 200);
}
