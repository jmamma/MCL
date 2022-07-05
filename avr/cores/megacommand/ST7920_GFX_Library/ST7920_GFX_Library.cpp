#include <avr/pgmspace.h>
#include <stdlib.h>

#include "Adafruit_GFX.h"
#include "ST7920_GFX_Library.h"
#include <SPI.h>
#include "helpers.h"

#ifdef SPI_HAS_TRANSACTION
SPISettings oledspi = SPISettings(16000000, MSBFIRST, SPI_MODE3);
#else
#define ADAFRUIT_SSD1305_SPI SPI_CLOCK_DIV2
#endif


uint8_t buffer[1024];		//This array serves as primitive "Video RAM" bufferer


uint8_t ST7920::getBuffer(uint16_t i) { return buffer[i]; }
uint8_t* ST7920::getBuffer() { return buffer; }

//This display is split into two halfs. Pages are 16bit long and pages are arranged in that way that are lied horizontaly instead of verticaly, unlike SSD1306 OLED, Nokia 5110 LCD, etc.
//After 8 horizonral page is written, it jumps to half of the screen (Y = 32) and continues until 16 lines of page have been written. After that, we have set cursor in new line.
void ST7920::drawPixel(int16_t x, int16_t y, uint16_t color) {
	if(x<0 || x>=ST7920_WIDTH || y<0 || y>=ST7920_HEIGHT) return;
  	uint8_t y0 = 0, x0 = 0;								//Define and initilize varilables for skiping rows
  	uint16_t data, n;										//Define variable for sending data itno bufferer (basicly, that is one line of page)
  	x0 = x % 16;
    x /= 16;
    data = 0x8000 >> x0;
  	n = (x * 2) + (y0) + (32 * y);

     if (!color) {
    	buffer[n] &= (~data >> 8);
    	buffer[n + 1] &= (~data & 0xFF);
    }
    else if (color == INVERT) {
     	buffer[n] ^= (~data >> 8);
    	buffer[n + 1] ^= (~data & 0xFF);
    }
    else {
    	buffer[n] |= (data >> 8);
    	buffer[n + 1] |= (data & 0xFF);
  	}
}
ST7920::ST7920(int8_t CS) : cs(CS), Adafruit_GFX(ST7920_WIDTH, ST7920_HEIGHT) {

}

void ST7920::begin(void) {
	SPI.begin();
	pinMode(cs, OUTPUT);
  	digitalWrite(cs, HIGH);
	ST7920Command(B00001100); //display on, cursor off
//    ST7920Command(0x01); // display clear
    
    delayMicroseconds(38);
    digitalWrite(cs, LOW);

}

void ST7920::clearDisplay() {
  	long* p = (long*)&buffer;
  	for (int i = 0; i < 256; i++) {
    	p[i] = 0;
  	}
}

void ST7920::display() {
  	int x = 0, y = 0, n = 0;
  	digitalWrite(cs, HIGH);
  	ST7920Command(B00100100); //EXTENDED INSTRUCTION SET
  	ST7920Command(B00100110); //EXTENDED INSTRUCTION SET
  	for (y = 0; y < 32; y++) {
    	ST7920Command(0x80 | y);
    	ST7920Command(0x80 | x);
    	for (x = 0; x < 8; x++) {
      		ST7920Data(buffer[n++]);
      		ST7920Data(buffer[n++]);
    	}
        x+=8;
        n+=16;
  	}
  	digitalWrite(cs, LOW);
}

void ST7920::invertDisplay() {
  	long* p = (long*)&buffer;
  	for(int i = 0; i<256; i++) {
    	p[i] = ~p[i];
  	}
}

void ST7920::ST7920Data(uint8_t data) { //RS = 1 RW = 0
    SPI.beginTransaction(oledspi);
  	SPI.transfer(B11111010);
  	SPI.transfer((data & B11110000));
 	SPI.transfer((data & B00001111) << 4);
  	SPI.endTransaction();
  	delayMicroseconds(40);
}

void ST7920::ST7920Command(uint8_t data) { //RS = 0 RW = 0
  	SPI.beginTransaction(oledspi);
  	SPI.transfer(B11111000);
  	SPI.transfer((data & B11110000));
  	SPI.transfer((data & B00001111) << 4);
  	SPI.endTransaction();
  	delayMicroseconds(40);
}
