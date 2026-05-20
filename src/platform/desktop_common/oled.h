// oled.h — desktop / wasm OLED shim.
//
// MCL's UI code targets the Adafruit_GFX surface on hardware. On the host
// build we don't have a real display, so this class is a software
// rasteriser that renders into a 128x64 1bpp framebuffer.
//
// Framebuffer layout (matches the wasm export contract):
//   * row-major horizontal byte packing, MSB = leftmost pixel of the byte
//   * stride = OLED_WIDTH / 8 bytes per row
//   * total size = stride * OLED_HEIGHT bytes
//
// The wasm host (mcl_framebuffer_offset / _stride / _width / _height)
// reads this directly. See MCL/src/platform/wasm/exports.cpp.
//
// PLATFORM_WASM: display() calls host_display_dirty() so the host can
// flush its blit cache. Native desktop is a no-op.

#pragma once

#include "Arduino.h"
#include <stdint.h>
#include <stddef.h>

#define OLED_DISPLAY

#define BLACK  0
#define WHITE  1
#define INVERT 2

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// Adafruit GFXfont/GFXglyph come from gfxfont.h (bundled in Adafruit_GFX
// under .pio/libdeps). MCL's Fonts/ headers — TomThumb, Elektrothic, etc.
// — emit static GFXfont structs at file scope and hand them to setFont().
#include "gfxfont.h"

class Oled {
public:
    Oled() = default;

    // Lifecycle ---------------------------------------------------------
    void init_display();
    void display();
    void sleep() {}
    void wake()  {}

    // Framebuffer access ------------------------------------------------
    uint8_t* getBuffer() { return buffer_; }
    static constexpr int width()  { return OLED_WIDTH;  }
    static constexpr int height() { return OLED_HEIGHT; }
    static constexpr int stride() { return OLED_WIDTH / 8; }

    // Pixel + fills -----------------------------------------------------
    void clearDisplay();
    void fillScreen(uint16_t color);
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    void drawFastHLine (int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine (int16_t x, int16_t y, int16_t h, uint16_t color);
    void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        drawFastHLine(x, y, w, color);
    }
    void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        drawFastVLine(x, y, h, color);
    }
    void drawLine (int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint16_t color);
    void fillTriangle_3px(int16_t x0, int16_t y0, uint16_t color);

    // Bitmaps -----------------------------------------------------------
    // All four overloads use Adafruit's 1bpp row-major, MSB-first packing:
    // the leftmost pixel of each row is the MSB of the first byte.
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                    int16_t w, int16_t h, uint16_t color,
                    bool flip_vert = false, bool flip_horiz = false);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                    int16_t w, int16_t h, uint16_t color, uint16_t bg,
                    bool flip_vert = false, bool flip_horiz = false);
    void drawBitmap(int16_t x, int16_t y, uint8_t* bitmap,
                    int16_t w, int16_t h, uint16_t color,
                    bool flip_vert = false, bool flip_horiz = false) {
        drawBitmap(x, y, (const uint8_t*)bitmap, w, h, color, flip_vert, flip_horiz);
    }
    void drawBitmap(int16_t x, int16_t y, uint8_t* bitmap,
                    int16_t w, int16_t h, uint16_t color, uint16_t bg,
                    bool flip_vert = false, bool flip_horiz = false) {
        drawBitmap(x, y, (const uint8_t*)bitmap, w, h, color, bg, flip_vert, flip_horiz);
    }

    // Text --------------------------------------------------------------
    void setCursor(int16_t x, int16_t y) { cursor_x_ = x; cursor_y_ = y; }
    int16_t getCursorX() const { return cursor_x_; }
    int16_t getCursorY() const { return cursor_y_; }
    void setTextSize(uint8_t s)                    { text_size_ = s ? s : 1; }
    void setTextColor(uint16_t c)                  { text_fg_ = c; text_bg_ = c; text_transparent_bg_ = true; }
    void setTextColor(uint16_t fg, uint16_t bg)    { text_fg_ = fg; text_bg_ = bg; text_transparent_bg_ = false; }
    void setTextWrap(bool w)                       { text_wrap_ = w; }
    // setFont(nullptr) reverts to the built-in classic 5x7. Any other
    // pointer is taken to be an Adafruit GFXfont — rendering switches to
    // the variable-width / variable-height glyph path that Adafruit_GFX
    // implements (see Adafruit_GFX.cpp:1175). Used by MCL's TomThumb /
    // Elektrothic font pages.
    void setFont(const GFXfont* f = nullptr) { gfx_font_ = f; }
    const GFXfont* getFont() const           { return gfx_font_; }

    size_t write(uint8_t c);
    void   print(const char* s);
    void   print(char c)        { write((uint8_t)c); }
    void   print(int v, int base = 10);
    void   print(unsigned v, int base = 10);
    void   print(long v, int base = 10)            { print((int)v, base); }
    void   print(unsigned long v, int base = 10)   { print((unsigned)v, base); }
    void   print(float v, int decimals = 2);
    void   println();
    void   println(const char* s)                  { print(s); println(); }
    void   println(char c)                         { print(c); println(); }
    void   println(int v, int base = 10)           { print(v, base); println(); }
    void   println(unsigned v, int base = 10)      { print(v, base); println(); }
    void   println(long v, int base = 10)          { print(v, base); println(); }
    void   println(unsigned long v, int base = 10) { print(v, base); println(); }
    void   println(float v, int decimals = 2)      { print(v, decimals); println(); }

    // Textbox — temporary toast overlay drawn on top of the framebuffer
    // from display() for ~800ms. Lifecycle mirrors rp2040/oled.cpp:386.
    static constexpr uint16_t delay_time = 800;
    uint16_t textbox_clock = 0;
    char     textbox_str [17] = {0};
    char     textbox_str2[17] = {0};
    bool     textbox_enabled = false;
    void init_textbox();
    void textbox  (const char* text, const char* text2);
    // PROGMEM on AVR; on host PROGMEM is empty so these are plain const char*.
    void textbox_P(const char* text_P, const char* text2_P);
    void textbox_P(const char* text_P);
    void draw_textbox(char* text, char* text2);
    void draw_textbox(const char* text1, const char* text2);

private:
    // Renders one glyph using whatever font is currently selected.
    void drawCharImpl(int16_t x, int16_t y, uint8_t c, uint16_t fg, uint16_t bg,
                      bool opaque_bg, uint8_t size);

    uint8_t  buffer_[OLED_WIDTH * OLED_HEIGHT / 8] = {0};

    int16_t  cursor_x_ = 0;
    int16_t  cursor_y_ = 0;
    uint16_t text_fg_  = WHITE;
    uint16_t text_bg_  = BLACK;
    bool     text_transparent_bg_ = true;
    uint8_t  text_size_ = 1;
    bool     text_wrap_ = true;
    const GFXfont* gfx_font_ = nullptr;  // nullptr = built-in classic 5x7
};

extern Oled oled_display;
