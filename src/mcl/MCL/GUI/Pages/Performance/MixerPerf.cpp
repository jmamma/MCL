/* Justin Mammarella jmamma@gmail.com 2026 */

#include "GUI/Pages/Performance/MixerPerf.h"

#include "GUI/Pages/CommonPages.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "MCLGUI.h"
#include "SeqTrackUtil.h"
#include "../../../../Drivers/MD/MDParams.h"
#include "../../../../Drivers/MidiDevice.h"

namespace MixerPerf {

bool available(MidiDevice *device) {
  return SeqTrackUtil::is_md_device(device);
}

uint8_t mixer_param_for_encoder(uint8_t encoder_idx) {
  switch (encoder_idx) {
  case 1:
    return MODEL_FLTF;
  case 2:
    return MODEL_FLTW;
  case 3:
    return MODEL_FLTQ;
  default:
    return MODEL_LEVEL;
  }
}

void load_locks(Encoder *const *encoders, uint8_t locks[4][4],
                uint8_t state) {
  for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
    PerfEncoder *enc = (PerfEncoder *)encoders[n];
    uint8_t val = locks[state][n];
    if (val < 128) {
      enc->cur = val;
      enc->resend = true;
    }
  }
}

bool handle_preview_lock_edits(Encoder *const *encoders,
                               uint8_t locks[4][4],
                               uint8_t preview_mute_set, bool notes_on) {
  if (!key_interface.is_key_down(MDX_KEY_NO) || preview_mute_set == 255 ||
      notes_on) {
    return false;
  }

  bool handled = false;
  for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
    PerfEncoder *enc = (PerfEncoder *)encoders[n];
    if (!enc->hasChanged()) {
      continue;
    }
    if (BUTTON_DOWN(Buttons.ENCODER1 + n)) {
      GUI.ignoreNextEvent(Buttons.ENCODER1 + n);
    }
    locks[preview_mute_set][n] = enc->cur;
    enc->old = enc->cur;
    handled = true;
  }
  return handled;
}

void func_enc_check() {
  perf_page.func_enc_check();
}

void encoder_send() {
  perf_page.encoder_send();
}

bool should_show_encoder(Encoder *encoder, uint16_t &used_clock,
                         bool activity, uint16_t timeout) {
  if (activity) {
    used_clock = read_clock_ms() + timeout + 1;
  }
  return mcl_gui.show_encoder_value(encoder, timeout);
}

uint8_t display_value(Encoder *encoder, uint8_t locks[4][4],
                      uint8_t preview_mute_set, uint8_t encoder_idx,
                      bool &highlight) {
  highlight =
      preview_mute_set != 255 && locks[preview_mute_set][encoder_idx] != 255;
  return highlight ? locks[preview_mute_set][encoder_idx] : encoder->cur;
}

void clear_scenes(Encoder *encoder) {
  ((PerfEncoder *)encoder)->clear_scenes();
}

void scene_autofill(Encoder *encoder) {
  ((PerfEncoder *)encoder)->scene_autofill();
}

bool handle_preview_lock_button(Encoder *encoder, uint8_t locks[4][4],
                                uint8_t preview_mute_set,
                                uint8_t encoder_idx, bool pressed) {
  if (preview_mute_set == 255 || encoder_idx >= GUI_NUM_ENCODERS) {
    return false;
  }
  if (!pressed) {
    locks[preview_mute_set][encoder_idx] = 255;
    return true;
  }
  if (locks[preview_mute_set][encoder_idx] == 255) {
    GUI.ignoreNextEvent(Buttons.ENCODER1 + encoder_idx);
    locks[preview_mute_set][encoder_idx] = encoder->cur;
  }
  return true;
}

} // namespace MixerPerf
