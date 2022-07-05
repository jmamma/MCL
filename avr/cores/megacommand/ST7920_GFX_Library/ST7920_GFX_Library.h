#if ARDUINO >= 100
 #include "Arduino.h"
 #define WIRE_WRITE Wire.write
#else
 #include "WProgram.h"
  #define WIRE_WRITE Wire.send
#endif

#include <SPI.h>
#include <Adafruit_GFX.h>

#define ST7920_HEIGHT 	32		//32 pixels tall display
#define ST7920_WIDTH	128		//128 pixels wide display

#define BLACK 0
#define WHITE 1
#define INVERT 2

class ST7920 : public Adafruit_GFX {
 public:
    ST7920(int8_t CS);
    void begin(void);
  	void clearDisplay(void);
  	void display();
    void invertDisplay();
    void invertDisplay(uint8_t i) { invertDisplay(); };
    void textbox(const char *text, const char *text2, uint16_t delay = 800) {
            //TODO 
    }
  	void drawPixel(int16_t x, int16_t y, uint16_t color);

    uint8_t getBuffer(uint16_t i);
    uint8_t* getBuffer();

    bool textbox_enabled = false;
    bool screen_saver = false;
 private:
 	int8_t cs;
 	void ST7920Data(uint8_t data);
  	void ST7920Command(uint8_t data);

};


