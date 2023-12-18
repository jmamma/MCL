/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFDATATRACK_H__
#define PERFDATATRACK_H__
#include "WProgram.h"
#include "MCLMemory.h"
#include "MD.h"

#define PERF_SETTINGS NUM_NUM_PERF_PARAMS

static uint8_t get_param_device(uint8_t dest, uint8_t param) {
  if (dest <= NUM_MD_TRACKS) {
    return MD.kit.params[dest - 1][param];
  } else {
    switch (dest - NUM_MD_TRACKS - 1) {
    case MD_FX_ECHO - MD_FX_ECHO:
      return MD.kit.delay[param];
      break;
    case MD_FX_DYN - MD_FX_ECHO:
      return MD.kit.dynamics[param];
      break;

    case MD_FX_REV - MD_FX_ECHO:
      return MD.kit.reverb[param];
      break;
    case MD_FX_EQ - MD_FX_ECHO:
      return MD.kit.eq[param];
      break;
    }
  }
  return 255;
}



class PerfParam {
public:
  uint8_t dest;
  uint8_t param;
  uint8_t val;

};

#define LEARN_MIN 1

class PerfScene {
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

class PerfData {
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
    PerfScene *s = &scenes[scene];
    s->init();
  }

  void init_params() {
    for (uint8_t n = 0; n < NUM_SCENES; n++) {
      clear_scene(n);
    }
  }


};

class PerfFade {
public:
  PerfFade() {
    dest = 0;
    param = 0;
    min = 255;
    max = 255;
  }
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
       if (f->dest == 0) { return 255; }
       if (f->dest == dest && f->param == param) { return n; }
    }
    return 255;
  }

  void populate(PerfScene *s1, PerfScene *s2) {
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
           uint8_t v = get_param_device(p->dest, p->param);
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
           uint8_t v = get_param_device(p->dest, p->param);
           if (m != 255) {
             f = &fades[m];
             DEBUG_PRINTLN("exists");
           }
           else {
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
};


#endif /* PERFDATATRACK_H__ */
