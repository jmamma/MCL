#include "PolyPage.h"
#include "MCLGUI.h"
#include "SeqPages.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
#include "MCLSysConfig.h"
#include "Project.h"
#include "TomThumb.h"

namespace {

void ptc_group_label(uint8_t group, char *out) {
  if (group == PTC_GROUP_OFF || group == PTC_GROUP_LOCAL) {
    strcpy(out, "--");
    return;
  }
  mcl_gui.put_value_at(group, out);
}

void ptc_group_box_label(uint8_t group, char *out) {
  if (group == PTC_GROUP_OFF || group == PTC_GROUP_LOCAL) {
    strcpy(out, "--");
    return;
  }
  mcl_gui.put_value_at(group, out);
}

MCLEncoder channel_select_encoder(PTC_MIDI_GROUP_MAX, 0, ENCODER_RES_SEQ);

uint8_t group_from_encoder_value(int value) {
  if (value <= 0) {
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
  uint8_t group = ptc_groups.group_for_track(track);
  return group == PTC_GROUP_LOCAL ? PTC_GROUP_OFF : group;
}

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

  for (uint8_t i = 0; i < 16; i++) {
    uint8_t col = i & 0x7;
    uint8_t row = i >> 3;
    uint8_t x = col * 16;
    uint8_t y = 11 + row * 11;
    uint8_t group = ptc_groups.group_for_track(i);
    bool is_selected = IS_BIT_SET16(selected_tracks, i);
    bool is_note = note_interface.is_note(i);

    if (is_note || is_selected) {
      oled_display.fillRect(x, y, 15, 10, WHITE);
    } else {
      oled_display.fillRect(x, y, 15, 10, BLACK);
    }

    if (!is_note && !is_selected) {
      oled_display.drawRect(x, y, 15, 10, WHITE);
    }

    char label[3] = {};
    ptc_group_box_label(group, label);
    uint8_t text_x = label[1] ? x + 4 : x + 6;
    oled_display.setCursor(text_x, y + 8);
    oled_display.setTextColor((is_note || is_selected) ? BLACK : WHITE);
    oled_display.print(label);

    if (group == PTC_GROUP_LOCAL && !is_note && !is_selected) {
      oled_display.drawFastHLine(x + 3, y + 2, 9, WHITE);
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
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    if (IS_BIT_SET16(selected_tracks, i)) {
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
  oled_display.print(" ");
  char label[4] = {};
  ptc_group_label(selected_group, label);
  oled_display.println(label);

  draw_mask();

  if (selected_tracks != trigled_mask) {
    update_selection_leds(selected_tracks);
  }

}

bool PolyPage::handleEvent(gui_event_t *event) {
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES:
      case MDX_KEY_NO:
        goto exit;
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
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
  exit:
    save_ptc_groups();
    mcl.popPage();
    GUI.currentPage()->init();
    return true;
  }

  return false;
}

PolyPage poly_page(&channel_select_encoder);
