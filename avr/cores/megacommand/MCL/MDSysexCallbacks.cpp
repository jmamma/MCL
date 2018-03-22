#include "MDSysexCallbacks.h"

void MDSysexCallbacks::setup() {
  MDSysexListener.setup();
  MDSysexListener.addOnStatusResponseCallback(
      this, (md_status_callback_ptr_t)&MDHandler2::onStatusResponseCallback);
  MDSysexListener.addOnPatternMessageCallback(
      this, (md_callback_ptr_t)&MDHandler2::onPatternMessage);
  MDSysexListener.addOnKitMessageCallback(
      this, (md_callback_ptr_t)&MDHandler2::onKitMessage);
}

void MDSysexCallbacks::onStatusResponseCallback(uint8_t type, uint8_t value) {
  switch (type) {
  case MD_CURRENT_KIT_REQUEST:
    MD.currentKit = value;

  case MD_CURRENT_GLOBAL_SLOT_REQUEST:
    MD.currentGlobal = value;

    break;

  case MD_CURRENT_PATTERN_REQUEST:
    MD.currentPattern = value;
    break;
  }
}

/*A kit has been received by the Minicommand in the form of a Sysex message
  which is residing in memory*/

void MDSysexCallbacks::onKitMessage() {
  setLed2();
  /*If mcl_actions.patternswitch == PATTERN_STORE then the Kit request is for the purpose of
   * obtaining track data*/
  if (!MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
    return;
  }

  if (mcl_actions.patternswitch == PATTERN_STORE) {
  }

  /*Patternswitch == 6, store pattern in memory*/
  /*load up tracks and kit from a pattern that is different from the one
   * currently loaded*/
  if (mcl_actions.patternswitch == 6) {
    //   if (MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

    for (int i = 0; i < 16; i++) {
      if ((i + grid.cur_col + (grid.cur_row * GRID_WIDTH)) < (128 * GRID_WIDTH)) {
        /*Store the track at the  into Minicommand memory by moving the data
         * from a Pattern object into a Track object*/
        temptrack.store_track_in_grid(i, i, grid.cur_row);
      }
      /*Update the encoder page to show current Grids*/
      page.display();
    }
    /*If the pattern can't be retrieved from the sysex data then there's been a
     * problem*/

    mcl_actions.patternswitch = PATTERN_UDEF;

    //  }
  }

  if (mcl_actions.patternswitch == 7) {
    //  if (MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
    if (param3.effect == MD_FX_ECHO) {
      param3.setValue(MD.kit.delay[param3.fxparam]);
    } else {
      param3.setValue(MD.kit.reverb[param3.fxparam]);
    }
    if (param4.effect == MD_FX_ECHO) {
      param4.setValue(MD.kit.delay[param4.fxparam]);
    } else {
      param4.setValue(MD.kit.reverb[param4.fxparam]);
    }
    //   }
    mcl_actions.patternswitch = PATTERN_UDEF;
  }
  clearLed2();
}

void onPatternMessage() {
  setLed2();

  /*Reverse track callback*/
  if (mcl_actions.patternswitch == 5) {
    /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec
     * object. The MD header is 5 bytes long, hence the offset and length
     * change*/

    /*
             if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen -
       5)) {

              //Get current track number from MD
                           int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
                           int i = 0;

              //Data structure for holding the parameter locks of a single step
                           uint8_t templocks[24];

              //Retrevieve trig masks and store them temporarily

                           uint64_t temptrig =
       pattern_rec.trigPatterns[curtrack]; uint64_t tempslide =
       pattern_rec.slidePatterns[curtrack]; uint64_t tempaccent =
       pattern_rec.accentPatterns[curtrack]; uint64_t tempswing =
       pattern_rec.swingPatterns[curtrack];

              //Reversing a track requires reflecting a track about its mid
       point
              //We can therefore start from the beginning and end of the
       pattern, swapping triggers and working our way to the middle

                         while (i < pattern_rec.patternLength / 2) {

              //Initialize locks
                                  for (uint8_t g = 0; g < (24); g++) {
       templocks[g] = 254; }

              //If there is a trigger, then we'll need to swap it
                                 if
       (IS_BIT_SET64(pattern_rec.trigPatterns[curtrack],i)) {

                                    //backup the locks

                                           for (int m = 0; m < 24; m++) {
                                                 if
       (IS_BIT_SET32(pattern_rec.lockPatterns[curtrack], m)) { int8_t idx =
       pattern_rec.paramLocks[curtrack][m]; int8_t value =
       pattern_rec.locks[idx][ i];

                                                       if (value != 254) {
       templocks[m] = value; }
                                                 }

                                            }



                                  }

                                  //Clear the existing locks for the step
                                  clear_step_locks(curtrack,i);



                                  if
       (IS_BIT_SET64(pattern_rec.trigPatterns[curtrack],pattern_rec.patternLength
       - i - 1)) { SET_BIT64(pattern_rec.trigPatterns[curtrack],i);
                                  }
                                     else {
       CLEAR_BIT64(pattern_rec.trigPatterns[curtrack],i); }
                                   //Copy the parameter locks of the loop step,
       to the new step for (int m = 0; m < 24; m++) { if
       (IS_BIT_SET32(pattern_rec.lockPatterns[curtrack], m)) { int8_t idx =
       pattern_rec.paramLocks[curtrack][m]; int8_t param = m; int8_t value =
       pattern_rec.locks[idx][pattern_rec.patternLength - i - 1];

                                                   if (value != 254) {
       pattern_rec.addLock(curtrack, i, param, value); }
                                               }

                                            }

                                   //Clear the existing locks for the step
                                   clear_step_locks(curtrack,
       pattern_rec.patternLength - i - 1) ;

                                // pattern_rec.clearTrig(curtrack,
       pattern_rec.patternLength - i - 1 ); if (IS_BIT_SET64(temptrig,i))  {
       SET_BIT64(pattern_rec.trigPatterns[curtrack],pattern_rec.patternLength -
       i - 1 ); } else {
       CLEAR_BIT64(pattern_rec.trigPatterns[curtrack],pattern_rec.patternLength
       - i - 1 ); }


                                 uint8_t h = 0;
                                 while (h < (24)) {
                                   if (templocks[h] != 254) {
                                   pattern_rec.addLock(curtrack,
       pattern_rec.patternLength - i - 1 , h, templocks[h]);
                                   }
                                   h++;
                                 }




                                //If there is a accent trigger on the loop step,
       copy the accent trigger to the new step if
       (IS_BIT_SET64(tempaccent,pattern_rec.patternLength - i - 1)) {
       SET_BIT64(pattern_rec.accentPatterns[curtrack],i); } else {
       CLEAR_BIT64(pattern_rec.accentPatterns[curtrack],i); } if
       (IS_BIT_SET64(tempaccent,i)) {
       SET_BIT64(pattern_rec.accentPatterns[curtrack],pattern_rec.patternLength
       - i - 1); } else {
       CLEAR_BIT64(pattern_rec.accentPatterns[curtrack],pattern_rec.patternLength
       - i - 1); }

                                          //If there is a accent trigger on the
       loop step, copy the accent trigger to the new step if
       (IS_BIT_SET64(tempslide,pattern_rec.patternLength - i - 1)) {
       SET_BIT64(pattern_rec.slidePatterns[curtrack],i); } else {
       CLEAR_BIT64(pattern_rec.slidePatterns[curtrack],i); } if
       (IS_BIT_SET64(tempslide,i)) {
       SET_BIT64(pattern_rec.slidePatterns[curtrack],pattern_rec.patternLength -
       i - 1); } else {
       CLEAR_BIT64(pattern_rec.slidePatterns[curtrack],pattern_rec.patternLength
       - i - 1); }
                                          //If there is a accent trigger on the
       loop step, copy the accent trigger to the new step if
       (IS_BIT_SET64(tempswing,pattern_rec.patternLength - i - 1)) {
       SET_BIT64(pattern_rec.swingPatterns[curtrack],i); } else {
       CLEAR_BIT64(pattern_rec.swingPatterns[curtrack],i); } if
       (IS_BIT_SET64(tempswing,i)) {
       SET_BIT64(pattern_rec.swingPatterns[curtrack],pattern_rec.patternLength -
       i - 1); } else {
       CLEAR_BIT64(pattern_rec.swingPatterns[curtrack],pattern_rec.patternLength
       - i - 1); }
                                          //If there is a accent trigger on the
       loop step, copy the accent trigger to the new step

                              i++;
                          }

                                 //Define sysex encoder objects for the Pattern
       and Kit ElektronDataToSysexEncoder encoder(&MidiUart);

                      setLed();

                      //Send the encoded pattern to the MD via sysex
                      pattern_rec.toSysex(encoder);
                      clearLed();
                      mcl_actions.patternswitch = PATTERN_UDEF;

             }
    */
    //         else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
  }

  // Loop track switch
  else if (mcl_actions.patternswitch == 4) {
    // Retrieve the pattern from the Sysex buffer and store it in the
    // pattern_rec object. The MD header is 5 bytes long, hence the offset and
    // length change
    if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {

      /*
                             int curtrack =
         MD.getCurrentTrack(CALLBACK_TIMEOUT); int i = encodervalue;

                             if (pattern_rec.patternLength < encodervalue) {
         encodervalue = pattern_rec.patternLength; }

                                 while (i < pattern_rec.patternLength) {
                              //Loop for x amount of steps, where x =
         encodervalue. Eg loop the first 4 steps or 8 steps for (int n = 0; n <
         encodervalue; n++) {



                           //Clear the locks

                                    clear_step_locks(curtrack,i+n);


                                   //Check to see if the current bit is set in
         the loop if (IS_BIT_SET64(pattern_rec.trigPatterns[curtrack],n)) {
                                   //Set the new step to match the loop step


                                      SET_BIT64(pattern_rec.trigPatterns[curtrack],i
         + n);
                                      //Copy the parameter locks of the loop
         step, to the new step for (int m = 0; m < 24; m++) { if
         (IS_BIT_SET32(pattern_rec.lockPatterns[curtrack], m)) { int8_t idx =
         pattern_rec.paramLocks[curtrack][m]; int8_t param = m; int8_t value =
         pattern_rec.locks[idx][n]; if (value != 254) {
         pattern_rec.addLock(curtrack, i+n, param, value); }
                                                 }
                                       }
                                   }
                                   else {
         CLEAR_BIT64(pattern_rec.trigPatterns[curtrack],i + n); }
                                  //If there is a accent trigger on the loop
         step, copy the accent trigger to the new step if
         (IS_BIT_SET64(pattern_rec.accentPatterns[curtrack],n)) {
         SET_BIT64(pattern_rec.accentPatterns[curtrack],i + n); } else {
         CLEAR_BIT64(pattern_rec.accentPatterns[curtrack],i + n); }
                                  //If there is a accent trigger on the loop
         step, copy the accent trigger to the new step if
         (IS_BIT_SET64(pattern_rec.swingPatterns[curtrack],n)) {
         SET_BIT64(pattern_rec.swingPatterns[curtrack],i + n); } else {
         CLEAR_BIT64(pattern_rec.swingPatterns[curtrack],i + n); }
                                   //If there is a slide trigger on the loop
         step, copy the slide trigger to the new step if
         (IS_BIT_SET64(pattern_rec.slidePatterns[curtrack],n)) {
         SET_BIT64(pattern_rec.slidePatterns[curtrack],i + n); } else {
         CLEAR_BIT64(pattern_rec.slidePatterns[curtrack],i + n); }
                                }
                                i = i + encodervalue;
                            }

                                   //Define sysex encoder objects for the
         Pattern and Kit ElektronDataToSysexEncoder encoder(&MidiUart);

                        setLed();

                        //Send the encoded pattern to the MD via sysex
                        pattern_rec.toSysex(encoder);
                        clearLed();
      */
      mcl_actions.patternswitch = PATTERN_UDEF;
    }
    //   else { GUI.flash_strings_fill("SYSEX", "ERROR");  }
  }

  /*If mcl_actions.patternswitch == PATTERN_STORE, the pattern receiveed is for storing
     track data*/
  else if (mcl_actions.patternswitch == PATTERN_STORE) {

    /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec
     * object. The MD header is 5 bytes long, hence the offset and length
     * change*/
    //        if (pattern_rec.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen
    //        - 5)) {

    // mcl_actions.patternswitch = PATTERN_UDEF;
    //      }
    /*If the pattern can't be retrieved from the sysex data then there's been a
     * problem*/
    //  else { GUI.flash_strings_fill("SYSEX", "ERROR");  }

  }
  /*If mcl_actions.patternswitch == 1, the pattern receiveed is for sending a track to the
     MD.*/
  else if (mcl_actions.patternswitch == 1) {

    /*Retrieve the pattern from the Sysex buffer and store it in the pattern_rec
     * object. The MD header is 5 bytes long, hence the offset and length
     * change*/

    // else { GUI.flash_strings_fill("SYSEX", "ERROR"); }
  }
  clearLed2();
}
