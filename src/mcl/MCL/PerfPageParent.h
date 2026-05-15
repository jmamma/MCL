/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFPAGEPARENT_H__
#define PERFPAGEPARENT_H__

#include "../Drivers/DeviceContext.h"
#include "GUI.h"
#include "MCLEncoder.h"

class PerfPageParent {
public:

  PerfPageParent() { };

  void draw_dest(uint8_t knob, uint8_t value, bool dest = true,
                 DeviceIdx device_idx = DeviceIdx::None);
  void draw_param(uint8_t knob, uint8_t dest, uint8_t param,
                  DeviceIdx device_idx = DeviceIdx::None);

  void setup();

  void init();
  void cleanup();
  virtual void config_encoders() {};

};

#endif /* PERFPAGEPARENT_H__ */
