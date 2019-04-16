#include "MCL.h"

void MDEvents::setup() { midi_events.setup_callbacks(); }
void MDMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  //Don't update lock params if we're recording locks.
  if ((GUI.currentPage() == &seq_rlck_page) && (MidiClock.state == 2)) { return; }

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  DEBUG_PRINTLN(channel);
  DEBUG_PRINTLN(param);
  DEBUG_PRINTLN(value);
  if (param > 119) { return; }
  uint8_t track;
  uint8_t track_param;
  uint8_t param_true = 0;
  if (param >= 16) {
    if (param < 63) {
      param = param - 16;
      track = (param / 24) + (channel - MD.global.baseChannel) * 4;
      track_param = param - ((param / 24) * 24);
    } else if (param >= 72) {
      param = param - 72;
      track = (param / 24) + 2 + (channel - MD.global.baseChannel) * 4;
      track_param = param - ((param / 24) * 24);
    }

    MD.kit.params[track][track_param] = value;
    mcl_seq.md_tracks[track].update_param(track_param, value);

    md_events.last_md_param = track_param;
  } else {
    if (param < 16) {
      track = param - 8 + (channel - MD.global.baseChannel) * 4;
      //+ (channel - MD.global.baseChannel);
      MD.kit.levels[track] = value;
    }
  }

}

void MDMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {}

void MDMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MDMidiEvents::onControlChangeCallback_Midi);
  state = true;
}

void MDMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MDMidiEvents::onControlChangeCallback_Midi);
  state = false;
}

MDEvents md_events;
