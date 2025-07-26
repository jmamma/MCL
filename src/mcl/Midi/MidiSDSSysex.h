
#ifndef MidiSDSSYSEX_H__
#define MidiSDSSYSEX_H__

#include "Midi.h"
#include "MidiSDSMessages.h"
#include "MidiSDS.h"
#include "MidiSysex.h"
#include "WProgram.h"
#include "Wav.h"

class MidiSDSSysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup MidiSDS_sysex_listener
   *
   * @{
   **/

public:
  bool isSDSMessage;
  uint8_t packetNumber;
  uint8_t sds_slot;
  char sds_name[4];
  bool sds_name_rec = false;
  MidiSDSSysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0x7E;
    ids[1] = 0x00;
    ids[2] = 0x01;
    msgType = 255;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();
  inline void data_packet();
  void dump_request();
  void dump_header();
  void ack();
  void nak();
  void cancel();
  void setup(MidiClass *_midi);
  void cleanup(MidiClass *_midi);
  void wait();
  void eof();
  /* @} */
};

extern MidiSDSSysexListenerClass MidiSDSSysexListener;

/* @} @} */

#endif /* MidiSDSSYSEX_H__ */
