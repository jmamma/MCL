/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef TURBOLIGHT_H__
#define TURBOLIGHT_H__


class TurboLight {
public:
  uint8_t turbomidi_sysex_header[6] = {0xF0, 0x00, 0x20, 0x3c, 0x00, 0x00};
  uint32_t tmSpeeds[12] = {31250,  31250,  62500,  104062, 125000, 156250,
                           208125, 250000, 312500, 415625, 500000, 625000};
  void send_header(uint8_t cmd, MidiUartClass *uart);
  void set_speed(uint8_t speed, MidiUartClass *uart);
  uint8_t lookup_speed(uint8_t speed);
};

extern TurboLight turbo_light;

#endif /* TURBOLIGHT_H__ */
