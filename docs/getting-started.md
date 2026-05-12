# Getting Started

This guide walks you from a blank Raspberry Pi Pico to a fully working Pocket Gambler.

---

## Prerequisites

### Software

| Tool | Version | Notes |
|------|---------|-------|
| arm-none-eabi-gcc | 12+ | ARM cross-compiler |
| cmake | 3.13+ | Build system |
| pico-sdk | 1.5+ | Raspberry Pi Pico SDK |
| OpenSCAD | 2021+ | For case STL generation |
| Git | any | |

#### Installing pico-sdk

```bash
git clone https://github.com/raspberrypi/pico-sdk.git --recurse-submodules
export PICO_SDK_PATH=$(pwd)/pico-sdk
```

Add the export to your `~/.bashrc` or `~/.zshrc` to make it permanent.

#### Ubuntu / Debian

```bash
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

#### macOS (Homebrew)

```bash
brew install cmake
brew install --cask gcc-arm-embedded
```

#### Windows

Install the Raspberry Pi Pico Windows Installer from the official docs:
https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf (Appendix B)

---

## Building the Firmware

```bash
git clone https://github.com/djtroyal/pocket-gambler.git
cd pocket-gambler/firmware
mkdir build
cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
make -j4
```

After a successful build you will find `pocket_gambler.uf2` in the `build/` directory.

### Build output files

| File | Purpose |
|------|---------|
| `pocket_gambler.uf2` | Drag-and-drop firmware image |
| `pocket_gambler.elf` | Debuggable ELF binary |
| `pocket_gambler.bin` | Raw binary |
| `pocket_gambler.hex` | Intel HEX format |

---

## Flashing the Firmware

1. Hold the **BOOTSEL** button on your Raspberry Pi Pico
2. While holding BOOTSEL, connect the Pico to your computer via micro-USB
3. Release BOOTSEL — a USB mass storage drive named **`RPI-RP2`** will appear
4. Drag and drop `pocket_gambler.uf2` onto the `RPI-RP2` drive
5. The drive disappears and the Pico reboots automatically into the game

The game splash screen ("POCKET GAMBLER") should appear on the OLED within 1 second.

### Reflashing

The same procedure works for firmware updates. You do NOT need to erase first.

---

## Generating STL Files

```bash
cd pocket-gambler/case

# Generate STL files (requires OpenSCAD in PATH)
openscad -o pocket-gambler-body.stl pocket-gambler-body.scad
openscad -o pocket-gambler-lid.stl pocket-gambler-lid.scad
openscad -o pocket-gambler-battery-door.stl pocket-gambler-battery-door.scad
```

Or open each `.scad` file in the OpenSCAD GUI, press **F6** to render, then
**File → Export → Export as STL**.

See [`../case/print-settings.md`](../case/print-settings.md) for print settings.

---

## Hardware Assembly

See [`assembly-guide.md`](assembly-guide.md) for step-by-step assembly instructions.

---

## First Boot

After assembly and flashing:

1. Install 2× AAA batteries (alkaline or NiMH)
2. Flip the power switch to ON
3. The OLED splash screen appears — press any button to continue
4. The main menu shows three game options
5. Use **UP/DOWN** to select a game, press **A** or **START** to play

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| OLED blank | Wrong I2C address | Check SSD1306 module — some use 0x3D. Edit `SSD1306_I2C_ADDR` in `ssd1306.h` |
| OLED blank | No power to OLED | Verify VCC connected to Pico 3V3OUT (pin 36), not VBUS |
| Buttons unresponsive | Wrong GPIO | Re-check wiring against `hardware/wiring-guide.md` |
| No sound | Active buzzer used | Replace with passive piezo (firmware uses PWM) |
| Low battery immediately | Voltage divider wrong | Verify 100kΩ / 200kΩ ratio; R1 to VSYS, R2 to GND |
| Pico not recognized as RPI-RP2 | BOOTSEL not held | Hold BOOTSEL BEFORE plugging USB |
