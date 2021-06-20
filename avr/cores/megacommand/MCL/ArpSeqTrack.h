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

  uint8_t enabled : 4;
  uint8_t range : 4;
  uint8_t oct : 4;
  uint8_t rate : 4;
  uint8_t mode;

  uint8_t fine_tune;
  uint64_t note_mask[2]; //input notes
};

//Ephemeral
class ArpSeqTrack : public ArpSeqData, public SeqTrackBase  {

public:
  uint8_t last_note_on;
  uint8_t idx;

  ArpSeqTrack() : SeqTrackBase() {
    active = ARP_TRACK_TYPE;
    init();
  }

  void clear_notes() {
    len = 0;
    idx = 0;
    memset(note_mask,0,sizeof(note_mask));
    last_note_on = 255;
  }

  void init() {
    speed = SEQ_SPEED_2X;
    rate = 1;
    length = 1 << rate; //Arp rate is function of length
    enabled = false;
    range = 0;
    oct = 1;
    fine_tune = 0;
    clear_notes();
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
  void render(uint8_t mode_, uint8_t oct_, uint8_t fine_tune_, uint8_t range_, uint64_t *note_mask_);

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
