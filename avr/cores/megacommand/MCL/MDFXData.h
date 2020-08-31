#pragma once

class MDFXData {
public:
  bool enable_reverb;
  bool enable_delay;
  bool enable_eq;
  bool enable_dynamics;
  /** The settings of the reverb effect. f**/
  uint8_t reverb[8];
  /** The settings of the delay effect. **/
  uint8_t delay[8];
  /** The settings of the EQ effect. **/
  uint8_t eq[8];
  /** The settings of the compressor effect. **/
  uint8_t dynamics[8];
};
