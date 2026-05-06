#pragma once

#include "MidiSeqTrackData.h"
#include "GridTrack.h"
#include "ExtSeqTrack.h"
#include "SeqTrack.h"
#include "../../TBD/TbdP4SoundData.h"
#include "platform.h"

#ifdef PLATFORM_TBD

class GridLink;

class MidiSeqSlideData {
public:
  int32_t x0 = 0;
  int32_t x1 = 0;
  int32_t accum = 0;
  int32_t delta = 0;
  uint16_t y0 = 0;
  uint16_t y1 = 0;

  void init() {
    x0 = 0;
    x1 = 0;
    accum = 0;
    delta = 0;
    y0 = 0;
    y1 = 0;
  }
};

class MidiSeqTrack : public SeqTrackCond {
public:
  MidiSeqTrackData seq_data;
  TbdP4SoundData p4_sound;

  static uint8_t epoch;

  uint16_t tick_counter = 0;
  uint64_t note_buffer[2] = {0};
  NoteVector notes_on[NUM_NOTES_ON];
  uint8_t notes_on_count = 0;
  bool notesoff_pending = false;
  MidiSeqSlideData locks_slide_data[MIDI_SEQ_NUM_LOCKS];
  uint16_t locks_slide_next_lock_val[MIDI_SEQ_NUM_LOCKS] = {0};
  uint8_t locks_slide_next_lock_step[MIDI_SEQ_NUM_LOCKS] = {0};
  uint8_t locks_slide_next_lock_timing[MIDI_SEQ_NUM_LOCKS] = {0};
  uint8_t locks_slides_recalc = 255;
  uint16_t locks_slides_idx = 0;

  MidiSeqTrack();

  void reset();
  void seq(MidiUartClass *uart_);
  void recalc_slides();

  uint16_t ticks_per_step() const;
  uint16_t speed_multiplier_int() const { return ticks_per_step(); }
  int32_t event_tick(uint8_t step, const MidiSeqEvent &event) const;
  uint8_t page_timing_to_data(uint16_t timing) const;
  uint16_t data_timing_to_page(uint8_t timing) const;
  uint8_t step_from_tick(uint32_t tick) const {
    uint16_t tps = ticks_per_step();
    return tps == 0 ? 0 : tick / tps;
  }
  uint16_t timing_from_tick(uint32_t tick) const;

  void set_channel(uint8_t channel);
  uint8_t channel() const;
  void set_speed(uint8_t new_speed, bool timing_adjust = false);

  void init_notes_on();
  bool note_on_at(uint8_t idx, NoteVector &note) const;
  void add_notes_on(uint8_t step, int16_t timing, uint8_t value,
                    uint8_t velocity);
  uint8_t find_notes_on(uint8_t value) const;
  void remove_notes_on(uint8_t value);

  void note_on(uint8_t note, uint8_t velocity = 100,
               MidiUartClass *uart_ = nullptr);
  void note_off(uint8_t note, uint8_t velocity = 0,
                MidiUartClass *uart_ = nullptr);
  void buffer_notesoff();
  void reset_params();
  void send_cc(uint8_t cc, uint8_t value, MidiUartClass *uart_ = nullptr);
  void pitch_bend(uint16_t value, MidiUartClass *uart_ = nullptr);
  void channel_pressure(uint8_t pressure, MidiUartClass *uart_ = nullptr);
  void after_touch(uint8_t note, uint8_t pressure,
                   MidiUartClass *uart_ = nullptr);

  bool add_note(uint32_t tick, uint32_t width, uint8_t note, uint8_t velocity,
                uint8_t condition = 0);
  bool del_note(uint32_t tick, uint32_t width = 0, uint8_t note = 0);
  void record_track_noteon(uint8_t note, uint8_t velocity);
  void record_track_noteoff(uint8_t note);
  bool record_lock(uint8_t type, uint16_t parameter, uint16_t value,
                   bool slide, uint8_t lock_flags = 0,
                   uint16_t default_value = 0);

  uint8_t selected_lock_param(uint8_t slot) const;
  void set_selected_lock_param(uint8_t slot, uint8_t param);
  bool set_selected_lock_control(uint8_t slot, uint8_t type,
                                 uint16_t parameter,
                                 uint16_t default_value = 0,
                                 uint8_t flags = 0);
  bool add_lock(uint8_t step, uint16_t timing, uint8_t param, uint8_t value,
                bool slide, uint8_t lock_idx);
  bool set_p4_lock(uint8_t step, uint16_t timing, uint8_t param,
                   uint8_t value, bool slide);
  bool record_p4_lock(uint8_t param, uint8_t value, bool slide);
  bool del_lock(uint32_t tick, uint8_t lock_idx, uint8_t value);
  uint8_t count_lock_event(uint8_t step, uint8_t lock_idx) const;
  uint8_t search_lock_idx(uint8_t lock_idx, uint8_t step, uint16_t &event_idx,
                          uint16_t &event_end) const;
  uint8_t search_note_off(uint8_t note, uint8_t step, uint16_t &event_idx,
                          uint16_t event_end) const;

  void clear_track_locks();
  void clear_track_locks(uint8_t lock_idx);

  void import_legacy_ext(const ExtSeqTrackData &legacy, const GridLink &link);

  void set_length(uint8_t len, bool expand = false) override;
  void clear_track(bool locks = true) override;
  void rotate_left() override {}
  void rotate_right() override {}
  void reverse() override {}
  void transpose(int8_t offset) override;

private:
  MidiUartClass *port_ = nullptr;

  void update_legacy_progress_counter();
  uint16_t add_event(uint8_t step, const MidiSeqEvent &event);
  void remove_event(uint16_t index);
  void handle_event(const MidiSeqEvent &event, uint8_t step,
                    uint16_t bucket_start);
  void send_lock_value(const MidiSeqLockDefinition &lock,
                       const MidiSeqEvent &event);
  void send_p4_lock_value(uint8_t param, uint16_t value14);
  void prepare_slide(uint8_t lock_idx, int32_t x0, int32_t x1, uint16_t y0,
                     uint16_t y1);
  void send_slides();
  void find_next_locks(uint16_t curidx, uint8_t step, bool *find_array);
  void invalidate_lock_slide(uint8_t lock_idx, uint8_t step);
  uint16_t find_note_event(uint8_t step, uint8_t note, bool note_on,
                           uint16_t &start_idx) const;
};

#endif // PLATFORM_TBD
