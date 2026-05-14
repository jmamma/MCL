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
  memset(&trigPatterns, 0, sizeof(trigPatterns) + sizeof(lockPatterns) + 4 * 8 + 6);
  memset(&accentPatterns, 0, 8 * 3 * 16 + 1);

  memset(paramLocks, -1, sizeof(paramLocks));

  memset(locks, -1, sizeof(locks) + sizeof(lockTracks) + sizeof(lockParams));

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

#if !defined(__AVR__)
  version = 0x03;
  memset(ext_microtiming, 0, sizeof(ext_microtiming));
  memset(ext_step_flags, 0, sizeof(ext_step_flags));
  memset(ext_locks_params, 0, sizeof(ext_locks_params));
  memset(ext_track_lengths, 0, sizeof(ext_track_lengths));
  memset(ext_track_speeds, 0xFF, sizeof(ext_track_speeds));
  patternTempo = 0;
  chain_change = 0;
  memset(ext_locks, -1, sizeof(ext_locks));
  memset(ext_lockTracks, -1, sizeof(ext_lockTracks));
  memset(ext_lockParams, -1, sizeof(ext_lockParams));
#endif
}
/*
void MDPattern::clear_step_locks(uint8_t track, uint8_t step) {
  for (uint8_t p = 0; p < 24; p++) {
    int8_t idxn = getLockIdx(track, p);
    if (idxn != -1) {
      locks[idxn][step] = 254;
    }
  }
}
*/
bool MDPattern::fromSysex(MidiClass *midi) {

  init();
  SysexView sysex(midi->midiSysex);
  uint16_t len = sysex.get_recordLen() - 5;
  uint16_t offset = 5;

#if !defined(__AVR__)
  version = sysex.getByte(6);
  bool is_spsx_pat = (version == 0x40);

  if (!is_spsx_pat) {
#endif
    if ((len != (0xACA - 6)) && (len != (0x1521 - 6))) {
      DEBUG_PRINTLN(F("WRONG LENGTH"));
      return false;
    }
    isExtraPattern = (len == (0x1521 - 6));
#if !defined(__AVR__)
  }
#endif

  if (!ElektronHelper::checkSysexChecksum(sysex, offset, len)) {

    DEBUG_PRINTLN(F("bad checksum"));
    return false;
  }

  origPosition = sysex.getByte(8);
  ElektronSysexDecoder decoder(sysex, offset + 0xA - 6);
  decoder.get32(trigPatterns, 16);

  decoder.start7Bit();
  decoder.get32(lockPatterns, 16);

  decoder.start7Bit();

  decoder.get32(&accentPattern);

  decoder.get32(&slidePattern);
  decoder.get32(&swingPattern);

  decoder.get32(&swingAmount);

  decoder.stop7Bit();

  accentAmount = decoder.gget8();
  patternLength = decoder.gget8();
  doubleTempo = decoder.gget8();
  scale = decoder.gget8();
  kit = decoder.gget8();
  numLockedRows = decoder.gget8();

#if !defined(__AVR__)
  if (is_spsx_pat) {
    isExtraPattern = (patternLength > 32);
  }
#endif

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

  // Build lock tracking for V3 (params 0-23 only)
  // SPSX defers until extension is decoded so high bits are available
#if !defined(__AVR__)
  if (!is_spsx_pat) {
#endif
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
#if !defined(__AVR__)
  }
#endif

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

#if !defined(__AVR__)
  if (is_spsx_pat) {
    decoder.start7Bit();
    decoder.startRLE();

    for (uint8_t t = 0; t < 16; t++) {
      decoder.get((uint8_t*)ext_microtiming[t], 64);
      decoder.get(ext_step_flags[t], 64);
      decoder.get(ext_locks_params[t], MD_PATTERN_LOCK_SLOTS);
    }

    decoder.get(ext_track_lengths, 16);
    decoder.get(ext_track_speeds, 16);

    // High 32 bits of lockPatterns (params 24-33)
    decoder.get32hi(lockPatterns, 16);

    // Expanded lock rows. Wire format mirrors the host (src/host/Midi/MDPattern.cpp):
    // numRows is a little-endian u16, then for rows 64..numRows-1 the encoder
    // emits 32 bytes of low-step locks per row, followed (for 64-step / extra
    // patterns) by 32 bytes of high-step locks per row.
    uint8_t lo = decoder.gget8();
    uint8_t hi = decoder.gget8();
    uint16_t totalRows = (uint16_t)(lo | (hi << 8));

    if (totalRows > 64) {
      uint16_t cap = (totalRows < MAX_LOCK_ROWS) ? totalRows : MAX_LOCK_ROWS;
      for (uint16_t i = 64; i < cap; i++) {
        decoder.get((uint8_t*)lock_row(i), 32);
      }
      // Past-cap rows: drop them (MCL refuses patterns deeper than MAX_LOCK_ROWS)
      if (totalRows > MAX_LOCK_ROWS) {
        decoder.skip((totalRows - MAX_LOCK_ROWS) * 32);
      }
      if (isExtraPattern) {
        for (uint16_t i = 64; i < cap; i++) {
          decoder.get((uint8_t*)(lock_row(i) + 32), 32);
        }
        if (totalRows > MAX_LOCK_ROWS) {
          decoder.skip((totalRows - MAX_LOCK_ROWS) * 32);
        }
      }
    }

    lo = decoder.gget8();
    hi = decoder.gget8();
    patternTempo = (uint16_t)(lo | (hi << 8));
    chain_change = decoder.gget8();

    decoder.stopRLE();

    // Rebuild lock tracking with full param range (0-33). MCL stores up to
    // MAX_LOCK_ROWS rows: rows 0..63 in the base lockTracks/lockParams arrays,
    // rows 64..MAX_LOCK_ROWS-1 in ext_lockTracks/ext_lockParams.
    numRows = 0;
    for (uint8_t i = 0; i < 16; i++) {
      for (uint8_t j = 0; j < maxParams; j++) {
        if (IS_BIT_SET64(lockPatterns[i], j)) {
          if (numRows < MAX_LOCK_ROWS) {
            paramLocks[i][j] = (int16_t)numRows;
            set_lock_track_param(numRows, (int16_t)i, (int16_t)j);
          }
          numRows++;
        }
      }
    }
  }
#endif

  return true;
}

uint16_t MDPattern::toSysex() {
  ElektronDataToSysexEncoder encoder(MD.uart);
  return toSysex(&encoder);
}


uint16_t MDPattern::toSysex(ElektronDataToSysexEncoder *encoder) {
#ifdef MDPATTERN_TOSYSEX_ENABLE
  DEBUG_PRINT_FN();
  isExtraPattern = patternLength > 32;

  if ((MidiClock.state == 2) && (MD.midi->uart->speed > 62500)) {
    //DEBUG_PRINTLN(F("using throttle"));
    //encoder->throttle = true;
    //float swing = (float) MD.swing_last / 16385.0;
    //encoder.throttle_mod12 = round(swing * 12);
  }
  cleanupLocks();
  recalculateLockPatterns();

  uint16_t sysexLength = isExtraPattern ? 0x151d : 0xac6;

#if !defined(__AVR__)
  bool use_spsx = MD.is_spsx;
  uint8_t ver = use_spsx ? 0x40 : 0x03;
  uint8_t paramLimit = use_spsx ? SPS_PARAMS_PER_TRACK : 24;
#else
  uint8_t ver = 0x03;
  uint8_t paramLimit = 24;
#endif

  ElektronHelper::beginSysexEncode(encoder, machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr), MD_PATTERN_MESSAGE_ID, ver, origPosition);

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
  encoder->pack8((numRows < 255) ? (uint8_t)numRows : 255);

  encoder->start7Bit();

  uint8_t lockIdx = 0;
  for (uint8_t track = 0; track < 16; track++) {
    for (uint8_t param = 0; param < paramLimit; param++) {
      // paramLocks values can exceed 63 on SPSX patterns loaded from sysex
      // (rows 64..MAX_LOCK_ROWS-1 live in ext_locks). Use lock_row() so the
      // access stays bounded; ext rows are also re-emitted in the SPSX block
      // below for the host decoder.
      int16_t lock = paramLocks[track][param];
      if ((lock != -1) && (lockIdx < 64)) {
        encoder->pack((const uint8_t*)lock_row((uint16_t)lock), 32);
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
      for (uint8_t param = 0; param < paramLimit; param++) {
        int16_t lock = paramLocks[track][param];
        if ((lock != -1) && (lockIdx < 64)) {
          encoder->pack((const uint8_t*)(lock_row((uint16_t)lock) + 32), 32);
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

#if !defined(__AVR__)
  if (use_spsx) {
    encoder->start7Bit();
    encoder->startRLE();

    for (uint8_t t = 0; t < 16; t++) {
      encoder->pack((const uint8_t*)ext_microtiming[t], 64);
      encoder->pack(ext_step_flags[t], 64);
      encoder->pack(ext_locks_params[t], MD_PATTERN_LOCK_SLOTS);
    }

    encoder->pack(ext_track_lengths, 16);
    encoder->pack(ext_track_speeds, 16);

    encoder->pack32hi(lockPatterns, 16);

    uint16_t out_numRows = (numRows < MAX_LOCK_ROWS) ? numRows : MAX_LOCK_ROWS;
    // numRows (little-endian u16)
    encoder->pack8(out_numRows & 0xFF);
    encoder->pack8((out_numRows >> 8) & 0xFF);

    // Extra lock rows (64..numRows-1). Symmetric with fromSysex above:
    // first 32 bytes per row (low steps), then if isExtraPattern another
    // 32 bytes per row (high steps).
    if (numRows > 64) {
      uint16_t cap = (numRows < MAX_LOCK_ROWS) ? numRows : MAX_LOCK_ROWS;
      for (uint16_t i = 64; i < cap; i++) {
        encoder->pack((const uint8_t*)lock_row(i), 32);
      }
      if (isExtraPattern) {
        for (uint16_t i = 64; i < cap; i++) {
          encoder->pack((const uint8_t*)(lock_row(i) + 32), 32);
        }
      }
    }

    // Per-pattern tempo (little-endian u16)
    encoder->pack8((uint8_t)(patternTempo & 0xFF));
    encoder->pack8((uint8_t)((patternTempo >> 8) & 0xFF));
    encoder->pack8(chain_change);

    encoder->stopRLE();
    encoder->finish();
  }
#endif

  encoder->finishChecksum();

#if !defined(__AVR__)
  if (use_spsx) {
    // SPSX length is variable due to RLE; use actual encoded length
    return encoder->finish() + 1; // +1 for F7
  }
#endif
  return sysexLength + 5;
#else
  return 0;
#endif
}
void MDPattern::recalculateLockPatterns() {
  for (uint8_t track = 0; track < 16; track++) {
    lockPatterns[track] = 0;
    for (uint8_t param = 0; param < maxParams; param++) {
      if (paramLocks[track][param] != -1) {
#if !defined(__AVR__)
        SET_BIT64(lockPatterns[track], param);
#else
        SET_BIT32(lockPatterns[track], param);
#endif
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
/*
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
*/
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
#if !defined(__AVR__)
  int16_t _paramLocks[SPS_PARAMS_PER_TRACK];
#else
  int8_t _paramLocks[24];
#endif

  _trigPattern = trigPatterns[srcTrack];
  _accentPattern = accentPatterns[srcTrack];
  _slidePattern = slidePatterns[srcTrack];
  _swingPattern = swingPatterns[srcTrack];
  for (uint8_t i = 0; i < maxParams; i++) {
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
  for (uint8_t i = 0; i < maxParams; i++) {
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
