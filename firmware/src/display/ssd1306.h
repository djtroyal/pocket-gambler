#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>

#define SSD1306_I2C_ADDR   0x3C
#define SSD1306_WIDTH      128
#define SSD1306_HEIGHT     64
#define SSD1306_PAGES      8

#define SSD1306_I2C_PORT   i2c0
#define SSD1306_SDA_PIN    4
#define SSD1306_SCL_PIN    5
#define SSD1306_I2C_SPEED  400000

extern uint8_t g_framebuffer[SSD1306_PAGES][SSD1306_WIDTH];

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_display(void);
void ssd1306_display_on(void);
void ssd1306_display_off(void);
void ssd1306_set_contrast(uint8_t contrast);

void ssd1306_draw_pixel(int x, int y, bool on);
void ssd1306_draw_hline(int x, int y, int w);
void ssd1306_draw_vline(int x, int y, int h);
void ssd1306_draw_rect(int x, int y, int w, int h);
void ssd1306_fill_rect(int x, int y, int w, int h);
void ssd1306_invert_rect(int x, int y, int w, int h);
void ssd1306_draw_bitmap(int x, int y, const uint8_t *bmp, int w, int h);

void ssd1306_draw_char(int x, int y, char c);
void ssd1306_draw_string(int x, int y, const char *s);
void ssd1306_draw_string_centered(int y, const char *s);

int  ssd1306_string_width(const char *s);

#endif
