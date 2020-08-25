#include "MDTrack.h"
#include "MCL.h"

void MDTrack::load_immediate(uint8_t tracknumber) {
  place_track_in_kit(tracknumber, &(MD.kit));
  load_seq_data(tracknumber);
  store_in_mem(tracknumber);
}

void MDTrack::get_machine_from_kit(uint8_t tracknumber) {
  //  trackName[0] = '\0';
  memcpy(machine.params, MD.kit.params[tracknumber], 24);

  machine.track = tracknumber;
  machine.level = MD.kit.levels[tracknumber];
  machine.model = MD.kit.models[tracknumber];

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

void MDTrack::place_track_in_kit(uint8_t tracknumber, MDKit *kit, bool levels) {

  memcpy(kit->params[tracknumber], &(machine.params), 24);
  if (levels) {
    kit->levels[tracknumber] = machine.level;
  }
  kit->models[tracknumber] = machine.model;

  if (machine.lfo.destinationTrack == tracknumber) {

    machine.lfo.destinationTrack = tracknumber;
  }
  // sanity check.
  if (machine.lfo.destinationTrack > 15) {
    DEBUG_PRINTLN("warning: lfo dest was out of bounds");
    machine.lfo.destinationTrack = tracknumber;
  }
  memcpy(&(kit->lfos[tracknumber]), &machine.lfo, sizeof(machine.lfo));
  /*
  DEBUG_PRINTLN("LFO");
  DEBUG_DUMP(kit->lfos[tracknumber].destinationTrack);
  DEBUG_DUMP(kit->lfos[tracknumber].destinationParam);
  DEBUG_DUMP(kit->lfos[tracknumber].shape1);
  DEBUG_DUMP(kit->lfos[tracknumber].shape2);
  DEBUG_DUMP(kit->lfos[tracknumber].type);
  DEBUG_DUMP(kit->lfos[tracknumber].speed);
  DEBUG_DUMP(kit->lfos[tracknumber].depth);
  DEBUG_DUMP(kit->lfos[tracknumber].mix);
  */
  if ((machine.trigGroup < 16) && (machine.trigGroup != tracknumber)) {
    kit->trigGroups[tracknumber] = machine.trigGroup;
  } else {
    kit->trigGroups[tracknumber] = 255;
  }

  if ((machine.muteGroup < 16) && (machine.muteGroup != tracknumber)) {
    kit->muteGroups[tracknumber] = machine.muteGroup;
  } else {
    kit->muteGroups[tracknumber] = 255;
  }
}

void MDTrack::init() {
  machine.init();
  seq_data.init();
}

void MDTrack::load_seq_data(uint8_t tracknumber) {
  if (active == EMPTY_TRACK_TYPE) {
    mcl_seq.md_tracks[tracknumber].clear_track();
  } else {
    memcpy(mcl_seq.md_tracks[tracknumber].data(), seq_data.data(),
           sizeof(seq_data));
    mcl_seq.md_tracks[tracknumber].speed = chain.speed;
    mcl_seq.md_tracks[tracknumber].length = chain.length;
    if (mcl_seq.md_tracks[tracknumber].speed < SEQ_SPEED_1X) {
      mcl_seq.md_tracks[tracknumber].speed = SEQ_SPEED_1X;
      mcl_seq.md_tracks[tracknumber].clear_slide_data();
    }
    mcl_seq.md_tracks[tracknumber].oneshot_mask = 0;
    mcl_seq.md_tracks[tracknumber].set_length(
        mcl_seq.md_tracks[tracknumber].length);
    mcl_seq.md_tracks[tracknumber].update_params();
  }
}

void MDTrack::place_track_in_sysex(uint8_t tracknumber) {
  place_track_in_kit(tracknumber, &(MD.kit));
  load_seq_data(tracknumber);
}

void MDTrack::scale_seq_vol(float scale) {
  for (uint8_t n = 0; n < NUM_MD_STEPS; n++) {
    auto idx = seq_data.get_lockidx(n);
    for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
      if (seq_data.steps[n].is_lock(c)) {
        if ((seq_data.locks_params[c] == MODEL_LFOD) ||
            (seq_data.locks_params[c] == MODEL_VOL)) {
          seq_data.locks[idx] =
              (uint8_t)(scale * (float)(seq_data.locks[idx]));
          if (seq_data.locks[idx] > 127) {
            seq_data.locks[idx] = 127;
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

void MDTrack::normalize() {
  float scale = machine.normalize_level();
  scale_seq_vol(scale);
}

bool MDTrack::store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge,
                            bool online) {
  active = MD_TRACK_TYPE;

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (tracknumber != 255 && online == true) {
    get_machine_from_kit(tracknumber);

    chain.length = mcl_seq.md_tracks[tracknumber].length;
    chain.speed = mcl_seq.md_tracks[tracknumber].speed;

    if (merge > 0) {
      DEBUG_PRINTLN("auto merge");
      MDSeqTrack md_seq_track;
      if (merge == SAVE_MERGE) {
        // Load up internal sequencer data
        memcpy(md_seq_track.data(), mcl_seq.md_tracks[tracknumber].data(),
               sizeof(MDSeqTrackData));
      }
      if (merge == SAVE_MD) {
        md_seq_track.init();
        chain.length = MD.pattern.patternLength;
        chain.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
        DEBUG_PRINTLN("SAVE_MD");
      }
      // merge md pattern data with seq_data
      md_seq_track.merge_from_md(tracknumber, &(MD.pattern));
      // copy merged data in to this track object's seq data for writing to SD
      memcpy(this->seq_data.data(), md_seq_track.data(),
             sizeof(MDSeqTrackData));
    } else {
      memcpy(this->seq_data.data(), mcl_seq.md_tracks[tracknumber].data(),
             sizeof(MDSeqTrackData));
    }
    // Normalise track levels
    if (mcl_cfg.auto_normalize == 1) {
      normalize();
    }
  }
  // Write data to sd
  len = sizeof(MDTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, tracknumber, row);

  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }
  DEBUG_DUMP(chain.length);
  DEBUG_PRINTLN("Track stored in grid");
  DEBUG_PRINT(tracknumber);
  DEBUG_PRINT(" ");
  DEBUG_PRINT(row);
  DEBUG_PRINT("model");
  return true;
}

void MDTrack::on_copy(int16_t s_col, int16_t d_col, bool destination_same) {
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
