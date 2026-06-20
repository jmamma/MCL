#include "GUI/Pages/Sequencer/SeqTrackSelectPage.h"

#ifdef PLATFORM_TBD

#include "Devices/DeviceManager.h"
#include "Elektrothic.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "GUI/Pages/Sequencer/SeqPage.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "MCL.h"
#include "MCLGUI.h"
#include "Sequencer/SeqTrackUtil.h"
#include "TBD/TBD.h"
#include "TomThumb.h"
#include "../../../../Drivers/MidiDevice.h"

SeqTrackSelectPage seq_track_select_page;

namespace {

constexpr uint8_t kOverlayY = 0;
constexpr uint8_t kOverlayH = 32;

void draw_track_number(uint8_t y_top, uint8_t track) {
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(MCLGUI::pane_trackid_x,
                         y_top + MCLGUI::pane_trackid_y);
  if (track < 10) {
    oled_display.print('0');
  }
  oled_display.print(track);
}

void draw_device_toggle(uint8_t y_top, const char *primary,
                        const char *secondary, bool primary_active) {
  oled_display.setFont(&TomThumb);
  const uint8_t active_y = y_top + (primary_active ? MCLGUI::pane_label_md_y
                                                   : MCLGUI::pane_label_ex_y);
  oled_display.fillRect(MCLGUI::pane_label_x, active_y,
                        MCLGUI::pane_label_w, MCLGUI::pane_label_h, WHITE);

  oled_display.setCursor(MCLGUI::pane_label_x + 1,
                         y_top + MCLGUI::pane_label_md_y + 6);
  oled_display.setTextColor(primary_active ? BLACK : WHITE);
  oled_display.print(primary);

  oled_display.setCursor(MCLGUI::pane_label_x + 1,
                         y_top + MCLGUI::pane_label_ex_y + 6);
  oled_display.setTextColor(primary_active ? WHITE : BLACK);
  oled_display.print(secondary);
  oled_display.setTextColor(WHITE, BLACK);
}

} // namespace

SeqTrackSelectPage::SeqTrackSelectPage()
    : LightPage(&track_encoder_),
      track_encoder_(127, -127, 1) {}

void SeqTrackSelectPage::begin() {
  sync_from_seq_page();
  if (GUI.overlay != this) {
    GUI.setOverlay(this);
  } else {
    update_leds();
  }
}

void SeqTrackSelectPage::end() {
  if (GUI.overlay == this) {
    GUI.clearOverlay();
    TBD.restore_ui_overlay();
  } else {
    active_ = false;
  }
}

bool SeqTrackSelectPage::is_active() const {
  return active_ && GUI.overlay == this;
}

void SeqTrackSelectPage::toggle_device() {
  DeviceIdx next = device_idx_ == DeviceIdx::Primary ? DeviceIdx::Secondary
                                                     : DeviceIdx::Primary;
  if (SeqPage::device_for_seq_idx(next) == nullptr) {
    return;
  }
  device_idx_ = next;
  SeqPage::select_device_idx(device_idx_);
  track_ = current_track();
  uint8_t count = track_count();
  if (count > 0 && track_ >= count) {
    track_ = count - 1;
  }
  update_leds();
}

void SeqTrackSelectPage::init() {
  active_ = true;
  sync_from_seq_page();
  update_leds();
}

void SeqTrackSelectPage::cleanup() {
  active_ = false;
  mcl_gui.reset_trigleds();
  if (mcl.currentPage() == GRID_PAGE) {
    grid_page.send_row_led();
  }
}

void SeqTrackSelectPage::sync_from_seq_page() {
  DeviceIdx tbd_ui_idx = TBD.ui_device_idx();
  if ((tbd_ui_idx == DeviceIdx::Primary ||
       tbd_ui_idx == DeviceIdx::Secondary) &&
      SeqPage::device_for_seq_idx(tbd_ui_idx) == &TBD) {
    device_idx_ = tbd_ui_idx;
  } else {
    device_idx_ = SeqPage::current_device_idx();
  }
  track_ = current_track();
}

uint8_t SeqTrackSelectPage::track_count() const {
  MidiDevice *device = SeqPage::device_for_seq_idx(device_idx_);
  if (device == nullptr) return 0;
  DeviceMixerCapability *mixer = device->mixer();
  if (mixer != nullptr) {
    return mixer->track_count(DeviceContext::for_device(device, device_idx_));
  }
#ifdef EXT_TRACKS
  if (device_idx_ == DeviceIdx::Secondary) {
    return SeqTrackUtil::track_count(false);
  }
#endif
  return SeqTrackUtil::track_count(true);
}

uint8_t SeqTrackSelectPage::current_track() const {
#ifdef EXT_TRACKS
  if (device_idx_ == DeviceIdx::Secondary) {
    return last_ext_track;
  }
#endif
  return last_primary_track;
}

void SeqTrackSelectPage::update_leds() const {
  uint8_t count = track_count();
  if (count > 16) count = 16;
  if (track_ >= count) {
    mcl_gui.set_trigleds(0, TRIGLED_EXCLUSIVE);
    return;
  }
  mcl_gui.set_trigleds((uint16_t)(1U << track_), TRIGLED_EXCLUSIVE);
}

void SeqTrackSelectPage::move_track(int16_t delta) {
  uint8_t count = track_count();
  if (count == 0 || delta == 0) return;
  int16_t next = (int16_t)track_ + delta;
  while (next < 0) {
    next += count;
  }
  while (next >= count) {
    next -= count;
  }
  select_track((uint8_t)next);
}

bool SeqTrackSelectPage::select_track_via_tbd_ui(uint8_t track) {
  if (!TBD.is_ui_active() || TBD.ui_device_idx() != device_idx_) {
    return false;
  }
  if (SeqPage::device_for_seq_idx(device_idx_) != &TBD) {
    return false;
  }
  if (!TBD.select_ui_track(track)) {
    return false;
  }
  track_ = current_track();
  update_leds();
  return true;
}

bool SeqTrackSelectPage::select_track(uint8_t track) {
  uint8_t count = track_count();
  if (count == 0 || track >= count) {
    return true;
  }

  if (select_track_via_tbd_ui(track)) {
    return true;
  }

  MidiDevice *device = SeqPage::device_for_seq_idx(device_idx_);
  if (device == nullptr) return true;

  SeqPage::select_device_idx(device_idx_);
  seq_step_page.select_track(device, track, false);
  DeviceMixerCapability *mixer = device->mixer();
  if (mixer != nullptr) {
    mixer->select_track(DeviceContext::for_device(device, device_idx_), track);
  }
  track_ = track;
  update_leds();
  return true;
}

void SeqTrackSelectPage::loop() {
  if (!active_) return;
  if (track_encoder_.hasChanged()) {
    int16_t delta = (int16_t)track_encoder_.cur;
    track_encoder_.cur = 0;
    track_encoder_.old = 0;
    move_track(delta);
  }
}

void SeqTrackSelectPage::display() {
  oled_display.fillRect(0, kOverlayY, 128, kOverlayH, BLACK);

  const uint8_t display_track = track_ + 1;
  const bool primary_active = device_idx_ == DeviceIdx::Primary;
  draw_device_toggle(kOverlayY, SeqPage::device_idx_name(DeviceIdx::Primary),
                     SeqPage::device_idx_name(DeviceIdx::Secondary),
                     primary_active);
  draw_track_number(kOverlayY, display_track);

  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setCursor(34, kOverlayY + 4);
  oled_display.print("TRACK SELECT");
  oled_display.setCursor(34, kOverlayY + 17);
  oled_display.print(SeqPage::device_idx_name(device_idx_));
}

#endif // PLATFORM_TBD
