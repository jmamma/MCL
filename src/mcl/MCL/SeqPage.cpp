#include "SeqPage.h"
#include "../Drivers/A4/A4.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#if !defined(__AVR__)
#include "DevicePanelRef.h"
#endif
#include "DeviceParamResolver.h"
#include "MCLClipBoard.h"
#include "MCLFeatureConfig.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MCLStrings.h"
#include "MDTrack.h"
#include "PtcGroups.h"
#include "ResourceManager.h"
#include "SeqPages.h"
#include "SeqExtStepTrackRef.h"
#include "SeqStepTrackRef.h"
#include "SeqTrackUtil.h"
#ifdef PLATFORM_TBD
#include "MidiSetup.h"
#endif

namespace {

constexpr uint8_t kSeqPageVisibleSteps = 16;
constexpr uint8_t kSeqPageTrackMaskWidth = 16;
const char *const kMaskInfoLabels[] PROGMEM = {
    mclstr_trig,
    mclstr_mute,
    mclstr_swing,
    mclstr_slide,
    mclstr_lock,
};

#if defined(__AVR__)
#define seq_panel_popup_text(...) MD.popup_text(__VA_ARGS__)
#define seq_panel_popup_text_P(...) MD.popup_text_P(__VA_ARGS__)
#else
#define seq_panel_popup_text(...) DevicePanelRef::popup_text(__VA_ARGS__)
#define seq_panel_popup_text_P(...) DevicePanelRef::popup_text_P(__VA_ARGS__)
#endif

#if defined(__AVR__)
bool seq_page_uses_non_md_primary_step_tracks() {
  return false;
}
#else
bool seq_page_uses_primary_step_tracks() {
  PageIndex page = mcl.currentPage();
  if (!seq_step_tracks_available()) {
    return false;
  }
  if (page == SEQ_STEP_PAGE) {
    return true;
  }
  return page == SEQ_PTC_PAGE &&
         SeqPage::current_device_idx() == DeviceIdx::Primary;
}

bool seq_page_uses_non_md_primary_step_tracks() {
  return seq_page_uses_primary_step_tracks() &&
         !SeqTrackUtil::is_md_device(DeviceParamResolver::device_for_idx(DeviceIdx::Primary));
}
#endif

#if !defined(__AVR__)
bool seq_page_uses_signed_microtiming() {
  if (!seq_page_uses_primary_step_tracks()) {
    return false;
  }
  return seq_step_active_track().uses_signed_microtiming();
}
#endif

uint8_t seq_page_condition_count() {
#if defined(__AVR__)
  return NUM_TRIG_CONDITIONS;
#else
  if (seq_page_uses_primary_step_tracks()) {
    return seq_step_active_track().condition_count();
  }
  return NUM_TRIG_CONDITIONS;
#endif
}

bool seq_page_condition_label(uint8_t condition, bool plock, bool marker,
                              char *out) {
#if defined(__AVR__)
  (void)condition;
  (void)plock;
  (void)marker;
  (void)out;
  return false;
#else
  if (!seq_page_uses_primary_step_tracks()) {
    return false;
  }
  seq_step_active_track().condition_label(condition, plock, marker, out);
  return true;
#endif
}

uint8_t seq_page_step_offset() {
  return SeqPage::page_select * kSeqPageVisibleSteps;
}

uint8_t seq_page_visible_step(uint8_t step_key) {
  return step_key + seq_page_step_offset();
}

} // namespace

uint8_t SeqPage::page_select = 0;

MidiDevice *SeqPage::midi_device = &MD;
MidiDevice *opt_midi_device_capture;
DeviceIdx opt_midi_device_idx_capture = DeviceIdx::Primary;

uint8_t SeqPage::last_param_id = 0;
uint8_t SeqPage::last_rec_event = 0;

uint8_t SeqPage::page_count = 4;

uint8_t SeqPage::pianoroll_mode = 0;

uint8_t SeqPage::mask_type = MASK_PATTERN;
uint8_t SeqPage::param_select = 0;

uint8_t SeqPage::last_pianoroll_mode = 0;

uint8_t SeqPage::velocity = 100;
uint8_t SeqPage::cond = 0;
uint8_t SeqPage::slide = true;
uint8_t SeqPage::md_micro = false;

bool SeqPage::show_seq_menu = false;
bool SeqPage::toggle_device = true;

uint16_t SeqPage::mute_mask = 0;

uint8_t SeqPage::step_select = 255;

bool SeqPage::is_midi_model = false;

uint32_t SeqPage::last_md_model = 255;

#if defined(__AVR__)
constexpr bool kSeqSlotsCanSharePhysicalDevice = false;
#else
constexpr bool kSeqSlotsCanSharePhysicalDevice = true;
#endif

MidiDevice *SeqPage::device_for_seq_idx(DeviceIdx device_idx) {
  return device_idx == DeviceIdx::Secondary
             ? device_manager.secondary_device()
             : device_manager.primary_device();
}

bool SeqPage::devices_share_physical() {
  return kSeqSlotsCanSharePhysicalDevice &&
         device_manager.primary_device() == device_manager.secondary_device();
}

DeviceIdx SeqPage::current_device_idx() {
#ifdef EXT_TRACKS
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
    return DeviceIdx::Secondary;
  }
#endif
  if (devices_share_physical()) {
    return static_cast<DeviceIdx>(mcl_cfg.seq_dev);
  }
  return midi_device == device_manager.secondary_device() ? DeviceIdx::Secondary
                                                          : DeviceIdx::Primary;
}

bool SeqPage::idx_is_md_device(DeviceIdx device_idx) {
  if (devices_share_physical() && device_idx == DeviceIdx::Secondary) {
    return false;
  }
  return SeqTrackUtil::is_md_device(device_for_seq_idx(device_idx));
}

bool SeqPage::device_is_md(MidiDevice *device) {
  if (devices_share_physical() && device == device_manager.primary_device()) {
    return idx_is_md_device(current_device_idx());
  }
  return SeqTrackUtil::is_md_device(device);
}

bool SeqPage::active_device_is_md() {
  return idx_is_md_device(current_device_idx());
}

void SeqPage::select_device_idx(DeviceIdx device_idx) {
  mcl_cfg.seq_dev = static_cast<uint8_t>(device_idx);
  midi_device = device_for_seq_idx(device_idx);
  opt_midi_device_capture = midi_device;
  opt_midi_device_idx_capture = device_idx;
}

static inline DeviceIdx seq_idx_for_device(MidiDevice *device) {
  if (!SeqPage::devices_share_physical() &&
      device == device_manager.secondary_device()) {
    return DeviceIdx::Secondary;
  }
  if (SeqPage::devices_share_physical() &&
      device == device_manager.primary_device()) {
    return SeqPage::current_device_idx();
  }
  return DeviceIdx::Primary;
}

static inline bool opt_capture_is_md_device() {
  if (SeqPage::devices_share_physical() &&
      opt_midi_device_capture == device_manager.primary_device()) {
    return SeqPage::idx_is_md_device(opt_midi_device_idx_capture);
  }
  return SeqTrackUtil::is_md_device(opt_midi_device_capture);
}

static inline const char *seq_device_idx_name(DeviceIdx device_idx) {
  if (SeqPage::devices_share_physical()) {
    return device_idx == DeviceIdx::Secondary ? "TB2" : "TB1";
  }
  return SeqPage::device_for_seq_idx(device_idx)->name;
}

uint8_t opt_speed = 1;
uint8_t opt_swing = 50;
uint8_t opt_trackid = 1;
uint8_t opt_copy = 0;
uint8_t opt_paste = 0;
uint8_t opt_clear = 0;
uint8_t opt_shift = 0;
uint8_t opt_reverse = 0;
uint8_t opt_transpose = 12;
uint8_t opt_lfo_mult = 0;
uint8_t opt_clear_step = 0;
uint8_t opt_length = 0;
uint8_t opt_channel = 0;
uint8_t opt_undo = 255;
uint8_t opt_undo_track = 255;

uint16_t trigled_mask = 0;
uint16_t locks_on_step_mask = 0;

bool SeqPage::recording = false;

uint8_t SeqPage::last_midi_state = 0;
uint8_t SeqPage::last_step = 255;

static MCLEncoder *opt_param1_capture = nullptr;
static MCLEncoder *opt_param2_capture = nullptr;

static const char seq_note_names_upper[] PROGMEM =
    "C C#D D#E F F#G G#A A#B ";

void seq_copy_note_name(uint8_t note, char *out) {
  uint8_t offset = note * 2;
  out[0] = pgm_read_byte(seq_note_names_upper + offset);
  out[1] = pgm_read_byte(seq_note_names_upper + offset + 1);
  out[2] = '\0';
}

void seq_copy_note_label(uint8_t note_num, char *out) {
  uint8_t oct = note_num / 12;
  uint8_t note = note_num - 12 * oct;
  seq_copy_note_name(note, out);
  mcl_gui.put_value_at(oct, out + 2);
}

uint8_t copy_mask = 0;

static inline uint8_t selected_track_index(bool is_md_device) {
#ifdef EXT_TRACKS
  return is_md_device ? last_primary_track : last_ext_track;
#else
  (void)is_md_device;
  return last_primary_track;
#endif
}

static inline SeqTrackCond &selected_track(bool is_md_device) {
  return SeqTrackUtil::get_track(is_md_device,
                                 selected_track_index(is_md_device));
}

static inline bool seq_page_uses_step_track_ops(bool is_md_device) {
  return is_md_device || seq_page_uses_non_md_primary_step_tracks();
}

SeqStepTrackRef seq_page_step_track_for(uint8_t track) NOINLINE();
SeqStepTrackRef seq_page_step_track_for(uint8_t track) {
  return seq_step_track_for(track);
}

SeqStepTrackRef seq_page_active_step_track() NOINLINE();
SeqStepTrackRef seq_page_active_step_track() {
  return seq_step_active_track();
}

static inline uint8_t seq_page_step_track_count() {
  return seq_step_track_count();
}

static inline bool seq_page_active_step_track_uses_poly_mask() {
  SeqStepTrackRef track = seq_page_active_step_track();
  return !track.selects_track_locally();
}

static inline uint16_t seq_page_active_poly_mask() {
  return ptc_groups.mask_for_track(last_primary_track);
}

enum SeqTrackMenuOp : uint8_t {
  SEQ_TRACK_OP_ROTATE_LEFT,
  SEQ_TRACK_OP_ROTATE_RIGHT,
  SEQ_TRACK_OP_REVERSE,
  SEQ_TRACK_OP_TRANSPOSE,
};

static inline bool seq_track_menu_op_for_key(uint8_t key, SeqTrackMenuOp &op) {
  switch (key) {
  case MDX_KEY_LEFT:
    op = SEQ_TRACK_OP_ROTATE_LEFT;
    return true;
  case MDX_KEY_RIGHT:
    op = SEQ_TRACK_OP_ROTATE_RIGHT;
    return true;
  case MDX_KEY_UP:
    op = SEQ_TRACK_OP_REVERSE;
    return true;
  default:
    return false;
  }
}

static inline void apply_track_menu_op(SeqStepTrackRef &track,
                                       SeqTrackMenuOp op, int8_t offset) {
  switch (op) {
  case SEQ_TRACK_OP_ROTATE_LEFT:
    track.rotate_left();
    break;
  case SEQ_TRACK_OP_ROTATE_RIGHT:
    track.rotate_right();
    break;
  case SEQ_TRACK_OP_REVERSE:
    track.reverse();
    break;
  case SEQ_TRACK_OP_TRANSPOSE:
    track.transpose(offset);
    break;
  }
}

static inline void apply_track_menu_op(SeqTrackCond &track, SeqTrackMenuOp op,
                                       int8_t offset) {
  switch (op) {
  case SEQ_TRACK_OP_ROTATE_LEFT:
    track.rotate_left();
    break;
  case SEQ_TRACK_OP_ROTATE_RIGHT:
    track.rotate_right();
    break;
  case SEQ_TRACK_OP_REVERSE:
    track.reverse();
    break;
  case SEQ_TRACK_OP_TRANSPOSE:
    track.transpose(offset);
    break;
  }
}

#if !defined(__AVR__)
static inline void apply_track_menu_op(StepSeqTrackCond &track,
                                       SeqTrackMenuOp op, int8_t offset) {
  switch (op) {
  case SEQ_TRACK_OP_ROTATE_LEFT:
    track.rotate_left();
    break;
  case SEQ_TRACK_OP_ROTATE_RIGHT:
    track.rotate_right();
    break;
  case SEQ_TRACK_OP_REVERSE:
    track.reverse();
    break;
  case SEQ_TRACK_OP_TRANSPOSE:
    track.transpose(offset);
    break;
  }
}
#endif

static inline void apply_md_track_menu_op(SeqTrackMenuOp op) {
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    apply_track_menu_op(
        static_cast<StepSeqTrackCond &>(mcl_seq.spsx_tracks[last_primary_track]), op,
        0);
    return;
  }
#endif
  apply_track_menu_op(
      static_cast<SeqTrackCond &>(mcl_seq.md_tracks[last_primary_track]), op, 0);
}

static inline void apply_track_menu_op(bool is_md_device, bool apply_all,
                                       SeqTrackMenuOp op, int8_t offset = 0) {
  if (seq_page_uses_step_track_ops(is_md_device)) {
    uint8_t len = apply_all ? seq_page_step_track_count() : 1;
    uint8_t start = apply_all ? 0 : last_primary_track;
    for (uint8_t i = start; i < start + len; i++) {
      auto track = seq_page_step_track_for(i);
      apply_track_menu_op(track, op, offset);
    }
  } else if (apply_all) {
    uint8_t len = SeqTrackUtil::track_count(is_md_device);
    for (uint8_t i = 0; i < len; ++i) {
      apply_track_menu_op(SeqTrackUtil::get_track(is_md_device, i), op, offset);
    }
  } else {
    apply_track_menu_op(selected_track(is_md_device), op, offset);
  }
}

static inline uint8_t ext_track_channel(uint8_t track) {
  return SeqExtStepTrackRef::track(track).channel();
}

static inline void set_ext_track_channel(uint8_t track, uint8_t channel) {
  SeqExtStepTrackRef::track(track).set_channel(channel);
}

static inline void clear_ext_track(uint8_t track) {
  SeqExtStepTrackRef::track(track).clear_track();
}

static inline void clear_ext_track_locks(uint8_t track) {
  SeqExtStepTrackRef::track(track).clear_track_locks();
}

static inline void clear_ext_track_locks(uint8_t track, uint8_t lock_idx) {
  SeqExtStepTrackRef::track(track).clear_track_locks(lock_idx);
}

static inline void copy_step_to_clipboard(SeqStepTrackRef track, uint8_t step,
                                          uint8_t clip_idx) {
#if !defined(__AVR__)
  track.copy_step(step, &mcl_clipboard.steps[clip_idx],
                  &mcl_clipboard.spsx_steps[clip_idx]);
#else
  track.copy_step(step, &mcl_clipboard.steps[clip_idx]);
#endif
}

static inline void paste_step_from_clipboard(SeqStepTrackRef track,
                                             uint8_t step, uint8_t clip_idx) {
#if !defined(__AVR__)
  track.paste_step(step, &mcl_clipboard.steps[clip_idx],
                   &mcl_clipboard.spsx_steps[clip_idx]);
#else
  track.paste_step(step, &mcl_clipboard.steps[clip_idx]);
#endif
}

static inline void display_popup(const char *str_P) {
  oled_display.textbox_P(str_P);
  seq_panel_popup_text_P(str_P);
}

static inline void display_popup(const char *str1_P, const char *str2_P) {
  oled_display.textbox_P(str1_P, str2_P);
  seq_panel_popup_text_P(str1_P, str2_P);
}

static inline void open_enhanced_swing_window() {
  if (MD.connected && MD.global.extendedMode == 2) {
    MD.draw_open_swing();
  }
}

static inline void close_enhanced_swing_window() {
  if (MD.connected && MD.global.extendedMode == 2) {
    MD.draw_close_swing();
  }
}

static inline bool should_show_enhanced_swing_window() {
  return mcl.currentPage() == SEQ_STEP_PAGE && SeqPage::mask_type == MASK_SWING;
}

bool enhanced_swing_window_suspended = false;

static inline void suspend_enhanced_swing_window() {
  if (!enhanced_swing_window_suspended &&
      should_show_enhanced_swing_window()) {
    close_enhanced_swing_window();
    enhanced_swing_window_suspended = true;
  }
}

static inline void restore_enhanced_swing_window() {
  if (!enhanced_swing_window_suspended) {
    return;
  }
  enhanced_swing_window_suspended = false;
  if (should_show_enhanced_swing_window()) {
    open_enhanced_swing_window();
  }
}

void SeqPage::setup() {}

void SeqPage::check_and_set_page_select() {
  reset_undo();
  uint8_t track_length = seq_page_active_step_track().length();
  if (page_select >= page_count || seq_page_step_offset() >= track_length) {
    page_select = 0;
  }
  ElektronDevice *elektron_dev = midi_device->asElektronDevice();
  if (!active_device_is_md()) {
    DEBUG_PRINTLN("bad device");
  }
  if (elektron_dev != nullptr) {
    elektron_dev->set_seq_page(page_select);
  }
}

void SeqPage::toggle_record() {
  recording = !recording;
  if (recording) {
    enable_record();
  } else {
    disable_record();
  }
}

void SeqPage::enable_record() {
  seq_page_active_step_track().set_step_edit_rec_mode(2);
  recording = true;
  GUI_hardware.led.rec_active = true;
  setLed2();
  oled_display.textbox_P(mclstr_rec);
}

void SeqPage::disable_record() {
  seq_page_active_step_track().set_step_edit_rec_mode(
      mcl.currentPage() == SEQ_STEP_PAGE);
  recording = false;
  GUI_hardware.led.rec_active = false;
  clearLed2();
}

void SeqPage::init() {
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 0;
  disable_record();
  page_count = 4;
  config_encoders();
  seqpage_midi_events.setup_callbacks();

  toggle_device = true;
  DEBUG_PRINTLN("seq page init");

  constexpr uint32_t base_seq_menu_entries =
      menu_entry_mask(SEQ_MENU_TRACK) | menu_entry_mask(SEQ_MENU_SPEED) |
      menu_entry_mask(SEQ_MENU_COPY) | menu_entry_mask(SEQ_MENU_CLEAR_TRACK) |
      menu_entry_mask(SEQ_MENU_PASTE) | menu_entry_mask(SEQ_MENU_SHIFT) |
      menu_entry_mask(SEQ_MENU_REVERSE) |
      menu_entry_mask(SEQ_MENU_TRANSPOSE) | menu_entry_mask(SEQ_MENU_QUANT);
  seq_menu_page.menu.set_enabled_entry_mask(base_seq_menu_entries);
  /*
  if (mcl_cfg.track_select == 1) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, false);
  } else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  }
  */
  last_rec_event = 255;
  last_md_model = MD.kit.models[MD.currentTrack];

  R.Clear();
  R.use_icons_knob();
  R.use_machine_names_short();
  R.use_machine_param_names();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void SeqPage::cleanup() {
  seqpage_midi_events.remove_callbacks();
  note_interface.init_notes();
  close_enhanced_swing_window();
  enhanced_swing_window_suspended = false;
  disable_record();
  GUI_hardware.led.reset_trigleds();
  if (show_seq_menu) {
    encoders[0] = opt_param1_capture;
    encoders[1] = opt_param2_capture;
    show_seq_menu = false;
  }
}

void SeqPage::params_reset() {
  if (!SeqTrackUtil::is_md_device(device_manager.primary_device())) {
    return;
  }
  if (MidiClock.state != 2) {
    MDTrack md_track;
    md_track.machine.model = MD.kit.models[last_primary_track];
    MD.assignMachineBulk(last_primary_track, &md_track.machine, 255, 0, true);
    MD.setTrackParam(last_primary_track, 0, MD.kit.params[last_primary_track][0]);
  }
}

void SeqPage::bootstrap_record() {
  if (mcl.currentPage() != SEQ_STEP_PAGE &&
      mcl.currentPage() != SEQ_EXTSTEP_PAGE &&
      mcl.currentPage() != SEQ_PTC_PAGE) {
    mcl.setPage(SEQ_STEP_PAGE);
  }
  key_interface.send_md_leds(TRIGLED_OVERLAY);
  enable_record();
}

void SeqPage::config_mask_info(bool silent) {
  if (mask_type == MASK_LOCK || mask_type == MASK_LOCKS_ON_STEP) {
    mask_type = MASK_SWING;
  } else if (mask_type > MASK_LOCKS_ON_STEP) {
    mask_type = MASK_PATTERN;
  }
  const char *label =
      reinterpret_cast<const char *>(pgm_read_ptr(&kMaskInfoLabels[mask_type]));
  mclstr_copy_progmem(info2, label, sizeof(info2));
  if (!silent && seq_page_active_step_track().uses_kit_sound()) {
    char str[16] = "EDIT ";
    strcat(str, info2);
    if (mask_type == MASK_PATTERN) {
      close_enhanced_swing_window();
      enhanced_swing_window_suspended = false;
      seq_panel_popup_text((uint8_t)-1, 2);
    } else {
      seq_page_active_step_track().popup_text(str, 1);
      if (mask_type == MASK_SWING) {
        if (show_seq_menu) {
          suspend_enhanced_swing_window();
        } else {
          enhanced_swing_window_suspended = false;
          open_enhanced_swing_window();
        }
      } else {
        close_enhanced_swing_window();
        enhanced_swing_window_suspended = false;
      }
    }
  }
}

void SeqPage::toggle_ext_mask(uint8_t track) {
  uint8_t ext_tracks = SeqTrackUtil::track_count(false);
  if (track > 6) {
    track -= 8;
    if (track >= ext_tracks) {
      return;
    }
    SeqTrackUtil::toggle_mute(false, track);
  } else {
    if (track >= ext_tracks) {
      return;
    }
    select_device_idx(DeviceIdx::Secondary);
    MidiDevice *dev = device_for_seq_idx(DeviceIdx::Secondary);
    select_track(dev, track);
    opt_trackid = last_ext_track + 1;
    auto &active_track = SeqTrackUtil::get_seq_track(false, last_ext_track);
    opt_speed = active_track.speed;
    opt_length = active_track.length;
    opt_channel = ext_track_channel(last_ext_track) + 1;
  }
}

void SeqPage::select_track(MidiDevice *device, uint8_t track, bool send) {
  reset_undo();
  const DeviceIdx device_idx = seq_idx_for_device(device);
  bool is_md_device = idx_is_md_device(device_idx);
#if !defined(__AVR__)
  DeviceContext step_ctx = DeviceContext::for_device(device, device_idx);
  if (!is_md_device &&
      device->step_tracks()->available(step_ctx)) {
    if (track >= device->step_tracks()->track_count(step_ctx)) {
      return;
    }
    last_primary_track = track;
    SeqStepTrackRef base_track = seq_page_active_step_track();
    opt_speed = base_track.speed();
    opt_swing = base_track.swing_amount() + 50;
    opt_length = base_track.length();
    check_and_set_page_select();
    GUI.currentPage()->config();
    return;
  }
#endif
  if (is_md_device) {
    DEBUG_PRINTLN("setting md track");
    opt_undo = 255;
    DEBUG_PRINTLN(track);
    if (track >= NUM_MD_TRACKS) {
      return;
    }
    last_primary_track = track;
    is_midi_model = ((MD.kit.models[last_primary_track] & 0xF0) == MID_01_MODEL);
    auto &base_track = SeqTrackUtil::get_seq_track(true, last_primary_track);
    opt_swing = seq_page_active_step_track().swing_amount() + 50;
    seq_page_active_step_track().sync_step_edit(
        base_track.length, base_track.speed, base_track.step_count);
    check_and_set_page_select();
    if (mcl_cfg.track_select && send) {
      MD.currentTrack = track;
      device->mixer()->select_track(
          DeviceContext::for_device(device, device_idx), track);
    }
  }
#ifdef EXT_TRACKS
  else {
    DEBUG_PRINTLN("setting ext track");
    last_ext_track = min(track, SeqTrackUtil::track_count(false) - 1);
    auto &active_track = SeqTrackUtil::get_seq_track(false, last_ext_track);
    seq_page_active_step_track().sync_step_edit(
        min(active_track.length, 64), active_track.speed,
        active_track.step_count);
  }
#endif
  GUI.currentPage()->config();
  // config_encoders();
}

bool SeqPage::display_mute_mask(MidiDevice *device, uint8_t offset) {
  uint16_t last_mute_mask = mute_mask;

  bool is_md_device = device_is_md(device);
  if (devices_share_physical() && offset > 0 &&
      device == device_manager.secondary_device()) {
    is_md_device = false;
  }
  mute_mask = 0;

  // Hack to display last_ext_track
  if (offset > 0 && !is_md_device) {
    SET_BIT16(mute_mask, last_ext_track);
  }

  SeqTrackUtil::for_each_seq_track(is_md_device,
                                   [&](SeqTrack &seq_track, uint8_t idx) {
                                     if (seq_track.mute_state == SEQ_MUTE_OFF) {
                                       uint8_t d = offset + idx;
                                       if (d < kSeqPageTrackMaskWidth) {
                                         SET_BIT16(mute_mask, d);
                                       }
                                     }
                                   });
  if (last_mute_mask != mute_mask) {
    mcl_gui.set_trigleds(mute_mask,
                         is_md_device ? TRIGLED_MUTE : TRIGLED_EXCLUSIVE);
    return true;
  }
  return false;
}

void SeqPage::capture_seq_menu_values(bool is_md_device) {
  if (seq_page_uses_step_track_ops(is_md_device)) {
    SeqStepTrackRef bt = seq_page_active_step_track();
    opt_trackid = last_primary_track + 1;
    opt_speed = bt.speed();
    opt_swing = bt.swing_amount() + 50;
    opt_length = bt.length();
  } else {
    auto &active_track = SeqTrackUtil::get_seq_track(false, last_ext_track);
    opt_trackid = last_ext_track + 1;
    opt_speed = active_track.speed;
    opt_length = active_track.length;
    opt_channel = ext_track_channel(last_ext_track) + 1;
  }
}

void SeqPage::apply_seq_menu_values(bool same_slot) {
  if (same_slot) {
    opt_speed_handler();
    opt_swing_handler();
    opt_length_handler();
    opt_channel_handler();
  }
}

bool SeqPage::apply_seq_menu_row(uint8_t row_entry, void (*row_func)()) {
  (void)row_entry;
  if (row_func != NULL) {
    row_func();
    return true;
  }
  return false;
}

bool SeqPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    return false;
  }

  if (EVENT_CMD(event)) {
#ifdef MCL_HAS_EXTENDED_PANEL_INPUT
    if (show_seq_menu) {
      suspend_enhanced_swing_window();
      return seq_menu_page.handleEvent(event);
    }
#endif
    if (key_interface.is_key_down(MDX_KEY_PATSONG)) {
      suspend_enhanced_swing_window();
      return seq_menu_page.handleEvent(event);
    }
    uint8_t key = event->source;
    if (note_interface.get_first_md_note() != 255) {
      return false;
    }

    if (event->mask == EVENT_BUTTON_PRESSED &&
        key_interface.is_key_down(MDX_KEY_FUNC)) {
      if (key == MDX_KEY_LEFT || key == MDX_KEY_RIGHT ||
          key == MDX_KEY_UP || key == MDX_KEY_DOWN) {
        return false;
      }
      SeqTrackMenuOp op;
      if (seq_track_menu_op_for_key(key, op)) {
        bool is_md_device = opt_capture_is_md_device();
        if (seq_page_uses_step_track_ops(is_md_device)) {
          SeqStepTrackRef track = seq_page_active_step_track();
          apply_track_menu_op(track, op, 0);
        } else {
          apply_md_track_menu_op(op);
        }
        return true;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_SCALE:
        page_select += 1;
        check_and_set_page_select();
        return true;
      }
    }
  }
  if (EVENT_BUTTON(event)) {
#ifndef MCL_HAS_EXTENDED_PANEL_INPUT
    // A not-ignored WRITE (BUTTON4) release event triggers sequence page select
    if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
      page_select += 1;
      check_and_set_page_select();
      return true;
    }
#endif

    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      // If MD trig is held and BUTTON3 is pressed, launch note menu
      if (!show_seq_menu) {
        suspend_enhanced_swing_window();
        show_seq_menu = true;
        opt_midi_device_capture = midi_device;
        opt_midi_device_idx_capture = current_device_idx();
        bool is_md_device = opt_capture_is_md_device();
        capture_seq_menu_values(is_md_device);

        opt_param1_capture = (MCLEncoder *)encoders[0];
        opt_param2_capture = (MCLEncoder *)encoders[1];
        encoders[0] = &seq_menu_value_encoder;
        encoders[1] = &seq_menu_entry_encoder;
        seq_menu_page.init(false);
        seq_menu_page.gen_menu_device_names();
        mcl_cfg.seq_dev = static_cast<uint8_t>(opt_midi_device_idx_capture);
        return true;
      }
    }

    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
      encoders[0] = opt_param1_capture;
      encoders[1] = opt_param2_capture;
      // oled_display.clearDisplay();
      void (*row_func)() = NULL;
      uint8_t row_entry = 255;
      if (show_seq_menu) {
        row_func =
            seq_menu_page.menu.get_row_function(seq_menu_page.encoders[1]->cur);
        row_entry =
            seq_menu_page.menu.get_item_index(seq_menu_page.encoders[1]->cur);
        MidiDevice *old_dev = midi_device;
        DeviceIdx old_slot = opt_midi_device_idx_capture;
        select_device_idx(static_cast<DeviceIdx>(mcl_cfg.seq_dev));
        opt_midi_device_capture = midi_device;
        opt_midi_device_idx_capture = current_device_idx();
        if (mcl.currentPage() == SEQ_PTC_PAGE) {
          seq_ptc_page.last_midi_device = midi_device;
          seq_ptc_page.last_midi_device_idx = opt_midi_device_idx_capture;
        }
        apply_seq_menu_values(old_dev == midi_device &&
                              old_slot == opt_midi_device_idx_capture);
      }
      if (apply_seq_menu_row(row_entry, row_func)) {
        show_seq_menu = false;
        init();
        restore_enhanced_swing_window();
        return true;
      }
      if (show_seq_menu && seq_menu_page.enter()) {
        show_seq_menu = false;
        restore_enhanced_swing_window();
        return true;
      }

      show_seq_menu = false;
      init_encoders_used_clock();
      init();
      restore_enhanced_swing_window();
      return true;
    }
  }
  return false;
}

void SeqPage::draw_lock_mask(const uint8_t offset, const uint64_t &lock_mask,
                             const uint8_t step_count, const uint8_t length,
                             const bool show_current_step) {
  mcl_gui.draw_leds(MCLGUI::seq_x0, MCLGUI::led_y, offset, lock_mask,
                    step_count, length, show_current_step);
}

void SeqPage::shed_mask(uint64_t &mask, uint8_t length, uint8_t offset) {
  mask <<= (64 - length);
  mask >>= (64 - length);
  mask = mask >> offset;
}

void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  uint64_t mask;
  SeqTrackUtil::with_md_track(last_primary_track, [&](auto &track) {
    track.get_mask(&mask, MASK_LOCKS_ON_STEP);
    shed_mask(mask, track.length, 0);
    draw_lock_mask(offset, mask, track.step_count, track.length,
                   show_current_step);
  });
}

void SeqPage::draw_mask(const uint8_t offset, const uint64_t &pattern_mask,
                        const uint8_t step_count, const uint8_t length,
                        const uint64_t &mute_mask, const uint64_t &slide_mask) {
  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, offset, pattern_mask,
                     step_count, length, mute_mask, slide_mask);
}

void SeqPage::draw_mask(uint8_t offset, uint8_t device,
                        bool show_current_step) {

  if (device == DEVICE_MD) {
    uint64_t mask, mute_mask = 0, slide_mask = 0;
    uint64_t led_mask = 0;
    uint8_t step_count, length;

    SeqTrackUtil::with_md_track(last_primary_track, [&](auto &track) {
      step_count = track.step_count;
      length = track.length;
      track.get_mask(&mask, MASK_PATTERN);

      switch (mask_type) {
      case MASK_PATTERN:
        led_mask = mask;
        mute_mask = track.mute_mask;
        break;
      case MASK_MUTE:
        mute_mask = track.mute_mask;
        led_mask = mute_mask;
        break;
      case MASK_SLIDE:
        track.get_mask(&slide_mask, MASK_SLIDE);
        led_mask = slide_mask;
        break;
      case MASK_SWING:
        track.get_mask(&slide_mask, MASK_SWING);
        led_mask = slide_mask;
        break;
      }
    });

    shed_mask(led_mask, length, offset);
    draw_mask(offset, mask, step_count, length, mute_mask, slide_mask);

    if (recording)
      return;

    uint64_t locks_on_step_mask_ = 0;
    SeqTrackUtil::with_md_track(last_primary_track, [&](auto &track) {
      track.get_mask(&locks_on_step_mask_, MASK_LOCKS_ON_STEP);
    });
    shed_mask(locks_on_step_mask_, length, offset);

    SeqStepTrackRef active_track = seq_page_active_step_track();
    uint16_t led_mask16 = (uint16_t)led_mask;
    uint16_t locks_on_step_mask16 = (uint16_t)locks_on_step_mask_;
    if (led_mask16 != trigled_mask) {
      trigled_mask = led_mask16;
      active_track.set_step_edit_trig_leds(trigled_mask, TRIGLED_STEPEDIT);
      if (mask_type == MASK_MUTE) {
        active_track.set_step_edit_trig_leds(mask, TRIGLED_STEPEDIT, 1);
        GUI_hardware.led.set_trigleds(mask, TRIGLED_STEPEDIT, 1);
      }
    }
    if (locks_on_step_mask16 != locks_on_step_mask) {
      locks_on_step_mask = locks_on_step_mask16;
      active_track.set_step_edit_trig_leds(locks_on_step_mask,
                                           TRIGLED_STEPEDIT, 1);
    }

    GUI_hardware.led.set_trigleds(led_mask16, TRIGLED_STEPEDIT);
    GUI_hardware.led.set_trigleds(locks_on_step_mask, TRIGLED_STEPEDIT, 1);
    if (MidiClock.state == 2) {
      GUI_hardware.led.toggle_trigled(step_count - offset);
    }
  }
}

// from knob value to step value
uint8_t SeqPage::translate_to_step_conditional(uint8_t condition,
                                               /*OUT*/ bool *plock) {
  uint8_t num_cond = seq_page_condition_count();
  if (condition > num_cond) {
    condition = condition - num_cond;
    *plock = true;
  } else {
    *plock = false;
  }
  return condition;
}

// from step value to knob value
uint8_t SeqPage::translate_to_knob_conditional(uint8_t condition,
                                               /*IN*/ bool plock) {
  uint8_t num_cond = seq_page_condition_count();
  if (plock) {
    condition = condition + num_cond;
  }
  return condition;
}

void SeqPage::draw_knob_conditional(uint8_t cond) {
  char K[4];
  conditional_str(K, cond);
  // draw_knob(0, PRG_TO_RAM("COND"), K);
  draw_knob(0, mclstr_cond, K);
}

void SeqPage::conditional_str(char *s, uint8_t c, bool m) {
  if (!s)
    return;

  uint8_t num_cond = seq_page_condition_count();
  bool plock = c > num_cond;

  if (plock)
    c -= num_cond;

  if (seq_page_condition_label(c, plock, m, s)) {
    return;
  }

  // Legacy MD condition names
  static const char PROGMEM ptab[] = "12579"; // probability digits

  char a = 'L', b = '1';
  if (c) {
    if (c <= 8) {
      b = '0' + c; // L2..L8
    } else if (c <= 13) {
      a = 'P'; // P1..P9
      b = pgm_read_byte(&ptab[c - 9]);
    } else if (c == 14) {
      a = '1';
      b = 'S'; // 1S
    }
  }

  s[0] = a;
  s[1] = b;
  uint8_t i = 2;

  if (plock)
    s[i++] = m ? '+' : '^';

  s[i] = '\0';
}

void SeqPage::draw_knob_timing(uint8_t timing, uint8_t timing_mid) {
  char K[5];
  mclstr_copy_progmem(K, mclstr_dash, sizeof(K));

#if !defined(__AVR__)
  if (seq_page_uses_signed_microtiming() || timing_mid == 0) {
    // SPSX: timing param is microtiming mapped to 0..254 (center=127)
    int8_t mt = (int8_t)(timing - 127);
    if (mt < 0) {
      K[0] = '-';
      mcl_gui.put_value_at(-mt, K + 1);
    } else if (mt > 0) {
      K[0] = '+';
      mcl_gui.put_value_at(mt, K + 1);
    }
    K[4] = '\0';
    // mt == 0: keep dashes
    draw_knob(1, mclstr_utim, K);
    return;
  }
#endif

  // Legacy MD timing display
  if (timing != 0) {
    if (timing < timing_mid) {
      mcl_gui.put_value_at(timing_mid - timing, K + 1);
    } else {
      K[0] = '+';
      mcl_gui.put_value_at(timing - timing_mid, K + 1);
    }
  }
  draw_knob(1, mclstr_utim, K);
}

void SeqPage::length_handler(uint8_t length, bool multi) {
  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device)) {
    uint16_t poly_mask = seq_page_active_poly_mask();
    bool is_poly = seq_page_active_step_track_uses_poly_mask() &&
                   poly_mask;
    if (multi) {
      for (uint8_t i = 0; i < seq_page_step_track_count(); i++) {
        seq_page_step_track_for(i).set_length(length);
      }
    } else if (is_poly) {
      for (uint8_t c = 0; poly_mask; c++, poly_mask >>= 1) {
        if (poly_mask & 1) {
          seq_page_step_track_for(c).set_length(length);
        }
      }
    } else {
      seq_page_active_step_track().set_length(length);
    }
    seq_page_active_step_track().sync_step_edit();
    seq_param3.cur = length;
  } else {
#ifdef EXT_TRACKS
    if (multi) {
      SeqTrackUtil::for_each_track(false,
                                   [&](SeqTrackCond &track, uint8_t idx) {
                                     track.set_length(length);
                                     seq_extparam4.cur = length;
                                   });
    } else {
      selected_track(false).set_length(length);
      seq_extparam4.cur = length;
    }
#endif
  }
}

void pattern_len_handler(EncoderParent *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  if (!enc_->hasChanged()) {
    return;
  }
  seq_step_page.length_handler(enc_->cur, BUTTON_DOWN(Buttons.BUTTON4));
  GUI.ignoreNextEvent(Buttons.BUTTON4);
}

void opt_length_handler() { seq_step_page.length_handler(opt_length); }

void opt_channel_handler() {
  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device)) {
  } else {
    set_ext_track_channel(last_ext_track, opt_channel - 1);
  }
}

void opt_mask_handler() {
  if (SeqPage::mask_type == MASK_LOCK ||
      SeqPage::mask_type == MASK_LOCKS_ON_STEP) {
    SeqPage::mask_type = MASK_SWING;
  }
  seq_step_page.config_mask_info(false);
}

void opt_trackid_handler() {
  seq_step_page.select_track(opt_midi_device_capture, opt_trackid - 1);
}

void opt_speed_handler() {

  bool is_md_device = opt_capture_is_md_device();

  if (seq_page_uses_step_track_ops(is_md_device)) {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      for (uint8_t i = 0; i < seq_page_step_track_count(); i++) {
        seq_page_step_track_for(i).request_speed_change(opt_speed);
      }
      GUI.ignoreNextEvent(Buttons.BUTTON4);
    } else {
      SeqStepTrackRef active_track = seq_page_active_step_track();
      if (active_track.request_speed_change(opt_speed)) {
        active_track.sync_step_edit(active_track.length(), opt_speed,
                                    active_track.step_count());
      }
    }
    seq_step_page.config_encoders();
    return;
  }

  if (BUTTON_DOWN(Buttons.BUTTON4)) {
    SeqTrackUtil::for_each_track(is_md_device,
                                 [&](SeqTrackCond &track, uint8_t) {
                                   track.request_speed_change(opt_speed);
                                 });
    GUI.ignoreNextEvent(Buttons.BUTTON4);
  } else {
    auto &active_track = selected_track(is_md_device);
    if (active_track.request_speed_change(opt_speed) && is_md_device) {
      seq_page_active_step_track().sync_step_edit(
          active_track.length, opt_speed, active_track.step_count);
    }
  }

#ifdef EXT_TRACKS
  seq_extstep_page.config_encoders();
#endif
}

void opt_swing_handler() {
  bool is_md_device = opt_capture_is_md_device();
  if (!seq_page_uses_step_track_ops(is_md_device)) {
    return;
  }
  uint8_t amount = opt_swing > 50 ? (uint8_t)(opt_swing - 50) : 0;
  if (amount > 30) {
    amount = 30;
  }
  if (BUTTON_DOWN(Buttons.BUTTON4)) {
    for (uint8_t i = 0; i < seq_page_step_track_count(); i++) {
      seq_page_step_track_for(i).set_swing_amount(amount);
    }
    GUI.ignoreNextEvent(Buttons.BUTTON4);
  } else {
    seq_page_active_step_track().set_swing_amount(amount);
  }
}

void opt_clear_track_handler() {
  uint8_t copy = false;
  bool is_md_device = opt_capture_is_md_device();
  bool use_step_tracks = seq_page_uses_step_track_ops(is_md_device) ||
                         !(mcl.currentPage() == SEQ_PTC_PAGE ||
                           mcl.currentPage() == SEQ_EXTSTEP_PAGE);
  if (opt_undo != 255) {
    if (opt_undo != opt_clear) {
      opt_undo = 255;
      goto COPY;
    }
    opt_paste = opt_clear;
    opt_paste_track_handler();
    return;
  } else {
  COPY:
    copy = true;
  }
  if (use_step_tracks) {
    if (opt_clear == 2) {

      SeqStepTrackRef active_track = seq_page_active_step_track();
      if (active_track.uses_kit_sound()) {
        seq_panel_popup_text(2);
        oled_display.textbox_P(mclstr_clear_md, mclstr_tracks);
      } else {
        oled_display.textbox_P(mclstr_clear, mclstr_tracks);
      }
      oled_display.display();
      uint8_t old_mutes[16];
      for (uint8_t n = 0; n < seq_page_step_track_count(); n++) {
        SeqStepTrackRef track = seq_page_step_track_for(n);
        old_mutes[n] = track.mute_state();
        track.set_mute_state(SEQ_MUTE_ON);
      }
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      for (uint8_t n = 0; n < seq_page_step_track_count(); ++n) {
        SeqStepTrackRef track = seq_page_step_track_for(n);
        track.clear_track();
        track.set_mute_state(old_mutes[n]);
      }
    } else if (opt_clear == 1) {
      uint16_t poly_mask = seq_page_active_poly_mask();
      bool is_poly = seq_page_active_step_track_uses_poly_mask() &&
                     poly_mask;
      SeqStepTrackRef active_track = seq_page_active_step_track();
      if (is_poly) {
        display_popup(mclstr_clear, mclstr_poly_tracks);
      } else if (active_track.uses_kit_sound()) {
        display_popup(mclstr_clear, mclstr_track);
      } else {
        oled_display.textbox_P(mclstr_clear, mclstr_track);
      }
      oled_display.display();
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      if (is_poly) {
        for (uint8_t c = 0; poly_mask; c++, poly_mask >>= 1) {
          if (poly_mask & 1) {
            mcl_clipboard.copy_sequencer_track(c);
            seq_page_step_track_for(c).clear_track();
          }
        }
      } else {
        seq_page_active_step_track().clear_track();
      }
    }
  } else {
    if (copy) {
      opt_copy_track_handler(opt_clear);
    }
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < SeqTrackUtil::track_count(false); n++) {
        clear_ext_track(n);
      }
      oled_display.textbox_P(mclstr_clear, mclstr_ext_tracks);
    } else if (opt_clear == 1) {
      clear_ext_track(last_ext_track);
      oled_display.textbox_P(mclstr_clear, mclstr_ext_track);
    }
  }
  opt_clear = 0;
}

void opt_clear_locks_handler() {

  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device)) {
    if (opt_clear == 2) {
      if (seq_page_active_step_track().uses_kit_sound()) {
        oled_display.textbox_P(mclstr_clear_md, mclstr_locks);
      } else {
        oled_display.textbox_P(mclstr_clear, mclstr_locks);
      }
      for (uint8_t i = 0; i < seq_page_step_track_count(); i++) {
        seq_page_step_track_for(i).clear_locks();
      }
    } else if (opt_clear == 1) {
      oled_display.textbox_P(mclstr_clear, mclstr_locks);
      seq_page_active_step_track().clear_locks();
    }
  } else {
    if (opt_clear == 2) {
      oled_display.textbox_P(mclstr_clear, mclstr_locks);
      clear_ext_track_locks(last_ext_track);
    }
    if (opt_clear == 1) {
      oled_display.textbox_P(mclstr_clear, mclstr_lock);
      if (SeqPage::pianoroll_mode > 0) {
        clear_ext_track_locks(last_ext_track, SeqPage::pianoroll_mode - 1);
      }
    }
    // TODO ext locks
  }
  opt_clear = 0;
}

void opt_clear_all_tracks_handler() {
  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device)) {
  }
#ifdef EXT_TRACKS
  else {
    clear_ext_track(last_ext_track);
  }
#endif
}

void opt_clear_all_locks_handler() {
  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device)) {
  }
#ifdef EXT_TRACKS
  else {
    // TODO ext locks
  }
#endif
}

void opt_copy_track_handler_cb() { opt_copy_track_handler(255); }

void opt_copy_track_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  bool is_md_device = opt_capture_is_md_device();
  bool use_step_tracks = seq_page_uses_step_track_ops(is_md_device);
  if (op != 255) {
    opt_copy = op;
    opt_undo = op;
    silent = true;
  } else {
    const uint8_t copy_slot = use_step_tracks ? 4 : 0;
    SET_BIT(copy_mask, copy_slot + opt_copy);
  }
  DEBUG_PRINTLN("copying");
  DEBUG_PRINTLN(opt_copy);
  DEBUG_PRINTLN(op);
  DEBUG_PRINTLN("/end");
  if (opt_copy == 2) {

    if (use_step_tracks) {
      if (!silent) {
        if (seq_page_active_step_track().uses_kit_sound()) {
          oled_display.textbox_P(mclstr_copy_md, mclstr_tracks);
          seq_panel_popup_text(1);
        } else {
          oled_display.textbox_P(mclstr_copy, mclstr_tracks);
        }
        oled_display.display();
      }
      mcl_clipboard.copy_sequencer();
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
        oled_display.textbox_P(mclstr_copy_ext, mclstr_tracks);
        oled_display.display();
      }
      mcl_clipboard.copy_sequencer(NUM_MD_TRACKS);
    }
#endif
  }
  if (opt_copy == 1) {
    if (use_step_tracks) {
      if (!silent) {
        oled_display.textbox_P(mclstr_copy, mclstr_track);
        if (seq_page_active_step_track().uses_kit_sound()) {
          seq_panel_popup_text(4);
        }
      }
      mcl_clipboard.copy_track = last_primary_track;
      mcl_clipboard.copy_sequencer_track(last_primary_track);
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
        seq_panel_popup_text_P(mclstr_copy_ext);
        oled_display.textbox_P(mclstr_copy, mclstr_ext_track);
      }
      mcl_clipboard.copy_track = last_ext_track + NUM_MD_TRACKS;
      mcl_clipboard.copy_sequencer_track(last_ext_track + NUM_MD_TRACKS);
    }
#endif
  }
  opt_copy = 0;
}

void opt_paste_track_handler() {
  bool undo = false;
  bool is_md_device = opt_capture_is_md_device();
  bool use_step_tracks = seq_page_uses_step_track_ops(is_md_device);
  if (opt_undo != 255) {
    undo = true;
  } else {
    const uint8_t copy_slot = use_step_tracks ? 4 : 0;
    if (!IS_BIT_SET(copy_mask, copy_slot + opt_paste)) {
      return;
    }
  }
  if (opt_paste == 2) {

    if (use_step_tracks) {
      if (!undo) {
        if (seq_page_active_step_track().uses_kit_sound()) {
          oled_display.textbox_P(mclstr_paste_md, mclstr_tracks);
          seq_panel_popup_text(3);
        } else {
          oled_display.textbox_P(mclstr_paste, mclstr_tracks);
        }
        oled_display.display();
      } else {
        oled_display.textbox_P(mclstr_undo, mclstr_tracks);
        oled_display.display();
        if (seq_page_active_step_track().uses_kit_sound()) {
          seq_panel_popup_text(22);
        }
      }
      mcl_clipboard.paste_sequencer();
    } else {
      if (!undo) {
        display_popup(mclstr_paste_ext, mclstr_tracks);
      } else {
        display_popup(mclstr_undo, mclstr_ext_tracks);
      }
      oled_display.display();
      mcl_clipboard.paste_sequencer(NUM_MD_TRACKS);
    }
  }
  if (opt_paste == 1) {
    if (use_step_tracks) {
      bool is_poly = false;
      if (!undo) {
        oled_display.textbox_P(mclstr_paste, mclstr_track);
        if (seq_page_active_step_track().uses_kit_sound()) {
          seq_panel_popup_text(6);
        }
      } else {
        oled_display.textbox_P(mclstr_undo, mclstr_track);
        uint16_t poly_mask = seq_page_active_poly_mask();
        is_poly = seq_page_active_step_track_uses_poly_mask() &&
                  poly_mask;
        if (seq_page_active_step_track().uses_kit_sound()) {
          seq_panel_popup_text(23);
        }
      }
      if (is_poly) {
        uint16_t poly_mask = seq_page_active_poly_mask();
        for (uint8_t c = 0; poly_mask; c++, poly_mask >>= 1) {
          if (poly_mask & 1) {
            mcl_clipboard.paste_sequencer_track(c, c);
          }
        }
      } else {
        mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                            last_primary_track);
      }
    } else {
      if (!undo) {
        display_popup(mclstr_paste, mclstr_ext_track);
      } else {
        display_popup(mclstr_undo, mclstr_ext_track);
      }
      mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                          last_ext_track + NUM_MD_TRACKS);
    }
  }
  opt_undo = 255;
  opt_paste = 0;
}

void opt_clear_page_handler() {
  if (opt_undo != 255) {
    if (opt_undo != PAGE_UNDO) {
      opt_undo = 255;
      goto CLEAR;
    }
    opt_paste_page_handler();
    return;
  } else {
  CLEAR:
    opt_copy_page_handler(PAGE_UNDO);
  }
  SeqStepTrackRef track = seq_page_active_step_track();
  oled_display.textbox_P(mclstr_clear, mclstr_page);
  if (track.uses_kit_sound()) {
    seq_panel_popup_text(57);
  }
  for (uint8_t n = 0; n < kSeqPageVisibleSteps; n++) {
    uint8_t step = seq_page_visible_step(n);
    if (step >= track.length())
      return;
    track.clear_step(step);
  }
  track.clean_params();
}

void opt_copy_page_handler_cb() { opt_copy_page_handler(255); }

void opt_copy_page_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_undo = op;
    silent = true;
  }

  SeqStepTrackRef track = seq_page_active_step_track();
  if (!silent) {
    oled_display.textbox_P(mclstr_copy, mclstr_page);
    if (track.uses_kit_sound()) {
      seq_panel_popup_text(54);
    }
  }
  uint8_t track_len = track.length();
  for (uint8_t n = 0; n < kSeqPageVisibleSteps; n++) {
    uint8_t step = seq_page_visible_step(n);
    if (step >= track_len)
      return;
    copy_step_to_clipboard(track, step, n);
  }
}

void opt_paste_page_handler() {
  SeqStepTrackRef track = seq_page_active_step_track();
  if (opt_undo == PAGE_UNDO) {
    opt_undo = 255;
    oled_display.textbox_P(mclstr_undo, mclstr_page);
    if (track.uses_kit_sound()) {
      seq_panel_popup_text(55);
    }
  } else {
    oled_display.textbox_P(mclstr_paste, mclstr_page);
    if (track.uses_kit_sound()) {
      seq_panel_popup_text(56);
    }
  }
  uint8_t track_len = track.length();
  for (uint8_t n = 0; n < kSeqPageVisibleSteps; n++) {
    uint8_t step = seq_page_visible_step(n);
    if (step >= track_len)
      return;
    paste_step_from_clipboard(track, step, n);
  }
}

void opt_clear_step_handler() {
  if (opt_undo != 255) {
    if (opt_undo != STEP_UNDO) {
      opt_undo = 255;
      goto CLEAR;
    }
    opt_paste_step_handler();
    return;
  } else {
  CLEAR:
    opt_copy_step_handler(STEP_UNDO);
  }
  uint8_t step = seq_page_visible_step(SeqPage::step_select);
  SeqStepTrackRef track = seq_page_active_step_track();
  oled_display.textbox_P(mclstr_clear_step);
  if (track.uses_kit_sound()) {
    seq_panel_popup_text_P(mclstr_clear_step);
  }
  track.clear_step(step);
  track.clean_params();
}

void opt_copy_step_handler_cb() { opt_copy_step_handler(255); }

void opt_copy_step_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_undo = op;
    silent = true;
  }
  SeqStepTrackRef track = seq_page_active_step_track();
  if (!silent) {
    oled_display.textbox_P(mclstr_copy_step);
    if (track.uses_kit_sound()) {
      seq_panel_popup_text_P(mclstr_copy_step);
    }
  }
  uint8_t step = seq_page_visible_step(SeqPage::step_select);
  copy_step_to_clipboard(track, step, 0);
}

void opt_paste_step_handler() {
  SeqStepTrackRef track = seq_page_active_step_track();
  if (opt_undo == STEP_UNDO) {
    opt_undo = 255;
    oled_display.textbox_P(mclstr_undo_step);
    if (track.uses_kit_sound()) {
      seq_panel_popup_text_P(mclstr_undo_step);
    }
  } else {
    oled_display.textbox_P(mclstr_paste_step);
    if (track.uses_kit_sound()) {
      seq_panel_popup_text_P(mclstr_paste_step);
    }
  }
  uint8_t step = seq_page_visible_step(SeqPage::step_select);
  paste_step_from_clipboard(track, step, 0);
}

void opt_mute_step_handler() {
  SeqStepTrackRef track = seq_page_active_step_track();
  for (uint8_t n = 0; n < kSeqPageVisibleSteps; n++) {
    if (note_interface.is_note_on(n)) {
      uint8_t s = seq_page_visible_step(n);
      track.toggle_mute(s);
    }
  }
}

void opt_clear_step_locks_handler() {
  if (opt_clear_step == 1) {
    oled_display.textbox_P(mclstr_clear, mclstr_locks);
    bool is_md_device = opt_capture_is_md_device();
    if (seq_page_uses_step_track_ops(is_md_device) &&
        seq_page_active_step_track().uses_kit_sound()) {
      seq_panel_popup_text(14);
    }
  }
  for (uint8_t n = 0; n < kSeqPageVisibleSteps; n++) {
    if (note_interface.is_note_on(n)) {

      bool is_md_device = opt_capture_is_md_device();
      if (seq_page_uses_step_track_ops(is_md_device)) {
        uint8_t s = seq_page_visible_step(n);
        seq_page_active_step_track().clear_step_locks(s);
      }
    }
  }
  opt_clear_step = 0;
}

void opt_shift_track_handler() {
  bool is_md_device = opt_capture_is_md_device();
  switch (opt_shift) {
  case 1:
    apply_track_menu_op(is_md_device, false, SEQ_TRACK_OP_ROTATE_LEFT);
    break;
  case 2:
    apply_track_menu_op(is_md_device, false, SEQ_TRACK_OP_ROTATE_RIGHT);
    break;
  case 3:
    apply_track_menu_op(is_md_device, true, SEQ_TRACK_OP_ROTATE_LEFT);
    break;
  case 4:
    apply_track_menu_op(is_md_device, true, SEQ_TRACK_OP_ROTATE_RIGHT);
    break;
  }
}
void opt_reverse_track_handler() {

  if (opt_reverse == 0) {
    return;
  }
  bool is_md_device = opt_capture_is_md_device();
  bool apply_all = opt_reverse == 2;
  apply_track_menu_op(is_md_device, apply_all, SEQ_TRACK_OP_REVERSE);
}

void opt_transpose_track_handler() {
  bool is_all = opt_transpose >= 25;
  int8_t transpose_value = is_all ? (opt_transpose - 37) : (opt_transpose - 12);
  if (transpose_value == 0) {
    return;
  }
  oled_display.textbox_P(mclstr_transpose);
  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device) &&
      seq_page_active_step_track().uses_kit_sound()) {
    seq_panel_popup_text_P(mclstr_transpose);
  }
  apply_track_menu_op(is_md_device, is_all, SEQ_TRACK_OP_TRANSPOSE,
                      transpose_value);
}

void seq_menu_handler() {}

void SeqPage::config_as_trackedit() {

  constexpr uint8_t clear_track_mask = 1 << (SEQ_MENU_CLEAR_TRACK & 7);
  constexpr uint8_t clear_locks_mask = 1 << (SEQ_MENU_CLEAR_LOCKS & 7);
  seq_menu_page.menu.disabled_entry_mask[SEQ_MENU_CLEAR_TRACK >> 3] &=
      ~clear_track_mask;
  seq_menu_page.menu.disabled_entry_mask[SEQ_MENU_CLEAR_LOCKS >> 3] |=
      clear_locks_mask;
}

void SeqPage::config_as_lockedit() {

  constexpr uint8_t clear_track_mask = 1 << (SEQ_MENU_CLEAR_TRACK & 7);
  constexpr uint8_t clear_locks_mask = 1 << (SEQ_MENU_CLEAR_LOCKS & 7);
  seq_menu_page.menu.disabled_entry_mask[SEQ_MENU_CLEAR_TRACK >> 3] |=
      clear_track_mask;
  seq_menu_page.menu.disabled_entry_mask[SEQ_MENU_CLEAR_LOCKS >> 3] &=
      ~clear_locks_mask;
}

bool SeqPage::md_track_change_check() {
  if (last_primary_track != MD.currentTrack ||
      last_md_model != MD.kit.models[MD.currentTrack]) {
    last_md_model = MD.kit.models[MD.currentTrack];
    select_track(&MD, MD.currentTrack, false);
    return true;
  }
  return false;
}

void SeqPage::loop() {

  opt_midi_device_capture = midi_device;
  opt_midi_device_idx_capture = current_device_idx();

  if (last_midi_state != MidiClock.state) {
    last_midi_state = MidiClock.state;
    DEBUG_DUMP("hii")
  }

  //  md_track_change_check();

  if (show_seq_menu) {
    suspend_enhanced_swing_window();
    seq_menu_page.loop();
    if (!seq_page_uses_non_md_primary_step_tracks() &&
        !opt_capture_is_md_device() && opt_trackid > NUM_EXT_TRACKS) {
      // lock trackid to [1..4]
      opt_trackid = min(opt_trackid, NUM_EXT_TRACKS);
      seq_menu_value_encoder.cur = opt_trackid;
    }
    return;
  }
}

void SeqPage::draw_page_index(bool show_page_index, uint8_t _playing_idx) {
  //  draw page index
  uint8_t pidx_x = pidx_x0;
  bool blink = MidiClock.getBlinkHint(true);

  uint8_t playing_idx = _playing_idx;
  if (_playing_idx == 255) {
    uint8_t step_count;
    if (seq_page_uses_non_md_primary_step_tracks()) {
      step_count = seq_page_active_step_track().step_count();
    } else if (active_device_is_md()) {
      step_count = SeqTrackUtil::get_seq_track(true, last_primary_track).step_count;
    }
#ifdef EXT_TRACKS
    else {
      step_count =
          SeqTrackUtil::get_seq_track(false, last_ext_track).step_count;
    }
#endif
    playing_idx = (step_count % 16) / 4;
  }
  uint8_t w = pidx_w;

  for (uint8_t i = 0; i < page_count; ++i) {
    oled_display.drawRect(pidx_x, pidx_y, w, pidx_h, WHITE);

    // highlight page_select
    if ((page_select == i) && (show_page_index)) {
      oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, w - 2, WHITE);
    }

    // blink playing_idx
    if (playing_idx == i && blink) {
      if ((page_select == i) && (show_page_index)) {
        oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, w - 2, BLACK);
      } else {
        oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, w - 2, WHITE);
      }
    }

    pidx_x += w + 1;
  }
}
//  ref: design/Sequencer.png
void SeqPage::display() {

  bool is_md = active_device_is_md();
  if (seq_page_uses_non_md_primary_step_tracks()) {
    is_md = true;
  }
  uint8_t track_id = last_primary_track;
  if (!is_md) {
    track_id = last_ext_track;
  }
  track_id += 1;

  //  draw current active track
  mcl_gui.draw_panel_number(track_id);

  mcl_gui.draw_panel_toggle(seq_device_idx_name(DeviceIdx::Primary),
                            seq_device_idx_name(DeviceIdx::Secondary),
                            current_device_idx() == DeviceIdx::Primary);
  //  draw stop/play/rec state
  mcl_gui.draw_panel_status(recording, MidiClock.state == 2);

  if (display_page_index) {
    draw_page_index();
  }
  //  draw info lines
  mcl_gui.draw_panel_labels(info1, info2);

  if (show_seq_menu) {
    constexpr uint8_t width = 52;
    oled_display.setFont(&TomThumb);
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    seq_menu_page.draw_menu(128 - width, 8, width);
  }
}

void SeqPage::draw_knob_frame() {
  mcl_gui.draw_knob_frame();
  // draw frame
}

void SeqPage::draw_knob(uint8_t i, const char *title, const char *text) {
  mcl_gui.draw_knob(i, title, text);
}

void SeqPage::draw_knob(uint8_t i, Encoder *enc, const char *title) {
  mcl_gui.draw_knob(i, enc, title);
}

void SeqPageMidiEvents::onMidiStartCallback(uint32_t clock_count) {
  (void)clock_count;
  if (SeqPage::recording) {
    oled_display.textbox_P(mclstr_rec);
  }
}

void SeqPageMidiEvents::setup_callbacks() {
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&SeqPageMidiEvents::onMidiStartCallback);
}

void SeqPageMidiEvents::remove_callbacks() {
  MidiClock.removeOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&SeqPageMidiEvents::onMidiStartCallback);
}
