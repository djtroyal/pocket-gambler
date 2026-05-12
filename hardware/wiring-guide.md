# Pocket Gambler — Wiring Guide

## Overview

All connections are made on a 7×9cm perfboard (or custom PCB). The Raspberry Pi Pico sits
centrally; the OLED and buttons connect directly via wire jumpers.

---

## 1. Power Rail

```
2× AAA Batteries (3V nominal)
  [+] ──── SPDT Slide Switch ──── Pico VSYS (Pin 39)
  [-] ──────────────────────────── Pico GND  (Pin 38 or any GND pin)
```

- **VSYS** accepts 1.8–5.5V; the onboard SMPS regulates it to 3.3V for the RP2040
- Fresh alkaline AAA = 1.5V each = **3.0V** total
- Depleted = ~1.2V each = **2.4V** total (RP2040 still operates correctly)
- Add a **100nF ceramic capacitor** from VSYS (pin 39) to GND as close to the pin as possible

> **Why VSYS and not VBUS?** VBUS (pin 40) expects 5V USB input. Feeding 3V here would
> underpower the Pico's internal 5V rail. VSYS is the correct input for battery power.

---

## 2. Display (SSD1306 I2C OLED)

| OLED Pin | Pico Pin | GPIO |
|----------|----------|------|
| VCC      | Pin 36   | 3V3(OUT) |
| GND      | Pin 38   | GND |
| SDA      | Pin 6    | GPIO4 |
| SCL      | Pin 7    | GPIO5 |

- I2C address: **0x3C** (most common); if your module has an address select jumper, leave it as-is
- Pull-up resistors: most SSD1306 breakout modules include 4.7kΩ pull-ups. If yours does not,
  add 4.7kΩ from SDA to 3V3 and 4.7kΩ from SCL to 3V3
- Firmware enables software pull-ups as a fallback

---

## 3. Buttons (8× tactile switches)

All buttons are **active-low**: one pin to GPIO, other pin to GND.  
Internal pull-ups are enabled in firmware — no external resistors needed.

| Button   | Function          | Pico GPIO | Pico Pin |
|----------|-------------------|-----------|----------|
| SW1      | D-Pad UP          | GPIO6     | Pin 9    |
| SW2      | D-Pad DOWN        | GPIO7     | Pin 10   |
| SW3      | D-Pad LEFT        | GPIO8     | Pin 11   |
| SW4      | D-Pad RIGHT       | GPIO9     | Pin 12   |
| SW5      | A (Hit/Call/Chk)  | GPIO10    | Pin 14   |
| SW6      | B (Stand/Fold)    | GPIO11    | Pin 15   |
| SW7      | START             | GPIO12    | Pin 16   |
| SW8      | SELECT            | GPIO13    | Pin 17   |

### Button layout (front face, approximate):
```
         ┌──────────────────────────────┐
         │  [OLED DISPLAY WINDOW]       │
         │                              │
         │  [UP]                [A]     │
         │ [LEFT][RIGHT]    [SELECT]    │
         │  [DOWN]              [B]     │
         │       [START]                │
         └──────────────────────────────┘
```

---

## 4. Piezo Buzzer (optional)

| Buzzer Pin | Connection |
|------------|------------|
| + (longer) | GPIO14 (Pico Pin 19) |
| - (shorter)| GND |

- Uses **PWM** output; firmware drives it with the RP2040 PWM slice 7A
- Only passive buzzers work. Active/self-oscillating buzzers will not respond to PWM frequency

---

## 5. Battery Voltage Monitor

A simple resistor divider lets the firmware measure battery level via ADC:

```
VSYS ──[R1: 100kΩ]──┬── GPIO26 (ADC0, Pico Pin 31)
                    │
                  [R2: 200kΩ]
                    │
                   GND
```

**Math:**
- V_adc = V_bat × 200k / (100k + 200k) = V_bat × 2/3
- At 3.0V battery: V_adc = 2.0V → ADC reading ≈ 2482 (of 4095)
- At 2.4V battery: V_adc = 1.6V → ADC reading ≈ 1985 (of 4095)
- Firmware maps 1985–2482 → 0–100% battery level

> **Note:** These resistor values are non-critical. Any pair with a 2:1 ratio works
> (e.g., 47kΩ + 100kΩ). The ADC input must stay below 3.3V.

---

## 6. Power Switch

The SPDT slide switch breaks the positive battery rail:

```
Battery [+] ─── [SW9 center pin]
[SW9 output pin] ─── Pico VSYS
[SW9 unused pin] ─── floating (or GND for safety)
```

Use a 3-pin SPDT on 2.54mm pitch. Mount flush with case edge.

---

## 7. Complete Connection Summary

| Pico Pin | GPIO | Connected To |
|----------|------|--------------|
| 6        | GPIO4| OLED SDA |
| 7        | GPIO5| OLED SCL |
| 9        | GPIO6| SW1 (UP) |
| 10       | GPIO7| SW2 (DOWN) |
| 11       | GPIO8| SW3 (LEFT) |
| 12       | GPIO9| SW4 (RIGHT) |
| 14       | GPIO10| SW5 (A) |
| 15       | GPIO11| SW6 (B) |
| 16       | GPIO12| SW7 (START) |
| 17       | GPIO13| SW8 (SELECT) |
| 19       | GPIO14| Buzzer + |
| 31       | GPIO26| Battery divider mid-point |
| 36       | 3V3 OUT | OLED VCC |
| 38       | GND | Common ground |
| 39       | VSYS | Battery + (through power switch) |

---

## 8. Decoupling Capacitors

Place as close to their respective power pins as possible:

- **C1 (100nF):** VSYS (pin 39) to GND — suppresses battery supply noise
- **C2 (100nF):** OLED VCC to GND — suppresses display switching noise

---

## 9. Programming / Flashing

The Pico's USB port (micro-USB) is used for firmware flashing. Mount the USB connector
accessible through the case side cutout. To flash:

1. Hold the **BOOTSEL** button on the Pico
2. Connect USB to a computer while holding BOOTSEL
3. Release BOOTSEL — a drive named `RPI-RP2` appears
4. Drag `pocket_gambler.uf2` onto the drive
5. The Pico reboots automatically and runs the firmware

No USB connection is needed during normal play; the device runs on batteries.
