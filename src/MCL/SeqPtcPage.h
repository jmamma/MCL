/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPTCPAGE_H__
#define SEQPTCPAGE_H__

#include "MidiActivePeering.h"
#include "Scales.h"
#include "SeqPage.h"
#include "SeqPages.h"

#define MAX_POLY_NOTES 16

#define POLY_EVENT 0xF0
#define CTRL_EVENT 0xE0
#define TRIG_EVENT 0xD0
#define NO_EVENT 0x00

extern scale_t *scales[24];

class SeqPtcMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_midi(MidiClass *midi);
  void cleanup_midi(MidiClass *midi);
  void setup_callbacks();
  void remove_callbacks();

  void note_on(uint8_t *msg, uint8_t channel_event);
  void note_off(uint8_t *msg, uint8_t channel_event);

  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);

  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);

  void onChannelPressureCallback_Midi2(uint8_t *msg);
  void onAfterTouchCallback_Midi2(uint8_t *msg);
  void onPitchWheelCallback_Midi2(uint8_t *msg);
};

class SeqPtcPage : public SeqPage, public ClockCallback {

public:
  bool re_init = false;
  uint8_t transpose = 0;
  int8_t poly_notes[MAX_POLY_NOTES];
  uint8_t poly_order[MAX_POLY_NOTES];

  uint8_t dev_note_channels[NUM_DEVS];
  uint64_t dev_note_masks[NUM_DEVS][2];
  uint64_t note_mask[2];

  bool scale_padding;
  bool cc_link_enable;

  uint8_t octs[NUM_DEVS];
  uint8_t fine_tunes[NUM_DEVS];

  uint8_t find_arp_track(uint8_t channel_event);
  MidiDevice *last_midi_device = nullptr;

  SeqPtcMidiEvents midi_events;
  SeqPtcPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
             Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  uint8_t seq_ext_pitch(uint8_t note_num);
  uint8_t process_ext_event(uint8_t note_num, bool note_type, uint8_t channel);
  uint8_t get_machine_pitch(uint8_t track, uint8_t note_num,
                            uint8_t fine_tune = 255);
  uint8_t get_next_voice(uint8_t pitch, uint8_t track_number, uint8_t channel_event);
  uint8_t calc_scale_note(uint8_t note_num, bool padded = false);
  void record(uint8_t pitch, uint8_t tracknumber);
  void trig_md(uint8_t note_num, uint8_t track_number = 255, uint8_t channel_event = CTRL_EVENT,
               uint8_t fine_tune = 255, MidiUartParent *uart_ = nullptr);

  void note_on_ext(uint8_t note_num, uint8_t velocity,
                   uint8_t track_number = 255, MidiUartParent *uart_ = nullptr);
  void note_off_ext(uint8_t note_num, uint8_t velocity,
                    uint8_t track_number = 255,
                    MidiUartParent *uart_ = nullptr);

  void buffer_notesoff_ext(uint8_t track_number);

  void clear_trig_fromext(uint8_t note_num);

  uint8_t get_note_from_machine_pitch(uint8_t track_number, uint8_t pitch);

  uint8_t is_md_midi(uint8_t channel);
  virtual void config_encoders();
  void init_poly();

  void render_arp(bool recalc_notemask_, MidiDevice *midi_dev, uint8_t track);

  void recalc_notemask();
  void draw_popup_octave();
  void draw_popup_transpose();

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void setup();
  virtual void cleanup();
  virtual void loop();
  virtual void init();
  virtual void config();
};

#endif /* SEQPTCPAGE_H__ */
