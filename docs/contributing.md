# Contributing

Contributions of all kinds are welcome — firmware improvements, new game features,
hardware optimizations, case design variants, and documentation.

---

## Getting Started

1. Fork the repository on GitHub
2. Create a feature branch: `git checkout -b feature/my-improvement`
3. Make your changes
4. Test on real hardware before submitting
5. Open a Pull Request against `main`

---

## Code Style (Firmware)

- **Language:** C11
- **Naming:** `snake_case` for all identifiers
- **Indent:** 4 spaces (no tabs)
- **Line length:** 100 characters max
- **Comments:** Only when the WHY is non-obvious. No docstrings.
- **Headers:** Include guards using `#ifndef MODULE_H` pattern

Example:
```c
// Use-case: why this magic constant exists
#define BATT_DEBOUNCE_READS 4  // average 4 samples to filter ADC noise

void power_init(void) {
    set_sys_clock_khz(48000, true);  // 48MHz saves ~10mA vs 125MHz default
    adc_init();
    adc_gpio_init(BATTERY_ADC_GPIO);
}
```

---

## Adding a New Game

1. Create `firmware/src/games/mygame.h` and `mygame.c`
2. Follow the pattern of existing games: init, start_hand/round, action functions, render
3. Add `src/games/mygame.c` to `firmware/CMakeLists.txt`
4. Add an entry to the `AppState` enum in `main.c`
5. Add menu entry and handler in `main.c`

### Constraints
- Game state struct must fit in available RAM (~200KB SRAM total on RP2040)
- All game state should live in a single struct (no dynamic allocation)
- Render function should call `ssd1306_clear()` + draw + `ssd1306_display()` only
- Sounds should use existing `buzzer_*` functions

---

## Hardware Changes

- Hardware design files use CERN-OHL-S v2. By submitting hardware changes you agree
  to license them under the same terms.
- If you modify the schematic, update `hardware/schematic/schematic.txt` and `hardware/wiring-guide.md`
- If you change the pin mapping, update both firmware (`buttons.h`, `ssd1306.h`, etc.)
  AND the hardware docs

---

## Case Design Changes

- Case files are parametric OpenSCAD. When changing dimensions, update the parameter
  block at the top of the file — do not hardcode values into modules.
- If you change `body_w`, `body_h`, or `body_d`, update all three `.scad` files and
  `print-settings.md` to stay in sync.
- Include a brief note in your PR about what printer/tolerance you tested with.

---

## Testing Checklist

Before opening a PR, please test:

- [ ] All three games play through without crashing
- [ ] Battery indicator updates correctly
- [ ] Sleep (30s idle) and wake work correctly
- [ ] All 8 buttons respond
- [ ] Sound plays on win/lose/deal events
- [ ] Menu navigation works
- [ ] Firmware builds cleanly with no warnings (`make -j4`)

---

## Issue Reporting

Please include:
- Hardware revision (Pico vs Pico H, OLED module model if known)
- Firmware commit hash
- Steps to reproduce
- Observed vs expected behavior

---

## License

By contributing to this repository you agree that your contributions will be
licensed under the same license as the component you are contributing to:

- Firmware contributions: **MIT License**
- Hardware contributions: **CERN-OHL-S v2**
- Documentation contributions: **CC-BY-4.0**
