#include "MCL_impl.h"

void MNMTrack::load_immediate(uint8_t tracknumber) {
  DEBUG_PRINT_FN();
  place_track_in_kit(tracknumber, &(MNM.kit));
  load_seq_data(tracknumber);
  store_in_mem(tracknumber);
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


void MNMTrack::place_track_in_kit(uint8_t tracknumber, MNMKit *kit, bool levels) {
  DEBUG_PRINT_FN();

  memcpy(kit->parameters[tracknumber], machine.params, 72);
  if (levels) {
    kit->levels[tracknumber] = machine.level;
  }
  kit->models[tracknumber] = machine.model;

  memcpy(MNM.kit.destPages + tracknumber, machine.modifier.destPage,  6*2);
  memcpy(MNM.kit.destParams + tracknumber, machine.modifier.destParam, 6*2);
  memcpy(MNM.kit.destRanges + tracknumber, machine.modifier.range, 6*2);

  MNM.kit.hpKeyTrack = machine.modifier.HPKeyTrack;
  MNM.kit.lpKeyTrack = machine.modifier.LPKeyTrack;
  MNM.kit.mirrorLR = machine.modifier.mirrorLR;
  MNM.kit.mirrorUD = machine.modifier.mirrorUD;

  MNM.kit.trigLegatoAmp = machine.trig.legatoAmp;
  MNM.kit.trigLegatoFilter = machine.trig.legatoFilter;
  MNM.kit.trigLegatoLFO = machine.trig.legatoLFO;
  MNM.kit.trigPortamento = machine.trig.portamento;
  MNM.kit.trigTracks[tracknumber] = machine.trig.track;

  MNM.kit.types[tracknumber] = machine.type;
}

bool MNMTrack::store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge,
                                  bool online) {

  DEBUG_PRINT_FN();
  active = MNM_TRACK_TYPE;

  if (tracknumber != 255 && online == true) {
    get_machine_from_kit(tracknumber);

    chain.length = mcl_seq.ext_tracks[tracknumber].length;
    chain.speed = mcl_seq.ext_tracks[tracknumber].speed;

    if (merge > 0) {
      // TODO decode MNM track data from a pattern...
    } else {
      memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));
    }
  }
  // Write data to sd
  return proj.write_grid((uint8_t *)(this), sizeof(MNMTrack), tracknumber, row);
}

