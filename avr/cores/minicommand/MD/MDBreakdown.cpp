/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "MD.h"
#include "MDBreakdown.h"

const char *MDBreakdown::repeatSpeedNames[REPEAT_SPEED_CNT] = {
  "2 BARS ",
  "1 BAR  ",
  "2 BEATS",
  "1 BEAT ",
  "1 8TH  ",
  "1 16TH "
};

const uint8_t MDBreakdown::repeatSpeedMask[REPEAT_SPEED_CNT] = {
  32 - 1,
  16 - 1, 
  8  - 1,
  4  - 1,
  2  - 1,
  1  - 1
};

const char *MDBreakdown::breakdownNames[BREAKDOWN_CNT] = {
  "NONE   ",
  "2 BARS ",
  "4 BARS ",
  "2+4 BAR"
};

void MDBreakdown::setup() {
  MidiClock.addOn16Callback(this, (midi_clock_callback_ptr_t)&MDBreakdown::on16Callback);
}

void MDBreakdown::startBreakdown() {
  breakdownActive = true;
}

void MDBreakdown::stopBreakdown() {
  breakdownActive = false;
  restorePlayback = true;
}

void MDBreakdown::on16Callback() {
  if (restorePlayback && !storedBreakdownActive) {
    uint8_t val = (MidiClock.div16th_counter) % 32;
    if ((val % 4) == 0) {
      restorePlayback = false;
      MD.sliceTrack32(ramP1Track, val, 32, true);
      return;
    }
  }
  if (supaTriggaActive) {
    doSupatrigga();
  } 
  else if (breakdownActive || storedBreakdownActive) {
    doBreakdown();
  } 
  else {
    uint8_t val = (MidiClock.div16th_counter) % 32;
    if (val == 0) {
      MD.sliceTrack32(ramP1Track, 0 , 32);
    }
  }

}

void MDBreakdown::doBreakdown() {
  if (muted)
    return;
  
  uint8_t val = (MidiClock.div16th_counter) % 64;

  switch (breakdown) {
  case BREAKDOWN_NONE:
    break;
      
  case BREAKDOWN_2_BARS:
    if (val == 24 || val == 56) {
      MD.sliceTrack32(ramP1Track, 24, 32, true);
      return;
    }
    break;

  case BREAKDOWN_4_BARS:
    if (val == 56) {
      MD.sliceTrack32(ramP1Track, 24, 32, true);
      return;
    }
    break;

  case BREAKDOWN_2_AND_4_BARS:
    if (val == 56) {
      MD.sliceTrack32(ramP1Track, 24, 32, true);
      return;
    } 
    else if (val == 24) {
      MD.sliceTrack32(ramP1Track, 24, 32, true);
      return;
    } 
    else if (val == 28) {
      MD.sliceTrack32(ramP1Track, 24, 32, true);
      return;
    }
    break;

  }

  if ((val % 8) == 0 || ((val & repeatSpeedMask[repeatSpeed]) == 0)) {
    MD.sliceTrack32(ramP1Track, val & repeatSpeedMask[repeatSpeed], 32);
  } 
}

void MDBreakdown::startSupatrigga() {
  supaTriggaActive = true;
}

void MDBreakdown::stopSupatrigga() {
  restorePlayback = true;
  supaTriggaActive = false;
}

void MDBreakdown::doSupatrigga() {
  uint8_t val = (MidiClock.div16th_counter) % 32;
  if ((val % 4) == 0) {
    uint8_t from = 0, to = 0;
    if (random(100) > 50) {
      from = random(0, 6);
      to = random(from + 2, 8);
    } 
    else {
      from = random(2, 8);
      to = random(0, from - 2);
    }
    MD.sliceTrack32(ramP1Track, from * 4, to * 4);
  }
}

MDBreakdown mdBreakdown;
