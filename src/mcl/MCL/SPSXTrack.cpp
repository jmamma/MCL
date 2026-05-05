#include "SPSXTrack.h"
#include "../Drivers/MD/MD.h"
#include "MCLActions.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "Shared.h"

#if !defined(__AVR__)

void SPSXTrack::init() {
  machine.init();
  seq_version = SPSX_SEQ_VERSION_LEGACY;
  seq_data.legacy.init();
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
#if !defined(__AVR__)
  if (seq_version == SPSX_SEQ_VERSION_SPSX) {
    SPSXSeqTrack *spsx_seq_track = (SPSXSeqTrack *)seq_track;
    memcpy(spsx_seq_track->SPSXSeqTrackData::data(),
           seq_data.spsx.data(), sizeof(SPSXSeqTrackData));
    load_link_data(seq_track);
    spsx_seq_track->clear_mutes();
    spsx_seq_track->set_length(spsx_seq_track->length);
    spsx_seq_track->notes.init();
    return;
  }
#endif
  if (seq_version == SPSX_SEQ_VERSION_LEGACY) {
    MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;
    memcpy(md_seq_track->data(), seq_data.legacy.data(), sizeof(MDSeqTrackData));
    load_link_data(seq_track);
    md_seq_track->clear_mutes();
    md_seq_track->set_length(md_seq_track->length);
    md_seq_track->notes.first_trig = true;
    return;
  }
  // Unknown / corrupted seq_version: load an empty sequence rather than
  // memcpy garbage through the wrong union member.
  MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;
  md_seq_track->init();
  load_link_data(seq_track);
  md_seq_track->set_length(link.length);
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

bool SPSXTrack::store_in_grid(uint8_t column, uint16_t row,
                              SeqTrack *seq_track, uint8_t merge,
                              bool online, Grid *grid) {
  active = MDSPSX_TRACK_TYPE;

#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    seq_version = SPSX_SEQ_VERSION_SPSX;

    SPSXSeqTrack *spsx_seq_track = (SPSXSeqTrack *)seq_track;
    spsx_seq_track->store_mute_state();

    if (column != 255 && online) {
      get_machine_from_kit(column);
      link.length = seq_track->length;
      link.speed = seq_track->speed;

      if (merge > 0) {
        SPSXSeqTrack temp_seq_track;
        if (merge == SAVE_MERGE) {
          memcpy(temp_seq_track.SPSXSeqTrackData::data(),
                 spsx_seq_track->SPSXSeqTrackData::data(),
                 sizeof(SPSXSeqTrackData));
        }
        if (merge == SAVE_MD) {
          link.length = MD.pattern.patternLength;
          link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
        }
        temp_seq_track.length = link.length;
        temp_seq_track.speed = link.speed;
        temp_seq_track.merge_from_md(column, &MD.pattern);
        memcpy(seq_data.spsx.data(), temp_seq_track.SPSXSeqTrackData::data(),
               sizeof(SPSXSeqTrackData));
      } else {
        memcpy(seq_data.spsx.data(), spsx_seq_track->SPSXSeqTrackData::data(),
               sizeof(SPSXSeqTrackData));
      }

      if (mcl_cfg.auto_normalize == 1) normalize();
      MD.setOrigParams(column, &machine);
    }
  } else
#endif
  {
    seq_version = SPSX_SEQ_VERSION_LEGACY;

    MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;
    md_seq_track->store_mute_state();

    if (column != 255 && online) {
      get_machine_from_kit(column);
      link.length = seq_track->length;
      link.speed = seq_track->speed;

      if (merge > 0) {
        MDSeqTrack temp_seq_track;
        temp_seq_track.init();
        if (merge == SAVE_MERGE) {
          memcpy(temp_seq_track.data(), md_seq_track->data(),
                 sizeof(MDSeqTrackData));
        }
        if (merge == SAVE_MD) {
          link.length = MD.pattern.patternLength;
          link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
        }
        temp_seq_track.length = link.length;
        temp_seq_track.speed = link.speed;
        temp_seq_track.merge_from_md(column, &(MD.pattern));
        memcpy(seq_data.legacy.data(), temp_seq_track.data(),
               sizeof(MDSeqTrackData));
      } else {
        memcpy(seq_data.legacy.data(), md_seq_track->data(),
               sizeof(MDSeqTrackData));
      }

      if (mcl_cfg.auto_normalize == 1) normalize();
      MD.setOrigParams(column, &machine);
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
