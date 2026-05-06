/* Justin Mammarella jmamma@gmail.com 2026 */

#pragma once

#include "platform.h"
#include <stdint.h>
#include <string.h>

#ifdef PLATFORM_TBD

struct SeqExtParsedControl {
  bool has_value = false;
  uint8_t ctrl_type = 0;
  uint16_t parameter = 0;
  uint16_t value = 0;
};

class SeqExtMidiControlState {
public:
  static constexpr uint8_t CTRL_OFF = 0;
  static constexpr uint8_t CTRL_NRPN = 2;
  static constexpr uint8_t CTRL_RPN = 3;

  SeqExtMidiControlState() { reset(); }

  void reset() {
    memset(param_msb_, 0xFF, sizeof(param_msb_));
    memset(param_lsb_, 0xFF, sizeof(param_lsb_));
    memset(value_msb_, 0xFF, sizeof(value_msb_));
    memset(value_lsb_, 0x00, sizeof(value_lsb_));
    memset(ctrl_type_, CTRL_OFF, sizeof(ctrl_type_));
  }

  bool parse_cc(uint8_t channel, uint8_t cc, uint8_t value,
                SeqExtParsedControl &out) {
    channel &= 0x0F;
    out = SeqExtParsedControl();

    switch (cc) {
    case 99: // NRPN MSB
      ctrl_type_[channel] = CTRL_NRPN;
      param_msb_[channel] = value & 0x7F;
      value_msb_[channel] = 0xFF;
      value_lsb_[channel] = 0;
      return true;
    case 98: // NRPN LSB
      ctrl_type_[channel] = CTRL_NRPN;
      param_lsb_[channel] = value & 0x7F;
      value_msb_[channel] = 0xFF;
      value_lsb_[channel] = 0;
      return true;
    case 101: // RPN MSB
      ctrl_type_[channel] = CTRL_RPN;
      param_msb_[channel] = value & 0x7F;
      value_msb_[channel] = 0xFF;
      value_lsb_[channel] = 0;
      return true;
    case 100: // RPN LSB
      ctrl_type_[channel] = CTRL_RPN;
      param_lsb_[channel] = value & 0x7F;
      value_msb_[channel] = 0xFF;
      value_lsb_[channel] = 0;
      if (param_msb_[channel] == 127 && param_lsb_[channel] == 127) {
        ctrl_type_[channel] = CTRL_OFF;
      }
      return true;
    case 6: // Data Entry MSB
      value_msb_[channel] = value & 0x7F;
      return make_value(channel, out);
    case 38: // Data Entry LSB
      value_lsb_[channel] = value & 0x7F;
      if (value_msb_[channel] == 0xFF) return true;
      return make_value(channel, out);
    default:
      return false;
    }
  }

private:
  bool make_value(uint8_t channel, SeqExtParsedControl &out) const {
    if (ctrl_type_[channel] == CTRL_OFF ||
        param_msb_[channel] == 0xFF || param_lsb_[channel] == 0xFF ||
        value_msb_[channel] == 0xFF) {
      return true;
    }

    out.has_value = true;
    out.ctrl_type = ctrl_type_[channel];
    out.parameter =
        ((uint16_t)(param_msb_[channel] & 0x7F) << 7) |
        (param_lsb_[channel] & 0x7F);
    out.value =
        ((uint16_t)(value_msb_[channel] & 0x7F) << 7) |
        (value_lsb_[channel] & 0x7F);
    return true;
  }

  uint8_t ctrl_type_[16];
  uint8_t param_msb_[16];
  uint8_t param_lsb_[16];
  uint8_t value_msb_[16];
  uint8_t value_lsb_[16];
};

#endif // PLATFORM_TBD
