/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPTCPAGE_H__
#define SEQPTCPAGE_H__

#include "MidiActivePeering.h"
#include "Scales.h"
#include "SeqPage.h"
#include "SeqPages.h"

#define MAX_POLY_NOTES 16

extern scale_t *scales[24];

void ptc_pattern_len_handler(Encoder *enc);

class SeqPtcMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
};

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
class SeqPtcPage : public SeqPage, public ClockCallback {

public:
  bool re_init = false;
  uint8_t focus_track = 255;
  uint8_t key = 0;
  uint8_t poly_count = 0;
  uint8_t poly_max = 0;
  uint8_t last_midi_state = 0;
  int8_t poly_notes[MAX_POLY_NOTES];
  uint64_t note_mask = 0;
  uint16_t deferred_timer = 0;
  const uint8_t render_defer_time = 50;

  SeqPtcMidiEvents midi_events;
  SeqPtcPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
             Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  uint8_t calc_poly_count();
  uint8_t seq_ext_pitch(uint8_t note_num);
  uint8_t get_machine_pitch(uint8_t track, uint8_t note_num);
  uint8_t get_next_voice(uint8_t pitch);
  uint8_t calc_scale_note(uint8_t note_num);

  void trig_md(uint8_t note_num);
  void trig_md_fromext(uint8_t note_num);
  void clear_trig_fromext(uint8_t note_num);

  void config_encoders();
  void init_poly();
  void queue_redraw();

  bool arp_enabled = false;
  uint8_t arp_notes[ARP_MAX_NOTES];
  uint8_t arp_len;

  uint8_t arp_idx;
  uint8_t arp_base;
  uint8_t arp_dir;
  uint8_t arp_count;
  uint8_t arp_mod12_counter;

  void setup_arp();
  void remove_arp();
  void render_arp();

  void recalc_notemask();

  uint8_t arp_get_next_note_up(int8_t);
  uint8_t arp_get_next_note_down(uint8_t);
  ALWAYS_INLINE() void on_192_callback();
  ALWAYS_INLINE() void onMidiStopCallback();

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void setup();
  virtual void cleanup();
  virtual void loop();
  virtual void init();
  virtual void config();
};

#endif /* SEQPTCPAGE_H__ */
