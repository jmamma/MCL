#ifndef MNM_ENCODERS_H__
#define MNM_ENCODERS_H__

#include <GUI.h>
#include <MNM.h>

#ifndef HOST_MIDIDUINO
class MNMEncoder : public CCEncoder {
 public:
  uint8_t track;
  uint8_t param;

  virtual uint8_t getCC();
  virtual uint8_t getChannel();
  virtual void initCCEncoder(uint8_t _channel, uint8_t _cc);
  void initMNMEncoder(uint8_t _track = 0, uint8_t _param = 0, char *_name = NULL, uint8_t init = 0);

  MNMEncoder(uint8_t _track = 0, uint8_t _param = 0, char *_name = NULL, uint8_t init = 0);
  void loadFromKit();
};

class MNMTrackFlashEncoder : public RangeEncoder {
 public:
 MNMTrackFlashEncoder(char *_name = NULL, uint8_t init = 0) : RangeEncoder(0, 5, _name, init) {
  }

  virtual void displayAt(int i);
};

class MNMTrackAllFlashEncoder : public RangeEncoder {
 public:
 MNMTrackAllFlashEncoder(char *_name = NULL, uint8_t init = 0) : RangeEncoder(0, 8, _name, init) {
  }

  static const uint8_t MULTITRIG_TRACK = 6;
  static const uint8_t MULTIMAP_TRACK  = 7;
  static const uint8_t AUTO_TRACK      = 8;
  
  virtual void displayAt(int i);
};
#endif

#endif /* MNM_ENCODERS_H__ */
