#include "SPSXTrack.h"
#include "MD.h"
#include "Grid/MCLActions.h"
#include "Sequencer/MCLSeq.h"
#include "MCLSysConfig.h"
#include "Shared.h"

#if !defined(__AVR__)

namespace {

int8_t legacy_md_microtiming_to_spsx(int8_t microtiming, uint8_t speed) {
  if (microtiming == 0) {
    return 0;
  }

  uint16_t ticks_per_step = SeqTrack::get_speed_multiplier_int(speed);
  int16_t ticks = SeqTrack::microtiming_to_ticks(microtiming, ticks_per_step);
  int32_t numerator = (int32_t)ticks * 512;
  int16_t spsx_microtiming =
      numerator >= 0
          ? (int16_t)((numerator + ticks_per_step / 2) / ticks_per_step)
          : (int16_t)-(((-numerator) + ticks_per_step / 2) / ticks_per_step);
  if (spsx_microtiming < -127) {
    spsx_microtiming = -127;
  } else if (spsx_microtiming > 127) {
    spsx_microtiming = 127;
  }
  return (int8_t)spsx_microtiming;
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
  dest.mute_mask = src.mute_mask;
  dest.slide_mask = src.slide_mask;
  dest.swing_mask = src.swing_mask;
  dest.swing_amount = src.swing_amount;

  memset(dest.locks_params, 0, sizeof(dest.locks_params));
  memcpy(dest.locks_params, src.locks_params, sizeof(src.locks_params));

  uint16_t src_lock = 0;
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    const MDSeqStepDescriptor &src_step = src.steps[step];
    SPSXSeqStepDescriptor &dest_step = dest.steps[step];

    uint8_t legacy_locks = src_step.locks;
    dest_step.locks = 0;
    dest_step.cond_plock = src_step.cond_plock;
    dest_step.cond_id = src_step.cond_id;
    dest.microtiming[step] =
        legacy_md_microtiming_to_spsx(src.microtiming[step], speed);

    if (src_step.trig) {
      SPSX_SET_BIT64(dest.trig_mask, step);
    }

    for (uint8_t lock = 0; lock < NUM_LOCKS; lock++) {
      uint8_t lock_mask = 1 << lock;
      if (!(legacy_locks & lock_mask)) {
        continue;
      }
      if (src_lock < NUM_MD_LOCK_SLOTS && src.locks_params[lock]) {
        dest.set_track_locks_i(step, lock, src.locks[src_lock]);
      }
      src_lock++;
    }
  }
  dest.clean_params();
}

void finalize_spsx_seq_load(SPSXSeqTrack &track) {
  if (track.track_length != 0) {
    track.length = track.track_length;
  }
  if (track.track_speed != 0xFF) {
    track.speed = track.track_speed;
  }
  track.clear_oneshot();
  track.set_length(track.length);
  track.set_speed(track.speed, track.speed, false);
  track.notes.init();
}

} // namespace

void SPSXTrack::init() {
  machine.init();
  seq_storage.init_storage();
  load_fade.init();
}

void SPSXTrack::clear_track() {
  init();
}

void SPSXTrack::on_storage_loaded() {
  if (version >= SPSX_TRACK_LOCK37_STORAGE_VERSION) {
    return;
  }

  constexpr size_t kParameterGrowth =
      SPS_PARAMS_PER_TRACK - SPS_PARAMS_V1_PER_TRACK;
  constexpr size_t kLockTableGrowth =
      SPSX_NUM_LOCKS - SPSX_NUM_LOCKS_V1;
  static_assert(kParameterGrowth == 3,
                "SPSXTrack migration assumes three added machine parameters");
  static_assert(kLockTableGrowth == 3,
                "SPSXTrack migration assumes three added lock parameters");
  static_assert(kParameterGrowth == kLockTableGrowth,
                "machine and lock-table migrations must remain coordinated");
  static_assert(sizeof(SPSXTrackSeqStorage::SeqDataUnion) ==
                    sizeof(SPSXSeqTrackData),
                "SPS-X sequence data must remain the storage union's largest member");
  static_assert(sizeof(SPSXSeqTrackData) > kLockTableGrowth,
                "invalid legacy SPS-X sequence size");
  static_assert(sizeof(SPSMachine) > kParameterGrowth,
                "invalid legacy SPS-X machine size");

  // Version 5 stored a 34-parameter SPSMachine followed by a sequence union
  // whose SPS-X member also had 34 lock mappings. Both members grew by three,
  // so the old sequence storage starts three bytes before its current address,
  // while the old mod/fade suffix starts six bytes before its current address.
  // Snapshot the complete legacy pieces before moving either prefix.
  constexpr size_t kLegacyMachineBytes =
      sizeof(SPSMachine) - kParameterGrowth;
  constexpr size_t kLegacySeqDataBytes =
      sizeof(SPSXSeqTrackData) - kLockTableGrowth;
  uint8_t legacy_machine[kLegacyMachineBytes];
  uint8_t legacy_seq_data[kLegacySeqDataBytes];
  SeqTrackModStorage legacy_mod;
  TrackLoadFadeData legacy_fade;

  auto *new_machine = reinterpret_cast<uint8_t *>(&machine);
  auto *legacy_seq_storage =
      reinterpret_cast<uint8_t *>(&seq_storage) - kParameterGrowth;
  memcpy(legacy_machine, new_machine, sizeof(legacy_machine));
  const uint8_t legacy_seq_version = legacy_seq_storage[0];
  memcpy(legacy_seq_data, legacy_seq_storage + sizeof(uint8_t),
         sizeof(legacy_seq_data));
  memcpy(&legacy_mod,
         legacy_seq_storage + sizeof(uint8_t) + sizeof(legacy_seq_data),
         sizeof(legacy_mod));
  memcpy(&legacy_fade,
         legacy_seq_storage + sizeof(uint8_t) + sizeof(legacy_seq_data) +
             sizeof(legacy_mod),
         sizeof(legacy_fade));

  // Preserve the 34 original values, initialize BUS1-3, then move every
  // machine field following params to its new offset.
  memcpy(new_machine, legacy_machine, SPS_PARAMS_V1_PER_TRACK);
  memset(new_machine + SPS_PARAMS_V1_PER_TRACK, 0, kParameterGrowth);
  memcpy(new_machine + SPS_PARAMS_PER_TRACK,
         legacy_machine + SPS_PARAMS_V1_PER_TRACK,
         sizeof(legacy_machine) - SPS_PARAMS_V1_PER_TRACK);

  seq_storage.seq_version = legacy_seq_version;
  memset(&seq_storage.seq_data, 0, sizeof(seq_storage.seq_data));
  if (legacy_seq_version == SPSX_SEQ_VERSION_SPSX) {
    memcpy(seq_storage.seq_data.spsx.data(), legacy_seq_data,
           sizeof(legacy_seq_data));
    auto &spsx = seq_storage.seq_data.spsx;
    const uint8_t legacy_swing_amount =
        spsx.locks_params[SPSX_NUM_LOCKS_V1];
    auto *legacy_locks =
        spsx.locks_params + SPSX_NUM_LOCKS_V1 + sizeof(uint8_t);
    memmove(spsx.locks, legacy_locks, sizeof(spsx.locks));
    memset(spsx.locks_params + SPSX_NUM_LOCKS_V1, 0,
           SPSX_NUM_LOCKS - SPSX_NUM_LOCKS_V1);
    spsx.swing_amount = legacy_swing_amount;
  } else {
    static_assert(sizeof(MDSeqTrackData) <= kLegacySeqDataBytes,
                  "legacy MD sequence no longer fits the version-5 union");
    memcpy(seq_storage.seq_data.legacy.data(), legacy_seq_data,
           sizeof(MDSeqTrackData));
  }

  memcpy(static_cast<SeqTrackModStorage *>(&seq_storage), &legacy_mod,
         sizeof(legacy_mod));
  memcpy(&load_fade, &legacy_fade, sizeof(legacy_fade));
  version = SPSX_TRACK_LOCK37_STORAGE_VERSION;
}

uint16_t SPSXTrack::grid_slot_label(GridSlotLabelContext ctx) {
  auto tmp = getMDMachineNameShort(ctx.model, 2);
  if (!tmp) {
    return 0;
  }
  return make_grid_slot_label(tmp[0], tmp[1]);
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

bool SPSXTrack::transition_cache(uint8_t tracknumber, GridSlot slotnumber) {
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

void SPSXTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
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
                                GridSlot slotnumber) {
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

    if (seq_storage.seq_version == SPSX_SEQ_VERSION_SPSX) {
      memcpy(spsx_seq_track->SPSXSeqTrackData::data(),
             seq_storage.seq_data.spsx.data(),
             sizeof(SPSXSeqTrackData));
    } else if (seq_storage.seq_version == SPSX_SEQ_VERSION_LEGACY) {
      convert_legacy_seq_to_spsx(seq_storage.seq_data.legacy,
                                 *spsx_seq_track);
    } else {
      spsx_seq_track->SPSXSeqTrackData::init();
    }

    finalize_spsx_seq_load(*spsx_seq_track);
  } else if (seq_storage.seq_version == SPSX_SEQ_VERSION_LEGACY) {
    MDSeqTrack *md_seq_track = static_cast<MDSeqTrack *>(seq_track);
    memcpy(md_seq_track->data(), seq_storage.seq_data.legacy.data(),
           sizeof(MDSeqTrackData));
    load_link_data(seq_track);
    md_seq_track->clear_oneshot();
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

  SeqTrack::load_mod_data(seq_track, seq_storage.mod(), true);
}

void SPSXTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  MD.insertMachineInKit(tracknumber, &machine, true);
  load_seq_data(seq_track);
  if (tracknumber == 0)
    mcl_seq.set_spsx_accent_amount((uint8_t)(reserved & 0x0F));
}

#if !defined(__AVR__)
void SPSXTrack::load_immediate_preserve_level(uint8_t tracknumber,
                                              SeqTrack *seq_track) {
  MD.insertMachineInKit(tracknumber, &machine, false);
  load_seq_data(seq_track);
  if (tracknumber == 0)
    mcl_seq.set_spsx_accent_amount((uint8_t)(reserved & 0x0F));
}
#endif

void SPSXTrack::load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
  if (tracknumber == 0)
    mcl_seq.set_spsx_accent_amount((uint8_t)(reserved & 0x0F));
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
    SeqTrackModData old_mod_data = seq_storage.mod();
    TrackLoadFadeData old_load_fade = load_fade;
    uint8_t old_seq_version = seq_storage.seq_version;
    MDSeqTrackData old_legacy_seq_data;
    if (old_seq_version == SPSX_SEQ_VERSION_LEGACY) {
      memcpy(&old_legacy_seq_data, &seq_storage.seq_data.legacy,
             sizeof(old_legacy_seq_data));
    }

    auto *md_track =
        static_cast<MDTrack *>(init_materialized_track_type(MD_TRACK_TYPE));
    md_track->link = old_link;
    md_track->mod_data = old_mod_data;
    md_track->load_fade = old_load_fade;
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

bool SPSXTrack::store_in_grid(GridSlot column, GridRow row,
                              SeqTrack *seq_track, uint8_t merge,
                              bool online, Grid *grid) {
  active = MDSPSX_TRACK_TYPE;
  uint8_t tracknumber = column & 0x0F;
  // Store the pattern-global amount in every SPS-X track header so a row
  // remains self-contained. Only destination track zero applies it on load.
  reserved = (uint8_t)(mcl_seq.spsx_accent_amount & 0x0F);
  if (merge == SAVE_MD_PATTERN_IMPORT)
    reserved = (uint8_t)((MD.pattern.accentAmount >> 3) & 0x0F);
  SeqTrack::store_mod_data(seq_storage.mod(), true, tracknumber);

#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    seq_storage.seq_version = SPSX_SEQ_VERSION_SPSX;

    SPSXSeqTrack *spsx_seq_track =
        seq_track ? static_cast<SPSXSeqTrack *>(seq_track) : nullptr;
    if (column != 255 && online && spsx_seq_track) {
      get_machine_from_kit(tracknumber);
      link.length = seq_track->length;
      link.set_speed(seq_track->speed);

      if (merge > 0) {
        SPSXSeqTrack temp_seq_track;
        if (merge == SAVE_MERGE) {
          memcpy(temp_seq_track.SPSXSeqTrackData::data(),
                 spsx_seq_track->SPSXSeqTrackData::data(),
                 sizeof(SPSXSeqTrackData));
        }
        if (merge == SAVE_MD_PATTERN_IMPORT) {
          link.length = MD.pattern.patternLength;
          link.set_speed(SEQ_SPEED_1X + MD.pattern.doubleTempo);
        }
        temp_seq_track.length = link.length;
        temp_seq_track.speed = link.speed_value();
        temp_seq_track.merge_from_md(tracknumber, &MD.pattern);
        link.length = temp_seq_track.length;
        link.set_speed(temp_seq_track.speed);
        memcpy(seq_storage.seq_data.spsx.data(),
               temp_seq_track.SPSXSeqTrackData::data(),
               sizeof(SPSXSeqTrackData));
      } else {
        memcpy(seq_storage.seq_data.spsx.data(),
               spsx_seq_track->SPSXSeqTrackData::data(),
               sizeof(SPSXSeqTrackData));
      }

      if (mcl_cfg.auto_normalize == 1) normalize();
      MD.setOrigParams(tracknumber, &machine);
    }
  } else
#endif
  {
    seq_storage.seq_version = SPSX_SEQ_VERSION_LEGACY;

    MDSeqTrack *md_seq_track =
        seq_track ? static_cast<MDSeqTrack *>(seq_track) : nullptr;

    if (column != 255 && online && md_seq_track) {
      get_machine_from_kit(tracknumber);
      link.length = seq_track->length;
      link.set_speed(seq_track->speed);

      if (merge > 0) {
        MDSeqTrack temp_seq_track;
        temp_seq_track.init();
        if (merge == SAVE_MERGE) {
          memcpy(temp_seq_track.data(), md_seq_track->data(),
                 sizeof(MDSeqTrackData));
        }
        if (merge == SAVE_MD_PATTERN_IMPORT) {
          link.length = MD.pattern.patternLength;
          link.set_speed(SEQ_SPEED_1X + MD.pattern.doubleTempo);
        }
        temp_seq_track.length = link.length;
        temp_seq_track.speed = link.speed_value();
        temp_seq_track.merge_from_md(tracknumber, &(MD.pattern));
        memcpy(seq_storage.seq_data.legacy.data(), temp_seq_track.data(),
               sizeof(MDSeqTrackData));
      } else {
        memcpy(seq_storage.seq_data.legacy.data(), md_seq_track->data(),
               sizeof(MDSeqTrackData));
      }

      if (mcl_cfg.auto_normalize == 1) normalize();
      MD.setOrigParams(tracknumber, &machine);
    }
  }

  bool ret = write_grid(_this(), get_store_size(), column, row, grid);
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
      uint16_t idx = seq_storage.seq_data.spsx.get_lockidx(n);
      for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        if (seq_storage.seq_data.spsx.steps[n].is_lock_bit(c)) {
          if ((seq_storage.seq_data.spsx.locks_params[c] == MODEL_LFOD + 1) ||
              (seq_storage.seq_data.spsx.locks_params[c] == MODEL_VOL + 1)) {
            seq_storage.seq_data.spsx.locks[idx] =
                (uint8_t)(scale *
                          (float)(seq_storage.seq_data.spsx.locks[idx] - 1)) +
                1;
            if (seq_storage.seq_data.spsx.locks[idx] > 128)
              seq_storage.seq_data.spsx.locks[idx] = 128;
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
    auto idx = seq_storage.seq_data.legacy.get_lockidx(n);
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      if (seq_storage.seq_data.legacy.steps[n].is_lock(c)) {
        if ((seq_storage.seq_data.legacy.locks_params[c] == MODEL_LFOD + 1) ||
            (seq_storage.seq_data.legacy.locks_params[c] == MODEL_VOL + 1)) {
          seq_storage.seq_data.legacy.locks[idx] =
              (uint8_t)(scale *
                        (float)(seq_storage.seq_data.legacy.locks[idx] - 1)) +
              1;
          if (seq_storage.seq_data.legacy.locks[idx] > 128)
            seq_storage.seq_data.legacy.locks[idx] = 128;
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

void SPSXTrack::on_copy(GridColumn s_col, GridColumn d_col, bool destination_same) {
  seq_storage.mod().remap_lfo_track_destinations(s_col, d_col, destination_same,
                                                NUM_GRID_X_LFO_TRACKS);
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
