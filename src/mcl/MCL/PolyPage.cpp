#include "PolyPage.h"
#include "MCLGUI.h"
#include "../Drivers/MD/MD.h"
#include "SeqPages.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"

namespace {

void ptc_group_label(uint8_t group, char *out) {
  if (group == PTC_GROUP_LOCAL) {
    strcpy(out, "LOC");
    return;
  }
  out[0] = 'C';
  mcl_gui.put_value_at(group, out + 1);
}

} // namespace

void PolyPage::init() {
  DEBUG_PRINT_FN();
  selected_group = ptc_groups.group_for_track(last_md_track);
  if (selected_group == PTC_GROUP_OFF) {
    selected_group = PTC_GROUP_LOCAL;
  }
  key_interface.on();
  note_interface.init_notes();
  mcl_gui.set_trigleds(ptc_groups.mask_for_group(selected_group),
                       TRIGLED_EXCLUSIVE);
}

void PolyPage::cleanup() {
  seq_ptc_page.init_poly();
  key_interface.off();
}

void PolyPage::draw_mask() {
  uint16_t selected_mask = ptc_groups.mask_for_group(selected_group);
  for (uint8_t i = 0; i < 16; i++) {

    uint8_t x = i * 8;

    bool is_note = note_interface.is_note(i);
    oled_display.fillRect(x, 2, 6, 6, is_note);
    if (is_note) {
    }
    else if (IS_BIT_SET16(selected_mask, i)) {
      oled_display.drawRect(x, 2, 6, 6, WHITE);

    }
    else if (ptc_groups.track_has_group(i)) {
      oled_display.drawFastVLine(x + 3, 2, 6, WHITE);
    }
    else {
      oled_display.drawFastHLine(x, 5, 6, WHITE);
    }
  }
}

void PolyPage::sync_legacy_mask() {
  mcl_cfg.poly_mask = ptc_groups.legacy_poly_mask();
}

void PolyPage::cycle_group(int8_t direction) {
  if (direction > 0) {
    selected_group = selected_group == PTC_GROUP_LOCAL ? PTC_MIDI_GROUP_MIN
                                                       : selected_group + 1;
    if (selected_group > PTC_MIDI_GROUP_MAX) {
      selected_group = PTC_GROUP_LOCAL;
    }
  } else {
    selected_group = selected_group == PTC_GROUP_LOCAL ? PTC_MIDI_GROUP_MAX
                                                       : selected_group - 1;
    if (selected_group < PTC_MIDI_GROUP_MIN) {
      selected_group = PTC_GROUP_LOCAL;
    }
  }
  trigled_mask = 0xffff;
}

void PolyPage::toggle_group(uint8_t i) {
  uint8_t group = ptc_groups.group_for_track(i);
  ptc_groups.set_track_group(i, group == selected_group ? PTC_GROUP_OFF
                                                        : selected_group);
  sync_legacy_mask();
}

void PolyPage::display() {
  oled_display.clearDisplay();

  oled_display.setCursor(0, 15);
  mcl_print_P(mclstr_voice_select);
  oled_display.print(" ");
  char label[4] = {};
  ptc_group_label(selected_group, label);
  oled_display.println(label);

  draw_mask();

  uint16_t selected_mask = ptc_groups.mask_for_group(selected_group);
  if (selected_mask != trigled_mask) {
    trigled_mask = selected_mask;
    mcl_gui.set_trigleds(selected_mask, TRIGLED_EXCLUSIVE);
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
    if (event->mask == EVENT_BUTTON_RELEASED) {
      //  if ((encoders[2]->getValue() == 0)) {

      //    toggle_mute(track);
      //// }

      // else {
      toggle_group(track);
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      note_interface.clear_note(track);
    }
    //  }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
  exit:
    sync_legacy_mask();
    mcl_cfg.write_cfg();
    mcl.popPage();
    GUI.currentPage()->init();
    return true;
  }

  return false;
}

PolyPage poly_page;
