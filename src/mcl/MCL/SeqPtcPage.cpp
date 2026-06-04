#include "SeqPtcPage.h"
#include "SeqPages.h"
#include "MCLGUI.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
#include "CommonPages.h"
#include "MCLSysConfig.h"
#include "MCLStrings.h"
#include "PtcGroups.h"
#include "SeqExtStepTrackApi.h"
#include "SeqExtStepTrackRef.h"
#include "SeqPtcTrackRef.h"
#include "SeqTrackUtil.h"
#ifdef PLATFORM_TBD
#include "../Drivers/TBD/TBD.h"
#endif

#define MIDI_LOCAL_MODE 0
#define NUM_KEYS 24

#ifdef PLATFORM_TBD
namespace {

constexpr int8_t kTbdPtcKeyMap[16] = {
    -1, 1, 3, -1, 6, 8, 10, -1,
     0, 2, 4,  5, 7, 9, 11, 12
};

constexpr uint16_t kTbdPtcBlackMask = 0b0000010101001010;
constexpr uint32_t kTbdPtcNaturalColor = 0xFFFFFF;
constexpr uint32_t kTbdPtcBlackColor = 0x0000A0;
constexpr uint32_t kTbdPtcActiveColor = 0xFF0000;

bool ptc_uses_tbd_primary_tracks() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}

void tbd_ptc_send_note(SeqPtcPage &page, uint8_t note, uint8_t mask) {
  if (ptc_uses_tbd_primary_tracks()) {
    page.handle_tbd_note_event(note, mask, true);
    return;
  }

  bool old_scale_padding = page.scale_padding;
  page.scale_padding = true;

  bool is_md = SeqPage::active_device_is_md();
  uint8_t channel_event = NO_EVENT;
  uint8_t note_num = note;

  if (is_md) {
    note_num += MIDI_NOTE_C4;
    channel_event = CTRL_EVENT;
  }

  uint8_t msg[] = {
      static_cast<uint8_t>(MIDI_NOTE_ON |
                           (is_md ? last_primary_track : last_ext_track)),
      note_num, 127};

  if (mask == EVENT_BUTTON_PRESSED) {
    page.midi_events.note_on(msg, channel_event);
  } else if (mask == EVENT_BUTTON_RELEASED) {
    page.midi_events.note_off(msg, channel_event);
  }

  page.scale_padding = old_scale_padding;
}

} // namespace
#endif

namespace {

DeviceIdx ptc_active_device_idx() {
  return SeqPage::current_device_idx();
}

uint8_t ptc_selected_track(DeviceIdx device_idx) {
  return device_idx == DeviceIdx::Primary ? last_primary_track : last_ext_track;
}

SeqTrack &ptc_seq_track(DeviceIdx device_idx, uint8_t track) {
  return SeqTrackUtil::seq_track(device_idx, track);
}

ArpSeqTrack &ptc_arp_track(DeviceIdx device_idx, uint8_t track) {
  return SeqTrackUtil::arp_track(device_idx, track);
}

bool ptc_ext_arp_enabled(uint8_t track) {
  return ptc_arp_track(DeviceIdx::Secondary, track).enabled;
}

bool ptc_uses_grid_x_tracks() {
  return ptc_active_device_idx() == DeviceIdx::Primary;
}

SeqTrack &ptc_active_track() {
  DeviceIdx device_idx = ptc_active_device_idx();
  return ptc_seq_track(device_idx, ptc_selected_track(device_idx));
}

void ptc_note_bit_set(uint64_t *mask, uint8_t bit) NOINLINE();
void ptc_note_bit_set(uint64_t *mask, uint8_t bit) {
  SET_BIT128_P(mask, bit);
}

void ptc_note_bit_clear(uint64_t *mask, uint8_t bit) NOINLINE();
void ptc_note_bit_clear(uint64_t *mask, uint8_t bit) {
  CLEAR_BIT128_P(mask, bit);
}

uint8_t ptc_ext_track_for_msg(uint8_t *msg) NOINLINE();
uint8_t ptc_ext_track_for_msg(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (seq_ptc_page.primary_channel_event(channel)) {
    return 255;
  }
  return mcl_seq.find_ext_track(channel);
}

uint16_t ptc_mask_for_event(uint8_t track, uint8_t channel_event,
                            uint8_t channel) {
  if (channel_event == POLY_EVENT) {
    uint16_t mask = ptc_groups.mask_for_midi_channel(channel);
    if (mask) {
      return mask;
    }
  }
  return ptc_groups.mask_for_track(track);
}

} // namespace

const scale_t * const scales[24] PROGMEM = {
    &chromaticScale, &ionianScale, &dorianScale, &phrygianScale, &lydianScale,
    &mixolydianScale, &aeolianScale, &locrianScale, &harmonicMinorScale,
    &melodicMinorScale,
    //&lydianDominantScale,
    //&wholeToneScale,
    //&wholeHalfStepScale,
    //&halfWholeStepScale,
    &majorPentatonicScale, &minorPentatonicScale, &suspendedPentatonicScale,
    &inSenScale, &bluesScale, &majorBebopScale, &dominantBebopScale,
    &minorBebopScale, &majorArp, &minorArp, &majorMaj7Arp, &majorMin7Arp,
    &minorMin7Arp,
    //&minorMaj7Arp,
    &majorMaj7Arp9,
    //&majorMaj7ArpMin9,
    //&majorMin7Arp9,
    //&majorMin7ArpMin9,
    //&minorMin7Arp9,
    //&minorMin7ArpMin9,
    //&minorMaj7Arp9,
    //&minorMaj7ArpMin9
};

void SeqPtcPage::setup() {
  SeqPage::setup();
  init_poly();
  midi_events.setup_callbacks();
  octs[0] = 0;
  octs[1] = 1;
  fine_tunes[0] = 32;
  fine_tunes[1] = 32;
  memset(dev_note_masks, 0, sizeof(dev_note_masks));
  memset(dev_note_channels, 17, sizeof(dev_note_channels));
  memset(note_mask, 0, sizeof(note_mask));
  config_encoders();
  isSetup = true;
}
void SeqPtcPage::cleanup() {
#ifdef PLATFORM_TBD
  release_tbd_keyboard_notes();
#endif
  SeqPage::cleanup();
  last_midi_device = midi_device;
  last_midi_device_idx = current_device_idx();
  params_reset();
}
void SeqPtcPage::config_encoders() {
  if (show_seq_menu) { return; }
  ptc_param_len.min = 1;
  bool show_chan = true;

  bool grid_x_tracks = ptc_uses_grid_x_tracks();
  uint8_t dev = grid_x_tracks ? 0 : 1;

  encoders[0]->cur = octs[dev];
  encoders[1]->cur = fine_tunes[dev];
  SeqTrack &track = ptc_active_track();

  if (grid_x_tracks) {
    ptc_param_len.max = 64;
    ptc_param_len.cur = track.length;
    show_chan = false;
  }
#ifdef EXT_TRACKS
  else {
    ptc_param_len.max = (uint8_t)128;
    ptc_param_len.cur = track.length;
  }
#endif
  seq_menu_page.menu.enable_entry(SEQ_MENU_CHANNEL, show_chan);
}

void SeqPtcPage::reset_poly_voices() {
  for (uint8_t x = 0; x < 16; x++) {
    voice_pitch[x] = -1;
    voice_order[x] = 0;
    voice_active[x] = false;
  }
}

void SeqPtcPage::init_poly() {
  reset_poly_voices();
  cc_link_enable = true;
}

void SeqPtcPage::init() {
  DEBUG_PRINT_FN();
  if (last_midi_device != nullptr) {
    select_device_idx(last_midi_device_idx);
  }
  SeqPage::init();
  seq_menu_page.menu.enable_entry(SEQ_MENU_DEVICE, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_KEY, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_POLY, true);
  bool grid_x_tracks = ptc_uses_grid_x_tracks();
  if (grid_x_tracks) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_SOUND, active_device_is_md());
    seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_MD, true);
  }
  else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_EXT, true);
  }
  cc_link_enable = true;
  scale_padding = false;
  ptc_param_len.handler = pattern_len_handler;
  DEBUG_PRINTLN(F("control mode:"));
  DEBUG_PRINTLN(mcl_cfg.uart2_ctrl_chan);
  key_interface.on();
#ifdef PLATFORM_TBD
  tbd_keyboard_hold_mask = 0;
  tbd_keyboard_led_refresh_ms = read_clock_ms();
  send_tbd_keyboard_leds();
#else
  key_interface.send_md_leds(TRIGLED_EXCLUSIVE);
#endif
  config();
  re_init = false;
}

void SeqPtcPage::config() {
  config_encoders();
  // config info labels
  constexpr uint8_t len1 = sizeof(info1);

  char str_first[3] = "--";
  char str_second[3] = "--";
  info1[0] = '\0';
#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD &&
      last_primary_track < mcl_seq.num_tbd_tracks) {
    tbd_p4_copy_sound_notice(mcl_seq.tbd_tracks[last_primary_track].p4_sound,
                             info1, len1);
  } else
#endif
  if (active_device_is_md()) {
    SeqPtcTrackRef::copy_track_label(last_primary_track, info1, len1);
  }
#ifdef EXT_TRACKS
  else {
    strncpy(str_first, device_manager.secondary_device()->name,
            sizeof(str_first) - 1);
    str_first[sizeof(str_first) - 1] = '\0';
    str_second[0] = 'T';
    str_second[1] = last_ext_track + '1';
  }
#endif
  if (info1[0] == '\0') {
    info1[0] = str_first[0];
    info1[1] = str_first[1];
    info1[2] = '>';
    info1[3] = str_second[0];
    info1[4] = str_second[1];
    info1[5] = '\0';
  }

  strcpy_P(info2, mclstr_chromat);
  display_page_index = false;

  // config menu
  config_as_trackedit();
}
void SeqPtcPage::loop() {
  if (re_init) {
    init();
  }
  bool scale_changed = ptc_param_scale.hasChanged();
  bool keyboard_param_changed = ptc_param_oct.hasChanged() ||
                                scale_changed ||
                                ptc_param_fine_tune.hasChanged();
  if (keyboard_param_changed) {
    bool grid_x_tracks = ptc_uses_grid_x_tracks();
    uint8_t dev = grid_x_tracks ? 0 : 1;
    octs[dev] = encoders[0]->cur;
    fine_tunes[dev] = encoders[1]->cur;

    uint8_t track = last_primary_track;
    if (!grid_x_tracks) {
      track = last_ext_track;
      buffer_notesoff_ext(last_ext_track);
    }
    render_arp(scale_changed, ptc_active_device_idx(), track);
  }
  SeqPage::loop();
#ifdef PLATFORM_TBD
  if (!show_seq_menu) {
    uint16_t now = read_clock_ms();
    if (keyboard_param_changed ||
        clock_diff(tbd_keyboard_led_refresh_ms, now) > 250) {
      tbd_keyboard_led_refresh_ms = now;
      send_tbd_keyboard_leds();
    }
  }
#endif
}
uint8_t SeqPtcPage::find_arp_track(uint8_t channel_event, uint8_t channel) {
  uint8_t track = last_primary_track;
  uint16_t mask = ptc_mask_for_event(track, channel_event, channel);
  if (mask) {
    return ptc_groups.first_track(mask);
  }
  return track;
}

void SeqPtcPage::render_arp(bool recalc_notemask_, DeviceIdx device_idx,
                            uint8_t track) {
  if (recalc_notemask_) {
    recalc_notemask();
  }

  if (track >= SeqTrackUtil::track_count(device_idx)) {
    return;
  }
  SeqTrack &seq = ptc_seq_track(device_idx, track);
  ArpSeqTrack &arp = ptc_arp_track(device_idx, track);

  if (seq.speed == SEQ_SPEED_3_4X || seq.speed == SEQ_SPEED_3_2X) {
    arp.speed = SEQ_SPEED_3_2X;
  } else {
    arp.speed = SEQ_SPEED_2X;
  }
  arp.render(arp_mode.cur, ptc_param_oct.cur, ptc_param_fine_tune.cur,
             arp_range.cur, note_mask);
}

void SeqPtcPage::display() {

  oled_display.clearDisplay();
  DeviceIdx device_idx = ptc_active_device_idx();
  bool is_primary = device_idx == DeviceIdx::Primary;
  bool is_poly = ptc_groups.track_has_group(last_primary_track);
  draw_knob_frame();
  char buf1[4];

  // draw OCTAVE
  mcl_gui.put_value_at(ptc_param_oct.getValue(), buf1);
  draw_knob(0, mclstr_oct, buf1);

  // draw FREQ
  uint8_t det = ptc_param_fine_tune.getValue();
  if (det == 32) {
    strcpy_P(buf1, mclstr_zero);
  } else {
    uint8_t mag;
    if (det < 32) {
      buf1[0] = '-';
      mag = 32 - det;
    } else {
      buf1[0] = '+';
      mag = det - 32;
    }
    mcl_gui.put_value_at(mag, buf1 + 1);
  }
  draw_knob(1, mclstr_det, buf1); // detune

  // draw LEN
  mcl_gui.put_value_at(ptc_param_len.getValue(), buf1);
  draw_knob(2, is_primary && is_poly ? mclstr_plen : mclstr_len, buf1);

  // draw SCALE
  strncpy_P(buf1, scale_names[ptc_param_scale.getValue()], 4);
  draw_knob(3, mclstr_sca, buf1);

  // draw TI keyboard

  oled_display.setFont(&TomThumb);
  oled_display.setCursor(105, 32);

  ArpSeqTrack &arp = ptc_arp_track(device_idx, ptc_selected_track(device_idx));
  if (is_poly) {
    mcl_print_P(mclstr_ply_label);
  }

  uint64_t *mask = note_mask;
  uint64_t display_mask[2];
  if (arp.enabled) {
    mcl_print_P(mclstr_arp);
    display_mask[0] = arp.note_mask[0];
    display_mask[1] = arp.note_mask[1];
    mask = display_mask;
  }

  mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, mask);
  SeqPage::display();
  if (show_seq_menu) {
    display_mute_mask(device_manager.secondary_device(), 8);
  }
}

uint8_t SeqPtcPage::calc_scale_note(uint8_t note_num, bool padded) {
  const scale_t *scale = (const scale_t *)pgm_read_ptr(&scales[ptc_param_scale.cur]);
  uint8_t size = pgm_read_byte(&scale->size);
  uint8_t oct;

  uint8_t d = size;
  if (padded) {
    d = 12;
  }
  oct = note_num / d;
  note_num = note_num - oct * d;

  uint8_t pos = note_num;

  if (padded) {
    // pos = note_num - (note_num / (size + 1)) * (size + 1);
    // pos = min(note_num, size);
    const uint16_t chromatic = 0b0000010101001010;
    if (chromatic & (1 << note_num)) {
      note_num--;
    }
    if (size < 12) {
       pos = (note_num * (size - 1) + 6) / 12;  // +6 for rounding

    }
  }

  return pgm_read_byte(&scale->pitches[pos]) + oct * 12 + transpose;
}

uint8_t SeqPtcPage::get_next_voice(uint8_t pitch, uint8_t track_number,
                                   uint8_t channel_event) {
  uint8_t voice = 255;
  uint16_t voice_mask = 0;

  if (channel_event == POLY_EVENT) {
    voice_mask = ptc_groups.mask_for_track(track_number);
    if (!voice_mask) {
      return 255;
    }
  } else if (channel_event == CTRL_EVENT) {
    voice_mask = ptc_groups.mask_for_track(track_number);

    // mono
    if (!voice_mask) {
      if (track_number >= SeqPtcTrackRef::track_count()) {
        return 255;
      }
      voice_active[track_number] = true;
      voice_pitch[track_number] = pitch;
      return track_number;
    }
  } else {
    return 255;
  }

  uint16_t candidate_mask = 0;
  uint8_t active_same_pitch = 255;
  uint8_t inactive_same_pitch = 255;
  uint8_t inactive_voice = 255;
  uint8_t oldest_voice = 255;
  uint8_t oldest_val = 0;

  // Preserve the old allocation order while only scanning the voice group once.
  uint16_t voice_bit = 1;
  for (uint8_t x = 0; voice_mask; x++, voice_mask >>= 1, voice_bit <<= 1) {
    if (!(voice_mask & 1) ||
        !SeqPtcTrackRef::is_poly_voice_track(x)) {
      continue;
    }
    candidate_mask |= voice_bit;
    if (voice_active[x]) {
      if (voice_pitch[x] == pitch) {
        active_same_pitch = x;
      }
      if (oldest_voice == 255 || voice_order[x] > oldest_val) {
        oldest_voice = x;
        oldest_val = voice_order[x];
      }
      continue;
    }
    if (voice_pitch[x] == pitch) {
      inactive_same_pitch = x;
    }
    inactive_voice = x;
  }

  if (active_same_pitch != 255) {
    voice = active_same_pitch;
  } else if (inactive_same_pitch != 255) {
    voice = inactive_same_pitch;
  } else if (inactive_voice != 255) {
    voice = inactive_voice;
  } else {
    voice = oldest_voice;
  }

  if (voice == 255) {
    return 255;
  }

  for (uint8_t x = 0; candidate_mask; x++, candidate_mask >>= 1) {
    if (candidate_mask & 1) {
      if (voice_order[x] <= voice_order[voice] && x != voice) {
        voice_order[x]++;
      }
    }
  }
  // set selected voice to be the latest note.

  voice_order[voice] = 0;
  voice_pitch[voice] = pitch;
  voice_active[voice] = true;

  return voice;
}

uint8_t SeqPtcPage::release_voice(uint8_t pitch, uint8_t track_number,
                                  uint8_t channel_event) {
  if (track_number >= SeqPtcTrackRef::track_count()) {
    return 255;
  }

  uint16_t voice_mask = ptc_groups.mask_for_track(track_number);
  if (channel_event == CTRL_EVENT && !voice_mask) {
    voice_active[track_number] = false;
    return track_number;
  }

  if (channel_event != POLY_EVENT && channel_event != CTRL_EVENT) {
    return 255;
  }

  for (uint8_t x = 0; voice_mask; x++, voice_mask >>= 1) {
    if ((voice_mask & 1) && SeqPtcTrackRef::is_poly_voice_track(x) &&
        voice_active[x] && voice_pitch[x] == pitch) {
      voice_active[x] = false;
      return x;
    }
  }

  return 255;
}

void SeqPtcPage::trig_primary(uint8_t note_num, uint8_t track_number,
                              uint8_t channel_event, uint8_t fine_tune,
                              MidiUartClass *uart_) {
  if (track_number == 255) {
    track_number = last_primary_track;
  }

  if (SeqPtcTrackRef::is_midi_voice_track(track_number)) {
    uint8_t record_pitch = note_num;
    if (SeqPtcTrackRef::trigger_voice(track_number, note_num, fine_tune,
                                      uart_, &record_pitch)) {
      record(record_pitch, track_number);
    }
    return;
  }

  uint8_t next_track = get_next_voice(note_num, track_number, channel_event);
  if (next_track > 15) { return; }

  uint8_t record_pitch = note_num;
  if (!SeqPtcTrackRef::trigger_voice(next_track, note_num, fine_tune, uart_,
                                     &record_pitch)) {
    return;
  }

  mixer_page.trig(next_track);
  record(record_pitch, next_track);
}

void SeqPtcPage::record(uint8_t pitch, uint8_t track) {
  if ((recording) && (MidiClock.state == 2)) {
    reset_undo();
    SeqPtcTrackRef::record_track(track, 127);
    SeqPtcTrackRef::record_pitch(track, pitch);
  }

}
void SeqPtcPage::note_on_ext(uint8_t note_num, uint8_t velocity,
                             uint8_t track_number, MidiUartClass *uart_) {
  if (track_number == 255) {
    track_number = last_ext_track;
  }
  auto &&track = SeqExtStepTrackRef::runtime_track(track_number);
  if (mcl_cfg.uart_note_fwd) track.note_on(note_num, velocity, uart_);
  reset_undo();
  SeqExtStepTrackRef::record_note_on(track, note_num, velocity);
}

void SeqPtcPage::note_off_ext(uint8_t note_num, uint8_t velocity,
                              uint8_t track_number, MidiUartClass *uart_) {
  if (track_number == 255) {
    track_number = last_ext_track;
  }
  auto &&track = SeqExtStepTrackRef::runtime_track(track_number);
  if (mcl_cfg.uart_note_fwd) track.note_off(note_num, velocity, uart_);
  reset_undo();
  SeqExtStepTrackRef::record_note_off(track, note_num);
}

void SeqPtcPage::buffer_notesoff_ext(uint8_t track_number) {
  SeqExtStepTrackRef::runtime_track(track_number).buffer_notesoff();
}

void SeqPtcPage::recalc_notemask() {
  memset(note_mask, 0, sizeof(note_mask));

  uint8_t dev = ptc_active_device_idx() == DeviceIdx::Primary ? 0 : 1;

  for (uint8_t i = 0; i < 128; i++) {
    if (IS_BIT_SET128_P(dev_note_masks[dev], i)) {
      uint8_t pitch = calc_scale_note(i, scale_padding);
      if (pitch > 127)
        continue;
      ptc_note_bit_set(note_mask, pitch);
    }
  }
}

void SeqPtcPage::draw_popup_transpose() {
  char str[] = "KEY:   ";
  char empty[] = "";
  seq_copy_note_name(transpose, str + 5);
  SeqPtcTrackRef::popup_text(str);
  oled_display.textbox(str, empty);
}

void SeqPtcPage::draw_popup_octave() {
  char str[] = "OCT:   ";
  mcl_gui.put_value_at(ptc_param_oct.cur, str + 5);
  SeqPtcTrackRef::popup_text(str);
}

#ifdef PLATFORM_TBD
bool SeqPtcPage::handle_tbd_note_event(uint8_t note, uint8_t mask,
                                       bool padded) {
  if (mask != EVENT_BUTTON_PRESSED && mask != EVENT_BUTTON_RELEASED) {
    return true;
  }
  if (last_primary_track >= mcl_seq.num_tbd_tracks) {
    return true;
  }

  bool old_scale_padding = scale_padding;
  scale_padding = padded;
  uint8_t pitch = process_ext_event(note, mask == EVENT_BUTTON_PRESSED,
                                    dev_note_channels[0], true);
  scale_padding = old_scale_padding;

  if (pitch == 255) {
    return true;
  }

  ArpSeqTrack &arp_track =
      SeqTrackUtil::arp_track(DeviceIdx::Primary, last_primary_track);
  arp_page.track_update(last_primary_track);
  render_arp(false, DeviceIdx::Primary, last_primary_track);
  bool arp_running = arp_track.enabled && MidiClock.state == 2;

  if (mask == EVENT_BUTTON_PRESSED) {
    if (!arp_running) {
      SeqPtcTrackRef::trigger_voice(last_primary_track, pitch);
    }
    if (!arp_running && recording && MidiClock.state == 2) {
      record(pitch, last_primary_track);
    }
  } else if (!arp_running) {
    SeqPtcTrackRef::release_voice(last_primary_track);
  }
  return true;
}

void SeqPtcPage::send_tbd_keyboard_leds() {
  uint16_t natural_mask = 0;
  uint16_t black_mask = 0;

  for (uint8_t i = 0; i < 16; i++) {
    int8_t note = kTbdPtcKeyMap[i];
    if (note < 0) continue;
    if (IS_BIT_SET16(kTbdPtcBlackMask, note % 12)) {
      SET_BIT16(black_mask, i);
    } else {
      SET_BIT16(natural_mask, i);
    }
  }

  mcl_gui.set_trigleds_local(0, TRIGLED_EXCLUSIVE);
  mcl_gui.set_trigleds_color(natural_mask, kTbdPtcNaturalColor);
  mcl_gui.set_trigleds_color(black_mask, kTbdPtcBlackColor);
  if (tbd_keyboard_hold_mask) {
    mcl_gui.set_trigleds_color(tbd_keyboard_hold_mask, kTbdPtcActiveColor);
  }
}

void SeqPtcPage::release_tbd_keyboard_notes() {
  uint16_t held = tbd_keyboard_hold_mask;
  if (!held) return;

  for (uint8_t i = 0; i < 16; i++) {
    if (!IS_BIT_SET16(held, i)) continue;
    int8_t note = kTbdPtcKeyMap[i];
    if (note >= 0) {
      tbd_ptc_send_note(*this, note, EVENT_BUTTON_RELEASED);
    }
  }

  tbd_keyboard_hold_mask = 0;
  send_tbd_keyboard_leds();
}

bool SeqPtcPage::handle_tbd_keyboard_event(uint8_t button, uint8_t mask) {
  if (button >= 16) return false;
  if (show_seq_menu) return false;

  if (mask != EVENT_BUTTON_PRESSED && mask != EVENT_BUTTON_RELEASED) {
    return true;
  }

  int8_t note = kTbdPtcKeyMap[button];
  if (note < 0) {
    return true;
  }

  if (mask == EVENT_BUTTON_RELEASED &&
      !IS_BIT_SET16(tbd_keyboard_hold_mask, button)) {
    send_tbd_keyboard_leds();
    return true;
  }

  if (mask == EVENT_BUTTON_PRESSED) {
    SET_BIT16(tbd_keyboard_hold_mask, button);
  } else {
    CLEAR_BIT16(tbd_keyboard_hold_mask, button);
  }

  tbd_ptc_send_note(*this, note, mask);
  send_tbd_keyboard_leds();
  return true;
}
#endif

bool SeqPtcPage::handleEvent(gui_event_t *event) {

  if (EVENT_NOTE(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t note = event->source;
    // Only route note-interface events that belong to the primary trig surface.
    if (!device_manager.port_supports(
            port, MidiDeviceCapability::MdTrigInterface)) {
      return false;
    }
    if (show_seq_menu) {
      if (mask == EVENT_BUTTON_PRESSED) {
#ifdef PLATFORM_TBD
        if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD &&
            device_manager.device_for_port(port) == &TBD) {
          opt_trackid = note + 1;
          note_interface.ignoreNextEvent(note);
          select_track(&TBD, note);
          seq_menu_page.select_item(0);
          return true;
        }
#endif
        toggle_ext_mask(note);
      }
      return true;
    }
    /*
    if (mask == EVENT_BUTTON_PRESSED) {
      SET_BIT128_P(dev_note_masks[0], note);
    } else {
      CLEAR_BIT128_P(dev_note_masks[0], note);
    }
    */

    // note interface presses are treated as musical notes here
#ifdef PLATFORM_TBD
    if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD &&
        device_manager.device_for_port(port) == &TBD) {
      handle_tbd_note_event(note, mask, false);
      send_tbd_keyboard_leds();
      return true;
    }
#endif
    scale_padding = false;
    bool is_md = active_device_is_md();
    uint8_t channel_event = NO_EVENT;

    if (is_md) {
      note += MIDI_NOTE_C4;
      channel_event = CTRL_EVENT;
    } else {
      //      note += MIDI_NOTE_C1;
    }
    uint8_t msg[] = {
        static_cast<uint8_t>(MIDI_NOTE_ON |
                             (is_md ? last_primary_track : last_ext_track)),
        note, 127};

    if (mask == EVENT_BUTTON_PRESSED) {
      midi_events.note_on(msg, channel_event);

    } else if (mask == EVENT_BUTTON_RELEASED) {
      midi_events.note_off(msg, channel_event);
    }

#ifdef PLATFORM_TBD
    send_tbd_keyboard_leds();
#else
    key_interface.send_md_leds(TRIGLED_EXCLUSIVE);
#endif
    // deferred trigger redraw to update TI keyboard feedback.

    return true;
  } // TI events

  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
#ifdef PLATFORM_TBD
    if (show_seq_menu) {
      return seq_menu_page.handleEvent(event);
    }
#endif
    if (key_interface.is_key_down(MDX_KEY_PATSONG)) {
      return seq_menu_page.handleEvent(event);
    }
    if (event->mask == EVENT_BUTTON_PRESSED &&
        !key_interface.is_key_down(MDX_KEY_FUNC) && key == MDX_KEY_SCALE) {
      select_device_idx(ptc_uses_grid_x_tracks() ? DeviceIdx::Secondary
                                                  : DeviceIdx::Primary);
      config();
      return true;
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
      if (BUTTON_DOWN(Buttons.BUTTON4)) {
        re_init = true;
        mcl.pushPage(POLY_PAGE);
        return true;
      }
#ifndef PLATFORM_TBD
      SeqExtStepTrackRef::active_track().init_notes_on();
      toggle_record();
      return true;
#endif
    }
    /*
      if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
        mcl.setPage(GRID_PAGE);
        return true;
      }
    */
    if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
      if (BUTTON_DOWN(Buttons.BUTTON1)) {
        re_init = true;
        mcl.pushPage(POLY_PAGE);
        return true;
      }
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      mute_mask = 128;
    }
    if (SeqPage::handleEvent(event)) {
      return true;
    }
  }
  return false;
}

uint8_t SeqPtcPage::seq_ext_pitch(uint8_t note_num) {
  uint8_t pitch = calc_scale_note(note_num, scale_padding);
  return (pitch < 128) ? pitch : 255;
}

uint8_t SeqPtcPage::process_ext_event(uint8_t note_num, bool note_type,
                                      uint8_t channel, bool primary_event) {

  uint8_t pitch = seq_ext_pitch(note_num);
  DeviceIdx device_idx =
      primary_event ? DeviceIdx::Primary : ptc_active_device_idx();
  uint8_t dev = device_idx == DeviceIdx::Primary ? 0 : 1;

  ArpSeqTrack &arp = ptc_arp_track(device_idx, ptc_selected_track(device_idx));
  dev_note_channels[dev] = channel;
  if (note_type) {
    bool notes_all_off = dev_note_masks[dev][0] == 0 &&
                         dev_note_masks[dev][1] == 0;

    if (notes_all_off) {
      arp.idx = 0;

      if (mcl_cfg.rec_quant == 0) {
        arp.mod12_counter = arp.get_ticks_per_step() - 2;
        arp.step_count = arp.length - 1;
      }
      if (arp_enabled.cur == ARP_LATCH) {
        memset(note_mask, 0, sizeof(note_mask));
      }
    }
    ptc_note_bit_set(dev_note_masks[dev], note_num);
    if (pitch != 255) {
      ptc_note_bit_set(note_mask, pitch);
    }
  } else {
    ptc_note_bit_clear(dev_note_masks[dev], note_num);
    if (arp_enabled.cur != ARP_LATCH && pitch != 255) {
      ptc_note_bit_clear(note_mask, pitch);
    }
  }
  if (pitch == 255) {
    return 255;
  }
  pitch += ptc_param_oct.cur * 12;
  return (pitch < 128) ? pitch : 255;
}

uint8_t SeqPtcPage::primary_channel_event(uint8_t channel) {
  if (mcl_cfg.uart2_poly_chan != MD_POLY_MODE_INT &&
      ptc_groups.group_for_midi_channel(channel) != PTC_GROUP_OFF) {
    return POLY_EVENT;
  }
  if (mcl_cfg.uart2_ctrl_chan - 1 == channel) {
    return CTRL_EVENT;
  }
  if (mcl_cfg.md_trig_channel - 1 == channel) {
    return TRIG_EVENT;
  }

  return NO_EVENT;
  /*
    return (mcl_cfg.uart2_ctrl_chan != MIDI_LOCAL_MODE) &&
           (mcl.currentPage() != SEQ_EXTSTEP_PAGE);
  */
}
void SeqPtcMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
#ifdef PLATFORM_TBD
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) return;
#endif
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t channel_event = seq_ptc_page.primary_channel_event(channel);

  if (channel_event) {
    if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) {
      SeqPage::select_device_idx(DeviceIdx::Primary);
    }
  } else {
    auto active_device = device_manager.secondary_device();
    uint8_t n = mcl_seq.find_ext_track(channel);
    if (n == 255) {
      return;
    }
    if (SeqPage::midi_device != active_device || (last_ext_track != n)) {
      SeqPage::select_device_idx(DeviceIdx::Secondary);
      last_ext_track = min(n, NUM_EXT_TRACKS - 1);
      seq_ptc_page.config();
    } else {
      SeqPage::select_device_idx(DeviceIdx::Secondary);
    }
  }
  uint8_t scale_padding_old = seq_ptc_page.scale_padding;
  seq_ptc_page.scale_padding = true;
  note_on(msg, channel_event);
  seq_ptc_page.scale_padding = scale_padding_old;
}

void SeqPtcMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
#ifdef PLATFORM_TBD
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) return;
#endif
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t channel_event = seq_ptc_page.primary_channel_event(channel);
  if (!channel_event) {
    uint8_t n = mcl_seq.find_ext_track(channel);
    if (n == 255) {
      return;
    }
  }
  uint8_t scale_padding_old = seq_ptc_page.scale_padding;
  seq_ptc_page.scale_padding = true;
  note_off(msg, channel_event);
  seq_ptc_page.scale_padding = scale_padding_old;
}

void SeqPtcMidiEvents::note_on(uint8_t *msg, uint8_t channel_event) {
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  DEBUG_PRINTLN("note on");
  DEBUG_DUMP(channel);

  // pitch - MIDI_NOTE_C4
  //
  uint8_t pitch;

  if (channel_event) {
    if (channel_event == TRIG_EVENT) {
      if (note_num < MIDI_NOTE_C4) {
        uint8_t pos = note_num - MIDI_NOTE_C2;
        if (pos > 15) {
          return;
        }
        if (SeqPtcTrackRef::is_midi_voice_track(pos)) {
          SeqPtcTrackRef::send_notes_on(pos);
        }
        else {
          SeqPtcTrackRef::trigger(pos, msg[2]);
        }
        if ((seq_ptc_page.recording) && (MidiClock.state == 2)) {
          reset_undo();
          SeqPtcTrackRef::record_track(pos, msg[2]);
        }
      }
    }
    note_num -= MIDI_NOTE_C4;

    pitch = seq_ptc_page.process_ext_event(note_num, true, channel, true);
    uint8_t n = seq_ptc_page.find_arp_track(channel_event, channel);
    arp_page.track_update(n);

    seq_ptc_page.render_arp(false, DeviceIdx::Primary, n);

    if (pitch == 255)
      return;
    ArpSeqTrack &arp_track = SeqTrackUtil::arp_track(DeviceIdx::Primary, n);

    if (!arp_track.enabled || (MidiClock.state != 2)) {
      seq_ptc_page.trig_primary(pitch, n, channel_event);
      if (mcl.currentPage() == SEQ_STEP_PAGE && channel_event == CTRL_EVENT) {
        seq_step_page.pitch_param = pitch;
      }

    }

    return;
  }
#ifdef EXT_TRACKS
  // otherwise, translate the message and send it back to MIDI2.
  pitch = seq_ptc_page.process_ext_event(note_num, true, channel);
  seq_ptc_page.config_encoders();

  arp_page.track_update();
  seq_ptc_page.render_arp(false, DeviceIdx::Secondary, last_ext_track);
  if (pitch == 255)
    return;

  seq_extstep_page.set_cur_y(pitch);

  if (!ptc_ext_arp_enabled(last_ext_track) || (MidiClock.state != 2)) {
    seq_ptc_page.note_on_ext(pitch, msg[2]);
  }
#endif
  return;
}

void SeqPtcMidiEvents::note_off(uint8_t *msg, uint8_t channel_event) {
  DEBUG_PRINTLN(F("note off midi2"));

  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t pitch;
  if (channel_event) {
    if (note_num < MIDI_NOTE_C4) {
      return;
    }
    note_num -= MIDI_NOTE_C4;

    pitch = seq_ptc_page.process_ext_event(note_num, false, channel, true);
    uint8_t n = seq_ptc_page.find_arp_track(channel_event, channel);
    seq_ptc_page.render_arp(false, DeviceIdx::Primary, n);
    if (pitch == 255) { return; }
    uint8_t voice = seq_ptc_page.release_voice(pitch, n, channel_event);
    if (voice == 255) { return; }
    ArpSeqTrack &arp_track = SeqTrackUtil::arp_track(DeviceIdx::Primary, n);
    if (!arp_track.enabled || (MidiClock.state != 2)) {
      SeqPtcTrackRef::release_voice(voice);
    }
    return;
  }

#ifdef EXT_TRACKS
  pitch = seq_ptc_page.process_ext_event(note_num, false, channel);

  seq_ptc_page.config_encoders();
  seq_ptc_page.render_arp(false, DeviceIdx::Secondary, last_ext_track);
  arp_page.track_update();

  if (pitch == 255)
    return;

  if (!ptc_ext_arp_enabled(last_ext_track)) {
    seq_ptc_page.note_off_ext(pitch, msg[2]);
  }

#endif
}

void SeqPtcMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
#ifdef PLATFORM_TBD
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) return;
#endif
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];

  bool send_uart2 = true;

  // CC_FWD
  //
  if (mcl_cfg.uart_cc_fwd) {
    mcl_seq.secondary_output->sendCC(channel, param, value);
    send_uart2 = false;
  }
  uint8_t channel_event = seq_ptc_page.primary_channel_event(channel);
  if (channel_event) {
    // If external keyboard controlling MD param, send parameter updates
    // to all polyphonic tracks
    if ((param < 16) || (param > 39)) {
      return;
    }
    param -= 16;
    // If Midi2 forwarding data to port 1 , ignore this to prevent double
    // messages.
    //

    if (mcl_cfg.midi_forward_2 == 1) {
      return;
    }
    if (channel_event == POLY_EVENT) {
      uint16_t mask = ptc_groups.mask_for_midi_channel(channel);
      for (uint8_t n = 0; mask; n++, mask >>= 1) {
        if (mask & 1) {
          SeqPtcTrackRef::set_param(n, param, value, nullptr, true);
        }
      }
    }
    return;
  }

  uint8_t n = mcl_seq.find_ext_track(channel);
  if (n == 255) {
    return;
  }

  // Send mod wheel CC#1 or bank select CC#0
  auto &&active_track = SeqExtStepTrackRef::runtime_track(n);
#ifdef PLATFORM_TBD
  SeqExtParsedControl parsed_control;
  if (SeqTrackUtil::use_midi_tracks_for_ext() &&
      control_state.parse_cc(channel, param, value, parsed_control)) {
    if (parsed_control.has_value && SeqPage::recording &&
        (MidiClock.state == 2) && !note_interface.notes_on) {
      active_track.locks().record_control_lock(
          parsed_control.ctrl_type, parsed_control.parameter,
          parsed_control.value, SeqPage::slide);
    }
    return;
  }
#endif
  if (send_uart2 && param < 2) {
    active_track.send_cc(param, value);
  }

#if defined(__AVR__)
  seq_extstep_page.handle_cc_lock_learn(n, param, value);
#endif

  if (SeqPage::recording && (MidiClock.state == 2) &&
      !note_interface.notes_on) {
    if (param != device_manager.secondary_device()->get_mute_cc()) {
      SeqExtStepTrackRef::record_cc_lock(active_track, param, value,
                                         SeqPage::slide);
    }
  }
  active_track.update_param(param, value);
}

void SeqPtcMidiEvents::onPitchWheelCallback_Midi2(uint8_t *msg) {
#ifdef PLATFORM_TBD
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) return;
#endif
  uint8_t n = ptc_ext_track_for_msg(msg);
  if (n == 255) {
    return;
  }
  uint16_t pitch = msg[1] | (msg[2] << 7);
  auto &&active_track = SeqExtStepTrackRef::runtime_track(n);
  active_track.pitch_bend(pitch);
  if (SeqPage::recording && (MidiClock.state == 2)) {
    SeqExtStepTrackRef::record_pitch_bend_lock(active_track, pitch,
                                               SeqPage::slide);
  }
}

void SeqPtcMidiEvents::onChannelPressureCallback_Midi2(uint8_t *msg) {
#ifdef PLATFORM_TBD
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) return;
#endif
  uint8_t n = ptc_ext_track_for_msg(msg);
  if (n == 255) {
    return;
  }
  auto &&active_track = SeqExtStepTrackRef::runtime_track(n);
  active_track.channel_pressure(msg[1]);
  if (SeqPage::recording && (MidiClock.state == 2)) {
    SeqExtStepTrackRef::record_channel_pressure_lock(active_track, msg[1]);
  }
}

void SeqPtcMidiEvents::onAfterTouchCallback_Midi2(uint8_t *msg) {
#ifdef PLATFORM_TBD
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) return;
#endif
  uint8_t n = ptc_ext_track_for_msg(msg);
  if (n == 255) {
    return;
  }
  auto &&active_track = SeqExtStepTrackRef::runtime_track(n);
  active_track.after_touch(msg[1], msg[2]);
#if !defined(__AVR__)
  if (SeqPage::recording && (MidiClock.state == 2)) {
    active_track.locks().record_control_lock(SEQ_EXT_LOCK_CTRL_POLY_PRESSURE,
                                             msg[1], msg[2], false);
  }
#endif
}

void SeqPtcMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  uint8_t display_polylink = 0;

  if (!seq_ptc_page.cc_link_enable) {
    return;
  }

  if (!SeqPtcTrackRef::parse_cc(channel, param, &track, &track_param)) {
    return;
  }
  if (track >= SeqPtcTrackRef::track_count()) {
    return;
  }
  if (SeqPtcTrackRef::is_mute_param(track_param)) {
    return;
  } // don't process mute
  uint16_t mask = ptc_groups.mask_for_track(track);
  if (mask) {
    for (uint8_t n = 0; mask; n++, mask >>= 1) {
      if ((mask & 1) && (n != track)) {
        if (SeqPtcTrackRef::can_polylink_param(track, n, track_param)) {
          SeqPtcTrackRef::set_param(n, track_param, value, nullptr, true);
          display_polylink = 1;
          if (mcl.currentPage() == MIXER_PAGE) { SET_BIT16(mixer_page.redraw_mask, n); }
        }
      }
      // in_sysex = 0;
    }
  }

  if (display_polylink && mcl.currentPage() != MIXER_PAGE) {
    oled_display.textbox_P(mclstr_poly, mclstr_link);
  }
}

void SeqPtcMidiEvents::setup_midi(MidiClass *midi) {
  if (midi == nullptr) return;
  midi->addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  midi->addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  midi->addOnPitchWheelCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onPitchWheelCallback_Midi2);
  midi->addOnAfterTouchCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onAfterTouchCallback_Midi2);
  midi->addOnChannelPressureCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onChannelPressureCallback_Midi2);
  midi->addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi2);
}

void SeqPtcMidiEvents::cleanup_midi(MidiClass *midi) {
  if (midi == nullptr) return;
  midi->removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  midi->removeOnPitchWheelCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onPitchWheelCallback_Midi2);
  midi->removeOnAfterTouchCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onAfterTouchCallback_Midi2);
  midi->removeOnChannelPressureCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onChannelPressureCallback_Midi2);
  midi->removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi2);
}

void SeqPtcMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  if (mcl_cfg.midi_ctrl_port == 1 || mcl_cfg.midi_ctrl_port == 3) {
    setup_midi(&Midi2);
  }
  if (mcl_cfg.midi_ctrl_port == 2 || mcl_cfg.midi_ctrl_port == 3) {
    setup_midi(&MidiUSB);
  }
#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    MidiDevice *secondary = device_manager.secondary_device();
    if (secondary != nullptr) {
      setup_midi(secondary->midi);
    }
  }
#endif
  MidiClass *param_midi = SeqPtcTrackRef::param_midi();
  if (param_midi != nullptr) {
    param_midi->addOnControlChangeCallback(
        this,
        (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  }
  state = true;
}

void SeqPtcMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }
  cleanup_midi(&Midi);
  cleanup_midi(&Midi2);
  cleanup_midi(&MidiUSB);
#ifdef PLATFORM_TBD
  cleanup_midi(&MidiP4);
#endif
  state = false;
}
