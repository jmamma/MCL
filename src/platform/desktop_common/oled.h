// oled.h — desktop placeholder. The real version will instantiate the same
// Adafruit_SSD1305 / DaDa_SSD1309 class that rp2040/oled.h does, against
// the no-op SPIClass in this directory. Step 1 keeps it as a minimal class
// with a getBuffer() that returns a fixed-size byte array so plugin code
// can compile against the symbol.
#pragma once

#include "Arduino.h"

// Matches the rp2040 oled.h definition. MCL uses #ifdef OLED_DISPLAY in places
// like TextInputPage.cpp (label `shift_release` is only emitted when OLED is on).
#define OLED_DISPLAY

#define BLACK  0
#define WHITE  1
#define INVERT 2

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

class Oled {
public:
    void init_display() {}
    void display()      {}
    uint8_t* getBuffer() { return buffer_; }

    // A grab-bag of methods MCL UI code calls. All no-ops on desktop for
    // step 1; the real Adafruit_GFX-derived implementation goes in later.
    void clearDisplay()                                 {}
    void drawPixel(int16_t /*x*/, int16_t /*y*/, uint16_t /*c*/) {}
    void fillRect (int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    void fillScreen(uint16_t /*c*/)                     {}
    void setTextSize(uint8_t /*s*/)                     {}
    void setTextColor(uint16_t /*c*/)                   {}
    void setTextColor(uint16_t /*fg*/, uint16_t /*bg*/) {}
    void setTextWrap(bool /*w*/)                        {}
    void setCursor(int16_t /*x*/, int16_t /*y*/)        {}
    int16_t getCursorX() const { return 0; }
    int16_t getCursorY() const { return 0; }
    void print(const char* /*s*/)                       {}
    // Template print covers print(int, HEX), print(uint16_t, HEX), etc.
    template <class T> void print(T /*v*/)              {}
    template <class T> void print(T /*v*/, int /*base*/) {}
    void println(const char* /*s*/)                     {}
    void println()                                      {}
    template <class T> void println(T /*v*/)            {}
    template <class T> void println(T /*v*/, int /*base*/) {}

    // Textbox helpers — match the rp2040 Oled surface so MCL UI code that
    // calls `oled_display.textbox_P(...)` compiles. No-op on desktop.
    uint16_t textbox_clock = 0;
    char     textbox_str [17] = {0};
    char     textbox_str2[17] = {0};
    bool     textbox_enabled = false;
    void init_textbox() {}
    void textbox  (const char* /*t*/, const char* /*t2*/) {}
    void textbox_P(const char* /*t_P*/, const char* /*t2_P*/) {}
    void textbox_P(const char* /*t_P*/) {}

    // Bitmap drawing — overloads MCL uses with optional bg + flip flags.
    void drawBitmap(int16_t, int16_t, const uint8_t[], int16_t, int16_t, uint16_t,
                    bool /*fv*/ = false, bool /*fh*/ = false) {}
    void drawBitmap(int16_t, int16_t, const uint8_t[], int16_t, int16_t, uint16_t,
                    uint16_t /*bg*/, bool /*fv*/ = false, bool /*fh*/ = false) {}
    void drawBitmap(int16_t, int16_t, uint8_t*, int16_t, int16_t, uint16_t,
                    bool /*fv*/ = false, bool /*fh*/ = false) {}
    void drawBitmap(int16_t, int16_t, uint8_t*, int16_t, int16_t, uint16_t,
                    uint16_t /*bg*/, bool /*fv*/ = false, bool /*fh*/ = false) {}
    void fillTriangle_3px(int16_t, int16_t, uint16_t) {}

    void draw_textbox(char*, char*) {}
    void draw_textbox(const char*, const char*) {}

    // Common Adafruit_GFX methods MCL UI code calls — no-op on desktop.
    void setFont(const void* /*font*/ = nullptr) {}
    void drawLine(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    void drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    void drawFastVLine(int16_t, int16_t, int16_t, uint16_t) {}
    void drawFastHLine(int16_t, int16_t, int16_t, uint16_t) {}
    void writeFastVLine(int16_t, int16_t, int16_t, uint16_t) {}
    void writeFastHLine(int16_t, int16_t, int16_t, uint16_t) {}

    // Power management — MCL toggles these on screensaver entry/exit.
    void sleep() {}
    void wake()  {}

    // getFont returns whatever was set; MCL stores it temporarily, restores
    // later. Return nullptr as a placeholder GFXfont*.
    const void* getFont() const { return nullptr; }

    // write(byte) — used by MCL for raw character output to Adafruit_GFX.
    size_t write(uint8_t /*b*/) { return 1; }

private:
    // 128 x 64 monochrome packed vertically (Adafruit_GFX SSD1305 layout):
    // width * height / 8 bytes.
    uint8_t buffer_[OLED_WIDTH * OLED_HEIGHT / 8] = {0};
};

extern Oled oled_display;
