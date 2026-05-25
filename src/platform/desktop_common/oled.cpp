// oled.cpp — software rasteriser for the desktop / wasm OLED shim.
//
// The framebuffer is row-major horizontal byte packing, MSB-first:
//
//   byte index = y * STRIDE + (x >> 3)
//   pixel bit  = 0x80 >> (x & 7)
//
// STRIDE = OLED_WIDTH / 8 = 16 bytes/row, total = 1024 bytes.
//
// Glyph rendering uses the classic Adafruit 5x7 font (glcdfont.c). Each
// glyph is 5 bytes × 8 rows: byte[col] holds one column of 8 pixels, bit
// 0 = top row, bit 7 = bottom row. We render at the cursor with optional
// integer scale.
//
// display() signals host_display_dirty() under PLATFORM_WASM so the host
// can flush its blit cache. On native desktop it's a no-op.

#include "oled.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"  // g_clock_ms, clock_diff, read_clock_ms

#if defined(PLATFORM_WASM)
#include "../wasm/host_imports.h"
#endif

Oled oled_display;

namespace {

// Adafruit classic 5x7 font, public-domain. Each glyph is 5 column bytes;
// bit 0 = top pixel, bit 7 = bottom pixel.
const uint8_t kFont5x7[256 * 5] = {
    0x00,0x00,0x00,0x00,0x00, 0x3E,0x5B,0x4F,0x5B,0x3E, 0x3E,0x6B,0x4F,0x6B,0x3E,
    0x1C,0x3E,0x7C,0x3E,0x1C, 0x18,0x3C,0x7E,0x3C,0x18, 0x1C,0x57,0x7D,0x57,0x1C,
    0x1C,0x5E,0x7F,0x5E,0x1C, 0x00,0x18,0x3C,0x18,0x00, 0xFF,0xE7,0xC3,0xE7,0xFF,
    0x00,0x18,0x24,0x18,0x00, 0xFF,0xE7,0xDB,0xE7,0xFF, 0x30,0x48,0x3A,0x06,0x0E,
    0x26,0x29,0x79,0x29,0x26, 0x40,0x7F,0x05,0x05,0x07, 0x40,0x7F,0x05,0x25,0x3F,
    0x5A,0x3C,0xE7,0x3C,0x5A, 0x7F,0x3E,0x1C,0x1C,0x08, 0x08,0x1C,0x1C,0x3E,0x7F,
    0x14,0x22,0x7F,0x22,0x14, 0x5F,0x5F,0x00,0x5F,0x5F, 0x06,0x09,0x7F,0x01,0x7F,
    0x00,0x66,0x89,0x95,0x6A, 0x60,0x60,0x60,0x60,0x60, 0x94,0xA2,0xFF,0xA2,0x94,
    0x08,0x04,0x7E,0x04,0x08, 0x10,0x20,0x7E,0x20,0x10, 0x08,0x08,0x2A,0x1C,0x08,
    0x08,0x1C,0x2A,0x08,0x08, 0x1E,0x10,0x10,0x10,0x10, 0x0C,0x1E,0x0C,0x1E,0x0C,
    0x30,0x38,0x3E,0x38,0x30, 0x06,0x0E,0x3E,0x0E,0x06, 0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
    0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x56,0x20,0x50,
    0x00,0x08,0x07,0x03,0x00, 0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00,
    0x2A,0x1C,0x7F,0x1C,0x2A, 0x08,0x08,0x3E,0x08,0x08, 0x00,0x80,0x70,0x30,0x00,
    0x08,0x08,0x08,0x08,0x08, 0x00,0x00,0x60,0x60,0x00, 0x20,0x10,0x08,0x04,0x02,
    0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x72,0x49,0x49,0x49,0x46,
    0x21,0x41,0x49,0x4D,0x33, 0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39,
    0x3C,0x4A,0x49,0x49,0x31, 0x41,0x21,0x11,0x09,0x07, 0x36,0x49,0x49,0x49,0x36,
    0x46,0x49,0x49,0x29,0x1E, 0x00,0x00,0x14,0x00,0x00, 0x00,0x40,0x34,0x00,0x00,
    0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08,
    0x02,0x01,0x59,0x09,0x06, 0x3E,0x41,0x5D,0x59,0x4E, 0x7C,0x12,0x11,0x12,0x7C,
    0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22, 0x7F,0x41,0x41,0x41,0x3E,
    0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x41,0x51,0x73,
    0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01,
    0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x1C,0x02,0x7F,
    0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E, 0x7F,0x09,0x09,0x09,0x06,
    0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x26,0x49,0x49,0x49,0x32,
    0x03,0x01,0x7F,0x01,0x03, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F,
    0x3F,0x40,0x38,0x40,0x3F, 0x63,0x14,0x08,0x14,0x63, 0x03,0x04,0x78,0x04,0x03,
    0x61,0x59,0x49,0x4D,0x43, 0x00,0x7F,0x41,0x41,0x41, 0x02,0x04,0x08,0x10,0x20,
    0x00,0x41,0x41,0x41,0x7F, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
    0x00,0x03,0x07,0x08,0x00, 0x20,0x54,0x54,0x78,0x40, 0x7F,0x28,0x44,0x44,0x38,
    0x38,0x44,0x44,0x44,0x28, 0x38,0x44,0x44,0x28,0x7F, 0x38,0x54,0x54,0x54,0x18,
    0x00,0x08,0x7E,0x09,0x02, 0x18,0xA4,0xA4,0x9C,0x78, 0x7F,0x08,0x04,0x04,0x78,
    0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x40,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
    0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x78,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78,
    0x38,0x44,0x44,0x44,0x38, 0xFC,0x18,0x24,0x24,0x18, 0x18,0x24,0x24,0x18,0xFC,
    0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x24, 0x04,0x04,0x3F,0x44,0x24,
    0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
    0x44,0x28,0x10,0x28,0x44, 0x4C,0x90,0x90,0x90,0x7C, 0x44,0x64,0x54,0x4C,0x44,
    0x00,0x08,0x36,0x41,0x00, 0x00,0x00,0x77,0x00,0x00, 0x00,0x41,0x36,0x08,0x00,
    0x02,0x01,0x02,0x04,0x02, 0x3C,0x26,0x23,0x26,0x3C, 0x1E,0xA1,0xA1,0x61,0x12,
    0x3A,0x40,0x40,0x20,0x7A, 0x38,0x54,0x54,0x55,0x59, 0x21,0x55,0x55,0x79,0x41,
    0x22,0x54,0x54,0x78,0x42, 0x21,0x55,0x54,0x78,0x40, 0x20,0x54,0x55,0x79,0x40,
    0x0C,0x1E,0x52,0x72,0x12, 0x39,0x55,0x55,0x55,0x59, 0x39,0x54,0x54,0x54,0x59,
    0x39,0x55,0x54,0x54,0x58, 0x00,0x00,0x45,0x7C,0x41, 0x00,0x02,0x45,0x7D,0x42,
    0x00,0x01,0x45,0x7C,0x40, 0x7D,0x12,0x11,0x12,0x7D, 0xF0,0x28,0x25,0x28,0xF0,
    0x7C,0x54,0x55,0x45,0x00, 0x20,0x54,0x54,0x7C,0x54, 0x7C,0x0A,0x09,0x7F,0x49,
    0x32,0x49,0x49,0x49,0x32, 0x3A,0x44,0x44,0x44,0x3A, 0x32,0x4A,0x48,0x48,0x30,
    0x3A,0x41,0x41,0x21,0x7A, 0x3A,0x42,0x40,0x20,0x78, 0x00,0x9D,0xA0,0xA0,0x7D,
    0x3D,0x42,0x42,0x42,0x3D, 0x3D,0x40,0x40,0x40,0x3D, 0x3C,0x24,0xFF,0x24,0x24,
    0x48,0x7E,0x49,0x43,0x66, 0x2B,0x2F,0xFC,0x2F,0x2B, 0xFF,0x09,0x29,0xF6,0x20,
    0xC0,0x88,0x7E,0x09,0x03, 0x20,0x54,0x54,0x79,0x41, 0x00,0x00,0x44,0x7D,0x41,
    0x30,0x48,0x48,0x4A,0x32, 0x38,0x40,0x40,0x22,0x7A, 0x00,0x7A,0x0A,0x0A,0x72,
    0x7D,0x0D,0x19,0x31,0x7D, 0x26,0x29,0x29,0x2F,0x28, 0x26,0x29,0x29,0x29,0x26,
    0x30,0x48,0x4D,0x40,0x20, 0x38,0x08,0x08,0x08,0x08, 0x08,0x08,0x08,0x08,0x38,
    0x2F,0x10,0xC8,0xAC,0xBA, 0x2F,0x10,0x28,0x34,0xFA, 0x00,0x00,0x7B,0x00,0x00,
    0x08,0x14,0x2A,0x14,0x22, 0x22,0x14,0x2A,0x14,0x08, 0x55,0x00,0x55,0x00,0x55,
    0xAA,0x55,0xAA,0x55,0xAA, 0xFF,0x55,0xFF,0x55,0xFF, 0x00,0x00,0x00,0xFF,0x00,
    0x10,0x10,0x10,0xFF,0x00, 0x14,0x14,0x14,0xFF,0x00, 0x10,0x10,0xFF,0x00,0xFF,
    0x10,0x10,0xF0,0x10,0xF0, 0x14,0x14,0x14,0xFC,0x00, 0x14,0x14,0xF7,0x00,0xFF,
    0x00,0x00,0xFF,0x00,0xFF, 0x14,0x14,0xF4,0x04,0xFC, 0x14,0x14,0x17,0x10,0x1F,
    0x10,0x10,0x1F,0x10,0x1F, 0x14,0x14,0x14,0x1F,0x00, 0x10,0x10,0x10,0xF0,0x00,
    0x00,0x00,0x00,0x1F,0x10, 0x10,0x10,0x10,0x1F,0x10, 0x10,0x10,0x10,0xF0,0x10,
    0x00,0x00,0x00,0xFF,0x10, 0x10,0x10,0x10,0x10,0x10, 0x10,0x10,0x10,0xFF,0x10,
    0x00,0x00,0x00,0xFF,0x14, 0x00,0x00,0xFF,0x00,0xFF, 0x00,0x00,0x1F,0x10,0x17,
    0x00,0x00,0xFC,0x04,0xF4, 0x14,0x14,0x17,0x10,0x17, 0x14,0x14,0xF4,0x04,0xF4,
    0x00,0x00,0xFF,0x00,0xF7, 0x14,0x14,0x14,0x14,0x14, 0x14,0x14,0xF7,0x00,0xF7,
    0x14,0x14,0x14,0x17,0x14, 0x10,0x10,0x1F,0x10,0x1F, 0x14,0x14,0x14,0xF4,0x14,
    0x10,0x10,0xF0,0x10,0xF0, 0x00,0x00,0x1F,0x10,0x1F, 0x00,0x00,0x00,0x1F,0x14,
    0x00,0x00,0x00,0xFC,0x14, 0x00,0x00,0xF0,0x10,0xF0, 0x10,0x10,0xFF,0x10,0xFF,
    0x14,0x14,0x14,0xFF,0x14, 0x10,0x10,0x10,0x1F,0x00, 0x00,0x00,0x00,0xF0,0x10,
    0xFF,0xFF,0xFF,0xFF,0xFF, 0xF0,0xF0,0xF0,0xF0,0xF0, 0xFF,0xFF,0xFF,0x00,0x00,
    0x00,0x00,0x00,0xFF,0xFF, 0x0F,0x0F,0x0F,0x0F,0x0F, 0x38,0x44,0x44,0x38,0x44,
    0xFC,0x4A,0x4A,0x4A,0x34, 0x7E,0x02,0x02,0x06,0x06, 0x02,0x7E,0x02,0x7E,0x02,
    0x63,0x55,0x49,0x41,0x63, 0x38,0x44,0x44,0x3C,0x04, 0x40,0x7E,0x20,0x1E,0x20,
    0x06,0x02,0x7E,0x02,0x02, 0x99,0xA5,0xE7,0xA5,0x99, 0x1C,0x2A,0x49,0x2A,0x1C,
    0x4C,0x72,0x01,0x72,0x4C, 0x30,0x4A,0x4D,0x4D,0x30, 0x30,0x48,0x78,0x48,0x30,
    0xBC,0x62,0x5A,0x46,0x3D, 0x3E,0x49,0x49,0x49,0x00, 0x7E,0x01,0x01,0x01,0x7E,
    0x2A,0x2A,0x2A,0x2A,0x2A, 0x44,0x44,0x5F,0x44,0x44, 0x40,0x51,0x4A,0x44,0x40,
    0x40,0x44,0x4A,0x51,0x40, 0x00,0x00,0xFF,0x01,0x03, 0xE0,0x80,0xFF,0x00,0x00,
    0x08,0x08,0x6B,0x6B,0x08, 0x36,0x12,0x36,0x24,0x36, 0x06,0x0F,0x09,0x0F,0x06,
    0x00,0x00,0x18,0x18,0x00, 0x00,0x00,0x10,0x10,0x00, 0x30,0x40,0xFF,0x01,0x01,
    0x00,0x1F,0x01,0x01,0x1E, 0x00,0x19,0x1D,0x17,0x12, 0x00,0x3C,0x3C,0x3C,0x3C,
    0x00,0x00,0x00,0x00,0x00
};

constexpr int STRIDE_BYTES = OLED_WIDTH / 8;

inline void set_pixel(uint8_t* buf, int x, int y, uint16_t color) {
    if ((unsigned)x >= OLED_WIDTH || (unsigned)y >= OLED_HEIGHT) return;
    uint8_t& b   = buf[y * STRIDE_BYTES + (x >> 3)];
    uint8_t mask = (uint8_t)(0x80 >> (x & 7));
    if (color == INVERT)     b ^= mask;
    else if (color == BLACK) b &= (uint8_t)~mask;
    else                     b |= mask;  // WHITE / anything else = pixel on
}

}  // namespace

// ── Lifecycle ────────────────────────────────────────────────────────────

void Oled::init_display() {
    clearDisplay();
}

void Oled::display() {
    // Mirrors rp2040/oled.cpp:419 — textbox overlay rendered on top of
    // whatever the GUI already drew this frame. After delay_time ms the
    // toast disables itself.
    if (textbox_enabled) {
        if (clock_diff(textbox_clock, g_clock_ms) < delay_time) {
            draw_textbox(textbox_str, textbox_str2);
        } else {
            textbox_enabled = false;
        }
    }
#if defined(PLATFORM_WASM)
    host_display_dirty();
#endif
}

// ── Pixel + fills ────────────────────────────────────────────────────────

void Oled::clearDisplay() {
    memset(buffer_, 0, sizeof(buffer_));
}

void Oled::fillScreen(uint16_t color) {
    if (color == BLACK) {
        memset(buffer_, 0x00, sizeof(buffer_));
    } else if (color == INVERT) {
        for (size_t i = 0; i < sizeof(buffer_); ++i) {
            buffer_[i] ^= 0xFF;
        }
    } else {
        memset(buffer_, 0xFF, sizeof(buffer_));
    }
}

void Oled::drawPixel(int16_t x, int16_t y, uint16_t color) {
    set_pixel(buffer_, x, y, color);
}

void Oled::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (y < 0 || y >= OLED_HEIGHT || w <= 0) return;
    if (x < 0)                { w += x; x = 0; }
    if (x + w > OLED_WIDTH)   { w = OLED_WIDTH - x; }
    if (w <= 0) return;
    for (int i = 0; i < w; ++i) set_pixel(buffer_, x + i, y, color);
}

void Oled::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (x < 0 || x >= OLED_WIDTH || h <= 0) return;
    if (y < 0)                 { h += y; y = 0; }
    if (y + h > OLED_HEIGHT)   { h = OLED_HEIGHT - y; }
    if (h <= 0) return;
    for (int i = 0; i < h; ++i) set_pixel(buffer_, x, y + i, color);
}

void Oled::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        set_pixel(buffer_, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Oled::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    drawFastHLine(x,         y,         w, color);
    drawFastHLine(x,         y + h - 1, w, color);
    drawFastVLine(x,         y,         h, color);
    drawFastVLine(x + w - 1, y,         h, color);
}

void Oled::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int j = 0; j < h; ++j) drawFastHLine(x, y + j, w, color);
}

void Oled::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0, y = r;
    set_pixel(buffer_, x0,     y0 + r, color);
    set_pixel(buffer_, x0,     y0 - r, color);
    set_pixel(buffer_, x0 + r, y0,     color);
    set_pixel(buffer_, x0 - r, y0,     color);
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        set_pixel(buffer_, x0 + x, y0 + y, color);
        set_pixel(buffer_, x0 - x, y0 + y, color);
        set_pixel(buffer_, x0 + x, y0 - y, color);
        set_pixel(buffer_, x0 - x, y0 - y, color);
        set_pixel(buffer_, x0 + y, y0 + x, color);
        set_pixel(buffer_, x0 - y, y0 + x, color);
        set_pixel(buffer_, x0 + y, y0 - x, color);
        set_pixel(buffer_, x0 - y, y0 - x, color);
    }
}

void Oled::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    drawFastVLine(x0, y0 - r, 2 * r + 1, color);
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0, y = r;
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
        drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

void Oled::fillTriangle(int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2, uint16_t color) {
    auto swap16 = [](int16_t& a, int16_t& b) { int16_t t = a; a = b; b = t; };

    // Sort by y ascending.
    if (y0 > y1) { swap16(y0, y1); swap16(x0, x1); }
    if (y1 > y2) { swap16(y1, y2); swap16(x1, x2); }
    if (y0 > y1) { swap16(y0, y1); swap16(x0, x1); }

    if (y0 == y2) {
        int16_t a = x0, b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int32_t dx01 = x1 - x0, dy01 = y1 - y0;
    int32_t dx02 = x2 - x0, dy02 = y2 - y0;
    int32_t dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    int16_t last = (y1 == y2) ? y1 : (int16_t)(y1 - 1);
    int16_t y;
    for (y = y0; y <= last; y++) {
        int32_t a = x0 + sa / dy01;
        int32_t b = x0 + sb / dy02;
        sa += dx01; sb += dx02;
        if (a > b) { int32_t t = a; a = b; b = t; }
        drawFastHLine((int16_t)a, y, (int16_t)(b - a + 1), color);
    }
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        int32_t a = x1 + sa / dy12;
        int32_t b = x0 + sb / dy02;
        sa += dx12; sb += dx02;
        if (a > b) { int32_t t = a; a = b; b = t; }
        drawFastHLine((int16_t)a, y, (int16_t)(b - a + 1), color);
    }
}

void Oled::fillTriangle_3px(int16_t x0, int16_t y0, uint16_t color) {
    // Hardware-compatible right-facing 3-column wedge used by play markers.
    drawFastVLine(x0, y0, 5, color);
    drawFastVLine(x0 + 1, y0 + 1, 3, color);
    drawPixel(x0 + 2, y0 + 2, color);
}

void Oled::setCursor(int16_t x, int16_t y) {
    cursor_x_ = x;
    cursor_y_ = y;
    debug_capture_segment_active_ = false;
}

// ── Bitmaps ──────────────────────────────────────────────────────────────

// Keep rp2040 Oled's historical parameter semantics: flip_vert mirrors X
// and flip_horiz mirrors Y.
void Oled::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                      int16_t w, int16_t h, uint16_t color,
                      bool flip_vert, bool flip_horiz) {
    if (w <= 0 || h <= 0) return;
    const int byteWidth = (w + 7) / 8;
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            uint8_t b = bitmap[j * byteWidth + (i >> 3)];
            if (b & (uint8_t)(0x80 >> (i & 7))) {
                int16_t dstX = flip_vert ? (int16_t)(x + w - i - 1) : (int16_t)(x + i);
                int16_t dstY = flip_horiz ? (int16_t)(y + h - j - 1) : (int16_t)(y + j);
                set_pixel(buffer_, dstX, dstY, color);
            }
        }
    }
}

void Oled::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                      int16_t w, int16_t h, uint16_t color, uint16_t bg,
                      bool flip_vert, bool flip_horiz) {
    if (w <= 0 || h <= 0) return;
    const int byteWidth = (w + 7) / 8;
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            uint8_t b = bitmap[j * byteWidth + (i >> 3)];
            uint16_t c = (b & (uint8_t)(0x80 >> (i & 7))) ? color : bg;
            int16_t dstX = flip_vert ? (int16_t)(x + w - i - 1) : (int16_t)(x + i);
            int16_t dstY = flip_horiz ? (int16_t)(y + h - j - 1) : (int16_t)(y + j);
            set_pixel(buffer_, dstX, dstY, c);
        }
    }
}

// ── Text ────────────────────────────────────────────────────────────────

void Oled::drawCharImpl(int16_t x, int16_t y, uint8_t c,
                        uint16_t fg, uint16_t bg, bool opaque_bg,
                        uint8_t size) {
    if (!gfx_font_) {
        // Classic built-in 5x7 font. Adafruit's layout: each glyph is 5
        // column bytes, bit 0 = top pixel. The 6th column is a 1-px spacer
        // emitted as `bg` (only when opaque), matching writeFastVLine in
        // Adafruit_GFX.cpp:1167.
        const uint8_t* glyph = &kFont5x7[(unsigned)c * 5];
        for (int col = 0; col < 5; ++col) {
            uint8_t line = glyph[col];
            for (int row = 0; row < 8; ++row, line >>= 1) {
                bool on = (line & 1) != 0;
                if (!on && !opaque_bg) continue;
                uint16_t pc = on ? fg : bg;
                if (size == 1) {
                    set_pixel(buffer_, x + col, y + row, pc);
                } else {
                    for (int dy = 0; dy < size; ++dy)
                        for (int dx = 0; dx < size; ++dx)
                            set_pixel(buffer_, x + col * size + dx,
                                               y + row * size + dy, pc);
                }
            }
        }
        if (opaque_bg) {
            // Inter-glyph spacer column painted in the bg colour.
            if (size == 1) {
                for (int row = 0; row < 8; ++row)
                    set_pixel(buffer_, x + 5, y + row, bg);
            } else {
                for (int dy = 0; dy < 8 * size; ++dy)
                    for (int dx = 0; dx < size; ++dx)
                        set_pixel(buffer_, x + 5 * size + dx, y + dy, bg);
            }
        }
        return;
    }

    // Custom GFXfont. Mirrors Adafruit_GFX.cpp:1175. (x, y) is the cursor
    // baseline; the glyph is drawn at (x + xOffset, y + yOffset). No
    // background paint — Adafruit_GFX explicitly documents the omission;
    // callers fillRect first when they want one.
    if (c < gfx_font_->first || c > gfx_font_->last) return;
    const GFXglyph* glyph  = &gfx_font_->glyph[c - gfx_font_->first];
    const uint8_t*  bitmap = gfx_font_->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width;
    uint8_t  h  = glyph->height;
    int8_t   xo = glyph->xOffset;
    int8_t   yo = glyph->yOffset;

    uint8_t bit = 0, bits = 0;
    for (uint8_t yy = 0; yy < h; ++yy) {
        for (uint8_t xx = 0; xx < w; ++xx) {
            if (!(bit++ & 7)) bits = bitmap[bo++];
            if (bits & 0x80) {
                if (size == 1) {
                    set_pixel(buffer_, x + xo + xx, y + yo + yy, fg);
                } else {
                    for (int dy = 0; dy < size; ++dy)
                        for (int dx = 0; dx < size; ++dx)
                            set_pixel(buffer_, x + (xo + xx) * size + dx,
                                               y + (yo + yy) * size + dy, fg);
                }
            }
            bits <<= 1;
        }
    }
    (void)opaque_bg; (void)bg;
}

size_t Oled::write(uint8_t c) {
    if (c == '\r') return 1;

    if (!gfx_font_) {
        if (c == '\n') {
            debugCaptureEndSegment();
            cursor_x_ = 0;
            cursor_y_ += 8 * text_size_;
            return 1;
        }
        if (text_wrap_ && cursor_x_ + 6 * text_size_ > OLED_WIDTH) {
            cursor_x_ = 0;
            cursor_y_ += 8 * text_size_;
            debug_capture_segment_active_ = false;
        }
        debugCaptureWrite(c);
        drawCharImpl(cursor_x_, cursor_y_, c, text_fg_, text_bg_,
                     !text_transparent_bg_, text_size_);
        cursor_x_ += 6 * text_size_;
        return 1;
    }

    // GFXfont path — variable advance per glyph.
    if (c == '\n') {
        debugCaptureEndSegment();
        cursor_x_ = 0;
        cursor_y_ += (int16_t)text_size_ * (int16_t)gfx_font_->yAdvance;
        return 1;
    }
    if (c < gfx_font_->first || c > gfx_font_->last) return 1;
    const GFXglyph* glyph = &gfx_font_->glyph[c - gfx_font_->first];
    if (glyph->width > 0 && glyph->height > 0) {
        int16_t xo = (int8_t)glyph->xOffset;
        if (text_wrap_ &&
            (cursor_x_ + (int16_t)text_size_ * (xo + glyph->width)) > OLED_WIDTH) {
            cursor_x_ = 0;
            cursor_y_ += (int16_t)text_size_ * (int16_t)gfx_font_->yAdvance;
            debug_capture_segment_active_ = false;
        }
        debugCaptureWrite(c);
        drawCharImpl(cursor_x_, cursor_y_, c, text_fg_, text_bg_,
                     !text_transparent_bg_, text_size_);
    }
    cursor_x_ += (int16_t)glyph->xAdvance * (int16_t)text_size_;
    return 1;
}

void Oled::debugCaptureTextBegin() {
    debug_capture_text_ = true;
    debug_capture_segment_active_ = false;
    debug_capture_text_len_ = 0;
    debug_capture_text_buf_[0] = '\0';
}

const char* Oled::debugCaptureTextEnd() {
    debug_capture_text_ = false;
    debug_capture_segment_active_ = false;
    debug_capture_text_buf_[debug_capture_text_len_] = '\0';
    return debug_capture_text_buf_;
}

void Oled::debugCaptureEndSegment() {
    debug_capture_segment_active_ = false;
}

void Oled::debugCaptureWrite(uint8_t c) {
    if (!debug_capture_text_)
        return;
    if (c < 0x20 || c >= 0x7f)
        return;

    if (!debug_capture_segment_active_) {
        if (debug_capture_text_len_ > 0)
            debugCaptureAppendChar('\n');
        char prefix[16];
        snprintf(prefix, sizeof(prefix), "%d,%d:", cursor_x_, cursor_y_);
        debugCaptureAppendString(prefix);
        debug_capture_segment_active_ = true;
    }
    debugCaptureAppendChar((char)c);
}

void Oled::debugCaptureAppendChar(char c) {
    if (debug_capture_text_len_ + 1 >= sizeof(debug_capture_text_buf_))
        return;
    debug_capture_text_buf_[debug_capture_text_len_++] = c;
    debug_capture_text_buf_[debug_capture_text_len_] = '\0';
}

void Oled::debugCaptureAppendString(const char* s) {
    if (!s)
        return;
    while (*s)
        debugCaptureAppendChar(*s++);
}

void Oled::print(const char* s) {
    if (!s) return;
    while (*s) write((uint8_t)*s++);
}

void Oled::print(int v, int base) {
    char buf[16]; int n = 0;
    int neg = 0;
    unsigned u;
    if (base == 10 && v < 0) { neg = 1; u = (unsigned)(-v); }
    else                     { u = (unsigned)v; }
    if (u == 0) buf[n++] = '0';
    while (u) {
        unsigned d = u % (unsigned)base;
        buf[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        u /= (unsigned)base;
    }
    if (neg) buf[n++] = '-';
    while (n--) write((uint8_t)buf[n]);
}

void Oled::print(unsigned v, int base) {
    char buf[16]; int n = 0;
    if (v == 0) buf[n++] = '0';
    while (v) {
        unsigned d = v % (unsigned)base;
        buf[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned)base;
    }
    while (n--) write((uint8_t)buf[n]);
}

void Oled::print(float v, int decimals) {
    if (v < 0) { write((uint8_t)'-'); v = -v; }
    int whole = (int)v;
    print((unsigned)whole);
    if (decimals > 0) {
        write((uint8_t)'.');
        float frac = v - (float)whole;
        for (int i = 0; i < decimals; ++i) {
            frac *= 10.0f;
            int digit = (int)frac;
            if (digit < 0) digit = 0;
            if (digit > 9) digit = 9;
            write((uint8_t)('0' + digit));
            frac -= (float)digit;
        }
    }
}

void Oled::println() {
    write((uint8_t)'\n');
}

// ── Textbox overlay ──────────────────────────────────────────────────────
//
// Mirrors rp2040/oled.cpp. Two short strings (max 16 chars each) shown in
// a centred boxed bar; auto-dismissed after delay_time ms. Host is
// PROGMEM-free so the _P variants are plain strncpy.

void Oled::init_textbox() {
    textbox_str[sizeof(textbox_str) - 1] = '\0';
    textbox_str2[sizeof(textbox_str2) - 1] = '\0';
    textbox_clock = read_clock_ms();
    textbox_enabled = true;
}

void Oled::textbox(const char* text, const char* text2) {
    strncpy(textbox_str,  text  ? text  : "", sizeof(textbox_str));
    strncpy(textbox_str2, text2 ? text2 : "", sizeof(textbox_str2));
    init_textbox();
}

void Oled::textbox_P(const char* text_P, const char* text2_P) {
    strncpy(textbox_str,  text_P  ? text_P  : "", sizeof(textbox_str));
    strncpy(textbox_str2, text2_P ? text2_P : "", sizeof(textbox_str2));
    init_textbox();
}

void Oled::textbox_P(const char* text_P) {
    strncpy(textbox_str, text_P ? text_P : "", sizeof(textbox_str));
    textbox_str2[0] = '\0';
    init_textbox();
}

void Oled::draw_textbox(const char* text1, const char* text2) {
    char str1[17];
    char str2[17];
    strncpy(str1, text1 ? text1 : "", sizeof(str1));
    strncpy(str2, text2 ? text2 : "", sizeof(str2));
    str1[sizeof(str1) - 1] = '\0';
    str2[sizeof(str2) - 1] = '\0';
    draw_textbox(str1, str2);
}

void Oled::draw_textbox(char* text, char* text2) {
    // Switch to the classic 5x7 font so the box geometry stays predictable
    // even when the active page selected a custom GFXfont.
    const GFXfont* oldfont = getFont();
    setFont(nullptr);

    const uint8_t font_width = 6;
    const uint8_t len1 = (uint8_t)strlen(text);
    const uint8_t len2 = (uint8_t)strlen(text2);
    uint8_t len_total = (uint8_t)(len1 + len2 + 2);
    const bool use_space = (len2 > 0);
    if (use_space) ++len_total;

    const uint8_t w = (uint8_t)(len_total * font_width);
    const uint8_t x = (uint8_t)(64 - w / 2);
    const uint8_t y = 8;

    fillRect(x - 1, y - 1, w + 2, 8 * 2 + 2, 0);
    drawRect(x, y, w, 8 * 2, 1);
    setCursor(x + font_width, y + 4);
    print(text);
    if (use_space) print(' ');
    print(text2);

    setFont(oldfont);
}
