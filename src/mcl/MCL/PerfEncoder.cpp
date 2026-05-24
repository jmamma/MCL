#include "PerfEncoder.h"
#include "DeviceParamResolver.h"
#include "MCLMemory.h"
#include "PerfData.h"
#include "MidiUart.h"
#include "MCLStrings.h"
#include "MCLSeq.h"
#include "PerfPageTargetRef.h"

#define DIV_1_127 (1.00f / 127.0f)

PerfScene PerfData::scenes[NUM_SCENES];

PerfEncoder::PerfEncoder(int _max, int _min, int _res, uint8_t _speed)
    : MCLEncoder(_max, _min, _res, _speed) {}

void PerfMorph::populate(PerfScene *s1, PerfScene *s2) {
  count = 0;
  if (s1 == nullptr && s2 == nullptr) { return; }
  if (s1 == nullptr) {
    s1 = s2;
    s2 = nullptr;
  }
  for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
    PerfFade *f = &fades[count];
    PerfParam *p = &s1->params[n];
    if (p->dest != 0) {
      f->dest = p->dest;
      f->param = p->param;
      uint8_t v = 0;
      bool has_current = PerfPageTargetRef::target(p->dest)
                             .get_param(p->param, &v);
      if (!has_current && p->val == 255) {
        continue;
      }
      if (!has_current) {
        v = p->val;
      }
      f->min = p->val == 255 ? v : p->val;
      f->max = v;
      DEBUG_PRINT("ADDING ");
      DEBUG_PRINT(f->min);
      DEBUG_PRINT(" ");
      DEBUG_PRINT(f->max);
      count++;
    }
  }
  if (s2 == nullptr) { return; }
  for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
    PerfFade *f = &fades[count];
    PerfParam *p = &s2->params[n];
    if (p->dest != 0) {
      uint8_t m = find_existing(p->dest, p->param);
      uint8_t v = 0;
      bool has_current = PerfPageTargetRef::target(p->dest)
                             .get_param(p->param, &v);
      if (!has_current && p->val == 255) {
        continue;
      }
      if (!has_current) {
        v = p->val;
      }
      if (m != 255) {
        f = &fades[m];
        DEBUG_PRINTLN("exists");
      } else {
        f->dest = p->dest;
        f->param = p->param;
        f->min = v;
        count++;
        DEBUG_PRINTLN("does not exist");
      }
      f->max = p->val == 255 ? v : p->val;
      DEBUG_PRINT("HERE ");
      DEBUG_PRINT(f->min);
      DEBUG_PRINT(" ");
      DEBUG_PRINT(f->max);
    }
  }
}

void PerfEncoder::send_param(uint8_t dest, uint8_t param, uint8_t val, MidiUartClass *uart_,MidiUartClass *uart2_) {
  PerfPageTargetRef::target(dest).set_param(param, val, uart_, uart2_);
}

void PerfEncoder::send_params(uint8_t cur_, PerfScene *s1, PerfScene *s2, MidiUartClass *uart_,MidiUartClass *uart2_) {
  if (uart_ == nullptr) { uart_ = mcl_seq.primary_output; }
  if (uart2_ == nullptr) { uart2_ = mcl_seq.secondary_output; }

  PerfMorph morph;

  morph.populate(s1, s2);

  for (uint8_t n = 0; n < morph.count; n++) {

    PerfFade *f = &morph.fades[n];
    DEBUG_PRINTLN("send para");
    DEBUG_PRINTLN(f->max);
    DEBUG_PRINTLN(f->min);

    uint8_t val = 0;
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
    send_value(cur, uart_, uart2_);
}

void PerfEncoder::send_value(uint8_t value, MidiUartClass *uart_,MidiUartClass *uart2_) {
    PerfScene *s1 = active_scene_a == 255 ? nullptr :  &perf_data.scenes[active_scene_a];
    PerfScene *s2 = active_scene_b == 255 ? nullptr :  &perf_data.scenes[active_scene_b];
    send_params(value, s1, s2, uart_, uart2_);
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
