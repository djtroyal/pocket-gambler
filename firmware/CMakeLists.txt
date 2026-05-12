cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pocket_gambler C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pocket_gambler
    src/main.c
    src/cards/cards.c
    src/games/blackjack.c
    src/games/poker.c
    src/games/holdem.c
    src/display/ssd1306.c
    src/display/ui.c
    src/input/buttons.c
    src/audio/buzzer.c
    src/power/power.c
)

target_include_directories(pocket_gambler PRIVATE src)

target_link_libraries(pocket_gambler
    pico_stdlib
    hardware_i2c
    hardware_pwm
    hardware_adc
    hardware_gpio
    hardware_timer
    hardware_sleep
    pico_unique_id
)

pico_add_extra_outputs(pocket_gambler)

# Disable stdio for production builds (saves ~3mA)
pico_enable_stdio_usb(pocket_gambler 0)
pico_enable_stdio_uart(pocket_gambler 0)
