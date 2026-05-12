#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include <stdbool.h>

#define BATTERY_ADC_GPIO     26
#define BATTERY_ADC_CHANNEL   0

// Voltage divider: 100kΩ high-side, 200kΩ low-side
// V_adc = V_bat × 200/(100+200) = V_bat × 2/3
// V_bat = V_adc × 3/2
// ADC ref = 3.3V, 12-bit (4096 counts)
// V_bat_mv = adc_raw × 3300 × 3 / (4096 × 2) = adc_raw × 9900 / 8192
#define ADC_TO_VBAT_MV(adc)  ((uint32_t)(adc) * 9900 / 8192)

#define BATT_MV_FULL   3000
#define BATT_MV_LOW    2400
#define BATT_MV_DEAD   2200

#define SLEEP_TIMEOUT_MS  30000

void     power_init(void);
uint16_t power_battery_mv(void);
uint8_t  power_battery_percent(void);
bool     power_is_low(void);
bool     power_is_critical(void);
void     power_enter_dormant(void);

#endif
