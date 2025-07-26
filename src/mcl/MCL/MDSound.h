/* Justin Mammarella jmamma@gmail.com 2018 */
#pragma once

#define KICK 1
#define SNARE 2
#define TOM 3
#define HATS 4
#define SOUND_ID 0xFFFA
#define SOUND_VERSION 2000

#include "MD.h"

class MDSoundData {
public:
  uint32_t id = SOUND_ID;
  uint32_t version = SOUND_VERSION;
  uint8_t machine_count = 0;
  uint8_t type;

  MDMachine machine1;

  MDMachine machine2;

  uint8_t sample_id1[8];
  uint8_t sample_id2[8];
};

class MDSound : public MDSoundData {
public:
  File file;

  bool fetch_sound(uint8_t track);
  bool write_sound();
  bool load_sound(uint8_t track);
  bool read_sound();

  bool read_data(void *data, uint32_t size, uint32_t position);
  bool write_data(void *data, uint32_t size, uint32_t position);

};

