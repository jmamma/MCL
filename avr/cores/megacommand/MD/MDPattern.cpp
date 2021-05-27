/* Copyright (c) 2009 - http://ruinwesen.com/ */
#include "Elektron.h"
#include "GUI.h"
#include "MD.h"
#include "MDMessages.h"
#include "MDParams.h"
#include "MDPattern.h"
#include "helpers.h"

#ifdef HOST_MIDIDUINO
#include <stdio.h>
#endif

// #include "GUI.h"

void MDPattern::clearPattern() {
  numRows = 0;

  //	m_memclr(this, sizeof(MDPattern));
  m_memclr(&trigPatterns, 8 * 16 + 4 * 16 + 4 * 8 + 6);
  m_memclr(&accentPatterns, 8 * 3 * 16 + 1);

  m_memset(paramLocks, sizeof(paramLocks), -1);
  m_memset(lockTracks, sizeof(lockTracks), -1);
  m_memset(lockParams, sizeof(lockParams), -1);
  m_memset(locks, sizeof(locks), -1);

  //	accentPattern = 0;
  //	slidePattern = 0;
  //	swingPattern = 0;
  //	accentAmount = 0;
  accentEditAll = 1;
  swingEditAll = 1;
  slideEditAll = 1;
  //	doubleTempo = 0;

  patternLength = 16;
  swingAmount = 50ULL << 14;
  //	origPosition = 0;
  //	kit = 0;
  //	scale = 0;
}

void MDPattern::clear_step_locks(uint8_t track, uint8_t step) {
  for (uint8_t p = 0; p < 24; p++) {
    int8_t idxn = getLockIdx(track, p);
    if (idxn != -1) {
      locks[idxn][step] = 254;
    }
  }
}

bool MDPattern::fromSysex(uint8_t *data, uint16_t len) {
  init();
  if ((len != (0xACA - 6)) && (len != (0x1521 - 6))) {
#ifndef HOST_MIDIDUINO
    GUI.flash_string_fill("WRONG LENGTH");
#else
    printf("WRONG LENGTH: %x\n", len);
#endif
    return false;
  }

  isExtraPattern = (len == (0x1521 - 6));

  if (!ElektronHelper::checkSysexChecksum(data, len)) {
    return false;
  }

  origPosition = data[3];
  ElektronSysexDecoder decoder(data + 0xA - 6);
  decoder.get32(trigPatterns, 16);

  decoder.start7Bit();
  decoder.get32(lockPatterns, 16);

  decoder.start7Bit();

  decoder.get32(&accentPattern);

  decoder.get32(&slidePattern);
  decoder.get32(&swingPattern);

  decoder.get32(&swingAmount);

  /*
   accentPattern = decoder.gget32();
   slidePattern  = decoder.gget32();
   swingPattern  = decoder.gget32();
   swingAmount   = decoder.gget32();
   */
  decoder.stop7Bit();

  accentAmount = decoder.gget8();
  patternLength = decoder.gget8();
  doubleTempo = decoder.gget8();
  scale = decoder.gget8();
  kit = decoder.gget8();
  numLockedRows = decoder.gget8();

  decoder.start7Bit();
  for (uint8_t i = 0; i < 64; i++) {
    decoder.get(locks[i], 32);
  }

  decoder.start7Bit();
  decoder.get32(&accentEditAll);
  decoder.get32(&slideEditAll);
  decoder.get32(&swingEditAll);
  decoder.get32(accentPatterns, 16);
  decoder.get32(slidePatterns, 16);
  decoder.get32(swingPatterns, 16);

  numRows = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 24; j++) {
      if (IS_BIT_SET32(lockPatterns[i], j)) {
        paramLocks[i][j] = numRows;
        lockTracks[numRows] = i;
        lockParams[numRows] = j;
        numRows++;
      }
    }
  }

  if (isExtraPattern) {
    decoder.start7Bit();
    decoder.get32hi(trigPatterns, 16);
    decoder.get32hi(&accentPattern);

    decoder.get32hi(&slidePattern);
    decoder.get32hi(&swingPattern);

    for (uint8_t i = 0; i < 64; i++) {
      decoder.get(locks[i] + 32, 32);
    }
    decoder.get32hi(accentPatterns, 16);
    decoder.get32hi(slidePatterns, 16);
    decoder.get32hi(swingPatterns, 16);
  }

  return true;
}

bool MDPattern::fromSysex(MidiClass *midi) {

  init();
  uint16_t len = midi->midiSysex.get_recordLen() - 5;
  uint16_t offset = 5;

  if ((len != (0xACA - 6)) && (len != (0x1521 - 6))) {
#ifndef HOST_MIDIDUINO
    GUI.flash_string_fill("WRONG LENGTH");
#else
    printf("WRONG LENGTH: %x\n", len);
#endif

    DEBUG_PRINTLN(F("WRONG LENGTH"));
    DEBUG_DUMP(len);
    return false;
  }

  isExtraPattern = (len == (0x1521 - 6));

  if (!ElektronHelper::checkSysexChecksum(midi, offset, len)) {

    DEBUG_PRINTLN(F("bad checksum"));
    return false;
  }

  origPosition = midi->midiSysex.getByte(3);
  ElektronSysexDecoder decoder(midi, offset + 0xA - 6);
  decoder.get32(trigPatterns, 16);

  decoder.start7Bit();
  decoder.get32(lockPatterns, 16);

  decoder.start7Bit();

  decoder.get32(&accentPattern);

  decoder.get32(&slidePattern);
  decoder.get32(&swingPattern);

  decoder.get32(&swingAmount);

  /*
   accentPattern = decoder.gget32();
   slidePattern  = decoder.gget32();
   swingPattern  = decoder.gget32();
   swingAmount   = decoder.gget32();
   */
  decoder.stop7Bit();

  accentAmount = decoder.gget8();
  patternLength = decoder.gget8();
  doubleTempo = decoder.gget8();
  scale = decoder.gget8();
  kit = decoder.gget8();
  numLockedRows = decoder.gget8();

  decoder.start7Bit();
  for (uint8_t i = 0; i < 64; i++) {
    decoder.get(locks[i], 32);
  }

  decoder.start7Bit();
  decoder.get32(&accentEditAll);
  decoder.get32(&slideEditAll);
  decoder.get32(&swingEditAll);
  decoder.get32(accentPatterns, 16);
  decoder.get32(slidePatterns, 16);
  decoder.get32(swingPatterns, 16);

  numRows = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 24; j++) {
      if (IS_BIT_SET32(lockPatterns[i], j)) {
        paramLocks[i][j] = numRows;
        lockTracks[numRows] = i;
        lockParams[numRows] = j;
        numRows++;
      }
    }
  }

  if (isExtraPattern) {
    decoder.start7Bit();
    decoder.get32hi(trigPatterns, 16);
    decoder.get32hi(&accentPattern);

    decoder.get32hi(&slidePattern);
    decoder.get32hi(&swingPattern);

    for (uint8_t i = 0; i < 64; i++) {
      decoder.get(locks[i] + 32, 32);
    }
    decoder.get32hi(accentPatterns, 16);
    decoder.get32hi(slidePatterns, 16);
    decoder.get32hi(swingPatterns, 16);
  }

  return true;
}

uint16_t MDPattern::toSysex() {
  ElektronDataToSysexEncoder encoder(&MidiUart);
  return toSysex(&encoder);
}


uint16_t MDPattern::toSysex(uint8_t *data, uint16_t len) {
  ElektronDataToSysexEncoder encoder(data);
  isExtraPattern = patternLength > 32;
  uint16_t sysexLength = isExtraPattern ? 0x151d : 0xac6;

  if (len < (sysexLength + 5))
    return 0;

  return toSysex(&encoder);
}

uint16_t MDPattern::toSysex(ElektronDataToSysexEncoder *encoder) {
  DEBUG_PRINT_FN();
  isExtraPattern = patternLength > 32;

  if ((MidiClock.state == 2) && (MD.midi->uart->speed > 62500)) {
    DEBUG_PRINTLN(F("using throttle"));
    encoder->throttle = true;
    //float swing = (float) MD.swing_last / 16385.0;
    //encoder.throttle_mod12 = round(swing * 12);
  }
  cleanupLocks();
  recalculateLockPatterns();

  /*
  int8_t paramLocks_[16][24];
  uint8_t locks_[64][64];
  int8_t lockTracks_[64];
  int8_t lockParams_[64];

  numRows = 0;
  for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 24; j++) {
          if (IS_BIT_SET32(lockPatterns[i], j)) {
              paramLocks_[i][j] = numRows;
              lockTracks_[numRows] = i;
              lockParams_[numRows] = j;
              paramLocks_[i][j] = numRows;
              int idx_ = paramLocks[i][j];
              for (int k = 0; k < 64; k++) {
                  locks_[k][numRows] = locks[k][idx_];
              }
              numRows++;
          }
      }
  }
   */
  uint16_t sysexLength = isExtraPattern ? 0x151d : 0xac6;

  encoder->stop7Bit();
  encoder->begin();
  encoder->pack(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
  encoder->pack8(MD_PATTERN_MESSAGE_ID);
  encoder->pack8(0x03); // version
  encoder->pack8(0x01);

  encoder->startChecksum();
  encoder->pack8(origPosition);

  encoder->start7Bit();
  encoder->pack32(trigPatterns, 16);
  encoder->reset();
  encoder->pack32(lockPatterns, 16);
  encoder->reset();

  encoder->pack32(accentPattern);

  encoder->pack32(slidePattern);
  encoder->pack32(swingPattern);
  encoder->pack32(swingAmount);

  encoder->stop7Bit();

  encoder->pack8(accentAmount);

  encoder->pack8(patternLength);
  encoder->pack8(doubleTempo);
  encoder->pack8(scale);
  encoder->pack8(kit);
  encoder->pack8(numLockedRows);

  encoder->start7Bit();

  uint8_t lockIdx = 0;
  for (uint8_t track = 0; track < 16; track++) {
    for (uint8_t param = 0; param < 24; param++) {
      int8_t lock = paramLocks[track][param];
      if ((lock != -1) && (lockIdx < 64)) {
        encoder->pack(locks[lock], 32);
        lockIdx++;
      }
    }
  }
  encoder->fill8(0xFF, 32 * (64 - lockIdx));
  encoder->reset(); // reset 7 bit

  encoder->pack32(accentEditAll);

  encoder->pack32(slideEditAll);
  encoder->pack32(swingEditAll);

  encoder->pack32(accentPatterns, 16);
  encoder->pack32(slidePatterns, 16);
  encoder->pack32(swingPatterns, 16);

  encoder->finish();

  if (isExtraPattern) {
    encoder->start7Bit();
    encoder->pack32hi(trigPatterns, 16);
    encoder->pack32hi(accentPattern);

    encoder->pack32hi(slidePattern);
    encoder->pack32hi(swingPattern);

    lockIdx = 0;
    for (uint8_t track = 0; track < 16; track++) {
      for (uint8_t param = 0; param < 24; param++) {
        int8_t lock = paramLocks[track][param];
        if ((lock != -1) && (lockIdx < 64)) {
          encoder->pack(locks[lock] + 32, 32);
          lockIdx++;
        }
      }
    }
    encoder->fill8(0xFF, 32 * (64 - lockIdx));
    encoder->pack32hi(accentPatterns, 16);

    encoder->pack32hi(slidePatterns, 16);
    encoder->pack32hi(swingPatterns, 16);

    encoder->finish();
  }

  encoder->finishChecksum();

  return sysexLength + 5;
}

void MDPattern::recalculateLockPatterns() {
  for (uint8_t track = 0; track < 16; track++) {
    lockPatterns[track] = 0;
    for (uint8_t param = 0; param < 24; param++) {
      if (paramLocks[track][param] != -1) {
        SET_BIT32(lockPatterns[track], param);
      }
    }
  }
}

/***************************************************************************
 *
 * Pattern edit functions
 *
 ***************************************************************************/

bool MDPattern::isTrackEmpty(uint8_t track) {
  return ((trigPatterns[track] == 0) && (lockPatterns[track] == 0));
}

void MDPattern::clearTrack(uint8_t track) {
  if (track >= 16)
    return;
  for (uint8_t i = 0; i < 64; i++) {
    clearTrig(track, i);
  }
  // XXX swing and slide
  clearTrackLocks(track);
}

void MDPattern::clearTrig(uint8_t track, uint8_t trig) {
  CLEAR_BIT64(trigPatterns[track], trig);
  for (uint8_t i = 0; i < 24; i++) {
    clearLock(track, trig, i);
  }
}

void MDPattern::setNote(uint8_t track, uint8_t step, uint8_t pitch) {
  // XXX real pitch conversion
  addLock(track, step, 0, pitch);
  setTrig(track, step);
}

/***************************************************************************
 *
 * Pattern manipulation functions
 *
 ***************************************************************************/

void MDPattern::swapTracks(uint8_t srcTrack, uint8_t dstTrack) {
  // slidePatterns, swingPattern, accentPatterns
  // paramLocks
  // lockPatterns
  // trigPatterns

  uint64_t _trigPattern;
  uint64_t _accentPattern;
  uint64_t _slidePattern;
  uint64_t _swingPattern;
  int8_t _paramLocks[24];

  _trigPattern = trigPatterns[srcTrack];
  _accentPattern = accentPatterns[srcTrack];
  _slidePattern = slidePatterns[srcTrack];
  _swingPattern = swingPatterns[srcTrack];
  for (uint8_t i = 0; i < 24; i++) {
    _paramLocks[i] = paramLocks[srcTrack][i];
  }

  trigPatterns[srcTrack] = trigPatterns[dstTrack];
  trigPatterns[dstTrack] = _trigPattern;
  accentPatterns[srcTrack] = accentPatterns[dstTrack];
  accentPatterns[dstTrack] = _accentPattern;
  slidePatterns[srcTrack] = slidePatterns[dstTrack];
  slidePatterns[dstTrack] = _slidePattern;
  swingPatterns[srcTrack] = swingPatterns[dstTrack];
  swingPatterns[dstTrack] = _swingPattern;
  for (uint8_t i = 0; i < 24; i++) {
    paramLocks[srcTrack][i] = paramLocks[dstTrack][i];
    paramLocks[dstTrack][i] = _paramLocks[i];
  }
}

void MDPattern::reverseTrack(uint8_t track) {}

void MDPattern::reversePattern() {
  for (uint8_t i = 0; i < 16; i++) {
    reverseTrack(i);
  }
}

void MDPattern::reverseBlockTrack(uint8_t track, uint8_t blockSize) {}

void MDPattern::reverseBlockPattern(uint8_t blockSize) {
  for (uint8_t i = 0; i < 16; i++) {
    reverseBlockTrack(i, blockSize);
  }
}
