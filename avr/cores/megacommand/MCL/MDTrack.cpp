#include "MCL_impl.h"

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

void MDTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  uint8_t n = slotnumber;
  bool send_level = false;
  DEBUG_DUMP(n);
  switch (mcl_actions.transition_level[n]) {
  case 1:
    DEBUG_PRINTLN("setting transition level to 0");
    send_level = true;
    machine.level = 0;
    break;
  case TRANSITION_UNMUTE:
    DEBUG_PRINTLN(F("unmuting"));
    MD.muteTrack(tracknumber, false);
    break;
  case TRANSITION_MUTE:
    DEBUG_PRINTLN(F("muting"));
    MD.muteTrack(tracknumber, true);
    break;
  default:
    break;
  }

  bool send = true;
  MD.sendMachine(tracknumber, &(machine), send_level, send);
}

void MDTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                              uint8_t slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
}

void MDTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  MD.insertMachineInKit(tracknumber, &(machine));
  load_seq_data(seq_track);
  store_in_mem(tracknumber);
}

void MDTrack::get_machine_from_kit(uint8_t tracknumber) {
  //  trackName[0] = '\0';
  memcpy(machine.params, MD.kit.params[tracknumber], 24);

  machine.track = tracknumber;
  machine.level = MD.kit.levels[tracknumber];
  machine.model = MD.kit.models[tracknumber]; //get_raw_model including tonal bit

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
}

void MDTrack::load_seq_data(SeqTrack *seq_track) {
  MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;

  memcpy(md_seq_track->data(), seq_data.data(), sizeof(seq_data));
  load_link_data(seq_track);
  md_seq_track->oneshot_mask = 0;
  md_seq_track->set_length(md_seq_track->length);
  md_seq_track->update_params();
}

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

void MDTrack::normalize() {
  float scale = machine.normalize_level();
  scale_seq_vol(scale);
}

bool MDTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track,
                            uint8_t merge, bool online) {
  active = MD_TRACK_TYPE;

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;

  if (column != 255 && online == true) {
    get_machine_from_kit(column);
    DEBUG_DUMP("online");
    link.length = seq_track->length;
    link.speed = seq_track->speed;

    if (merge > 0) {
      DEBUG_PRINTLN(F("auto merge"));
      MDSeqTrack temp_seq_track;
      temp_seq_track.init();
      if (merge == SAVE_MERGE) {
        // Load up internal sequencer data
        memcpy(temp_seq_track.data(), md_seq_track->data(),
               sizeof(MDSeqTrackData));
      }
      if (merge == SAVE_MD) {
        link.length = MD.pattern.patternLength;
        link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
        DEBUG_PRINTLN(F("SAVE_MD"));
      }
      temp_seq_track.length = link.length;
      temp_seq_track.speed = link.speed;

      // merge md pattern data with seq_data
      temp_seq_track.merge_from_md(column, &(MD.pattern));
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
  }
  // Write data to sd
  len = sizeof(MDTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, column, row);

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
