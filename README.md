# Pocket Gambler

An open-source retro handheld card game console. Play Blackjack, 5-Card Draw Poker, and Texas Hold'em on a monochrome OLED display — powered by 2x AAA batteries for **133+ hours of play**.

Inspired by the battery-powered travel poker games of the 1980s and 90s. Build one for ~$8 in parts.

---

## Games

| Game | Description |
|------|-------------|
| **Blackjack** | Classic 21 vs dealer. Hit, stand, double-down, split. 3:2 on blackjack. |
| **5-Card Draw** | Select cards to discard, draw replacements, beat the CPU hand. |
| **Texas Hold'em** | 2-player no-limit heads-up against an AI opponent. Full betting rounds. |

---

## Hardware

| Component | Cost |
|-----------|------|
| Raspberry Pi Pico (RP2040) | ~$4.00 |
| SSD1306 0.96" OLED (128x64 I2C) | ~$1.80 |
| 8x tactile buttons (6x6mm) | ~$0.40 |
| Power switch + buzzer + misc | ~$0.50 |
| 2x AAA battery holder | ~$0.50 |
| Perfboard / custom PCB | ~$0.80 |
| **Total BOM** | **~$7.92** |

See [`hardware/bom.csv`](hardware/bom.csv) for full component list with supplier links.

### Battery Life

- **Active play:** ~133 hours (RP2040 @ 48MHz + OLED ~8.5mA from 1200mAh AAA)
- **Standby (DORMANT):** ~1500 hours (0.8mA dormant mode, wakes on any button press)
- Auto-sleep after 30 seconds of inactivity

---

## Quick Start

### 1. Build the firmware

```bash
git clone https://github.com/djtroyal/pocket-gambler.git
cd pocket-gambler/firmware
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
make -j4
```

### 2. Flash to Pico

1. Hold **BOOTSEL** on the Raspberry Pi Pico
2. Connect USB while holding BOOTSEL
3. Drag `pocket_gambler.uf2` onto the `RPI-RP2` drive
4. Pico reboots and starts the game

### 3. Print the case

```bash
openscad -o body.stl case/pocket-gambler-body.scad
openscad -o lid.stl case/pocket-gambler-lid.scad
openscad -o battery-door.stl case/pocket-gambler-battery-door.scad
```

Print at 0.15mm layer height in PETG. See [`case/print-settings.md`](case/print-settings.md).

### 4. Wire it up

See [`hardware/wiring-guide.md`](hardware/wiring-guide.md) and
[`hardware/schematic/schematic.txt`](hardware/schematic/schematic.txt).

---

## Controls

| Button | Blackjack | Poker | Hold'em |
|--------|-----------|-------|---------|
| **A** | Hit | Mark/unmark discard | Check / Call |
| **B** | Stand | Confirm draw | Fold |
| **UP/DOWN** | Adjust bet | -- | Adjust raise amount |
| **LEFT/RIGHT** | Double (UP) / Split (RIGHT) | Move cursor | -- |
| **START** | Deal / Menu | Deal hand | Raise / Menu |
| **SELECT** | -- | Fold | -- |

---

## Repository Structure

```
pocket-gambler/
├── firmware/              # RP2040 C firmware (pico-sdk)
│   ├── CMakeLists.txt
│   ├── pico_sdk_import.cmake
│   └── src/
│       ├── main.c         # Top-level state machine
│       ├── cards/         # Deck, shuffle (hardware RNG), deal
│       ├── games/         # blackjack.c, poker.c, holdem.c
│       ├── display/       # SSD1306 I2C driver + UI rendering
│       ├── input/         # Button polling + 5ms debounce
│       ├── audio/         # PWM buzzer tones
│       └── power/         # Battery ADC + DORMANT sleep
├── hardware/
│   ├── bom.csv            # Full bill of materials
│   ├── wiring-guide.md    # Step-by-step wiring instructions
│   └── schematic/
│       └── schematic.txt  # ASCII schematic / netlist
├── case/
│   ├── pocket-gambler-body.scad         # Main shell (75x110x18mm)
│   ├── pocket-gambler-lid.scad          # Rear panel
│   ├── pocket-gambler-battery-door.scad # Sliding battery door
│   └── print-settings.md
└── docs/
    ├── getting-started.md
    ├── assembly-guide.md
    └── contributing.md
```

---

## Prerequisites

- **arm-none-eabi-gcc** 12+ (or similar ARM cross-compiler)
- **cmake** 3.13+
- **pico-sdk** v1.5+ (github.com/raspberrypi/pico-sdk)
- **OpenSCAD** 2021+ (for case rendering)

---

## License

| Component | License |
|-----------|---------|
| Firmware (`firmware/`) | MIT |
| Hardware design files (`hardware/`) | CERN-OHL-S v2 |
| Case design files (`case/`) | CERN-OHL-S v2 |
| Documentation (`docs/`) | CC-BY-4.0 |

Contributions welcome — see [`docs/contributing.md`](docs/contributing.md).
