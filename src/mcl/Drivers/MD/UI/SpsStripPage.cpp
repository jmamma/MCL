#include "SpsStripPage.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "../MD.h"
#include "../MDParams.h"
#include "SpsMode.h"
#include <string.h>

SpsStripPage sps_strip_page;

namespace {
// XOR-invert a rectangle in the OLED framebuffer (matches the same
// helper in SpsOverlayPage / SpsMode for locked-slot marking).
static void invert_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  for (uint8_t yy = y; yy < y + h; yy++) {
    for (uint8_t xx = x; xx < x + w; xx++) {
      oled_display.drawPixel(xx, yy, 2);
    }
  }
}
} // namespace

void SpsStripPage::display() {
  // Pages that own the panel palette skip the strip overlay entirely
  // (they're already current; we don't fight their rendering).
  PageIndex pg = mcl.currentPage();
  if (pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE) return;

  constexpr uint8_t y_top = 32;

  // Per-slot lock substitution (mirrors SpsMode::send_param's
  // step-edit detection): on SEQ_STEP_PAGE with a trig held the dial
  // and value reflect the held step's lock value, and the slot rect
  // gets XOR-inverted.
  Encoder lock_proxy[4];
  Encoder *encs[4];
  const char *labels[4];
  bool show[4];
  bool locked[4] = {false, false, false, false};
  uint8_t base = MD.ui.sps_mode.sub_page() * 4;
  uint8_t param_count = MD.ui.sps_mode.param_count();
  const uint8_t track = (MD.currentTrack < 16) ? MD.currentTrack : 0;
  uint8_t model = MD.kit.get_model(track);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    if (param >= param_count) {
      encs[i] = nullptr;
      labels[i] = nullptr;
      show[i] = false;
      continue;
    }
    uint8_t lock_v;
    if (MD.ui.sps_mode.active_step_lock(param, &lock_v)) {
      lock_proxy[i].cur = lock_v;
      lock_proxy[i].old = lock_v;
      encs[i] = &lock_proxy[i];
      locked[i] = true;
      show[i] = true;
    } else {
      encs[i] = &MD.ui.sps_mode.enc[i];
      show[i] = MD.ui.sps_mode.show_strip_value(i);
    }
    labels[i] = model_param_name(model, param);
  }
  mcl_gui.draw_encoder_strip(y_top, encs, labels, show);

  // Invert covering both dial + value text on locked slots.
  // Width = max(text_width, dial_width) + 2 px each side; height
  // covers the dial top through value text bottom (23 px).
  constexpr uint8_t kCellW = 32;
  constexpr uint8_t kDialW = 11;
  for (uint8_t i = 0; i < 4; i++) {
    if (!locked[i]) continue;
    char val[4];
    mcl_gui.put_value_at((uint8_t)encs[i]->cur, val);
    uint8_t vw = (uint8_t)strlen(val) * 6;
    uint8_t inner_w = (vw > kDialW) ? vw : kDialW;
    uint8_t cx = i * kCellW;
    uint8_t inv_x = cx + (kCellW - inner_w) / 2 - 2;
    invert_rect(inv_x, y_top + 3, inner_w + 4, 23);
  }
}

#endif // PLATFORM_TBD
