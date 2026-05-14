/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFDATATRACK_H__
#define PERFDATATRACK_H__
#include "platform.h"
#include "DeviceParamResolver.h"
#include "MCLMemory.h"
#include "MCLStrings.h"
#include "oled.h"

#define PERF_SETTINGS NUM_NUM_PERF_PARAMS

class ATTR_PACKED() PerfParam {
public:
  uint8_t dest;
  uint8_t param;
  uint8_t val;

};

#define LEARN_MIN 1

class ATTR_PACKED() PerfScene {
public:
  PerfParam params[NUM_PERF_PARAMS];
  uint8_t count;
  PerfScene() { }

  bool is_active() { return count > 0; }

  void init() {
    count = 0;
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      PerfParam *p = &params[a];
      p->dest = 0;
      p->param = 0;
      p->val = 255;
    }
  }

  void debug() {
    DEBUG_PRINT("count: "); DEBUG_PRINTLN(count);
    for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
    DEBUG_PRINT("PARAM "); DEBUG_PRINT(n); 
    DEBUG_PRINT(" DEST:"); DEBUG_PRINT(params[n].dest); 
    DEBUG_PRINT(" PARAM:"); DEBUG_PRINT(params[n].param);
    DEBUG_PRINT(" DEST:"); DEBUG_PRINT(params[n].dest); 
    DEBUG_PRINTLN("");
    }
  }

  uint8_t add_param(uint8_t dest, uint8_t param, uint8_t value) {

    uint8_t match = 255;
    uint8_t empty = 255;

    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      // Find first empty
      if (params[a].dest == 0) {
        if (empty == 255) { empty = a; }
      }
      if (params[a].dest == dest + 1 && params[a].param == param) {
        // Update existing, if matches
        match = a;
      }
    }

    uint8_t b = match;
    if (b == 255) {
      b = empty;
    }
    if (b == 255) {
      return 255;
    }

    params[b].dest = dest + 1;
    params[b].param = param;
    params[b].val = value;
    if (match == 255) {
      count++;
    }
    return b;
  }

  void clear_param(uint8_t dest, uint8_t param) {
    if (count == 0) { return; }
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      // Find match
      PerfParam *p = &params[a];
      if (p->dest == dest + 1 && p->param == param) {
         p->val = 255;
         p->dest = 0;
         p->param = 0;
         count--;
      }
    }
  }

  uint8_t find_empty() {
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      // Find first empty
      if (params[a].dest == 0) {
        return a;
      }
      // Update existing, if matches
    }
    return 255;
  }

  uint8_t find_match(uint8_t dest_, uint8_t param_) {

    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      PerfParam *p = &params[a];
      if (p->dest == dest_ + 1 && p->param == param_ && p->val != 255) {
          return a;
      }
    }
    return 255;
  }


};

class ATTR_PACKED() PerfData {
public:
  static PerfScene scenes[NUM_SCENES];

  uint8_t src;
  uint8_t param;
  uint8_t min;


  PerfData() { }

  void init() {
    src = param = min = 0;
    init_params();
  }

  void *data() const { return (void *)&scenes; }

  void update_src(uint8_t src_, uint8_t param_, uint8_t min_) {
    src = src_;
    param = param_;
    min = min_;
  }

  uint16_t get_active_scene_mask() {
    uint16_t mask = 0;
    for (uint8_t n = 0; n < NUM_SCENES; n++) {
       if (scenes[n].is_active()) {
         mask |= (1 << n);
       }
    }
    return mask;
  }

  uint8_t find_match(uint8_t dest_, uint8_t param_, uint8_t scene) {
    PerfScene *s = &scenes[scene];
    return s->find_match(dest_, param_);
  }

  void clear_param_scene(uint8_t dest_, uint8_t param_, uint8_t scene) {

    PerfScene *s = &scenes[scene];
    s->clear_param(dest_, param_);

  }

  uint8_t add_param(uint8_t dest_, uint8_t param_, uint8_t scene, uint8_t value) {

    PerfScene *s = &scenes[scene];
    return s->add_param(dest_,param_,value);
  }

  void clear_scene(uint8_t scene) {
    if (scene < NUM_SCENES) {
      PerfScene *s = &scenes[scene];
      s->init();
    }
  }

  void init_params() {
    for (uint8_t n = 0; n < NUM_SCENES; n++) {
      clear_scene(n);
    }
  }

  void scene_autofill(uint8_t scene) {
     oled_display.textbox_P(mclstr_fill, mclstr_scenes);
     if (scene >= NUM_SCENES) { return; }
     DeviceParamResolver::perf_scene_autofill(this, scene);
  }

};

class PerfFade {
public:
  uint8_t dest;
  uint8_t param;
  uint8_t min;
  uint8_t max;
};


class PerfMorph {
public:
  PerfFade fades[NUM_PERF_PARAMS * 2];
  uint8_t count;
  PerfMorph() {
  }

  uint8_t find_existing(uint8_t dest, uint8_t param) {
    for (uint8_t n = 0; n < count; n++) {
       PerfFade *f = &fades[n];
       if (f->dest == dest && f->param == param) { return n; }
    }
    return 255;
  }

  void populate(PerfScene *s1, PerfScene *s2);
};


#endif /* PERFDATATRACK_H__ */
