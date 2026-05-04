#include "BankPopupPage.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "GridPage.h"
#include "GridPages.h"
#include "GridLoadPage.h"
#include "MD.h"
#include "MDParams.h"
#include "MidiSetup.h"
#include "MidiActivePeering.h"
#include "MCLActions.h"
#include "Project.h"
#include "NoteInterface.h"

// ENC1 raw-tick threshold per bank step. Higher = slower; the panel
// encoder spits out many ticks per detent on TBD so we need a few of
// them to coalesce into one bank cycle.
#define BANK_POPUP_ENC_RES 2

// ENC1, range 0..7. We do the wrap manually in loop() so the encoder
// itself stays a normal range-clamped MCLEncoder.
MCLEncoder bank_popup_encoder(7, 0, BANK_POPUP_ENC_RES, 4);

BankPopupPage bank_popup_page(&bank_popup_encoder);

void BankPopupPage::setup() {
  // Stash the page we came from so close() can return there cleanly.
  if (grid_page.last_page == 255 && mcl.currentPage() != GRID_PAGE) {
    grid_page.last_page = mcl.currentPage();
  }
}

void BankPopupPage::init() {
  // Seed encoder + state from grid_page.bank.
  bank_popup_encoder.cur = grid_page.bank;
  bank_popup_encoder.old = grid_page.bank;
  last_enc_ = grid_page.bank;

  // State 2 = "popup up". The countdown on AVR is bank_popup == 2 timing
  // out; on TBD we never close on timeout — only via explicit close().
  grid_page.bank_popup = 2;
  grid_page.bank_popup_loadmask = 0;
  grid_page.bank_popup_first_trig = 255;
  grid_page.bank_popup_oled_visible = true;

  bool clear_states = false;
  key_interface.on(clear_states);

  // Sync MD.currentBank to the group of the seeded bank.
  uint8_t group = grid_page.bank / 4;
  if (MD.currentBank != group) {
    MD.currentBank = group;
    if (MD.connected) MD.press_bankgroup_button();
  }
  uint16_t *mask = (uint16_t *)&grid_page.row_states[0];
  mcl_gui.set_trigleds(mask[grid_page.bank], TRIGLED_EXCLUSIVENDYNAMIC);
  grid_page.send_row_led();
}

void BankPopupPage::cleanup() {
  // Mirror of GridPage::close_bank_popup minus the setPage step (handled
  // by the page-pop flow itself).
  if (MD.connected) MD.draw_close_bank();
  key_interface.off();
  grid_page.bank_popup = 0;
  grid_page.bank_popup_loadmask = 0;
  grid_page.bank_popup_first_trig = 255;
  grid_page.bank_popup_oled_visible = true;
  grid_page.last_page = NULL_PAGE;
  note_interface.init_notes();
  MD.set_trigleds(0, TRIGLED_EXCLUSIVENDYNAMIC, 1);
  mcl_gui.set_trigleds_local(0, TRIGLED_EXCLUSIVE);
}

void BankPopupPage::close() {
  if (grid_page.last_page != NULL_PAGE && grid_page.last_page != 255) {
    mcl.setPage(grid_page.last_page);
  } else {
    mcl.setPage(GRID_PAGE);
  }
}

void BankPopupPage::loop() {
  // Translate the bounded encoder value into a wrapping bank index.
  int8_t cur = (int8_t)bank_popup_encoder.cur;
  if (cur != last_enc_) {
    int8_t delta = cur - last_enc_;
    // Normalise — encoder is clamped at 0..7 so delta is small.
    if (delta != 0) {
      int8_t step = (delta > 0) ? 1 : -1;
      uint8_t n = (uint8_t)((delta > 0) ? delta : -delta);
      for (uint8_t i = 0; i < n; i++) step_bank(step, false);
    }
    // Re-sync the encoder display to the new bank so the next rotation
    // again produces a clean delta.
    last_enc_ = (int8_t)grid_page.bank;
    bank_popup_encoder.cur = grid_page.bank;
    bank_popup_encoder.old = grid_page.bank;
  }
}

void BankPopupPage::display() {
  // Draw the underlying grid view first so the bottom 32 px still shows
  // the normal page content; we then overlay the popup window on top.
  grid_page.display();

  if (!grid_page.bank_popup_oled_visible) return;

  // 8 banks in a 2×4 grid (group 0 top, group 1 bottom). Active bank is
  // filled inverse; the rest are dotted-corner outlines via drawRoundRect.
  // Window fills the bottom 32 px (y=32..63) at 96 px wide (centred at
  // x=16) so the underlying GridPage view in the top half stays visible.
  constexpr uint8_t kCellW = 20;
  constexpr uint8_t kCellH = 12;
  constexpr uint8_t kGap   = 4;
  constexpr uint8_t kPad   = 2;
  constexpr uint8_t kGridW = 4 * kCellW + 3 * kGap;          // 92
  constexpr uint8_t kGridH = 2 * kCellH + 1 * kGap;          // 28
  constexpr uint8_t kWinW  = kGridW + 2 * kPad;              // 96
  constexpr uint8_t kWinH  = kGridH + 2 * kPad;              // 32
  constexpr uint8_t kWinX  = 64 - kWinW / 2;                 // 16
  constexpr uint8_t kWinY  = 32;                             // bottom half
  constexpr uint8_t kGridX = kWinX + kPad;
  constexpr uint8_t kGridY = kWinY + kPad;

  oled_display.fillRect(kWinX - 1, kWinY - 1, kWinW + 2, kWinH + 2, BLACK);
  oled_display.drawRect(kWinX, kWinY, kWinW, kWinH, WHITE);

  for (uint8_t i = 0; i < 8; i++) {
    uint8_t row = i / 4;
    uint8_t col = i % 4;
    uint8_t cx  = kGridX + col * (kCellW + kGap);
    uint8_t cy  = kGridY + row * (kCellH + kGap);
    mcl_gui.draw_bank_cell(cx, cy, kCellW, kCellH,
                           (char)('A' + i),
                           /*active=*/(i == grid_page.bank));
  }
}

void BankPopupPage::step_bank(int8_t letter_delta, bool group_toggle) {
  uint8_t old_group = grid_page.bank / 4;
  uint8_t new_bank = group_toggle
                       ? (uint8_t)(grid_page.bank ^ 4)
                       : (uint8_t)((grid_page.bank + 8 + letter_delta) % 8);
  if (new_bank == grid_page.bank) return;
  grid_page.bank = new_bank;
  uint16_t *mask = (uint16_t *)&grid_page.row_states[0];
  mcl_gui.set_trigleds(mask[grid_page.bank], TRIGLED_EXCLUSIVENDYNAMIC);
  grid_page.send_row_led();
  uint8_t new_group = grid_page.bank / 4;
  if (new_group != old_group) {
    MD.currentBank = new_group;
    if (MD.connected) MD.press_bankgroup_button();
  }
  if (MD.connected) MD.draw_bank(grid_page.bank % 4);
}

void BankPopupPage::repaint_chain_leds() {
  uint16_t loadmask = grid_page.bank_popup_loadmask;
  uint16_t head_mask = (uint16_t)1 << grid_page.bank_popup_first_trig;
  uint16_t chained_mask = loadmask & ~head_mask;
  // Layer order: dim-red baseline of populated rows, then chained yellow,
  // then bright red head on top.
  constexpr uint32_t kRedDim    = ((uint32_t)48  << 16);
  constexpr uint32_t kYellow    = ((uint32_t)255 << 16) | ((uint32_t)200 << 8);
  constexpr uint32_t kRedBright = ((uint32_t)255 << 16);
  uint16_t *populated = (uint16_t *)&grid_page.row_states[0];
  mcl_gui.set_trigleds_color(populated[grid_page.bank], kRedDim);
  mcl_gui.set_trigleds_color(chained_mask, kYellow);
  mcl_gui.set_trigleds_color(head_mask, kRedBright);
}

bool BankPopupPage::handleEvent(gui_event_t *event) {
  // BUTTON1 (MCL_Y / NO / cancel): close the popup without picking.
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    close();
    return true;
  }

  // Trig presses: identical chain-load behaviour to the AVR popup
  // handler, but without the "close on no BANK key held" branch (TBD
  // has no real BANK key — popup stays up until close()).
  if (EVENT_NOTE(event)) {
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);
    if (device != &MD) return true;

    uint8_t track = event->source;
    if (track >= NUM_MD_TRACKS) return false;

    uint8_t row = grid_page.bank * 16 + track;

    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (note_interface.notes_all_off()) {
        grid_page.bank_popup_loadmask = 0;
        close();
      }
      return true;
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t load_mode_old = mcl_cfg.load_mode;
      uint8_t load_count = popcount16(grid_page.bank_popup_loadmask);

      if (load_count == 0) {
        grid_page.jump_to_row(row);
        if (load_mode_old != LOAD_AUTO) {
          mcl_cfg.load_mode = LOAD_MANUAL;
        }
        mcl_actions.init_chains();
      }
      if (load_count > 0) {
        mcl_cfg.load_mode = LOAD_QUEUE;
      }

      if (load_count == 1) {
        for (uint8_t n = 0; n < 16; n++) {
          if (IS_BIT_SET16(grid_page.bank_popup_loadmask, n)) {
            uint8_t r = grid_page.bank * 16 + n;
            CLEAR_BIT16(grid_page.bank_popup_loadmask, n);
            grid_page.load_row(n, r);
            break;
          }
        }
      }

      grid_page.load_row(track, row);

      if (grid_page.bank_popup_first_trig == 255) {
        grid_page.bank_popup_first_trig = track;
      }
      grid_page.bank_popup_oled_visible = false;
      repaint_chain_leds();

      mcl_cfg.load_mode = load_mode_old;
      return true;
    }
  }

  return false;
}

#endif // PLATFORM_TBD
