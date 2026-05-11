#include "SPSXTrack.h"
#include "MD.h"
#include "MCLActions.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "Shared.h"

#if !defined(__AVR__)

namespace {

uint8_t legacy_cond_to_spsx(uint8_t condition) {
  switch (condition) {
  case 0:
  case 1:
    return SPSX_COND_100PCT;
  case 2:
    return spsx_cond_iter_encode(2, 2);
  case 3:
    return spsx_cond_iter_encode(3, 3);
  case 4:
    return spsx_cond_iter_encode(4, 4);
  case 5:
    return spsx_cond_iter_encode(5, 5);
  case 6:
    return spsx_cond_iter_encode(6, 6);
  case 7:
    return spsx_cond_iter_encode(7, 7);
  case 8:
    return spsx_cond_iter_encode(8, 8);
  case 9:
    return SPSX_COND_10PCT;
  case 10:
    return SPSX_COND_25PCT;
  case 11:
    return SPSX_COND_50PCT;
  case 12:
    return SPSX_COND_75PCT;
  case 13:
    return SPSX_COND_90PCT;
  case 14:
    return SPSX_COND_ONESHOT;
  default:
    return SPSX_COND_100PCT;
  }
}

int8_t legacy_timing_to_spsx(uint8_t timing, uint8_t speed) {
  if (timing == 0) {
    return 0;
  }

  uint16_t timing_mid = SeqTrack::get_speed_multiplier_int(speed);
  uint16_t timing_quarter = timing_mid / 2;
  if (timing_quarter == 0) {
    timing_quarter = 1;
  }

  int16_t microtiming =
      ((int16_t)timing - (int16_t)timing_mid) * 127 / timing_quarter;
  if (microtiming < -127) {
    microtiming = -127;
  } else if (microtiming > 127) {
    microtiming = 127;
  }
  return (int8_t)microtiming;
}

void copy_spsx_machine_to_md(const SPSMachine &src, MDMachine &dest) {
  dest.init();
  memcpy(dest.params, src.params, MD_PARAMS_PER_TRACK);
  dest.track = src.track;
  dest.level = src.level;
  dest.model = src.model;
  dest.lfo = src.lfos[0];
  dest.trigGroup = src.trigGroup;
  dest.muteGroup = src.muteGroup;
}

void convert_legacy_seq_to_spsx(const MDSeqTrackData &src,
                                SPSXSeqTrack &dest) {
  uint8_t speed = dest.speed;
  dest.SPSXSeqTrackData::init();

  memcpy(dest.locks, src.locks, sizeof(src.locks));
  memset(dest.locks_params, 0, sizeof(dest.locks_params));
  memcpy(dest.locks_params, src.locks_params, sizeof(src.locks_params));

  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    const MDSeqStepDescriptor &src_step = src.steps[step];
    SPSXSeqStepDescriptor &dest_step = dest.steps[step];

    dest_step.locks = src_step.locks;
    dest_step.locks_enabled = src_step.locks_enabled;
    dest_step.cond_plock = src_step.cond_plock;
    dest_step.cond_id = legacy_cond_to_spsx(src_step.cond_id);
    dest.microtiming[step] = legacy_timing_to_spsx(src.timing[step], speed);

    if (src_step.trig) {
      SPSX_SET_BIT64(dest.trig_mask, step);
    }
    if (src_step.slide) {
      SPSX_SET_BIT64(dest.slide_mask, step);
    }
  }
}

void finalize_spsx_seq_load(SPSXSeqTrack &track) {
  if (track.track_length != 0) {
    track.length = track.track_length;
  }
  if (track.track_speed != 0xFF) {
    track.speed = track.track_speed;
  }
  track.clear_mutes();
  track.set_length(track.length);
  track.set_speed(track.speed, track.speed, false);
  track.notes.init();
}

} // namespace

void SPSXTrack::init() {
  machine.init();
  seq_version = SPSX_SEQ_VERSION_LEGACY;
  seq_data.legacy.init();
  mod_data.init();
}

void SPSXTrack::clear_track() {
  init();
}

void SPSXTrack::get_machine_from_kit(uint8_t tracknumber) {
  memcpy(machine.params, MD.kit.params[tracknumber], SPS_PARAMS_PER_TRACK);
  machine.track = tracknumber;
  machine.level = MD.kit.levels[tracknumber];
  machine.model = MD.kit.models[tracknumber];
  memcpy(&machine.lfos[0], &MD.kit.lfos[tracknumber],  sizeof(MDLFO));
  memcpy(&machine.lfos[1], &MD.kit.lfosB[tracknumber], sizeof(MDLFO));
  if (MD.kit.lfos[tracknumber].destinationTrack == tracknumber) {
    machine.lfos[0].destinationTrack = tracknumber;
    machine.track = tracknumber;
  }
  machine.trigGroup = MD.kit.trigGroups[tracknumber];
  machine.muteGroup = MD.kit.muteGroups[tracknumber];
}



uint16_t SPSXTrack::calc_latency(uint8_t tracknumber) {
  bool send_machine = false, send_level = false;
  uint16_t latency = MD.sendMachine(tracknumber, &machine, send_level, send_machine);
  if (mcl_actions.transition_level[tracknumber] == TRANSITION_MUTE ||
      mcl_actions.transition_level[tracknumber] == TRANSITION_UNMUTE) {
    latency += 3;
  }
  return latency;
}

uint8_t SPSXTrack::transition_countdown_resolution() {
  return mcl_seq.using_spsx_tracks ? SPSX_SEQ_INTERPOLATION
                                   : LEGACY_SEQ_INTERPOLATION;
}

bool SPSXTrack::transition_cache(uint8_t tracknumber, uint8_t slotnumber) {
  bool send_level = false;
  bool send = true;
  switch (mcl_actions.transition_level[slotnumber]) {
  case 1:
    send_level = true;
    machine.level = 0;
    break;
  }
  MD.sendMachineCache(tracknumber, &machine, send_level, send);
  return true;
}

void SPSXTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  // mute_state must follow the LIVE engine, not the slot's stored seq_version:
  // a legacy-saved slot played through an SPSX engine (or vice versa after a
  // mode switch) would otherwise write to the wrong array and the engine
  // would never see the mute change.
  switch (mcl_actions.transition_level[slotnumber]) {
  case TRANSITION_UNMUTE:
    MD.muteTrack(tracknumber, false);
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      mcl_seq.spsx_tracks[slotnumber].mute_state = SPSX_MUTE_OFF;
    } else
#endif
    {
      mcl_seq.md_tracks[slotnumber].mute_state = SEQ_MUTE_OFF;
    }
    break;
  case TRANSITION_MUTE:
    MD.muteTrack(tracknumber, true);
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      mcl_seq.spsx_tracks[slotnumber].mute_state = SPSX_MUTE_ON;
    } else
#endif
    {
      mcl_seq.md_tracks[slotnumber].mute_state = SEQ_MUTE_ON;
    }
    break;
  }
}

void SPSXTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                uint8_t slotnumber) {
  seq_track->cache_loaded = false;
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
}

void SPSXTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  if (mcl_seq.using_spsx_tracks) {
    SPSXSeqTrack *spsx_seq_track = static_cast<SPSXSeqTrack *>(seq_track);
    load_link_data(seq_track);

    if (seq_version == SPSX_SEQ_VERSION_SPSX) {
      memcpy(spsx_seq_track->SPSXSeqTrackData::data(), seq_data.spsx.data(),
             sizeof(SPSXSeqTrackData));
    } else if (seq_version == SPSX_SEQ_VERSION_LEGACY) {
      convert_legacy_seq_to_spsx(seq_data.legacy, *spsx_seq_track);
    } else {
      spsx_seq_track->SPSXSeqTrackData::init();
    }

    finalize_spsx_seq_load(*spsx_seq_track);
  } else if (seq_version == SPSX_SEQ_VERSION_LEGACY) {
    MDSeqTrack *md_seq_track = static_cast<MDSeqTrack *>(seq_track);
    memcpy(md_seq_track->data(), seq_data.legacy.data(), sizeof(MDSeqTrackData));
    load_link_data(seq_track);
    md_seq_track->clear_mutes();
    md_seq_track->set_length(md_seq_track->length);
    md_seq_track->notes.first_trig = true;
  } else {
    // SPSX/unknown data cannot be safely downcast to legacy lock storage without
    // rebuilding lock slots. Load an empty sequence rather than corrupting the
    // live track object.
    MDSeqTrack *md_seq_track = static_cast<MDSeqTrack *>(seq_track);
    md_seq_track->init();
    load_link_data(seq_track);
    md_seq_track->set_length(link.length);
  }

  SeqTrack::load_mod_data(seq_track, mod_data, true,
                          storage_version_at_least(SEQ_TRACK_ARP_STORAGE_VERSION),
                          storage_version_at_least(SEQ_TRACK_LFO_STORAGE_VERSION));
}

void SPSXTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  MD.insertMachineInKit(tracknumber, &machine);
  load_seq_data(seq_track);
}

void SPSXTrack::load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
}

void SPSXTrack::paste_track(uint8_t src_track, uint8_t dest_track,
                            SeqTrack *seq_track) {
  if (machine.trigGroup == src_track) machine.trigGroup = 255;
  if (machine.muteGroup == src_track) machine.muteGroup = 255;
  for (uint8_t li = 0; li < 2; li++) {
    if (machine.lfos[li].destinationTrack == src_track) {
      machine.lfos[li].destinationTrack = dest_track;
    }
  }
  load_immediate(dest_track, seq_track);
  MD.sendMachine(dest_track, &machine, true, true);
}

DeviceTrack *SPSXTrack::materialize_as(uint8_t track_type,
                                       uint8_t tracknumber,
                                       SeqTrack *seq_track) {
  (void)tracknumber;
  (void)seq_track;
  if (track_type == MDSPSX_TRACK_TYPE) {
    return this;
  }
  if (track_type == MD_TRACK_TYPE) {
    GridLink old_link = link;
    SPSMachine old_machine = machine;
    SeqTrackModData old_mod_data = mod_data;
    uint8_t old_seq_version = seq_version;
    MDSeqTrackData old_legacy_seq_data;
    if (old_seq_version == SPSX_SEQ_VERSION_LEGACY) {
      memcpy(&old_legacy_seq_data, &seq_data.legacy,
             sizeof(old_legacy_seq_data));
    }

    auto *md_track = static_cast<MDTrack *>(init_track_type(MD_TRACK_TYPE));
    md_track->link = old_link;
    md_track->mod_data = old_mod_data;
    copy_spsx_machine_to_md(old_machine, md_track->machine);

    if (old_seq_version == SPSX_SEQ_VERSION_LEGACY) {
      memcpy(md_track->seq_data.data(), old_legacy_seq_data.data(),
             sizeof(old_legacy_seq_data));
    } else {
      md_track->seq_data.init();
    }
    return md_track;
  }
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

bool SPSXTrack::store_in_grid(uint8_t column, uint16_t row,
                              SeqTrack *seq_track, uint8_t merge,
                              bool online, Grid *grid) {
  active = MDSPSX_TRACK_TYPE;
  uint8_t tracknumber = column & 0x0F;
  SeqTrack::store_mod_data(mod_data, true, tracknumber);

#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    seq_version = SPSX_SEQ_VERSION_SPSX;

    SPSXSeqTrack *spsx_seq_track =
        seq_track ? static_cast<SPSXSeqTrack *>(seq_track) : nullptr;
    if (spsx_seq_track) {
      spsx_seq_track->store_mute_state();
    }

    if (column != 255 && online && spsx_seq_track) {
      get_machine_from_kit(tracknumber);
      link.length = seq_track->length;
      link.speed = seq_track->speed;

      if (merge > 0) {
        SPSXSeqTrack temp_seq_track;
        if (merge == SAVE_MERGE) {
          memcpy(temp_seq_track.SPSXSeqTrackData::data(),
                 spsx_seq_track->SPSXSeqTrackData::data(),
                 sizeof(SPSXSeqTrackData));
        }
        if (merge == SAVE_MD_PATTERN_IMPORT) {
          link.length = MD.pattern.patternLength;
          link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
        }
        temp_seq_track.length = link.length;
        temp_seq_track.speed = link.speed;
        temp_seq_track.merge_from_md(tracknumber, &MD.pattern);
        link.length = temp_seq_track.length;
        link.speed = temp_seq_track.speed;
        memcpy(seq_data.spsx.data(), temp_seq_track.SPSXSeqTrackData::data(),
               sizeof(SPSXSeqTrackData));
      } else {
        memcpy(seq_data.spsx.data(), spsx_seq_track->SPSXSeqTrackData::data(),
               sizeof(SPSXSeqTrackData));
      }

      if (mcl_cfg.auto_normalize == 1) normalize();
      MD.setOrigParams(tracknumber, &machine);
    }
  } else
#endif
  {
    seq_version = SPSX_SEQ_VERSION_LEGACY;

    MDSeqTrack *md_seq_track =
        seq_track ? static_cast<MDSeqTrack *>(seq_track) : nullptr;
    if (md_seq_track) {
      md_seq_track->store_mute_state();
    }

    if (column != 255 && online && md_seq_track) {
      get_machine_from_kit(tracknumber);
      link.length = seq_track->length;
      link.speed = seq_track->speed;

      if (merge > 0) {
        MDSeqTrack temp_seq_track;
        temp_seq_track.init();
        if (merge == SAVE_MERGE) {
          memcpy(temp_seq_track.data(), md_seq_track->data(),
                 sizeof(MDSeqTrackData));
        }
        if (merge == SAVE_MD_PATTERN_IMPORT) {
          link.length = MD.pattern.patternLength;
          link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
        }
        temp_seq_track.length = link.length;
        temp_seq_track.speed = link.speed;
        temp_seq_track.merge_from_md(tracknumber, &(MD.pattern));
        memcpy(seq_data.legacy.data(), temp_seq_track.data(),
               sizeof(MDSeqTrackData));
      } else {
        memcpy(seq_data.legacy.data(), md_seq_track->data(),
               sizeof(MDSeqTrackData));
      }

      if (mcl_cfg.auto_normalize == 1) normalize();
      MD.setOrigParams(tracknumber, &machine);
    }
  }

  bool ret = write_grid(_this(), _sizeof(), column, row, grid);
  if (!ret) {
    DEBUG_PRINTLN(F("SPSX write failed"));
    return false;
  }
  return true;
}

void SPSXTrack::scale_seq_vol(float scale) {
#if !defined(__AVR__)
  if (has_spsx_seq()) {
    for (uint8_t n = 0; n < SPSX_NUM_MD_STEPS; n++) {
      uint16_t idx = seq_data.spsx.get_lockidx(n);
      for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        if (seq_data.spsx.steps[n].is_lock_bit(c)) {
          if ((seq_data.spsx.locks_params[c] == MODEL_LFOD + 1) ||
              (seq_data.spsx.locks_params[c] == MODEL_VOL + 1)) {
            seq_data.spsx.locks[idx] =
                (uint8_t)(scale * (float)(seq_data.spsx.locks[idx] - 1)) + 1;
            if (seq_data.spsx.locks[idx] > 128)
              seq_data.spsx.locks[idx] = 128;
          }
          ++idx;
        }
      }
    }
    return;
  }
#endif
  // Legacy path
  for (uint8_t n = 0; n < NUM_MD_STEPS; n++) {
    auto idx = seq_data.legacy.get_lockidx(n);
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      if (seq_data.legacy.steps[n].is_lock(c)) {
        if ((seq_data.legacy.locks_params[c] == MODEL_LFOD + 1) ||
            (seq_data.legacy.locks_params[c] == MODEL_VOL + 1)) {
          seq_data.legacy.locks[idx] =
              (uint8_t)(scale * (float)(seq_data.legacy.locks[idx] - 1)) + 1;
          if (seq_data.legacy.locks[idx] > 128)
            seq_data.legacy.locks[idx] = 128;
        }
        ++idx;
      }
    }
  }
}

void SPSXTrack::scale_vol(float scale) {
  normalize();
  machine.scale_vol(scale);
  scale_seq_vol(scale);
}

void SPSXTrack::normalize() {
  float scale = machine.normalize_level();
  scale_seq_vol(scale);
}

void SPSXTrack::on_copy(int16_t s_col, int16_t d_col, bool destination_same) {
  if (destination_same) {
    if (machine.trigGroup == s_col) machine.trigGroup = 255;
    if (machine.muteGroup == s_col) machine.muteGroup = 255;
    for (uint8_t li = 0; li < 2; li++) {
      if (machine.lfos[li].destinationTrack == s_col) {
        machine.lfos[li].destinationTrack = d_col;
      }
    }
  } else {
    int trig_dest = machine.trigGroup - s_col;
    int mute_dest = machine.muteGroup - s_col;
    for (uint8_t li = 0; li < 2; li++) {
      int lfo_dest = machine.lfos[li].destinationTrack - s_col;
      if (range_check(d_col + lfo_dest, 0, 15))
        machine.lfos[li].destinationTrack = d_col + lfo_dest;
      else
        machine.lfos[li].destinationTrack = 255;
    }
    if (range_check(d_col + trig_dest, 0, 15))
      machine.trigGroup = d_col + trig_dest;
    else
      machine.trigGroup = 255;
    if (range_check(d_col + mute_dest, 0, 15))
      machine.muteGroup = d_col + mute_dest;
    else
      machine.muteGroup = 255;
  }
}

#endif // !__AVR__
