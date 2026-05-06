#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "StepSeqTrack.h"
#include "TbdP4SoundData.h"

using TBDSeqTrackData = StepSeqTrackData;
using TBDSeqDataTrack = StepSeqDataTrack;

class TBDSeqTrack : public TBDSeqDataTrack {
public:
  TbdP4SoundData p4_sound;

  TBDSeqTrack();

  void reset();
  void seq(MidiUartClass *uart_);

protected:
  bool get_default_lock_value(uint8_t param_id, uint8_t &value) const override;
  void dispatch_slide_value(uint8_t param, uint8_t value,
                            uint8_t channel) override;

private:
  uint8_t active_note_ = 255;
  uint8_t active_note_channel_ = 0;
  uint16_t gate_ticks_remaining_ = 0;

  void service_gate();
  void send_active_note_off();
  void send_trig(uint8_t velocity = 100);
  void send_parameter_locks(uint8_t step, uint16_t lock_idx);
  void send_lock_value(uint8_t param, uint8_t value);
};

#endif // PLATFORM_TBD
