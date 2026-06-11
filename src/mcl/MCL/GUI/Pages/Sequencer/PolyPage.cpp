#include "GUI/Pages/Sequencer/PolyPage.h"
#include "MCLGUI.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "DeviceManager.h"
#include "../../../../Drivers/MidiDevice.h"
#include "MCLSysConfig.h"
#include "Project.h"
#include "TomThumb.h"

namespace {

bool is_midi_group(uint8_t group) {
  return group >= PTC_MIDI_GROUP_MIN && group <= PTC_MIDI_GROUP_MAX;
}

void print_ptc_group_label(uint8_t group) {
  if (!is_midi_group(group)) {
    oled_display.write('-');
    oled_display.write('-');
    return;
  }
  oled_display.print(group);
}

uint8_t ptc_group_text_x(uint8_t group, uint8_t x) {
  return is_midi_group(group) && group < 10 ? x + 6 : x + 4;
}

uint8_t group_from_encoder_value(uint8_t value) {
  if (value == 0) {
    return PTC_GROUP_OFF;
  }
  return value <= PTC_MIDI_GROUP_MAX ? value : PTC_MIDI_GROUP_MAX;
}

uint8_t encoder_value_from_group(uint8_t group) {
  if (group >= PTC_MIDI_GROUP_MIN && group <= PTC_MIDI_GROUP_MAX) {
    return group;
  }
  return 0;
}

uint8_t editable_group_for_track(uint8_t track) {
  return ptc_groups.group_for_track(track);
}

MCLEncoder channel_select_encoder(PTC_MIDI_GROUP_MAX, 0, ENCODER_RES_SEQ);

void sync_channel_encoder(Encoder *encoder, uint8_t group) {
  if (encoder != nullptr) {
    encoder->setValue(encoder_value_from_group(group));
  }
}

void update_selection_leds(uint16_t mask) {
  trigled_mask = mask;
  mcl_gui.set_trigleds(trigled_mask, TRIGLED_EXCLUSIVE);
}

} // namespace

void PolyPage::init() {
  DEBUG_PRINT_FN();
  selected_tracks = 0;
  first_held_track = 255;
  selected_group = editable_group_for_track(last_primary_track);
  sync_channel_encoder(encoders[0], selected_group);
  key_interface.on();
  note_interface.init_notes();
  update_selection_leds(selected_tracks);
}

void PolyPage::cleanup() {
  seq_ptc_page.init_poly();
  key_interface.off();
}

void PolyPage::draw_mask() {
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  uint16_t selected_mask = selected_tracks;
  uint8_t x = 0;
  uint8_t y = 11;
  for (uint8_t i = 0; i < 16; i++, selected_mask >>= 1) {
    uint8_t group = ptc_groups.group_for_track(i);
    bool is_selected = selected_mask & 1;
    bool is_note = note_interface.is_note(i);
    bool filled = is_note || is_selected;

    oled_display.fillRect(x, y, 15, 10, filled ? WHITE : BLACK);

    if (!filled) {
      oled_display.drawRect(x, y, 15, 10, WHITE);
    }

    oled_display.setCursor(ptc_group_text_x(group, x), y + 8);
    oled_display.setTextColor(filled ? BLACK : WHITE);
    print_ptc_group_label(group);

    x += 16;
    if (x == 128) {
      x = 0;
      y += 11;
    }
  }

  oled_display.setTextColor(WHITE);
  oled_display.setFont(oldfont);
}

void PolyPage::cycle_group(int8_t direction) {
  if (direction > 0) {
    selected_group = selected_group == PTC_GROUP_OFF ? PTC_MIDI_GROUP_MIN
                                                     : selected_group + 1;
    if (selected_group > PTC_MIDI_GROUP_MAX) {
      selected_group = PTC_GROUP_OFF;
    }
  } else {
    selected_group = selected_group == PTC_GROUP_OFF ? PTC_MIDI_GROUP_MAX
                                                     : selected_group - 1;
    if (selected_group < PTC_MIDI_GROUP_MIN) {
      selected_group = PTC_GROUP_OFF;
    }
  }
  sync_channel_encoder(encoders[0], selected_group);
  apply_selected_group();
}

void PolyPage::press_track(uint8_t i) {
  if (selected_tracks == 0) {
    first_held_track = i;
    selected_group = editable_group_for_track(i);
    sync_channel_encoder(encoders[0], selected_group);
  }
  SET_BIT16(selected_tracks, i);
  update_selection_leds(selected_tracks);
}

void PolyPage::apply_selected_group() {
  uint16_t mask = selected_tracks;
  for (uint8_t i = 0; mask != 0; ++i, mask >>= 1) {
    if (mask & 1) {
      ptc_groups.set_track_group(i, selected_group);
    }
  }
}

void PolyPage::release_track(uint8_t i) {
  CLEAR_BIT16(selected_tracks, i);
  if (selected_tracks == 0) {
    first_held_track = 255;
  } else if (i == first_held_track) {
    uint8_t track = ptc_groups.first_track(selected_tracks);
    first_held_track = track;
    selected_group = editable_group_for_track(track);
    sync_channel_encoder(encoders[0], selected_group);
  }
  update_selection_leds(selected_tracks);
}

void PolyPage::save_ptc_groups() {
  ptc_groups.store(mcl_cfg.ptc_group);
  if (mcl_cfg.write_cfg()) {
    proj.store_config_from_system();
  }
}

void PolyPage::loop() {
  if (encoders[0] != nullptr && encoders[0]->hasChanged()) {
    selected_group = group_from_encoder_value(encoders[0]->cur);
    apply_selected_group();
  }
}

void PolyPage::display() {
  oled_display.clearDisplay();

  oled_display.setCursor(0, 2);
  mcl_print_P(mclstr_channel_select);
  mcl_print_P(mclstr_space);
  print_ptc_group_label(selected_group);
  oled_display.println();

  draw_mask();

  if (selected_tracks != trigled_mask) {
    update_selection_leds(selected_tracks);
  }

}

bool PolyPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t track = event->source;
    if (!device_manager.port_supports(
            event->port, MidiDeviceCapability::MdTrigInterface)) {
      return true;
    }
    note_interface.draw_notes(0);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      press_track(track);
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      release_track(track);
      note_interface.clear_note(track);
    }
    return true;
  }

  if (EVENT_CMD(event)) {
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (event->source) {
      case MDX_KEY_YES:
      case MDX_KEY_NO:
        goto exit;
      case MDX_KEY_CLEAR:
        selected_group = PTC_GROUP_OFF;
        sync_channel_encoder(encoders[0], selected_group);
        apply_selected_group();
        return true;
      }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    cycle_group(-1);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    cycle_group(1);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
    goto exit;
  }

  return false;

exit:
  save_ptc_groups();
  mcl.popPage();
  GUI.currentPage()->init();
  return true;
}

PolyPage poly_page(&channel_select_encoder);
