#ifndef MIDIID_H__
#define MIDIID_H__

#include <inttypes.h>
//#include "MidiIDSysex.hh"

#define DEVICE_NULL 0xFF
#define DEVICE_MIDI 0xFE
#define DEVICE_MD 0x02
#define DEVICE_A4 0x06

class MidiID {
public:
  uint8_t manufacturer_id[3];
  uint8_t family_code[2] = {DEVICE_NULL, DEVICE_NULL};
  uint8_t family_member[2];
  char software_revision[4];
  MidiId() { init(); }
  void init();
  void send_id_request(uint8_t id, uint8_t port);
  bool getBlockingId(uint8_t id, uint8_t port, uint16_t timeout);
  uint8_t waitForId(uint16_t timeout);
  uint8_t get_id();
  void set_id(uint8_t id);
  char *get_name(char *str);
};

#endif /* MIDIID_H__ */
