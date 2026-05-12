# Assembly Guide

Complete step-by-step guide for building the Pocket Gambler hardware.

**Estimated time:** 2–3 hours for a first build

---

## Tools Required

- Soldering iron (fine tip, 350°C)
- Solder (60/40 or lead-free, 0.6mm recommended)
- Wire strippers
- Small Phillips screwdriver (for M2 screws)
- Flush cutters
- Multimeter (for continuity checks)
- 3D-printed case parts (see [`../case/print-settings.md`](../case/print-settings.md))

---

## Parts Checklist

Before starting, verify all parts from [`../hardware/bom.csv`](../hardware/bom.csv):

- [ ] Raspberry Pi Pico
- [ ] SSD1306 0.96" I2C OLED module (4-pin)
- [ ] 8x 6x6x5mm tactile push buttons
- [ ] 1x SPDT slide switch (3-pin, 2.54mm pitch)
- [ ] 1x passive piezo buzzer
- [ ] 1x 2xAAA battery holder
- [ ] 1x 100kΩ resistor (R1)
- [ ] 1x 200kΩ resistor (R2)
- [ ] 2x 100nF ceramic capacitor (C1, C2)
- [ ] 7x9cm perfboard
- [ ] 4x M2x6 pan head screws
- [ ] Hookup wire (26 AWG recommended)

---

## Step 1: Print and prepare the case

1. Print all three case parts per `case/print-settings.md`
2. Test-fit the lid onto the body — it should seat flush with light pressure
3. Test-fit the battery door — it should slide smoothly
4. Chase button holes with a 6.5mm drill bit if any are tight
5. Test-fit your tactile buttons through the holes before soldering

---

## Step 2: Prepare the perfboard

Cut the 7x9cm perfboard to approximately **65mm x 90mm** if needed (or use as-is).

Mark the PCB corners to align with the case standoffs:
```
┌─────────────────────────────────────────────────┐
│ ●                                           ● │   ● = M2 standoff hole
│                                               │
│         [PICO centered here]                  │
│                                               │
│  [OLED]      [buttons across top]             │
│ ●                                           ● │
└─────────────────────────────────────────────────┘
```

Drill or punch 2.2mm holes at the four corners for M2 standoff screws.

---

## Step 3: Mount the Raspberry Pi Pico

1. If using Pico H (pre-soldered headers), insert into perfboard
2. If using bare Pico, solder 2x20 male headers first, then insert
3. Orient the Pico so the USB connector faces toward the right side wall
   (the side with the USB cutout in the case)
4. Solder all 40 header pins to the perfboard

---

## Step 4: Install tactile buttons

Place all 8 tactile buttons per the layout:

```
Front face of PCB, aligning with case button holes:

  [SW1:UP]                    [SW5:A]
[SW3:L] [SW4:R]          [SW8:SEL]
  [SW2:DN]                   [SW6:B]

          [SW7:START]
```

Solder all 16 legs (2 per switch). One leg of each switch will connect to GPIO;
the other to GND.

---

## Step 5: Install the OLED module

1. The SSD1306 module mounts above the Pico, toward the top of the board
2. Solder 4 wire jumpers from the module to the perfboard pads near Pico pin 6 (GPIO4),
   pin 7 (GPIO5), pin 36 (3V3), and any GND pin
3. Position the module so the display face aligns with the display window in the case
4. Secure with a small amount of hot glue or foam tape on the back of the module if needed

---

## Step 6: Install power switch and battery holder

1. Mount the SPDT slide switch at the left edge of the board (aligning with the case slot)
2. Solder: battery holder (+) red wire → one end pin of switch
3. Solder: switch center/output pin → Pico VSYS (pin 39) via wire
4. Solder: battery holder (-) black wire → Pico GND (pin 38)
5. Add C1 (100nF) from VSYS to GND as close to pin 39 as possible

---

## Step 7: Install buzzer

1. Mount the passive piezo buzzer on the PCB near pin 19 (GPIO14)
2. Solder (+) longer lead to GPIO14
3. Solder (-) shorter lead to GND

---

## Step 8: Wire battery voltage divider

1. Solder R1 (100kΩ) from the Pico VSYS net (post-switch) to GPIO26 (pin 31)
2. Solder R2 (200kΩ) from GPIO26 (pin 31) to GND
3. Add C2 (100nF) from the OLED VCC rail to GND

---

## Step 9: Wire all buttons to GPIOs

Following the pin table in `hardware/wiring-guide.md`:

| Button | GPIO | Pico Pin |
|--------|------|----------|
| UP | GPIO6 | Pin 9 |
| DOWN | GPIO7 | Pin 10 |
| LEFT | GPIO8 | Pin 11 |
| RIGHT | GPIO9 | Pin 12 |
| A | GPIO10 | Pin 14 |
| B | GPIO11 | Pin 15 |
| START | GPIO12 | Pin 16 |
| SELECT | GPIO13 | Pin 17 |

For each button:
- One leg → corresponding GPIO pin via wire
- Other leg → GND (use a common GND bus on the perfboard)

---

## Step 10: Continuity check

Before powering on, use a multimeter to verify:

- [ ] VSYS to GND: no short (should read open circuit with switch OFF)
- [ ] 3V3 to GND: no short
- [ ] Each button GPIO to GND: no connection (open) when button NOT pressed
- [ ] Each button GPIO to GND: connected when button IS pressed
- [ ] OLED SDA/SCL pulled to 3V3 through pull-ups

---

## Step 11: Flash firmware and test

1. Connect Pico to USB (batteries not installed yet)
2. Hold BOOTSEL, connect USB, release BOOTSEL
3. Flash `pocket_gambler.uf2`
4. OLED should show splash screen within 1 second
5. Test all 8 buttons in the menu
6. Disconnect USB

---

## Step 12: Install in case

1. Slide PCB into the body, aligning standoff holes
2. Thread 4x M2x6 screws through PCB corners into the standoffs
3. Route the OLED ribbon/wires through the display window opening
4. Press the OLED module face against the display window from inside
5. Verify display is visible through the window

---

## Step 13: Install batteries and final test

1. Thread the battery holder leads through the battery compartment area
2. Attach the rear lid (press to snap, or use M2x6 screws if using screw mount)
3. Slide battery door open and install 2x AAA alkaline batteries
4. Slide battery door closed
5. Flip power switch ON
6. Test all games through a complete play session

---

## Done!

Your Pocket Gambler is complete. For issues, see the Troubleshooting section in
[`getting-started.md`](getting-started.md).
