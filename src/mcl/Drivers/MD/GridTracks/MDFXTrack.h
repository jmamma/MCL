/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "MDFXTrack.h"
#include "MDTrack.h"

#if !defined(__AVR__)
constexpr uint8_t MDFX_TRACK_STORAGE_VERSION_ROUTED_FX = 1;
#endif

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
#if !defined(__AVR__)
  uint8_t userBusFx[SPS_USER_BUS_FX_COUNT][SPS_USER_FX_PARAM_COUNT];
  uint8_t userPostFx[SPS_USER_FX_PARAM_COUNT];
#endif
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
#if !defined(__AVR__)
     memset(userBusFx, SPS_USER_FX_DEFAULT_PARAM, sizeof(userBusFx));
     memset(userPostFx, SPS_USER_FX_DEFAULT_PARAM, sizeof(userPostFx));
#endif
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

  uint16_t grid_slot_label(GridSlotLabelContext ctx) override {
    (void)ctx;
    return make_grid_slot_label('F', 'X');
  }
  virtual uint8_t get_model() override { return MDFX_TRACK_TYPE; }
#if !defined(__AVR__)
  uint8_t storage_version() const override {
    return MDFX_TRACK_STORAGE_VERSION_ROUTED_FX;
  }
  void on_storage_loaded() override {
    if (!storage_version_at_least(MDFX_TRACK_STORAGE_VERSION_ROUTED_FX)) {
      memset(userBusFx, SPS_USER_FX_DEFAULT_PARAM, sizeof(userBusFx));
      memset(userPostFx, SPS_USER_FX_DEFAULT_PARAM, sizeof(userPostFx));
      version = MDFX_TRACK_STORAGE_VERSION_ROUTED_FX;
    }
  }
#endif
  virtual void* get_sound_data_ptr() override { return &reverb; }
  virtual size_t get_sound_data_size() override {
    return sizeof(MDFXData) - 4 * sizeof(bool);
  }
};

static_assert(MEMORY_ALIGN(sizeof(MDFXTrack) - sizeof(void*)) <=
                  MDFX_TRACK_LEN,
              "MDFXTrack outgrew MDFX_TRACK_LEN");
