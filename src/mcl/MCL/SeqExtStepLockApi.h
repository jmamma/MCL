/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPLOCKAPI_H__
#define SEQEXTSTEPLOCKAPI_H__

#include "SeqExtStepTypes.h"
#include <stdint.h>
#include <stddef.h>

class ExtSeqTrack;
#ifdef PLATFORM_TBD
class MidiSeqTrack;
#endif

struct SeqExtStepLockParamInfo {
  bool active = false;
  bool p4_param = false;
  bool sendable = false;
  bool nrpn = false;
  bool macro = false;
  bool learn = false;
  uint8_t type = 0;
  uint16_t ctrl = 0;
  uint8_t ctrl_type = 0;
  uint16_t param_id = 0;
  uint16_t resolution = 128;
  int16_t min_value = 0;
  int16_t max_value = 127;
  int16_t default_value = 0;
  int16_t current_value = 0;
};

enum SeqExtStepLockCtrlType : uint8_t {
  SEQ_EXT_LOCK_CTRL_OFF = 0,
  SEQ_EXT_LOCK_CTRL_CC = 1,
  SEQ_EXT_LOCK_CTRL_NRPN = 2,
  SEQ_EXT_LOCK_CTRL_RPN = 3,
  SEQ_EXT_LOCK_CTRL_PITCH_BEND = 4,
  SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE = 5,
  SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE = 6,
  SEQ_EXT_LOCK_CTRL_POLY_PRESSURE = 7,
};

class SeqExtStepLockApi {
public:
  explicit SeqExtStepLockApi(ExtSeqTrack &track) : ext_track_(&track) {}
#ifdef PLATFORM_TBD
  explicit SeqExtStepLockApi(MidiSeqTrack &track) : midi_track_(&track) {}
#endif

  bool delete_lock(seq_extstep_tick_t tick, uint8_t lock_idx, uint8_t value);
  bool add_lock(uint8_t step, uint16_t timing, uint8_t param, uint8_t value,
                bool slide, uint8_t lock_idx);
  bool set_p4_param_lock(uint8_t step, uint16_t timing, uint8_t param,
                         uint8_t value, bool slide);
  bool record_p4_param_lock(uint8_t param, uint8_t value, bool slide);
  bool p4_param_lock_value(uint8_t step, uint8_t param,
                           uint8_t &value) const;
  uint8_t count_lock_event(uint8_t step, uint8_t lock_idx);
  uint8_t selected_lock_param(uint8_t slot) const;
  bool selected_lock_param_id(uint8_t slot, uint8_t &param_id) const;
  uint8_t selected_lock_menu_value(uint8_t slot) const;
  bool selected_lock_menu_editable(uint8_t slot) const;
  uint8_t lock_param_menu_max() const;
  bool lock_menu_value_info(uint8_t menu_value,
                            SeqExtStepLockParamInfo &info) const;
  uint8_t normalize_lock_menu_value(uint8_t menu_value,
                                    uint8_t old_value) const;
  void set_selected_lock_param(uint8_t slot, uint8_t param);
  void set_selected_lock_menu_value(uint8_t slot, uint8_t menu_value);
  bool set_selected_lock_control(uint8_t slot, uint8_t ctrl_type,
                                 uint16_t ctrl, uint16_t default_value = 0);
  bool selected_lock_param_info(uint8_t slot,
                                SeqExtStepLockParamInfo &info) const;
  bool copy_selected_lock_label(uint8_t slot, char *dst,
                                size_t dst_len) const;
  bool copy_lock_menu_value_label(uint8_t menu_value, char *dst,
                                  size_t dst_len) const;
  uint8_t selected_lock_current_ui_value(uint8_t slot) const;
  uint8_t lock_ui_value_from_control(uint8_t slot, uint8_t ctrl_type,
                                     uint16_t ctrl, uint16_t value) const;
  bool selected_lock_matches_control(uint8_t slot, uint8_t ctrl_type,
                                     uint16_t ctrl) const;
  bool copy_lock_value_label(uint8_t slot, uint8_t value, char *dst,
                             size_t dst_len) const;
  bool record_control_lock(uint8_t ctrl_type, uint16_t ctrl, uint16_t value,
                           bool slide);

private:
  static void copy_literal(const char *src, char *dst, size_t dst_len);
  static void append_uint16(uint16_t value, char *dst, size_t dst_len,
                            size_t &out);
  static void copy_param_number_label(char prefix, uint16_t number, char *dst,
                                      size_t dst_len);
  static void put_int16(int16_t value, char *dst, size_t dst_len);
  static uint16_t value14_from_value7(uint8_t value7);
  static int16_t param_value_from_value7(const SeqExtStepLockParamInfo &info,
                                         uint8_t value7);
  static uint8_t value7_from_param_value(const SeqExtStepLockParamInfo &info,
                                         int16_t value);
#ifdef PLATFORM_TBD
  static uint8_t ctrl_type_to_midi_lock_type(uint8_t ctrl_type);
  bool find_p4_control(uint8_t ctrl_type, uint16_t ctrl, uint16_t value,
                       uint8_t &lock_param, uint16_t &value14,
                       uint16_t &default_value14) const;
  static uint16_t value14_from_param_actual(
      const SeqExtStepLockParamInfo &info, int16_t value);
#endif
  static uint8_t value7_from_14(uint16_t value14);

  ExtSeqTrack *ext_track_ = nullptr;
#ifdef PLATFORM_TBD
  MidiSeqTrack *midi_track_ = nullptr;
#endif
};

#endif /* SEQEXTSTEPLOCKAPI_H__ */
