#include "MDTrack.h"
#include "MD.h"
#include "MCLActions.h"
#include "Shared.h"
#include "Sequencer/MCLSeq.h"
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

// Remap group/lfo references that point at src: trig/mute groups become
// unset (255), lfo destination follows to dest. Shared by paste_track and the
// destination_same branch of on_copy.
static void remap_groups_same(MDMachine &machine, uint8_t src, uint8_t dest)
    NOINLINE();
static void remap_groups_same(MDMachine &machine, uint8_t src, uint8_t dest) {
  if (machine.trigGroup == src) {
    machine.trigGroup = 255;
  }
  if (machine.muteGroup == src) {
    machine.muteGroup = 255;
  }
  if (machine.lfo.destinationTrack == src) {
    machine.lfo.destinationTrack = dest;
  }
}

void MDTrack::paste_track(uint8_t src_track, uint8_t dest_track,
                          SeqTrack *seq_track) {
  DEBUG_PRINTLN(F("paste seq track"));
  remap_groups_same(machine, src_track, dest_track);
  load_immediate(dest_track, seq_track);
  bool send_machine = true;
  bool send_level = true;
  MD.sendMachine(dest_track, &(machine), send_level, send_machine);
}

uint16_t MDTrack::grid_slot_label(GridSlotLabelContext ctx) {
  auto tmp = getMDMachineNameShort(ctx.model, 2);
  if (!tmp) {
    return 0;
  }
  return make_grid_slot_label(tmp[0], tmp[1]);
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
    TrackLoadFadeData old_load_fade = load_fade;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *spsx_track =
        static_cast<SPSXTrack *>(
            init_materialized_track_type(MDSPSX_TRACK_TYPE));
    spsx_track->link = old_link;
    spsx_track->seq_storage.mod() = old_mod_data;
    spsx_track->load_fade = old_load_fade;
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

static void md_apply_mute(uint8_t tracknumber, uint8_t n, bool mute,
                          uint8_t state) NOINLINE();
static void md_apply_mute(uint8_t tracknumber, uint8_t n, bool mute,
                          uint8_t state) {
  MD.muteTrack(tracknumber, mute);
  SeqTrackUtil::with_md_track(n, [state](auto &t) { t.mute_state = state; });
}

void MDTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
  uint8_t n = slotnumber;
  DEBUG_PRINTLN("transition send");
  switch (mcl_actions.transition_level[n]) {
  case TRANSITION_UNMUTE:
    DEBUG_PRINTLN(F("unmuting"));
    md_apply_mute(tracknumber, n, false, SEQ_MUTE_OFF);
    break;
  case TRANSITION_MUTE:
    DEBUG_PRINTLN(F("muting"));
    md_apply_mute(tracknumber, n, true, SEQ_MUTE_ON);
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

  // The memcpy above already copies destinationTrack, and machine.track was
  // assigned tracknumber above; when destinationTrack == tracknumber the
  // explicit re-assignments are no-ops, so they are omitted.

  machine.trigGroup = MD.kit.trigGroups[tracknumber];
  machine.muteGroup = MD.kit.muteGroups[tracknumber];
}

static void copy_md_seqdata(uint8_t *dst, const uint8_t *src) NOINLINE();
static void copy_md_seqdata(uint8_t *dst, const uint8_t *src) {
  memcpy(dst, src, sizeof(MDSeqTrackData));
}

static void merge_md_pattern_to_seq(MDTrack &track, MDSeqTrack *md_seq_track,
                                    uint8_t tracknumber, uint8_t merge)
    NOINLINE();
static void merge_md_pattern_to_seq(MDTrack &track, MDSeqTrack *md_seq_track,
                                    uint8_t tracknumber, uint8_t merge) {
  MDSeqTrack temp_seq_track;
  temp_seq_track.init();
  if (merge == SAVE_MERGE) {
    copy_md_seqdata(temp_seq_track.data(), md_seq_track->data());
  } else if (merge == SAVE_MD_PATTERN_IMPORT) {
    track.link.length = MD.pattern.patternLength;
    track.link.set_speed(SEQ_SPEED_1X + MD.pattern.doubleTempo);
    DEBUG_PRINTLN(F("SAVE_MD_PATTERN_IMPORT"));
  }
  temp_seq_track.length = track.link.length;
  temp_seq_track.speed = track.link.speed_value();
  temp_seq_track.merge_from_md(tracknumber, &(MD.pattern));
  copy_md_seqdata(track.seq_data.data(), temp_seq_track.data());
}

void MDTrack::init() {
  machine.init();
  seq_data.init();
  mod_data.init();
  load_fade.init();
}

void MDTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;
  uint8_t *dest = md_seq_track->data();
  copy_md_seqdata(dest, seq_data.data());
  load_link_data(seq_track);
  md_seq_track->clear_oneshot();
  md_seq_track->set_length(md_seq_track->length);
  md_seq_track->notes.first_trig = true;

  SeqTrack::load_mod_data(seq_track, mod_data, true);
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
  mcl_seq.md_arp_tracks[tracknumber].store_phase_data(mod_data.arp_phase());
  mcl_seq.grid_x_lfo_tracks[tracknumber].store_data(&mod_data.lfo);

  MDSeqTrack *md_seq_track =
      seq_track ? static_cast<MDSeqTrack *>(seq_track) : nullptr;

  if (column != 255 && online && md_seq_track) {
    get_machine_from_kit(tracknumber);
    DEBUG_DUMP("online");
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);

    if (merge > 0) {
      DEBUG_PRINTLN(F("auto merge"));
      merge_md_pattern_to_seq(*this, md_seq_track, tracknumber, merge);
    } else {
      copy_md_seqdata(this->seq_data.data(), md_seq_track->data());
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

// Shift a track-referencing column field when a slot is copied to a new
// column, clamping out-of-range references to 255 (unset).
static uint8_t remap_grid_col(uint8_t field, GridColumn s_col,
                              GridColumn d_col) {
  int dest = (int)d_col + (int)field - (int)s_col;
  return range_check(dest, 0, 15) ? (uint8_t)dest : 255;
}

void MDTrack::on_copy(GridColumn s_col, GridColumn d_col, bool destination_same) {
  // bit of a hack to keep lfos modulating the same track.
  if (destination_same) {
    remap_groups_same(machine, s_col, d_col);
  } else {
    machine.lfo.destinationTrack =
        remap_grid_col(machine.lfo.destinationTrack, s_col, d_col);
    machine.trigGroup = remap_grid_col(machine.trigGroup, s_col, d_col);
    machine.muteGroup = remap_grid_col(machine.muteGroup, s_col, d_col);
  }
}
