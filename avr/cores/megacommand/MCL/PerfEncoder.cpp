#include "PerfEncoder.h"
#include "MCLMemory.h"
#include "MD.h"
#include "PerfData.h"

#define DIV_1_127 (1.00f / 127.0f)

PerfScene PerfData::scenes[NUM_SCENES];

void PerfEncoder::send_param(uint8_t dest, uint8_t param, uint8_t val, MidiUartParent *uart_,MidiUartParent *uart2_) {
  if (uart_ == nullptr) { uart_ = &MidiUart; }
  if (uart2_ == nullptr) { uart2_ = &MidiUart2; }

  if (dest >= NUM_MD_TRACKS + 4) {
    uint8_t channel = dest - NUM_MD_TRACKS - 4;
    DEBUG_PRINTLN("send cc");
    DEBUG_PRINT(channel); DEBUG_PRINT(" "); DEBUG_PRINT(param); DEBUG_PRINT(" "); DEBUG_PRINTLN(val);
    uart2_->sendCC(channel, param, val);
  } else if (dest >= NUM_MD_TRACKS) {
    MD.setFXParam(param, val, MD_FX_ECHO + dest - NUM_MD_TRACKS, false, uart_);
  } else {
    MD.setTrackParam(dest, param, val, uart_, false);
  }
}

void PerfEncoder::send_params(uint8_t cur_, PerfScene *s1, PerfScene *s2, MidiUartParent *uart_,MidiUartParent *uart2_) {
  PerfMorph morph;

  morph.populate(s1, s2);

  for (uint8_t n = 0; n < morph.count; n++) {

    PerfFade *f = &morph.fades[n];
    DEBUG_PRINTLN("send para");
    DEBUG_PRINTLN(f->max);
    DEBUG_PRINTLN(f->min);

    uint8_t val = 0;
    if (f->max == 255 || f->min == 255) {
      continue;
    }
    int8_t range = f->max - f->min;
    int16_t q = cur_ * range;
    DEBUG_PRINTLN("range");
    DEBUG_PRINTLN(range);
    DEBUG_PRINTLN(cur);
    val = ((int16_t)q / (int16_t)127) + f->min;
    if (val > 127) {
      continue;
    }
    DEBUG_PRINTLN(val);
    send_param(f->dest - 1, f->param, val, uart_, uart2_);
  }
}
void PerfEncoder::send(MidiUartParent *uart_,MidiUartParent *uart2_) {
    PerfScene *s1 = active_scene_a == 255 ? nullptr :  &perf_data.scenes[active_scene_a];
    PerfScene *s2 = active_scene_b == 255 ? nullptr :  &perf_data.scenes[active_scene_b];
    send_params(cur, s1, s2, uart_, uart2_);
    resend = false;
}

int PerfEncoder::update(encoder_t *enc) {
  MCLEncoder::update(enc);
  // Update all params
  return cur;
}
void PerfEncoder::scene_autofill() {
  perf_data.scene_autofill(active_scene_b);
  cur = 127;
  old = 127;
}

void PerfEncoder::clear_scenes() {
  oled_display.textbox("CLEAR SCENES", "");
  perf_data.clear_scene(active_scene_a);
  perf_data.clear_scene(active_scene_b);
}
