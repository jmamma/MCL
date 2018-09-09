#include "MCL.h"
#include "MDTrack.h"

bool MDTrack::get_track_from_sysex(int tracknumber, uint8_t column) {

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

              locks[n].step = s;
              locks[n].param_number = i;
              locks[n].value = pattern_rec.locks[idx][s];
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
      trackName[c] = MD.kit.name[c];
      if (!grid_page.row_headers[grid_page.cur_row].active) { grid_page.row_headers[grid_page.cur_row].name[c] = MD.kit.name[c]; }
    } else {
      trackName[c] = ' ';
    }
    if (MD.kit.name[c] == '\0') {
      white_space = 1;
    }
  }
  m_memcpy(&seq_data, &mcl_seq.md_tracks[tracknumber], sizeof(seq_data));
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

void MDTrack::place_track_in_sysex(int tracknumber, uint8_t column) {
  // Check that the track is active, we don't want to write empty/corrupt data
  // to the MD
  if (active == MD_TRACK_TYPE) {
    for (int x = 0; x < 64; x++) {
      pattern_rec.clear_step_locks(tracknumber, x);
    }

    // pattern_rec.lockPatterns[tracknumber] = 0;
    // Write pattern lock data to pattern
    uint8_t a;
    pattern_rec.trigPatterns[tracknumber] = trigPattern;
    pattern_rec.accentPatterns[tracknumber] = accentPattern;
    pattern_rec.slidePatterns[tracknumber] = slidePattern;
    pattern_rec.swingPatterns[tracknumber] = swingPattern;

    for (a = length; a < pattern_rec.patternLength; a += length) {
      pattern_rec.trigPatterns[tracknumber] |= trigPattern << a;
      pattern_rec.accentPatterns[tracknumber] |= accentPattern << a;
      pattern_rec.slidePatterns[tracknumber] |= slidePattern << a;
      pattern_rec.swingPatterns[tracknumber] |= swingPattern << a;
    }

    for (int n = 0; n < arraysize; n++) {
      // DEBUG_PRINTLN();
      // DEBUG_PRINTLN("Adding");
      // DEBUG_PRINTLN(step[n]);
      // DEBUG_PRINTLN(param_number[n]);
      // DEBUG_PRINTLN(value[n]);

      //  if (arraysize > 5) {     GUI.flash_string_fill("greater than 5"); }
      for (a = 0; a < pattern_rec.patternLength; a += length) {
        pattern_rec.addLock(tracknumber, locks[n].step + a, locks[n].param_number,
                            locks[n].value);
      }
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

    m_memcpy(&mcl_seq.md_tracks[tracknumber], &seq_data, sizeof(seq_data));
    mcl_seq.md_tracks[tracknumber].update_params();
  }
}

bool MDTrack::load_track_from_grid(int32_t column, int32_t row, int m) {

  bool ret;
  int b = 0;

//  DEBUG_PRINT_FN();
  int32_t offset = grid.get_slot_offset(column, row);

  int32_t len;

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  len = (sizeof(MDTrack) - (LOCK_AMOUNT * 3));

  // len = (sizeof(MDTrack)  - (LOCK_AMOUNT * 3));

  ret = mcl_sd.read_data((uint8_t *)this, len, &proj.file);

  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }
    if ((arraysize < 0) || (arraysize > LOCK_AMOUNT)) {
      DEBUG_PRINTLN("lock array size is wrong");
      return false;
    }
    ret = mcl_sd.read_data((uint8_t *)&(this->locks[0]), arraysize * 3,
                           &proj.file);
    if (!ret) {
      DEBUG_PRINTLN("read failed");
      return false;
    }
  return true;
}

bool MDTrack::store_track_in_grid(int track, int32_t column, int32_t row) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  active = MD_TRACK_TYPE;

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  int32_t len;

  int32_t offset = grid.get_slot_offset(column, row);

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("seek failed");
    return false;
  }

  get_track_from_sysex(track, column);
  len = sizeof(MDTrack) - (LOCK_AMOUNT * 3);
  DEBUG_PRINTLN(len);
  ret = mcl_sd.write_data((uint8_t *)(this), len, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)&(this->locks[0]), arraysize * 3,
                          &proj.file);

  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

 uint8_t model = machine.model;
  grid_page.row_headers[grid_page.cur_row].update_model(column, model, DEVICE_MD);

  DEBUG_PRINTLN("Track stored in grid");
  DEBUG_PRINT(column);
  DEBUG_PRINT(" ");
  DEBUG_PRINT(row);
  DEBUG_PRINT("model");
  DEBUG_PRINT(model);
  return true;
}
