/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef PERFENCODER_H__
#define PERFENCODER_H__

#include "MCLEncoder.h"
#include "PerfData.h"

#define PERF_NAME_LENGTH 9

class PerfEncoder : public MCLEncoder {
    /**
       \addtogroup gui_rangeencoder_class
       @{
     **/

  public:
  char name[PERF_NAME_LENGTH] = "CONTROL";

  PerfData perf_data;

  uint8_t active_scene_a = 0;
  uint8_t active_scene_b = 1;

  bool resend = false;

  void init() { perf_data.init(); }
  void send(MidiUartClass *uart_ = nullptr,MidiUartClass *uart2_ = nullptr);
  void send_param(uint8_t dest, uint8_t param, uint8_t val, MidiUartClass *uart_ = nullptr, MidiUartClass *uart2_ = nullptr);
  void send_params(uint8_t cur_, PerfScene *s1, PerfScene *s2, MidiUartClass *uart_ = nullptr,MidiUartClass *uart2_ = nullptr);
  void scene_autofill();
  void clear_scenes();
  /**
       Create a new range-limited encoder with max and min value, short
       name, initial value, and handling function. The initRangeEncoder
       will be called with the constructor arguments.
     **/
    PerfEncoder(int _max = 127, int _min = 0, int res = 1) : MCLEncoder(_max , _min, res) {
    }

   virtual int update(encoder_t *enc);

};

#endif /* PerfENCODER_H__ */
