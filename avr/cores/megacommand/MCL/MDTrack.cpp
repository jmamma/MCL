#include "MDTrack.h"

bool getTrack_from_sysex(int tracknumber, uint8_t column) {

  active = TRUE;
  trigPattern = pattern_rec.trigPatterns[tracknumber];
  accentPattern = pattern_rec.accentPatterns[tracknumber];
  slidePattern = pattern_rec.slidePatterns[tracknumber];
  swingPattern = pattern_rec.swingPatterns[tracknumber];
  length = pattern_rec.patternLength;
  kitextra.swingAmount = pattern_rec.swingAmount;
  kitextra.accentAmount = pattern_rec.accentAmount;
  kitextra.patternLength = pattern_rec.patternLength;
  kitextra.doubleTempo = pattern_rec.doubleTempo;
  kitextra.scale = pattern_rec.scale;

  // Extract parameter lock data and store it in a useable data structure
  int n = 0;
  arraysize = 0;
  for (int i = 0; i < 24; i++) {
    if (IS_BIT_SET32(pattern_rec.lockPatterns[tracknumber], i)) {
      int8_t idx = pattern_rec.paramLocks[tracknumber][i];
      if (idx >= 0) {
        for (int s = 0; s < 64; s++) {

          if ((pattern_rec.locks[idx][s] <= 127) &&
              (pattern_rec.locks[idx][s] >= 0)) {
            if (IS_BIT_SET64(trigPattern, s)) {

              step[n] = s;
              param_number[n] = i;
              value[n] = pattern_rec.locks[idx][s];
              n++;
            }
          }
        }
      }
    }
  }

  //  itoa(n,&str[2],10);

  arraysize = n;

  /*Don't forget to copy the Machine data as well
    Which is obtained from the received Kit object MD.kit*/
  //  m_strncpy(kitName, MD.kit.name, 17);
  uint8_t white_space = 0;
  for (uint8_t c = 0; c < 17; c++) {
    if (white_space == 0) {
      kitName[c] = MD.kit.name[c];
      trackName[c] = MD.kit.name[c];
    } else {
      kitName[c] = ' ';
      trackName[c] = ' ';
    }
    if (MD.kit.name[c] == '\0') {
      white_space = 1;
    }
  }

  m_memcpy(&seq_data, &mcl_seq.md_tracks[tracknumber].seq_data,
           sizeof(seq_data));

  //  trackName[0] = '\0';
  m_memcpy(machine.params, MD.kit.params[tracknumber], 24);

  machine.track = tracknumber;
  machine.level = MD.kit.levels[tracknumber];
  machine.model = MD.kit.models[tracknumber];

  /*Check to see if LFO is modulating host track*/
  /*IF it is then we need to make sure that the LFO destination is updated to
   * the new row posiiton*/

  if (MD.kit.lfos[tracknumber].destinationTrack == tracknumber) {
    MD.kit.lfos[tracknumber].destinationTrack = column;
  }
  /*Copies Lfo data from the kit object into the machine object*/
  m_memcpy(&machine.lfo, &MD.kit.lfos[tracknumber], sizeof(machine.lfo));

  machine.trigGroup = MD.kit.trigGroups[tracknumber];
  machine.muteGroup = MD.kit.muteGroups[tracknumber];

  m_memcpy(&kitextra.reverb, &MD.kit.reverb, sizeof(kitextra.reverb));
  m_memcpy(&kitextra.delay, &MD.kit.delay, sizeof(kitextra.delay));
  m_memcpy(&kitextra.eq, &MD.kit.eq, sizeof(kitextra.eq));
  m_memcpy(&kitextra.dynamics, &MD.kit.dynamics, sizeof(kitextra.dynamics));
  origPosition = MD.kit.origPosition;
  patternOrigPosition = pattern_rec.origPosition;
}

void placeTrack_in_sysex(int tracknumber, uint8_t column) {
  // Check that the track is active, we don't want to write empty/corrupt data
  // to the MD
  if (active == MD_TRACK_TYPE) {
    for (int x = 0; x < 64; x++) {
      clear_step_locks(tracknumber, x);
    }

    // pattern_rec.lockPatterns[tracknumber] = 0;
    // Write pattern lock data to pattern
    pattern_rec.trigPatterns[tracknumber] = trigPattern;
    pattern_rec.accentPatterns[tracknumber] = accentPattern;
    pattern_rec.slidePatterns[tracknumber] = slidePattern;
    pattern_rec.swingPatterns[tracknumber] = swingPattern;

    for (int n = 0; n < arraysize; n++) {
      // DEBUG_PRINTLN();
      // DEBUG_PRINTLN("Adding");
      // DEBUG_PRINTLN(step[n]);
      // DEBUG_PRINTLN(param_number[n]);
      // DEBUG_PRINTLN(value[n]);

      //  if (arraysize > 5) {     GUI.flash_string_fill("greater than 5"); }
      pattern_rec.addLock(tracknumber, step[n], param_number[n], value[n]);
    }

    // Possible alternative for writing machinedata to the MD without sending
    // the entire Kit >> MD.setMachine(tracknumber, &machine)

    /*Don't forget to store the Kit data as well which is taken from the Track's
      /associated MDMachine object.*/

    /*if kit_sendmode == 1 then we're going to send an entire kit to the
      machinedrum inorder to load up the machine on the desired track In this
      case, we'll need to copy machine model to the kit MD.kit object which will
      be converted into a sysex message and sent to the MD*/
    /*if kit_sendmode == 0 then we'll load up the machine via sysex and Midi CC
     * messages without sending the kit*/

    m_memcpy(MD.kit.params[tracknumber], machine.params, 24);

    MD.kit.levels[tracknumber] = machine.level;
    MD.kit.models[tracknumber] = machine.model;

    if (machine.lfo.destinationTrack == column) {

      machine.lfo.destinationTrack = tracknumber;
    }

    m_memcpy(&MD.kit.lfos[tracknumber], &machine.lfo, sizeof(machine.lfo));

    MD.kit.trigGroups[tracknumber] = machine.trigGroup;
    MD.kit.muteGroups[tracknumber] = machine.muteGroup;

    m_memcpy(&mcl_seq.md_tracks[tracknumber].seq_data, &seq_data,
             sizeof(seq_data));
  }
}

bool load_track_from_grid(int32_t column, int32_t row, int m) {

  bool ret;
  int b = 0;

  DEBUG_PRINT_FN();
  int32_t offset =
      (int32_t)GRID_SLOT_BYTES +
      (column + (row * (int32_t)GRID_WIDTH)) * (int32_t)GRID_SLOT_BYTES;

  int32_t len;

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  len = (sizeof(MDTrack) - sizeof(kitextra) - (LOCK_AMOUNT * 3));

  // len = (sizeof(MDTrack)  - (LOCK_AMOUNT * 3));

  ret = mcl_sd.read_data((uint8_t *)this, len, &proj.file);

  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  if (m == 0) {

    ret =
        mcl_sd.read_data((uint8_t *)&(this->kitextra), sizeof(kitextra), &proj.file);
    if (!ret) {
      DEBUG_PRINTLN("read failed");
      return false;
    }

    ret =
        mcl_sd.read_data((uint8_t *)&(this->param_number[0]), arraysize, &proj.file);
    if (!ret) {
      DEBUG_PRINTLN("read failed");
      return false;
    }

    ret = mcl_sd.read_data((uint8_t *)&(this->value[0]), arraysize, &proj.file);
    if (!ret) {
      DEBUG_PRINTLN("read failed");
      return false;
    }

    ret = mcl_sd.read_data((uint8_t *)&(this->step[0]), arraysize, &proj.file);
    if (!ret) {
      DEBUG_PRINTLN("read failed");
      return false;
    }
  }
  return true;
}
bool store_track_in_grid(int track, int32_t column, int32_t row) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  int32_t len;
  int32_t offset =
      (int32_t)GRID_SLOT_BYTES +
      (column + (row * (int32_t)GRID_WIDTH)) * (int32_t)GRID_SLOT_BYTES;
  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("seek failed");
    return false;
  }

  getTrack_from_sysex(track, column);
  len = sizeof(MDTrack) - (LOCK_AMOUNT * 3);

  ret = mcl_sd.write_data((uint8_t *)(this), len, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  ret =
      mcl_sd.write_data((uint8_t *)&(this->param_number[0]), arraysize, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)&(this->value[0]), arraysize, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)&(this->step[0]), arraysize, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
