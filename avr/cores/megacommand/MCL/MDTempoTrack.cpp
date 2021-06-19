#include "MCL_impl.h"

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

void MDTempoTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  send_tempo();
}

void MDTempoTrack::get_tempo() { tempo = MidiClock.get_tempo(); }

bool MDTempoTrack::store_in_grid(uint8_t column, uint16_t row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online) {
  active = MDTEMPO_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_tempo();
    if (merge == SAVE_MD) {
        link.length = MD.pattern.patternLength;
        link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
    }
  }

  len = sizeof(MDTempoTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, column, row);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
