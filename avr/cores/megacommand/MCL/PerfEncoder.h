/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef PERFENCODER_H__
#define PERFENCODER_H__

#include "MCLEncoder.h"
#include "PerfData.h"

class PerfEncoder : public MCLEncoder {
    /**
       \addtogroup gui_rangeencoder_class
       @{
     **/

  public:
  PerfData perf_data;
  uint8_t active_scene_a = 0;
  uint8_t active_scene_b = 1;
  void send_params();
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
