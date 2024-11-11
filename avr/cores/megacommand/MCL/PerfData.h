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
    return MD.kit.get_fx_param(dest - NUM_MD_TRACKS - 1 + MD_FX_ECHO, param);
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
     oled_display.textbox("FILL SCENES", "");
     if (scene >= NUM_SCENES) { return; }

     uint8_t *params = (uint8_t *) &MD.kit.params;
     uint8_t *params_orig = (uint8_t *) &MD.kit.params_orig;

     for (uint8_t track = 0; track < 16; track++) {
       for (uint8_t param = 0; param < 24; param++) {
         if (MD.kit.params[track][param] != MD.kit.params_orig[track][param]) {
           if (add_param(track,param,scene,MD.kit.params[track][param]) != 255) {
             //Kit encoders go back to normal, for save.
             uint8_t val = MD.kit.params[track][param];
             MD.setTrackParam(track, param, MD.kit.params_orig[track][param], nullptr,
                     true);
             MD.setTrackParam(track, param, val, nullptr,
                     false);
           }
         }

       }
     }
     for (uint8_t n = 0; n < 8 * 4; n++) {
       uint8_t fx = n / 8;
       uint8_t param = n - fx * 8;
       //delay and reverb are flipped in memory
       if (fx == 0) { fx = 1; }
       else if (fx == 1) { fx = 0; }
       uint8_t *fxs = (uint8_t *) &MD.kit.reverb;
       uint8_t *fxs_orig = (uint8_t *) &MD.kit.fx_orig;
       if (fxs[n] != fxs_orig[n]) {
         if (add_param(fx + NUM_MD_TRACKS,param,scene,fxs[n]) != 255) {
           uint8_t val = fxs[n];
           MD.setFXParam(param, fxs_orig[n], fx + MD_FX_ECHO, true);
           MD.setFXParam(param, val, fx + MD_FX_ECHO, false);
         }
       }
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
