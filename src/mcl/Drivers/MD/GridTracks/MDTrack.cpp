#include "MDTrack.h"
#include "MD.h"
#include "MCLActions.h"
#include "Shared.h"
#include "MCLSeq.h"
#include "SeqTrackUtil.h"
#if !defined(__AVR__)
#include "SPSXTrack.h"
#endif

#if !defined(__AVR__)
namespace {

void copy_md_machine_to_spsx(const MDMachine &src, SPSMachine &dest) {
  dest.init();
  memset(dest.params, 0, sizeof(dest.params));
  memcpy(dest.params, src.params, MD_PARAMS_PER_TRACK);
  dest.track = src.track;
  dest.level = src.level;
  dest.model = src.model;
  dest.lfos[0] = src.lfo;
  dest.lfos[1].init(src.track);
  dest.trigGroup = src.trigGroup;
  dest.muteGroup = src.muteGroup;
}

} // namespace
#endif

void MDTrack::paste_track(uint8_t src_track, uint8_t dest_track,
                          SeqTrack *seq_track) {
  DEBUG_PRINTLN(F("paste seq track"));
  if (machine.trigGroup == src_track) {
    machine.trigGroup = 255;
  }
  if (machine.muteGroup == src_track) {
    machine.muteGroup = 255;
  }
  if (machine.lfo.destinationTrack == src_track) {
    machine.lfo.destinationTrack = dest_track;
  }
  load_immediate(dest_track, seq_track);
  bool send_machine = true;
  bool send_level = true;
  MD.sendMachine(dest_track, &(machine), send_level, send_machine);
}

#if !defined(__AVR__)
bool MDTrack::can_materialize_as(uint8_t track_type) {
  if (track_type == MDSPSX_TRACK_TYPE) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}

DeviceTrack *MDTrack::materialize_as(uint8_t track_type, uint8_t tracknumber,
                                     SeqTrack *seq_track) {
  (void)tracknumber;
  (void)seq_track;
  if (track_type == MDSPSX_TRACK_TYPE) {
    GridLink old_link = link;
    MDSeqTrackData old_seq_data;
    MDMachine old_machine = machine;
    SeqTrackModData old_mod_data = mod_data;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *spsx_track =
        static_cast<SPSXTrack *>(
            init_materialized_track_type(MDSPSX_TRACK_TYPE));
    spsx_track->link = old_link;
    spsx_track->seq_storage.mod() = old_mod_data;
    spsx_track->seq_storage.seq_version = SPSX_SEQ_VERSION_LEGACY;
    copy_md_machine_to_spsx(old_machine, spsx_track->machine);
    memcpy(spsx_track->seq_storage.seq_data.legacy.data(), old_seq_data.data(),
           sizeof(old_seq_data));
    return spsx_track;
  }
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}
#endif

uint16_t MDTrack::calc_latency(uint8_t tracknumber) {
  uint8_t n = tracknumber;
  uint16_t md_latency = 0;
  bool send_machine = false, send_level = false;

  md_latency += MD.sendMachine(n, &(machine), send_level, send_machine);
  if (mcl_actions.transition_level[n] == TRANSITION_MUTE ||
      mcl_actions.transition_level[n] == TRANSITION_UNMUTE) {
    md_latency += 3;
  }
  return md_latency;
}

bool MDTrack::transition_cache(uint8_t tracknumber, GridSlot slotnumber) {
  uint8_t n = slotnumber;
  bool send_level = false;
  bool send = true;
  switch (mcl_actions.transition_level[n]) {
  case 1:
    DEBUG_PRINTLN("setting transition level to 0");
    send_level = true;
    machine.level = 0;
    break;
  }
  MD.sendMachineCache(tracknumber, &(machine), send_level, send);
  return true;
}

void MDTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
  uint8_t n = slotnumber;
  DEBUG_PRINTLN("transition send");
  switch (mcl_actions.transition_level[n]) {
  case TRANSITION_UNMUTE:
    DEBUG_PRINTLN(F("unmuting"));
    MD.muteTrack(tracknumber, false);
    SeqTrackUtil::with_md_track(n, [](auto &t) { t.mute_state = SEQ_MUTE_OFF; });
    break;
  case TRANSITION_MUTE:
    DEBUG_PRINTLN(F("muting"));
    MD.muteTrack(tracknumber, true);
    SeqTrackUtil::with_md_track(n, [](auto &t) { t.mute_state = SEQ_MUTE_ON; });
    break;
  default:
    break;
  }
}

void MDTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                              GridSlot slotnumber) {
  seq_track->cache_loaded = false;
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  // load_seq_data(seq_track);
}

void MDTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load immediate");
  MD.insertMachineInKit(tracknumber, &(machine));
  load_seq_data(seq_track);
}

void MDTrack::load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load immediate");
  load_seq_data(seq_track);
}

void MDTrack::get_machine_from_kit(uint8_t tracknumber) {
  //  trackName[0] = '\0';
  memcpy(machine.params, MD.kit.params[tracknumber], MD_PARAMS_PER_TRACK);

  machine.track = tracknumber;
  machine.level = MD.kit.levels[tracknumber];
  machine.model =
      MD.kit.models[tracknumber]; // get_raw_model including tonal bit

  /*Check to see if LFO is modulating host track*/
  /*IF it is then we need to make sure that the LFO destination is updated to
   * the new row posiiton*/

  /*Copies Lfo data from the kit object into the machine object*/
  memcpy(&machine.lfo, &MD.kit.lfos[tracknumber], sizeof(machine.lfo));

  if (MD.kit.lfos[tracknumber].destinationTrack == tracknumber) {
    machine.lfo.destinationTrack = tracknumber;
    machine.track = tracknumber;
  }

  machine.trigGroup = MD.kit.trigGroups[tracknumber];
  machine.muteGroup = MD.kit.muteGroups[tracknumber];
}

void MDTrack::init() {
  machine.init();
  seq_data.init();
  mod_data.init();
}

void MDTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;
  uint8_t *dest = md_seq_track->data();
  memcpy(dest, seq_data.data(), sizeof(seq_data));
  md_seq_track->sync_swing_steps_from_mask();
  load_link_data(seq_track);
  md_seq_track->clear_mutes();
  md_seq_track->set_length(md_seq_track->length);
  md_seq_track->notes.first_trig = true;

  SeqTrack::load_mod_data(
      seq_track, mod_data, true,
      storage_version_at_least(SEQ_TRACK_MOD_STORAGE_VERSION));
}

#if defined(__AVR__)
static uint8_t scale_lock_value(uint8_t value, uint8_t scale) {
  return (((uint16_t)(value - 1) * scale) / 127) + 1;
}

void MDTrack::scale_seq_vol(uint8_t scale) {
  for (uint8_t n = 0; n < NUM_MD_STEPS; n++) {
    auto idx = seq_data.get_lockidx(n);
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      if (seq_data.steps[n].is_lock(c)) {
        if ((seq_data.locks_params[c] == MODEL_LFOD + 1) ||
            (seq_data.locks_params[c] == MODEL_VOL + 1)) {
          seq_data.locks[idx] = scale_lock_value(seq_data.locks[idx], scale);
          if (seq_data.locks[idx] > 128) {
            seq_data.locks[idx] = 128;
          }
        }
        ++idx;
      }
    }
  }
}
#else
void MDTrack::scale_seq_vol(float scale) {
  for (uint8_t n = 0; n < NUM_MD_STEPS; n++) {
    auto idx = seq_data.get_lockidx(n);
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      if (seq_data.steps[n].is_lock(c)) {
        if ((seq_data.locks_params[c] == MODEL_LFOD + 1) ||
            (seq_data.locks_params[c] == MODEL_VOL + 1)) {
          seq_data.locks[idx] =
              (uint8_t)(scale * (float)(seq_data.locks[idx] - 1)) + 1;
          if (seq_data.locks[idx] > 128) {
            seq_data.locks[idx] = 128;
          }
        }
        ++idx;
      }
    }
  }
}

void MDTrack::scale_vol(float scale) {
  normalize();
  machine.scale_vol(scale);
  scale_seq_vol(scale);
}
#endif

void MDTrack::normalize() {
  auto scale = machine.normalize_level();
  scale_seq_vol(scale);
}

bool MDTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track,
                            uint8_t merge, bool online, Grid *grid) {
  active = MD_TRACK_TYPE;

  bool ret;
  DEBUG_PRINT_FN();
  uint8_t tracknumber = column & 0x0F;
  mcl_seq.md_arp_tracks[tracknumber].store_data(&mod_data.arp);
  mcl_seq.grid_x_lfo_tracks[tracknumber].store_data(&mod_data.lfo);

  MDSeqTrack *md_seq_track =
      seq_track ? static_cast<MDSeqTrack *>(seq_track) : nullptr;
  if (md_seq_track) {
    md_seq_track->store_mute_state();
    md_seq_track->sync_swing_mask_from_steps();
  }

  if (column != 255 && online && md_seq_track) {
    get_machine_from_kit(tracknumber);
    DEBUG_DUMP("online");
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);

    if (merge > 0) {
      DEBUG_PRINTLN(F("auto merge"));
      MDSeqTrack temp_seq_track;
      temp_seq_track.init();
      if (merge == SAVE_MERGE) {
        // Load up internal sequencer data
        memcpy(temp_seq_track.data(), md_seq_track->data(),
               sizeof(MDSeqTrackData));
      }
      if (merge == SAVE_MD_PATTERN_IMPORT) {
        link.length = MD.pattern.patternLength;
        link.set_speed(SEQ_SPEED_1X + MD.pattern.doubleTempo);
        DEBUG_PRINTLN(F("SAVE_MD_PATTERN_IMPORT"));
      }
      temp_seq_track.length = link.length;
      temp_seq_track.speed = link.speed_value();

      // merge md pattern data with seq_data
      temp_seq_track.merge_from_md(tracknumber, &(MD.pattern));
      // copy merged data in to this track object's seq data for writing to SD
      memcpy(this->seq_data.data(), temp_seq_track.data(),
             sizeof(MDSeqTrackData));
    } else {
      memcpy(this->seq_data.data(), md_seq_track->data(),
             sizeof(MDSeqTrackData));
    }
    // Normalise track levels
    if (mcl_cfg.auto_normalize == 1) {
      normalize();
    }
    MD.setOrigParams(tracknumber, &machine);
  }
  // Write data to sd

  ret = write_grid(_this(), _sizeof(), column, row, grid);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  DEBUG_DUMP(link.length);
  DEBUG_PRINTLN(F("Track stored in grid"));
  DEBUG_PRINT(column);
  DEBUG_PRINT(F(" "));
  DEBUG_PRINT(row);
  DEBUG_PRINT(F("model"));
  return true;
}

void MDTrack::on_copy(GridColumn s_col, GridColumn d_col, bool destination_same) {
  // bit of a hack to keep lfos modulating the same track.
  if (destination_same) {
    if (machine.trigGroup == s_col) {
      machine.trigGroup = 255;
    }
    if (machine.muteGroup == s_col) {
      machine.muteGroup = 255;
    }
    if (machine.lfo.destinationTrack == s_col) {
      machine.lfo.destinationTrack = d_col;
    }
  } else {
    int lfo_dest = machine.lfo.destinationTrack - s_col;
    int trig_dest = machine.trigGroup - s_col;
    int mute_dest = machine.muteGroup - s_col;
    if (range_check(d_col + lfo_dest, 0, 15)) {
      machine.lfo.destinationTrack = d_col + lfo_dest;
    } else {
      machine.lfo.destinationTrack = 255;
    }
    if (range_check(d_col + trig_dest, 0, 15)) {
      machine.trigGroup = d_col + trig_dest;
    } else {
      machine.trigGroup = 255;
    }
    if (range_check(d_col + mute_dest, 0, 15)) {
      machine.muteGroup = d_col + mute_dest;
    } else {
      machine.muteGroup = 255;
    }
  }
}
