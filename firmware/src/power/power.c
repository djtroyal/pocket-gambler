#include "power.h"
#include "display/ssd1306.h"
#include "input/buttons.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"

void power_init(void) {
    // Lower system clock to 48 MHz to save ~10mA vs default 125 MHz
    set_sys_clock_khz(48000, true);

    // Init battery ADC
    adc_init();
    adc_gpio_init(BATTERY_ADC_GPIO);
    adc_select_input(BATTERY_ADC_CHANNEL);
}

uint16_t power_battery_mv(void) {
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += adc_read();
        sleep_us(100);
    }
    uint32_t avg = sum / 4;
    return (uint16_t)ADC_TO_VBAT_MV(avg);
}

uint8_t power_battery_percent(void) {
    uint16_t mv = power_battery_mv();
    if (mv >= BATT_MV_FULL) return 100;
    if (mv <= BATT_MV_DEAD) return 0;
    return (uint8_t)((uint32_t)(mv - BATT_MV_DEAD) * 100 / (BATT_MV_FULL - BATT_MV_DEAD));
}

bool power_is_low(void) {
    return power_battery_percent() < 10;
}

bool power_is_critical(void) {
    return power_battery_percent() < 5;
}

void power_enter_dormant(void) {
    ssd1306_display_off();

    // Configure wake-on-falling-edge for any button GPIO (6–13)
    for (int pin = BTN_GPIO_BASE; pin < BTN_GPIO_BASE + BTN_COUNT; pin++) {
        gpio_set_dormant_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
    }

    // Enter DORMANT — execution halts here until a button is pressed
    sleep_run_from_xosc();
    dormant_sleep();

    // After wake: re-initialize system clock (PLL stopped during dormant)
    set_sys_clock_khz(48000, true);

    // Clear dormant IRQs
    for (int pin = BTN_GPIO_BASE; pin < BTN_GPIO_BASE + BTN_COUNT; pin++) {
        gpio_set_dormant_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, false);
        gpio_acknowledge_irq(pin, GPIO_IRQ_EDGE_FALL);
    }

    // Re-initialize I2C for display (clock was stopped)
    ssd1306_init();
    ssd1306_display();
    buttons_clear_events();
}
