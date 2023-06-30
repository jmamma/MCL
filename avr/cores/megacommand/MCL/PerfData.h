/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFDATATRACK_H__
#define PERFDATATRACK_H__
#include "PerfData.h"
#include "WProgram.h"
#include "MCLMemory.h"
#include "MD.h"

#define NUM_PERF_PARAMS 16
#define NUM_SCENES 4
#define PERF_SETTINGS NUM_NUM_PERF_PARAMS

static uint8_t get_param_offset(uint8_t dest, uint8_t param) {
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
  uint8_t scenes[NUM_SCENES];

  uint8_t get_scene_value(uint8_t scene) {
     uint8_t v = scenes[scene];
     if (v == 255) {
        if (dest >= NUM_MD_TRACKS + 4) {
          //Todo MIDI default ?
          v = 0;
        }
        else {
          v = get_param_offset(dest, param);
        }
     }
     if (v == 255) {
       return 0;
     }
     return v;
  }

};

#define LEARN_MIN 1

class PerfData {
public:
  PerfParam params[NUM_PERF_PARAMS];

  uint8_t dest;
  uint8_t param;
  uint8_t min;

  uint8_t active_scenes;

  PerfData() { init_params(); }

  uint8_t find_match(uint8_t dest, uint8_t param, uint8_t scene) {
    uint8_t match = 255;
    uint8_t empty = 255;
    uint8_t s = scene - 1;
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      if (params[a].dest == dest + 1 && params[a].param == param && params[a].scenes[s] != 255) {
          return a;
      }
    }
    return 255;
  }

  uint8_t find_empty() {
    uint8_t match = 255;
    uint8_t empty = 255;

    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      // Find first empty
      if (params[a].dest == 0) {
        return a;
      }
      // Update existing, if matches
    }
    return 255;
  }

  bool check_scene_isempty(uint8_t scene, uint8_t dest = 255) {
    uint8_t match = 255;
    uint8_t empty = 255;
    uint8_t s = scene - 1;
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      if (dest == 255) {
         if (params[a].dest > 0  && params[a].scenes[s] != 255) {
          return false;
        }
      }
      else {
         if (params[a].dest == dest + 1  && params[a].scenes[s] != 255) {
          return false;
         }
      }
    }
    return true;
  }

  void clear_param_scene(uint8_t dest, uint8_t param, uint8_t scene) {
    uint8_t s = scene - 1;
    uint8_t match = 255;
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      // Find match
      if (params[a].dest == dest + 1 && params[a].param == param) {
         if (match == 255) { match = a; }
         params[a].scenes[s] = 255;
      }
    }
    if (check_scene_isempty(scene)) {
      active_scenes &= ~(1 << s);
    }
    for (uint8_t n = 0; n < NUM_SCENES; n++) {
      if (!check_scene_isempty(n + 1,dest)) {
        return;
      }
    }
    if (match == 255) { return; }
    params[match].dest = 0;
    params[match].param = 0;
  }

  uint8_t add_param(uint8_t dest, uint8_t param, uint8_t scene, uint8_t value) {
    uint8_t match = 255;
    uint8_t empty = 255;

    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      // Find first empty
      if (params[a].dest == 0) {
        empty = min(a, empty);
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

    if (scene > 0) {
      uint8_t s = scene - 1;
      active_scenes |= (1 << s);
      params[b].scenes[s] = value;
    }

    return b;
  }

  void *data() const { return (void *)&params; }
  void init_params() {
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      params[a].dest = 0;
      params[a].param = 0;
      memset(params[a].scenes,255,sizeof(params[a].scenes));
    }
  }
  void clear_scene(uint8_t s) {
    active_scenes &= ~(1 << s);
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      params[a].scenes[s] = 255;
    }
  }
};

#endif /* PERFDATATRACK_H__ */
