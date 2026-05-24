/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "MDFXTrack.h"
#include "MDTrack.h"


class ATTR_PACKED() MDFXData {
public:
  bool enable_reverb;
  bool enable_delay;
  bool enable_eq;
  bool enable_dynamics;
  /** The settings of the reverb effect. f**/
  uint8_t reverb[8];
  /** The settings of the delay effect. **/
  uint8_t delay[8];
  /** The settings of the EQ effect. **/
  uint8_t eq[8];
  /** The settings of the compressor effect. **/
  uint8_t dynamics[8];
};

class ATTR_PACKED() MDFXTrack : public AUXTrack, public MDFXData {
public:
  MDFXTrack() {
    active = MDFX_TRACK_TYPE;
  }
  size_t _sizeof() const {
     return sizeof(MDFXTrack) - sizeof(void*);
  }
  void init() {
     enable_reverb = false;
     enable_delay = false;
     enable_eq = false;
     enable_dynamics = false;
  }
  void init_defaults() override { init(); }

  void place_fx_in_kit();
  void get_fx_from_kit();
  void get_fx_from_kit_extra(KitExtra *kit_extra);

  uint16_t calc_latency(uint8_t tracknumber) override;
  uint16_t send_fx(bool send = true);
  void paste_track(uint8_t src_track, uint8_t dest_track, SeqTrack *seq_track) override;
  virtual void get_online_data(uint8_t merge) override;
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                              GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;

  virtual uint16_t get_track_size() override { return _sizeof(); }
  virtual uintptr_t get_region() override { return BANK1_MDFX_TRACK_START; }

  bool copy_grid_slot_label(const GridSlotLabelContext &ctx,
                            char label[3]) override {
    (void)ctx;
    label[0] = 'F';
    label[1] = 'X';
    label[2] = '\0';
    return true;
  }
  virtual uint8_t get_model() override { return MDFX_TRACK_TYPE; }
  virtual void* get_sound_data_ptr() override { return &reverb; }
  virtual size_t get_sound_data_size() override { return 8 * 4; }
};
