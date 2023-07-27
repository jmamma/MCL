#include "MCL_impl.h"

void MNMTrack::init() {
  machine.init(255);
  //seq_data.init();
}

uint16_t MNMTrack::calc_latency(uint8_t tracknumber) {

  return MNM.setMachine(tracknumber, tracknumber, false);
}

void MNMTrack::transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  uint8_t n = slotnumber;
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->is_generic_midi = false;
  load_seq_data(seq_track);
}

void MNMTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
    DEBUG_PRINTLN(F("here"));
    DEBUG_PRINTLN(F("send MNM track"));
   MNM.insertMachineInKit(tracknumber, &(machine));
   MNM.setMachine(tracknumber, tracknumber, true);
}


void MNMTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINT_FN();
  MNM.insertMachineInKit(tracknumber, &(machine));
  load_seq_data(seq_track);
}

void MNMTrack::load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINT_FN();
  load_seq_data(seq_track);
}

void MNMTrack::get_machine_from_kit(uint8_t tracknumber) {
  DEBUG_PRINT_FN();
  memcpy(machine.params, MNM.kit.parameters[tracknumber], 72);

  machine.track = tracknumber;
  machine.level = MNM.kit.levels[tracknumber];
  machine.model = MNM.kit.models[tracknumber];

  memcpy(machine.modifier.destPage, MNM.kit.destPages + tracknumber, 6*2);
  memcpy(machine.modifier.destParam, MNM.kit.destParams + tracknumber, 6*2);
  memcpy(machine.modifier.range, MNM.kit.destRanges + tracknumber, 6*2);
  machine.modifier.HPKeyTrack = MNM.kit.hpKeyTrack;
  machine.modifier.LPKeyTrack = MNM.kit.lpKeyTrack;
  machine.modifier.mirrorLR = MNM.kit.mirrorLR;
  machine.modifier.mirrorUD = MNM.kit.mirrorUD;

  machine.trig.legatoAmp = MNM.kit.trigLegatoAmp;
  machine.trig.legatoFilter = MNM.kit.trigLegatoFilter;
  machine.trig.legatoLFO = MNM.kit.trigLegatoLFO;
  machine.trig.portamento = MNM.kit.trigPortamento;
  machine.trig.track = MNM.kit.trigTracks[tracknumber];

  machine.type = MNM.kit.types[tracknumber];
}

bool MNMTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                                  bool online, Grid *grid) {

  DEBUG_PRINT_FN();
  active = MNM_TRACK_TYPE;
  uint8_t tracknumber = column;
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  if (tracknumber != 255 && online == true) {
    get_machine_from_kit(tracknumber);

    link.length = seq_track->length;
    link.speed = seq_track->speed;

    if (merge > 0) {
      // TODO decode MNM track data from a pattern...
    } else {
      DEBUG_PRINT("memcpy");
      memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
    }
  }
  // Write data to sd
  uint32_t len = sizeof(MNMTrack);
  bool ret = write_grid((uint8_t *)(this), len, column, row, grid);
  return ret;
}

