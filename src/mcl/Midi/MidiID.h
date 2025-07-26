#ifndef MIDIID_H__
#define MIDIID_H__

#include <inttypes.h>
//#include "MidiIDSysex.h"

#define DEVICE_MD 0x02
#define DEVICE_MNM 0x03
#define DEVICE_A4 0x06
#define DEVICE_NULL 0xFF
#define DEVICE_MIDI 0xFE

class MidiID {
public:
  uint8_t manufacturer_id[3];
  uint8_t family_code[2] = {DEVICE_NULL, DEVICE_NULL};
  uint8_t family_member[2];
  char software_revision[4];
  char name[16];
  MidiID() { init(); }
  void init();
  void send_id_request(uint8_t id, uint8_t port);
  bool getBlockingId(uint8_t id, uint8_t port, uint16_t timeout);
  uint8_t waitForId(uint8_t id, uint8_t port, uint16_t timeout);
  uint8_t get_id();
  void set_id(uint8_t id);
  char *get_name(char *str);
  void set_name(const char* str);
};

#endif /* MIDIID_H__ */
