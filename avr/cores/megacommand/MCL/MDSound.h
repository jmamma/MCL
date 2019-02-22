/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef MDSOUND_H__
#define MDSOUND_H__

#define KICK 1
#define SNARE 2
#define TOM 3
#define HATS 4

class MDSoundData {
public:
  char name[8];
  uint8_t machine_count = 0;
  uint8_t type;

  MDMachine machine1;
  MDLFO lfo1;

  MDMachine machine2;
  MDLFO lfo2;

  uint8_t sample_id1[8];
  uint8_t sample_id2[8];
};

class MDSound : MDSoundData {
public:
  File file;

  bool name_sound();
  bool fetch_sound(uint8_t track);
  bool write_sound();
  bool load_sound(uint8_t track);
  bool read_sound();

  bool read_data(void *data, uint32_t size, uint32_t position);
  bool write_data(void *data, uint32_t size, uint32_t position);

};

#endif /* MDSOUND_H__ */
