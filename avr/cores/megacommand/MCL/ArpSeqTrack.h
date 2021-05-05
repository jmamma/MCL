/* Justin Mammarella jmamma@gmail.com 2021 */

#pragma once

#include "MidiUartParent.h"
#include "SeqTrack.h"
#include "WProgram.h"

class ArpSeqData {
  public:
  bool arp_enabled;
  uint8_t arp_notes[ARP_MAX_NOTES]; //output notes
  
  uint8_t arp_len;
  uint8_t arp_idx;

  uint8_t mode;
  uint8_t oct;
  uint32_t note_mask; //input notes
}

//Ephemeral
class ArpSeqTrack : public SeqTrackBase  {

public:
  ArpSeqTrack() : SeqTrackBase() { 
    active = ARP_TRACK_TYPE;
    init();
  } 
  void init() {
    speed = SEQ_SPEED_2X;
    length = 16;
    arp_enabled = false; 
    arp_len = 0;  
    arp_idx = 0;  
    note_mask = 0;
  }

  ALWAYS_INLINE() void reset() {
    SeqTrackBase::reset();
  }

  ALWAYS_INLINE() void seq(MidiUartParent *uart_);
  void clear_track();
  void set_length(uint8_t len);
  void re_sync();
  void set_speed(uint8_t _speed);
  void render();
};

class MDArpSeqTrack : public ArpSeqTrack {
  public:
    MDArpSeqTrack() : ArpSeqTrack() {
      ArpSeqTrack::init();
      active = ARP_MD_TRACK_TYPE;
    }
}

class ExtArpSeqTrack : public ArpSeqTrack {
  public:
    ExtArpSeqTrack() : ArpSeqTrack() {
      ArpSeqTrack::init();
      active = ARP_EXT_TRACK_TYPE;
    }
}
