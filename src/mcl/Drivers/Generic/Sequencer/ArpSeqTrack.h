/* Justin Mammarella jmamma@gmail.com 2021 */

#pragma once

#include "MidiUart.h"
#include "SeqTrack.h"
#include "MidiClock.h"
#include "platform.h"
#include "GridTrack.h"
#include "SeqTrackModData.h"

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
#define ARP_RND2 18

#define ARP_ON 1
#define ARP_LATCH 2
#define ARP_LOCK 3
#define ARP_OFF 0

#define ARP_RATE_TRIG 0

#define ARP_MAX_NOTES 16 * 3

//Ephemeral
class ArpSeqTrack : public ArpSeqData, public SeqTrack  {

public:
  uint8_t notes[ARP_MAX_NOTES]; //output notes
  uint8_t len;
  uint8_t last_note_on;
  uint8_t idx;

  ArpSeqTrack() : SeqTrack() {}

  void clear_notes() {
    len = 0;
    idx = 0;
    memset(note_mask,0,sizeof(note_mask));
    last_note_on = 255;
  }

  void init() {
    ArpSeqData::init();
    speed = SEQ_SPEED_2X;
    length = 2;
    clear_notes();
  }

  ALWAYS_INLINE() void reset() {
    SeqTrack::reset();
  }

  void seq(MidiUartClass *uart_, MidiUartClass *uart2_);
  void clear_track();
  void re_sync();
  void set_speed(uint8_t speed_);
  void set_length(uint8_t length_);
  static uint8_t speed_for_parent_speed(uint8_t parent_speed);
  void load_data(const ArpSeqData &data, const ArpSeqPhaseData &phase,
                 uint8_t parent_speed);
  void store_data(ArpSeqData *data) const;
  void store_phase_data(ArpSeqPhaseData &phase) const;
  ALWAYS_INLINE() bool trigger() {
    if (!consumes_seq_trig()) {
      return false;
    }
    if (!count_down) {
      mod12_counter = 0;
    }
    return true;
  }
  bool consumes_seq_trig() const {
    return enabled && length == ARP_RATE_TRIG;
  }
  bool locks_note_set() const { return enabled == ARP_LOCK; }
  bool preserves_note_set() const {
    return enabled == ARP_LATCH || enabled == ARP_LOCK;
  }
  ALWAYS_INLINE() bool request_speed_change(uint8_t new_speed) {
    if (count_down) {
      return false;
    }
    if (!MidiClock.isStarted()) {
      if (speed == new_speed && !has_pending_speed_change()) {
        return false;
      }
      clear_pending_speed_change();
      set_speed(new_speed);
      return true;
    }
    return SeqTrack::request_speed_change(new_speed);
  }

  uint8_t get_next_note_up(int8_t cur);
  void render(uint8_t mode_, uint8_t oct_, uint8_t fine_tune_, uint8_t range_, const uint64_t *note_mask_);

protected:
  virtual void on_cycle_midpoint(MidiUartClass *uart_, MidiUartClass *uart2_);
  virtual void dispatch_note(uint8_t note, MidiUartClass *uart_,
                             MidiUartClass *uart2_) = 0;
  void dispatch_next_note(MidiUartClass *uart_, MidiUartClass *uart2_);
  virtual void on_render_begin();

};

class MDArpSeqTrack : public ArpSeqTrack {
  public:
    MDArpSeqTrack() : ArpSeqTrack() {
      active = MD_ARP_TRACK_TYPE;
    }
protected:
    void on_cycle_midpoint(MidiUartClass *uart_, MidiUartClass *uart2_) override;
    void dispatch_note(uint8_t note, MidiUartClass *uart_,
                       MidiUartClass *uart2_) override;
    void on_render_begin() override;
};

class ExtArpSeqTrack : public ArpSeqTrack {
  public:
    ExtArpSeqTrack() : ArpSeqTrack() {
      active = EXT_ARP_TRACK_TYPE;
    }
  protected:
    void dispatch_note(uint8_t note, MidiUartClass *uart_,
                       MidiUartClass *uart2_) override;
    void on_cycle_midpoint(MidiUartClass *uart_, MidiUartClass *uart2_) override;
    void on_render_begin() override;
};
