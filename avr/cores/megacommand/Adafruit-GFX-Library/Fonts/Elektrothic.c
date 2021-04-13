#include <avr/pgmspace.h>
#include "../gfxfont.h"

const uint8_t ElektrothicBitmaps[] PROGMEM = {
    // 74
                0x7B, 0xFC, 0xF3, 0xCF, 0x3F, 0xDE, 0xFD, 0xB6, 0xDB, 0xFB,
    0xF0, 0xCF, 0x73, 0x0F, 0xFF, 0xFB, 0xF0, 0xCE, 0x38, 0x3F, 0xFE, 0xCF,
    0x3C, 0xFF, 0x7C, 0x30, 0xC3, 0xFF, 0xFC, 0x3E, 0xFC, 0x3F, 0xFF, 0x7F,
    0xFC, 0x3E, 0xFF, 0x3F, 0xDF, 0xFF, 0xF0, 0xC7, 0x39, 0xCE, 0x30, 0x7B,
    0xFC, 0xDE, 0xFF, 0x3F, 0xDE, 0x7B, 0xFC, 0xFF, 0x7C, 0x3F, 0xFE,
    // 131
    };

/* {offset, width, height, advance cursor, x offset, y offset} */
const GFXglyph ElektrothicGlyphs[] PROGMEM = {
    {74 - 74, 6, 8, 7, 0, -8} // '0'
    ,
    {80 - 74, 3, 8, 4, 0, -8} // '1'
    ,
    {83 - 74, 6, 8, 7, 0, -8} // '2'
    ,
    {89 - 74, 6, 8, 7, 0, -8} // '3'
    ,
    {95 - 74, 6, 8, 7, 0, -8} // '4'
    ,
    {101 - 74, 6, 8, 7, 0, -8} // '5'
    ,
    {107 - 74, 6, 8, 7, 0, -8} // '6'
    ,
    {113 - 74, 6, 8, 7, 0, -8} // '7'
    ,
    {119 - 74, 6, 8, 7, 0, -8} // '8'
    ,
    {125 - 74, 6, 8, 7, 0, -8} // '9'
    // next is 131
};

const GFXfont Elektrothic PROGMEM = {(uint8_t *)ElektrothicBitmaps,
                                     (GFXglyph *)ElektrothicGlyphs, '0', '9',
                                     15};
