#include "PerfEncoder.h"
#include "DeviceParamResolver.h"
#include "MCLMemory.h"
#include "PerfData.h"
#include "MidiUart.h"
#include "MCLStrings.h"
#include "MCLSeq.h"

#define DIV_1_127 (1.00f / 127.0f)

PerfScene PerfData::scenes[NUM_SCENES];

PerfEncoder::PerfEncoder(int _max, int _min, int _res, uint8_t _speed)
    : MCLEncoder(_max, _min, _res, _speed) {}

void PerfEncoder::send_param(uint8_t dest, uint8_t param, uint8_t val, MidiUartClass *uart_,MidiUartClass *uart2_) {
  if (uart_ == nullptr) { uart_ = mcl_seq.primary_output; }
  if (uart2_ == nullptr) { uart2_ = mcl_seq.secondary_output; }
  DevicePerfTarget target = DeviceParamResolver::perf(dest);
  target.set_param(param, val, target.device_index() == 1 ? uart2_ : uart_);
}

void PerfEncoder::send_params(uint8_t cur_, PerfScene *s1, PerfScene *s2, MidiUartClass *uart_,MidiUartClass *uart2_) {
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
    send_param(f->dest, f->param, val, uart_, uart2_);
  }
}
void PerfEncoder::send(MidiUartClass *uart_,MidiUartClass *uart2_) {
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
  oled_display.textbox_P(mclstr_clear, mclstr_scenes);
  perf_data.clear_scene(active_scene_a);
  perf_data.clear_scene(active_scene_b);
}
