#include "MNMTrack.h"
#include "Sequencer/MCLSeq.h"
#include "MNM.h"

#if defined(__AVR__)
namespace {
constexpr uint16_t kMnmSetMachineLatency =
    sizeof(monomachine_sysex_hdr) + 2 + 4 + (56 * 3) + 3;
}
#endif

void MNMTrack::init() {
  machine.init(255);
  load_fade.init();
  //seq_data.init();
}

uint16_t MNMTrack::grid_slot_label(GridSlotLabelContext ctx) {
  auto tmp = getMNMMachineNameShort(ctx.model, 2);
  if (!tmp) {
    return 0;
  }
  return make_grid_slot_label(tmp[0], tmp[1]);
}

#if !defined(__AVR__)
uint16_t MNMMidiTrack::grid_slot_label(GridSlotLabelContext ctx) {
  auto tmp = getMNMMachineNameShort(ctx.model, 2);
  if (!tmp) {
    return 0;
  }
  return make_grid_slot_label(tmp[0], tmp[1]);
}
#endif

uint16_t MNMTrack::calc_latency(uint8_t tracknumber) {
#if defined(__AVR__)
  (void)tracknumber;
  return kMnmSetMachineLatency;
#else
  return MNM.setMachine(tracknumber, tracknumber, false);
#endif
}

void MNMTrack::transition_load(uint8_t tracknumber, SeqTrack* seq_track, GridSlot slotnumber) {
  transition_load_device(tracknumber, seq_track, slotnumber);
}

void MNMTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
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

bool MNMTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track, uint8_t merge,
                                  bool online, Grid *grid) {

  DEBUG_PRINT_FN();
  active = MNM_TRACK_TYPE;
  uint8_t tracknumber = column & 0xF;
  SeqTrack::store_mod_data(mod_data, false, tracknumber);
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  if (column != 255 && online == true) {
    get_machine_from_kit(tracknumber);

    link.length = seq_track->length;
    link.set_speed(seq_track->speed);

    if (merge > 0) {
      // TODO decode MNM track data from a pattern...
    } else {
      DEBUG_PRINT("memcpy");
      memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
    }
  }
  // Write data to sd
  return write_grid(_this(), get_track_size(), column, row, grid);
}

#if !defined(__AVR__)
bool MNMTrack::can_materialize_as(uint8_t track_type) {
  if (track_type == MNM_MIDI_TRACK_TYPE) {
    return true;
  }
  if (ExtTrack::can_materialize_legacy_ext(active, track_type)) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}
#endif

bool MNMTrack::materialized_storage_range(uint8_t track_type,
                                          uint16_t &source_offset,
                                          uint16_t &target_offset,
                                          uint16_t &len) {
  if (track_type != EXT_TRACK_TYPE) {
    return false;
  }
  source_offset =
      reinterpret_cast<uintptr_t>(&seq_data) -
      reinterpret_cast<uintptr_t>(_this());
  target_offset = ExtTrack::seq_payload_storage_offset();
  len = ExtTrack::seq_payload_storage_size();
  return true;
}

DeviceTrack *MNMTrack::materialize_as(uint8_t track_type,
                                      uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  (void)seq_track;
#if !defined(__AVR__)
  if (track_type == MNM_MIDI_TRACK_TYPE) {
    GridLink old_link = link;
    ExtSeqTrackData old_seq_data;
    SeqTrackModData old_mod_data = mod_data;
    MNMMachine old_machine = machine;
    TrackLoadFadeData old_load_fade = load_fade;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *midi_track =
        static_cast<MNMMidiTrack *>(
            init_materialized_track_type(MNM_MIDI_TRACK_TYPE));
    midi_track->import_legacy(old_link, old_seq_data, old_mod_data,
                              old_machine, old_load_fade, tracknumber);
    return midi_track;
  }
  if (ExtTrack::can_materialize_legacy_ext(active, track_type)) {
    return ExtTrack::materialize_legacy_ext(*this, link, seq_data, mod_data,
                                            track_type, tracknumber);
  }
#endif
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

#if !defined(__AVR__)
void MNMMidiTrack::import_legacy(const GridLink &old_link,
                                 const ExtSeqTrackData &old_seq_data,
                                 const SeqTrackModData &old_mod_data,
                                 const MNMMachine &old_machine,
                                 const TrackLoadFadeData &old_load_fade,
                                 uint8_t tracknumber) {
  machine = old_machine;
  load_fade = old_load_fade;
  import_legacy_ext_storage(old_link, old_seq_data, old_mod_data, tracknumber);
}

void MNMMidiTrack::init() {
  machine.init(255);
}

uint16_t MNMMidiTrack::calc_latency(uint8_t tracknumber) {
  return MNM.setMachine(tracknumber, tracknumber, false);
}

void MNMMidiTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                   GridSlot slotnumber) {
  MidiBackedDeviceTrack::transition_load(tracknumber, seq_track, slotnumber);
}

void MNMMidiTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
  DEBUG_PRINTLN(F("here"));
  DEBUG_PRINTLN(F("send MNM track"));
  MNM.insertMachineInKit(tracknumber, &(machine));
  MNM.setMachine(tracknumber, tracknumber, true);
}

void MNMMidiTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINT_FN();
  MNM.insertMachineInKit(tracknumber, &(machine));
  load_seq_data(seq_track);
}

void MNMMidiTrack::load_immediate_cleared(uint8_t tracknumber,
                                          SeqTrack *seq_track) {
  DEBUG_PRINT_FN();
  load_seq_data(seq_track);
}

void MNMMidiTrack::get_machine_from_kit(uint8_t tracknumber) {
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

bool MNMMidiTrack::store_in_grid(GridSlot column, GridRow row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online, Grid *grid) {
  (void)merge;
  DEBUG_PRINT_FN();
  active = MNM_MIDI_TRACK_TYPE;
  uint8_t tracknumber = column & 0x0F;
  if (column != 255 && online == true) {
    get_machine_from_kit(tracknumber);
  }
  store_midi_seq_data(column, seq_track);
  return write_grid(_this(), get_store_size(), column, row, grid);
}
#endif
