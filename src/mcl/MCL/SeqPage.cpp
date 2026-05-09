#include "SeqPage.h"
#include "SeqPages.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/A4/A4.h"
#include "ResourceManager.h"
#include "MCLClipBoard.h"
#include "MDTrack.h"
#include "MCLStrings.h"
#include "SeqStepTrackApi.h"
#include "SeqTrackUtil.h"
#ifdef PLATFORM_TBD
#include "MidiSetup.h"
#include "../Drivers/TBD/TBD.h"
#endif

namespace {

#if defined(PLATFORM_TBD)
bool seq_page_uses_tbd_step_tracks() {
  PageIndex page = mcl.currentPage();
  if ((page == SEQ_STEP_PAGE || page == SEQ_PTC_PAGE) &&
      mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    return true;
  }
  return false;
}
#else
bool seq_page_uses_tbd_step_tracks() { return false; }
#endif

#if !defined(__AVR__)
bool seq_page_uses_stepseq_conditions() {
#if defined(PLATFORM_TBD)
  if (seq_step_api_uses_tbd_tracks() && !seq_page_uses_tbd_step_tracks()) {
    return false;
  }
#endif
  return seq_step_api_active_track(seq_page_uses_tbd_step_tracks()).is_stepseq();
}
#endif

} // namespace

uint8_t SeqPage::page_select = 0;

MidiDevice *SeqPage::midi_device = &MD;
MidiDevice *opt_midi_device_capture;
uint8_t opt_midi_device_slot_capture = 1;

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

static inline uint8_t normalized_seq_slot(uint8_t slot) {
  return slot == 2 ? 2 : 1;
}

#if defined(__AVR__)
constexpr bool kSeqSlotsCanSharePhysicalDevice = false;
#else
constexpr bool kSeqSlotsCanSharePhysicalDevice = true;
#endif

MidiDevice *SeqPage::device_for_seq_slot(uint8_t slot) {
  return normalized_seq_slot(slot) == 2 ? device_manager.secondary_device()
                                       : device_manager.primary_device();
}

bool SeqPage::devices_share_physical() {
  return kSeqSlotsCanSharePhysicalDevice &&
         device_manager.primary_device() == device_manager.secondary_device();
}

uint8_t SeqPage::current_device_slot() {
#ifdef EXT_TRACKS
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
    return 2;
  }
#endif
#if defined(PLATFORM_TBD)
  if (seq_page_uses_tbd_step_tracks()) {
    return 1;
  }
#endif
  if (devices_share_physical()) {
    return normalized_seq_slot(mcl_cfg.seq_dev);
  }
  return midi_device == device_manager.secondary_device() ? 2 : 1;
}

bool SeqPage::slot_is_md_device(uint8_t slot) {
  slot = normalized_seq_slot(slot);
  if (devices_share_physical() && slot == 2) {
    return false;
  }
  return SeqTrackUtil::is_md_device(device_for_seq_slot(slot));
}

bool SeqPage::device_is_md(MidiDevice *device) {
  if (devices_share_physical() && device == device_manager.primary_device()) {
    return slot_is_md_device(current_device_slot());
  }
  return SeqTrackUtil::is_md_device(device);
}

bool SeqPage::active_device_is_md() {
  return slot_is_md_device(current_device_slot());
}

void SeqPage::select_device_slot(uint8_t slot) {
  mcl_cfg.seq_dev = normalized_seq_slot(slot);
  midi_device = device_for_seq_slot(mcl_cfg.seq_dev);
  opt_midi_device_capture = midi_device;
  opt_midi_device_slot_capture = mcl_cfg.seq_dev;
}

static inline uint8_t seq_slot_for_device(MidiDevice *device) {
  if (!SeqPage::devices_share_physical() &&
      device == device_manager.secondary_device()) {
    return 2;
  }
  if (SeqPage::devices_share_physical() &&
      device == device_manager.primary_device()) {
    return SeqPage::current_device_slot();
  }
  return 1;
}

static inline bool opt_capture_is_md_device() {
  if (SeqPage::devices_share_physical() &&
      opt_midi_device_capture == device_manager.primary_device()) {
    return SeqPage::slot_is_md_device(opt_midi_device_slot_capture);
  }
  return SeqTrackUtil::is_md_device(opt_midi_device_capture);
}

static inline void seq_copy_device_slot_name(uint8_t slot, char *dst,
                                             uint8_t len) {
  if (len == 0) return;
  if (SeqPage::devices_share_physical()) {
    strncpy(dst, normalized_seq_slot(slot) == 2 ? "TB2" : "TB1", len);
  } else {
    strncpy(dst, SeqPage::device_for_seq_slot(slot)->name, len);
  }
  dst[len - 1] = '\0';
}

uint8_t opt_speed = 1;
uint8_t opt_trackid = 1;
uint8_t opt_copy = 0;
uint8_t opt_paste = 0;
uint8_t opt_clear = 0;
uint8_t opt_shift = 0;
uint8_t opt_reverse = 0;
uint8_t opt_transpose = 12;
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

MusicalNotes number_to_note;

uint8_t copy_mask = 0;

static inline uint8_t selected_track_index(bool is_md_device) {
#ifdef EXT_TRACKS
  return is_md_device ? last_md_track : last_ext_track;
#else
  (void)is_md_device;
  return last_md_track;
#endif
}

static inline SeqTrackCond &selected_track(bool is_md_device) {
  return SeqTrackUtil::get_track(is_md_device,
                                 selected_track_index(is_md_device));
}

static inline bool seq_page_uses_step_track_ops(bool is_md_device) {
  return is_md_device || seq_page_uses_tbd_step_tracks();
}

static inline SeqStepTrackApi seq_page_step_track_for(uint8_t track) {
  return seq_step_api_track_for(track, seq_page_uses_tbd_step_tracks());
}

static inline SeqStepTrackApi seq_page_active_step_track() {
  return seq_step_api_active_track(seq_page_uses_tbd_step_tracks());
}

static inline uint8_t seq_page_step_track_count() {
  return seq_step_api_track_count(seq_page_uses_tbd_step_tracks());
}

enum SeqTrackMenuOp : uint8_t {
  SEQ_TRACK_OP_ROTATE_LEFT,
  SEQ_TRACK_OP_ROTATE_RIGHT,
  SEQ_TRACK_OP_REVERSE,
  SEQ_TRACK_OP_TRANSPOSE,
};

static inline void apply_track_menu_op(SeqStepTrackApi &track,
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

static inline void apply_track_menu_op(bool is_md_device, bool apply_all,
                                       SeqTrackMenuOp op,
                                       int8_t offset = 0) {
  if (seq_page_uses_step_track_ops(is_md_device)) {
    uint8_t len = apply_all ? seq_page_step_track_count() : 1;
    uint8_t start = apply_all ? 0 : last_md_track;
    for (uint8_t i = start; i < start + len; i++) {
      auto track = seq_page_step_track_for(i);
      apply_track_menu_op(track, op, offset);
    }
  } else if (apply_all) {
    uint8_t len = SeqTrackUtil::track_count(is_md_device);
    for (uint8_t i = 0; i < len; ++i) {
      apply_track_menu_op(SeqTrackUtil::get_track(is_md_device, i), op,
                          offset);
    }
  } else {
    apply_track_menu_op(selected_track(is_md_device), op, offset);
  }
}

static inline uint8_t ext_track_channel(uint8_t track) {
#if defined(PLATFORM_TBD)
  if (SeqTrackUtil::use_midi_tracks_for_ext()) {
    return mcl_seq.midi_tracks[track].channel();
  }
#endif
  return mcl_seq.ext_tracks[track].channel;
}

static inline void set_ext_track_channel(uint8_t track, uint8_t channel) {
#if defined(PLATFORM_TBD)
  if (SeqTrackUtil::use_midi_tracks_for_ext()) {
    if (mcl_seq.midi_tracks[track].channel() != channel) {
      mcl_seq.midi_tracks[track].buffer_notesoff();
      mcl_seq.midi_tracks[track].set_channel(channel);
    }
    return;
  }
#endif
  if (mcl_seq.ext_tracks[track].channel != channel) {
    mcl_seq.ext_tracks[track].buffer_notesoff();
    mcl_seq.ext_tracks[track].channel = channel;
  }
}

static inline void clear_ext_track(uint8_t track) {
  SeqTrackUtil::get_track(false, track).clear_track();
}

static inline void clear_ext_track_locks(uint8_t track) {
#if defined(PLATFORM_TBD)
  if (SeqTrackUtil::use_midi_tracks_for_ext()) {
    mcl_seq.midi_tracks[track].clear_track_locks();
    return;
  }
#endif
  mcl_seq.ext_tracks[track].clear_track_locks();
}

static inline void clear_ext_track_locks(uint8_t track, uint8_t lock_idx) {
#if defined(PLATFORM_TBD)
  if (SeqTrackUtil::use_midi_tracks_for_ext()) {
    mcl_seq.midi_tracks[track].clear_track_locks(lock_idx);
    return;
  }
#endif
  mcl_seq.ext_tracks[track].clear_track_locks(lock_idx);
}

static inline void copy_step_to_clipboard(SeqStepTrackApi track, uint8_t step,
                                          uint8_t clip_idx) {
#if !defined(__AVR__)
  track.copy_step(step, &mcl_clipboard.steps[clip_idx],
                  &mcl_clipboard.spsx_steps[clip_idx]);
#else
  track.copy_step(step, &mcl_clipboard.steps[clip_idx]);
#endif
}

static inline void paste_step_from_clipboard(SeqStepTrackApi track,
                                             uint8_t step,
                                             uint8_t clip_idx) {
#if !defined(__AVR__)
  track.paste_step(step, &mcl_clipboard.steps[clip_idx],
                   &mcl_clipboard.spsx_steps[clip_idx]);
#else
  track.paste_step(step, &mcl_clipboard.steps[clip_idx]);
#endif
}

static inline void display_popup(const char *str_P) {
  oled_display.textbox_P(str_P);
  MD.popup_text_P(str_P);
}

static inline void display_popup(const char *str1_P, const char *str2_P) {
  oled_display.textbox_P(str1_P, str2_P);
  MD.popup_text_P(str1_P, str2_P);
}

void SeqPage::setup() {}

void SeqPage::check_and_set_page_select() {
  reset_undo();
  uint8_t track_length = seq_page_active_step_track().length();
  if (page_select >= page_count || page_select * 16 >= track_length) {
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
  MD.set_rec_mode(2);
  recording = true;
  GUI_hardware.led.rec_active = true;
  setLed2();
  oled_display.textbox_P(mclstr_rec);
}

void SeqPage::disable_record() {
  MD.set_rec_mode((mcl.currentPage() == SEQ_STEP_PAGE));
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

  seq_menu_page.menu.enable_entry(SEQ_MENU_DEVICE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CHANNEL, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_MASK, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_KEY, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PIANOROLL, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_POLY, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SOUND, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_AUTOMATION, false);

  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_MD, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_EXT, false);
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
    md_track.machine.model = MD.kit.models[last_md_track];
    MD.assignMachineBulk(last_md_track, &md_track.machine, 255, 0, true);
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
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
  switch (mask_type) {
  case MASK_PATTERN:
    mclstr_copy_progmem(info2, mclstr_trig, sizeof(info2));
    break;
  case MASK_LOCK:
    mclstr_copy_progmem(info2, mclstr_lock, sizeof(info2));
    break;
  case MASK_SLIDE:
    mclstr_copy_progmem(info2, mclstr_slide, sizeof(info2));
    break;
  case MASK_MUTE:
    mclstr_copy_progmem(info2, mclstr_mute, sizeof(info2));
    break;
  }
  if (!silent && seq_page_active_step_track().uses_md_sound()) {
    char str[16] = "EDIT ";
    strcat(str, info2);
    if (mask_type == MASK_PATTERN) {
      MD.popup_text(-1, 2);
    } else {
      MD.popup_text(str, 1);
    }
  }
}

void SeqPage::toggle_ext_mask(uint8_t track) {
  if (track > 6) {
    track -= 8;
    if (track >= mcl_seq.num_ext_tracks) {
      return;
    }
    SeqTrackUtil::get_track(false, track).toggle_mute();
  } else {
    if (track >= mcl_seq.num_ext_tracks) {
      return;
    }
    select_device_slot(2);
    MidiDevice *dev = device_for_seq_slot(2);
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
  const uint8_t device_slot = seq_slot_for_device(device);
  bool is_md_device = slot_is_md_device(device_slot);
#if defined(PLATFORM_TBD)
  if (device == &TBD && seq_page_uses_tbd_step_tracks()) {
    if (track >= mcl_seq.num_tbd_tracks) {
      return;
    }
    last_md_track = track;
    SeqStepTrackApi base_track = seq_page_active_step_track();
    opt_speed = base_track.speed();
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
    last_md_track = track;
    is_midi_model = ((MD.kit.models[last_md_track] & 0xF0) == MID_01_MODEL);
    auto &base_track = SeqTrackUtil::get_seq_track(true, last_md_track);
    MD.sync_seqtrack(base_track.length, base_track.speed,
                     base_track.step_count);
    check_and_set_page_select();
    if (mcl_cfg.track_select && send) {
      MD.currentTrack = track;
      MD.setStatus(0x22, track);
    }
  }
#ifdef EXT_TRACKS
  else {
    DEBUG_PRINTLN("setting ext track");
    last_ext_track = min(track, NUM_EXT_TRACKS - 1);
    auto &active_track = SeqTrackUtil::get_seq_track(false, last_ext_track);
    MD.sync_seqtrack(min(active_track.length, 64), active_track.speed,
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
          if (d < 16) {
            SET_BIT16(mute_mask, d);
          }
        }
      });
  if (last_mute_mask != mute_mask) {
    mcl_gui.set_trigleds(mute_mask, is_md_device ? TRIGLED_MUTE : TRIGLED_EXCLUSIVE);
    return true;
  }
  return false;
}

bool SeqPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    return false;
  }

  if (EVENT_CMD(event)) {
#ifdef PLATFORM_TBD
    if (show_seq_menu) {
      return seq_menu_page.handleEvent(event);
    }
#endif
    if (key_interface.is_key_down(MDX_KEY_PATSONG)) {
      return seq_menu_page.handleEvent(event);
    }
    uint8_t key = event->source;
    uint8_t first_note = note_interface.get_first_md_note();
    if (first_note != 255) {
      return false;
    }

    if (event->mask == EVENT_BUTTON_PRESSED &&
        key_interface.is_key_down(MDX_KEY_FUNC)) {
      bool is_md_device = opt_capture_is_md_device();
      if (seq_page_uses_step_track_ops(is_md_device)) {
        SeqStepTrackApi track = seq_page_active_step_track();
        switch (key) {
        case MDX_KEY_LEFT:
          track.rotate_left();
          return true;
        case MDX_KEY_RIGHT:
          track.rotate_right();
          return true;
        case MDX_KEY_UP:
          track.reverse();
          return true;
        }
      }
      switch (key) {
      case MDX_KEY_LEFT:
        SeqTrackUtil::with_md_track(last_md_track, [](auto &t) { t.rotate_left(); });
        return true;
      case MDX_KEY_RIGHT:
        SeqTrackUtil::with_md_track(last_md_track, [](auto &t) { t.rotate_right(); });
        return true;
      case MDX_KEY_UP:
        SeqTrackUtil::with_md_track(last_md_track, [](auto &t) { t.reverse(); });
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
#ifndef PLATFORM_TBD
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
        show_seq_menu = true;
        opt_midi_device_capture = midi_device;
        opt_midi_device_slot_capture = current_device_slot();
        bool is_md_device = opt_capture_is_md_device();
        if (seq_page_uses_step_track_ops(is_md_device)) {
          SeqStepTrackApi bt = seq_page_active_step_track();
          opt_trackid = last_md_track + 1;
          opt_speed = bt.speed();
          opt_length = bt.length();
        } else {
          auto &active_track = SeqTrackUtil::get_seq_track(false, last_ext_track);
          opt_trackid = last_ext_track + 1;
          opt_speed = active_track.speed;
          opt_length = active_track.length;
          opt_channel = ext_track_channel(last_ext_track) + 1;
        }

        opt_param1_capture = (MCLEncoder *)encoders[0];
        opt_param2_capture = (MCLEncoder *)encoders[1];
        encoders[0] = &seq_menu_value_encoder;
        encoders[1] = &seq_menu_entry_encoder;
        seq_menu_page.init();
        seq_menu_page.gen_menu_device_names();
        seq_menu_page.gen_menu_transpose_names();
        mcl_cfg.seq_dev = opt_midi_device_slot_capture;
        return true;
      }
    }

    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
      encoders[0] = opt_param1_capture;
      encoders[1] = opt_param2_capture;
      // oled_display.clearDisplay();
      void (*row_func)();
      if (show_seq_menu) {
        row_func =
            seq_menu_page.menu.get_row_function(seq_menu_page.encoders[1]->cur);
        MidiDevice *old_dev = midi_device;
        uint8_t old_slot = opt_midi_device_slot_capture;
        select_device_slot(mcl_cfg.seq_dev);
        opt_midi_device_capture = midi_device;
        opt_midi_device_slot_capture = current_device_slot();
        if (mcl.currentPage() == SEQ_PTC_PAGE) {
          seq_ptc_page.last_midi_device = midi_device;
          seq_ptc_page.last_midi_device_slot = opt_midi_device_slot_capture;
        }
        if (old_dev == midi_device && old_slot == opt_midi_device_slot_capture) {
          opt_speed_handler();
          opt_length_handler();
          opt_channel_handler();
        }
      }
      if (row_func != NULL) {
        row_func();
        show_seq_menu = false;
        init();
        return true;
      }
      if (show_seq_menu && seq_menu_page.enter()) {
        show_seq_menu = false;
        return true;
      }

      show_seq_menu = false;
      init_encoders_used_clock();
      init();
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
  SeqTrackUtil::with_md_track(last_md_track, [&](auto &track) {
    track.get_mask(&mask, MASK_LOCKS_ON_STEP);
    shed_mask(mask, track.length, 0);
    draw_lock_mask(offset, mask, track.step_count, track.length, show_current_step);
  });
}

void SeqPage::draw_mask(const uint8_t offset, const uint64_t &pattern_mask,
                        const uint8_t step_count, const uint8_t length,
                        const uint64_t &mute_mask, const uint64_t &slide_mask,
                        const bool show_current_step) {
  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, offset, pattern_mask,
                     step_count, length, mute_mask, slide_mask);
}

void SeqPage::draw_mask(uint8_t offset, uint8_t device,
                        bool show_current_step) {

  if (device == DEVICE_MD) {
    uint64_t mask, lock_mask, mute_mask = 0, slide_mask = 0;
    uint64_t led_mask = 0;
    uint8_t step_count, length;

    SeqTrackUtil::with_md_track(last_md_track, [&](auto &track) {
      step_count = track.step_count;
      length = track.length;
      track.get_mask(&mask, MASK_PATTERN);

      switch (mask_type) {
      case MASK_PATTERN:
        led_mask = mask;
        mute_mask = track.mute_mask;
        break;
      case MASK_LOCK:
        track.get_mask(&lock_mask, MASK_LOCK);
        led_mask = lock_mask;
        mask = lock_mask;
        break;
      case MASK_MUTE:
        mute_mask = track.mute_mask;
        led_mask = mute_mask;
        break;
      case MASK_SLIDE:
        track.get_mask(&slide_mask, MASK_SLIDE);
        led_mask = slide_mask;
        break;
      }
    });

    shed_mask(led_mask, length, offset);
    draw_mask(offset, mask, step_count, length,
              mute_mask, slide_mask, show_current_step);

    if (recording)
      return;

    uint64_t locks_on_step_mask_ = 0;
    SeqTrackUtil::with_md_track(last_md_track, [&](auto &track) {
      track.get_mask(&locks_on_step_mask_, MASK_LOCKS_ON_STEP);
    });
    shed_mask(locks_on_step_mask_, length, offset);

    if ((uint16_t)led_mask != trigled_mask) {
      trigled_mask = led_mask;
      MD.set_trigleds(trigled_mask, TRIGLED_STEPEDIT);
      if (mask_type == MASK_MUTE) {
        MD.set_trigleds(mask, TRIGLED_STEPEDIT, 1);
        GUI_hardware.led.set_trigleds(mask, TRIGLED_STEPEDIT, 1);
      }
    }
    if ((uint16_t)locks_on_step_mask_ != locks_on_step_mask) {
      locks_on_step_mask = locks_on_step_mask_;
      MD.set_trigleds(locks_on_step_mask, TRIGLED_STEPEDIT, 1);
    }

    GUI_hardware.led.set_trigleds(led_mask, TRIGLED_STEPEDIT);
    GUI_hardware.led.set_trigleds(locks_on_step_mask, TRIGLED_STEPEDIT, 1);
    if (MidiClock.state == 2) { GUI_hardware.led.toggle_trigled(step_count - offset); }
  }
}

// from knob value to step value
uint8_t SeqPage::translate_to_step_conditional(uint8_t condition,
                                               /*OUT*/ bool *plock) {
  uint8_t num_cond = NUM_TRIG_CONDITIONS;
#if !defined(__AVR__)
  if (seq_page_uses_stepseq_conditions()) num_cond = SPSX_NUM_TRIG_CONDITIONS - 1;
#endif
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
  uint8_t num_cond = NUM_TRIG_CONDITIONS;
#if !defined(__AVR__)
  if (seq_page_uses_stepseq_conditions()) num_cond = SPSX_NUM_TRIG_CONDITIONS - 1;
#endif
  if (plock) {
    condition = condition + num_cond;
  }
  return condition;
}

void SeqPage::draw_knob_conditional(uint8_t cond) {
  char K[4];
  conditional_str(K, cond);
  //draw_knob(0, PRG_TO_RAM("COND"), K);
  draw_knob(0, mclstr_cond, K);
}

void SeqPage::conditional_str(char *s, uint8_t c, bool m) {
  if (!s) return;

  uint8_t num_cond = NUM_TRIG_CONDITIONS;
#if !defined(__AVR__)
  if (seq_page_uses_stepseq_conditions()) num_cond = SPSX_NUM_TRIG_CONDITIONS - 1;
#endif

  if (c > num_cond)
    c -= num_cond;

#if !defined(__AVR__)
  if (seq_page_uses_stepseq_conditions()) {
    // SPSX condition names
    const bool cond_plock = seq_param1.getValue() > num_cond;
    char a = '-', b = '-', d = '-';
    if (c == 0) {
      // 100% = default, show as empty
      a = '-'; b = '-'; d = '-';
    } else if (c <= SPSX_COND_10PCT) {
      // Percentage: 90,75,66,50,33,25,10
      static const uint8_t pcts[] = {90, 75, 66, 50, 33, 25, 10};
      uint8_t pct = pcts[c - 1];
      a = '0' + (pct / 10);
      b = '0' + (pct % 10);
      d = '%';
    } else if (c == SPSX_COND_ONESHOT) {
      a = '1'; b = 'S'; d = 'H';
    } else if (c == SPSX_COND_FIRST) {
      a = '1'; b = 'S'; d = 'T';
    } else if (c == SPSX_COND_NOT_FIRST) {
      a = '!'; b = '1'; d = 'S';
    } else if (c == SPSX_COND_FILL) {
      a = 'F'; b = 'I'; d = 'L';
    } else if (c == SPSX_COND_NOT_FILL) {
      a = '!'; b = 'F'; d = 'L';
    } else if (c == SPSX_COND_PRE) {
      a = 'P'; b = 'R'; d = 'E';
    } else if (c == SPSX_COND_NOT_PRE) {
      a = '!'; b = 'P'; d = 'R';
    } else if (c == SPSX_COND_NEI) {
      a = 'N'; b = 'E'; d = 'I';
    } else if (c == SPSX_COND_NOT_NEI) {
      a = '!'; b = 'N'; d = 'E';
    } else if (c >= SPSX_COND_ITER_BASE && c <= SPSX_COND_ITER_MAX) {
      uint8_t x, y;
      if (spsx_cond_iter_decode(c, x, y)) {
        a = '0' + x;
        b = '/';
        d = '0' + y;
      }
    }
    s[0] = a;
    s[1] = b;
    s[2] = cond_plock ? (m ? '+' : '^') : d;
    s[3] = '\0';
    return;
  }
#endif

  // Legacy MD condition names
  static const char PROGMEM ptab[] = "12579";  // probability digits

  char a = 'L', b = '1';
  if (c) {
    if (c <= 8) {
      b = '0' + c;             // L2..L8
    } else if (c <= 13) {
      a = 'P';                 // P1..P9
      b = pgm_read_byte(&ptab[c - 9]);
    } else if (c == 14) {
      a = '1'; b = 'S';        // 1S
    }
  }

  s[0] = a;
  s[1] = b;
  uint8_t i = 2;

  if (seq_param1.getValue() > num_cond)
    s[i++] = m ? '+' : '^';

  s[i] = '\0';
}

void SeqPage::draw_knob_timing(uint8_t timing, uint8_t timing_mid) {
  char K[5];
  mclstr_copy_progmem(K, mclstr_dash, sizeof(K));

#if !defined(__AVR__)
  if (seq_page_uses_stepseq_conditions() || timing_mid == 0) {
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
  if (timing == 0) {
  } else if ((timing < timing_mid) && (timing != 0)) {
    mcl_gui.put_value_at(timing_mid - timing, K + 1);
  } else {
    K[0] = '+';
    mcl_gui.put_value_at(timing - timing_mid, K + 1);
  }
  draw_knob(1, mclstr_utim, K);
}

void SeqPage::length_handler(uint8_t length, bool multi) {
  bool is_md_device = opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device)) {
    bool is_poly = !seq_page_uses_tbd_step_tracks() &&
                   IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
    if (multi) {
      for (uint8_t i = 0; i < seq_page_step_track_count(); i++) {
        seq_page_step_track_for(i).set_length(length);
      }
    } else if (mcl_cfg.poly_mask && is_poly) {
      for (uint8_t c = 0; c < 16; c++) {
        if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
          seq_page_step_track_for(c).set_length(length);
        }
      }
    } else {
      seq_page_active_step_track().set_length(length);
    }
    SeqStepTrackApi active_track = seq_page_active_step_track();
    if (active_track.uses_md_sound()) {
      MD.sync_seqtrack(active_track.length(), active_track.speed(),
                       active_track.step_count());
    }
    seq_param3.cur = length;
  } else {
#ifdef EXT_TRACKS
    if (multi) {
      SeqTrackUtil::for_each_track(false, [&](SeqTrackCond &track, uint8_t idx) {
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

void opt_mask_handler() { seq_step_page.config_mask_info(false); }

void opt_trackid_handler() {
  seq_step_page.select_track(opt_midi_device_capture, opt_trackid - 1);
}

void opt_speed_handler() {

  bool is_md_device =
      opt_capture_is_md_device();

  if (seq_page_uses_step_track_ops(is_md_device)) {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      for (uint8_t i = 0; i < seq_page_step_track_count(); i++) {
        seq_page_step_track_for(i).request_speed_change(opt_speed);
      }
      GUI.ignoreNextEvent(Buttons.BUTTON4);
    } else {
      SeqStepTrackApi active_track = seq_page_active_step_track();
      if (active_track.request_speed_change(opt_speed) &&
          active_track.uses_md_sound()) {
        MD.sync_seqtrack(active_track.length(), opt_speed,
                         active_track.step_count());
      }
    }
    seq_step_page.config_encoders();
    return;
  }

  if (BUTTON_DOWN(Buttons.BUTTON4)) {
    SeqTrackUtil::for_each_track(
        is_md_device,
        [&](SeqTrackCond &track, uint8_t) { track.request_speed_change(opt_speed); });
    GUI.ignoreNextEvent(Buttons.BUTTON4);
  } else {
    auto &active_track = selected_track(is_md_device);
    if (active_track.request_speed_change(opt_speed) && is_md_device) {
      MD.sync_seqtrack(active_track.length, opt_speed,
                       active_track.step_count);
    }
  }

#ifdef EXT_TRACKS
  seq_extstep_page.config_encoders();
#endif
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

      SeqStepTrackApi active_track = seq_page_active_step_track();
      if (active_track.uses_md_sound()) {
        MD.popup_text(2);
        oled_display.textbox_P(mclstr_clear_md, mclstr_tracks);
      } else {
        oled_display.textbox_P(mclstr_clear, mclstr_tracks);
      }
      oled_display.display();
      uint8_t old_mutes[16];
      for (uint8_t n = 0; n < seq_page_step_track_count(); n++) {
        SeqStepTrackApi track = seq_page_step_track_for(n);
        old_mutes[n] = track.mute_state();
        track.set_mute_state(SEQ_MUTE_ON);
      }
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      for (uint8_t n = 0; n < seq_page_step_track_count(); ++n) {
        SeqStepTrackApi track = seq_page_step_track_for(n);
        track.clear_track();
        track.set_mute_state(old_mutes[n]);
      }
    } else if (opt_clear == 1) {
      bool is_poly = !seq_page_uses_tbd_step_tracks() &&
                     IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
      SeqStepTrackApi active_track = seq_page_active_step_track();
      if (is_poly) {
        display_popup(mclstr_clear, mclstr_poly_tracks);
      } else if (active_track.uses_md_sound()) {
        display_popup(mclstr_clear, mclstr_track);
      } else {
        oled_display.textbox_P(mclstr_clear, mclstr_track);
      }
      oled_display.display();
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      if (is_poly) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
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
      for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
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
      if (seq_page_active_step_track().uses_md_sound()) {
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
        if (seq_page_active_step_track().uses_md_sound()) {
          oled_display.textbox_P(mclstr_copy_md, mclstr_tracks);
          MD.popup_text(1);
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
        if (seq_page_active_step_track().uses_md_sound()) {
          MD.popup_text(4);
        }
      }
      mcl_clipboard.copy_track = last_md_track;
      mcl_clipboard.copy_sequencer_track(last_md_track);
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
        MD.popup_text_P(mclstr_copy_ext);
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
        if (seq_page_active_step_track().uses_md_sound()) {
          oled_display.textbox_P(mclstr_paste_md, mclstr_tracks);
          MD.popup_text(3);
        } else {
          oled_display.textbox_P(mclstr_paste, mclstr_tracks);
        }
        oled_display.display();
      } else {
        oled_display.textbox_P(mclstr_undo, mclstr_tracks);
        oled_display.display();
        if (seq_page_active_step_track().uses_md_sound()) {
          MD.popup_text(22);
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
        if (seq_page_active_step_track().uses_md_sound()) {
          MD.popup_text(6);
        }
      } else {
        oled_display.textbox_P(mclstr_undo, mclstr_track);
        is_poly = !seq_page_uses_tbd_step_tracks() &&
                  IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
        if (seq_page_active_step_track().uses_md_sound()) {
          MD.popup_text(23);
        }
      }
      if (is_poly) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_clipboard.paste_sequencer_track(c, c);
          }
        }
      } else {
        mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                            last_md_track);
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
  SeqStepTrackApi track = seq_page_active_step_track();
  oled_display.textbox_P(mclstr_clear, mclstr_page);
  if (track.uses_md_sound()) {
    MD.popup_text(57);
  }
  for (uint8_t n = 0; n < 16; n++) {
    uint8_t step = n + SeqPage::page_select * 16;
    if (step >= track.length()) return;
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

  SeqStepTrackApi track = seq_page_active_step_track();
  if (!silent) {
    oled_display.textbox_P(mclstr_copy, mclstr_page);
    if (track.uses_md_sound()) {
      MD.popup_text(54);
    }
  }
  uint8_t track_len = track.length();
  for (uint8_t n = 0; n < 16; n++) {
    uint8_t step = n + SeqPage::page_select * 16;
    if (step >= track_len) return;
    copy_step_to_clipboard(track, step, n);
  }
}

void opt_paste_page_handler() {
  SeqStepTrackApi track = seq_page_active_step_track();
  if (opt_undo == PAGE_UNDO) {
    opt_undo = 255;
    oled_display.textbox_P(mclstr_undo, mclstr_page);
    if (track.uses_md_sound()) {
      MD.popup_text(55);
    }
  } else {
    oled_display.textbox_P(mclstr_paste, mclstr_page);
    if (track.uses_md_sound()) {
      MD.popup_text(56);
    }
  }
  uint8_t track_len = track.length();
  for (uint8_t n = 0; n < 16; n++) {
    uint8_t step = n + SeqPage::page_select * 16;
    if (step >= track_len) return;
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
  uint8_t step = SeqPage::step_select + SeqPage::page_select * 16;
  SeqStepTrackApi track = seq_page_active_step_track();
  oled_display.textbox_P(mclstr_clear_step);
  if (track.uses_md_sound()) {
    MD.popup_text_P(mclstr_clear_step);
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
  SeqStepTrackApi track = seq_page_active_step_track();
  if (!silent) {
    oled_display.textbox_P(mclstr_copy_step);
    if (track.uses_md_sound()) {
      MD.popup_text_P(mclstr_copy_step);
    }
  }
  uint8_t step = SeqPage::step_select + SeqPage::page_select * 16;
  copy_step_to_clipboard(track, step, 0);
}

void opt_paste_step_handler() {
  SeqStepTrackApi track = seq_page_active_step_track();
  if (opt_undo == STEP_UNDO) {
    opt_undo = 255;
    oled_display.textbox_P(mclstr_undo_step);
    if (track.uses_md_sound()) {
      MD.popup_text_P(mclstr_undo_step);
    }
  } else {
    oled_display.textbox_P(mclstr_paste_step);
    if (track.uses_md_sound()) {
      MD.popup_text_P(mclstr_paste_step);
    }
  }
  uint8_t step = SeqPage::step_select + SeqPage::page_select * 16;
  paste_step_from_clipboard(track, step, 0);
}

void opt_mute_step_handler() {
  SeqStepTrackApi track = seq_page_active_step_track();
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (note_interface.is_note_on(n)) {
      uint8_t s = n + SeqPage::page_select * 16;
      track.toggle_mute(s);
    }
  }
}

void opt_clear_step_locks_handler() {
  if (opt_clear_step == 1) {
    oled_display.textbox_P(mclstr_clear, mclstr_locks);
    bool is_md_device = opt_capture_is_md_device();
    if (seq_page_uses_step_track_ops(is_md_device) &&
        seq_page_active_step_track().uses_md_sound()) {
      MD.popup_text(14);
    }
  }
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (note_interface.is_note_on(n)) {

      bool is_md_device = opt_capture_is_md_device();
      if (seq_page_uses_step_track_ops(is_md_device)) {
        uint8_t s = n + SeqPage::page_select * 16;
        seq_page_active_step_track().clear_step_locks(s);
      } else {
        //        mcl_seq.ext_tracks[last_ext_track].clear_step_locks(
        //          SeqPage::step_select + SeqPage::page_select * 16);
      }
    }
  }
  opt_clear_step = 0;
}

void opt_shift_track_handler() {
  bool is_md_device =
      opt_capture_is_md_device();
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
  bool is_md_device =
      opt_capture_is_md_device();
  bool apply_all = opt_reverse == 2;
  apply_track_menu_op(is_md_device, apply_all, SEQ_TRACK_OP_REVERSE);
}

void opt_transpose_track_handler() {
  bool is_all = opt_transpose >= 25;
  int8_t transpose_value = is_all ? (opt_transpose - 37) : (opt_transpose - 12);
  if (transpose_value == 0) {  return; }
  oled_display.textbox_P(mclstr_transpose);
  bool is_md_device =
      opt_capture_is_md_device();
  if (seq_page_uses_step_track_ops(is_md_device) &&
      seq_page_active_step_track().uses_md_sound()) {
    MD.popup_text_P(mclstr_transpose);
  }
  apply_track_menu_op(is_md_device, is_all, SEQ_TRACK_OP_TRANSPOSE,
                      transpose_value);
}


void seq_menu_handler() {}

void SeqPage::config_as_trackedit() {

  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, false);
}

void SeqPage::config_as_lockedit() {

  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, true);
}

bool SeqPage::md_track_change_check() {
  if (last_md_track != MD.currentTrack ||
      last_md_model != MD.kit.models[MD.currentTrack]) {
    last_md_model = MD.kit.models[MD.currentTrack];
    select_track(&MD, MD.currentTrack, false);
    return true;
  }
  return false;
}

void SeqPage::loop() {

  opt_midi_device_capture = midi_device;
  opt_midi_device_slot_capture = current_device_slot();

  if (last_midi_state != MidiClock.state) {
    last_midi_state = MidiClock.state;
    DEBUG_DUMP("hii")
  }

  //  md_track_change_check();

  if (show_seq_menu) {
    seq_menu_page.loop();
    if (!seq_page_uses_tbd_step_tracks() &&
        !opt_capture_is_md_device() &&
        opt_trackid > NUM_EXT_TRACKS) {
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

  uint8_t playing_idx;
  if (_playing_idx == 255) {
    uint8_t step_count;
#if defined(PLATFORM_TBD)
    if (seq_page_uses_tbd_step_tracks()) {
      step_count = seq_page_active_step_track().step_count();
    } else
#endif
    if (active_device_is_md()) {
      step_count = SeqTrackUtil::get_seq_track(true, last_md_track).step_count;
    }
#ifdef EXT_TRACKS
    else {
      step_count = SeqTrackUtil::get_seq_track(false, last_ext_track).step_count;
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
#if defined(PLATFORM_TBD)
  if (seq_page_uses_tbd_step_tracks()) {
    is_md = true;
  }
#endif
  char int_name[8];
  char ext_name[8];
  seq_copy_device_slot_name(1, int_name, sizeof(int_name));
  seq_copy_device_slot_name(2, ext_name, sizeof(ext_name));

  uint8_t track_id = last_md_track;
  if (!is_md) {
    track_id = last_ext_track;
  }
  track_id += 1;

  //  draw current active track
  mcl_gui.draw_panel_number(track_id);

  mcl_gui.draw_panel_toggle(int_name, ext_name, current_device_slot() == 1);
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
    if (show_seq_menu) {
      seq_menu_page.draw_menu(128 - width, 8, width);
    }
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

void SeqPageMidiEvents::onMidiStartCallback() {
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
