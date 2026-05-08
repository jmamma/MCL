#include "MDTempoTrack.h"
#include "MidiClock.h"
#include "MD.h"
#include "MDTrack.h"
void MDTempoTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  send_tempo();
}

uint16_t MDTempoTrack::calc_latency(uint8_t tracknumber) {
  bool send = false;
  return send_tempo(send);
}

uint16_t MDTempoTrack::send_tempo(bool send) {
  return MD.setTempo(tempo, send);
}

void MDTempoTrack::load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
}

void MDTempoTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  send_tempo();
}

void MDTempoTrack::get_tempo() {
  if (MidiClock.uart_clock_recv == MD.uart) {
    uint16_t tp;
    if (MD.get_tempo(tp)) {
      tempo = (float)tp * 0.0416667f;
      return;
    }
  }
  //Fall back to clock measurement;
  tempo = MidiClock.get_tempo();
}

void MDTempoTrack::get_online_data(uint8_t merge) {
  get_tempo();
  update_link_from_pattern(merge);
}
