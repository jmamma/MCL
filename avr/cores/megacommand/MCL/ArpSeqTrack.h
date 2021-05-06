/* Justin Mammarella jmamma@gmail.com 2021 */

#pragma once

#include "MidiUartParent.h"
#include "SeqTrack.h"
#include "WProgram.h"
#include "GridTrack.h"

#define ARP_UP 0
#define ARP_DOWN 1
#define ARP_UPDOWN 2
#define ARP_DOWNUP 3
#define ARP_UPNDOWN 4
#define ARP_DOWNNUP 5
#define ARP_CONV 6
#define ARP_DIV 7
#define ARP_CONVDIV 8
#define ARP_PINKUP 9
#define ARP_PINKDOWN 10
#define ARP_THUMBUP 11
#define ARP_THUMBDOWN 12
#define ARP_UPP 13
#define ARP_DOWNP 14
#define ARP_UP2 15
#define ARP_DOWN2 16
#define ARP_RND 17

#define ARP_ON 1
#define ARP_LATCH 2
#define ARP_OFF 0

#define ARP_MAX_NOTES 16 * 3

class ArpSeqData {
  public:
  uint8_t notes[ARP_MAX_NOTES]; //output notes
  
  uint8_t len;
  uint8_t idx;

  uint8_t enabled;
  uint8_t mode;
  uint8_t oct;
  uint32_t note_mask; //input notes
};

//Ephemeral
class ArpSeqTrack : public ArpSeqData, public SeqTrackBase  {

public:
  uint8_t last_note_on;
  ArpSeqTrack() : SeqTrackBase() { 
    active = ARP_TRACK_TYPE;
    init();
  } 
  void init() {
    speed = SEQ_SPEED_2X;
    length = 16;
    enabled = false; 
    len = 0;  
    idx = 0;  
    note_mask = 0;
    last_note_on = 255;
  }

  ALWAYS_INLINE() void reset() {
    SeqTrackBase::reset();
  }

  ALWAYS_INLINE() void seq(MidiUartParent *uart_);
  void clear_track();
  void re_sync();
  void set_speed(uint8_t speed_);
  void set_length(uint8_t length_);

  uint8_t get_next_note_up(int8_t cur);
  void render(uint8_t mode_, uint8_t oct_, uint32_t note_mask_);

};

class MDArpSeqTrack : public ArpSeqTrack {
  public:
    MDArpSeqTrack() : ArpSeqTrack() {
      ArpSeqTrack::init();
      active = MD_ARP_TRACK_TYPE;
    }
};

class ExtArpSeqTrack : public ArpSeqTrack {
  public:
    ExtArpSeqTrack() : ArpSeqTrack() {
      ArpSeqTrack::init();
      active = EXT_ARP_TRACK_TYPE;
    }
};
