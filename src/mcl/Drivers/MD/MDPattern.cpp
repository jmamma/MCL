/* Copyright (c) 2009 - http://ruinwesen.com/ */
#include "Elektron.h"
#include "GUI.h"
#include "MD.h"
#include "MDMessages.h"
#include "MDParams.h"
#include "MDPattern.h"
#include "helpers.h"

#if !defined(__AVR__)
#include "MidiClock.h"
#include "MidiUart.h"
#endif

#ifdef HOST_MIDIDUINO
#include <stdio.h>
#endif

#if !defined(__AVR__)
namespace {

enum {
  MD_COMPACT_TRACKS = 16,
  MD_COMPACT_HEADER_BYTES = 16,
  MD_COMPACT_SETTINGS_BYTES = 40,
  MD_COMPACT_TRACK_BYTES = 40,
  MD_COMPACT_FIXED_BYTES =
      MD_COMPACT_HEADER_BYTES + MD_COMPACT_SETTINGS_BYTES +
      MD_COMPACT_TRACKS * MD_COMPACT_TRACK_BYTES,
  // 16 tracks * (37 lock lanes * (descriptor + 32 dense words) +
  // 64 microtiming words + 64 condition words).
  MD_COMPACT_MAX_WORDS = 21584
};

const uint8_t md_compact_magic[7] = {
    'S', 'P', 'S', 'X', 'P', 'T', 'N'};

struct MDCompactShape {
  uint8_t lanes[MD_COMPACT_TRACKS];
  uint8_t microtiming[MD_COMPACT_TRACKS];
  uint8_t conditions[MD_COMPACT_TRACKS];
  uint16_t words;
};

uint16_t md_compact_read16(const uint8_t *data) {
  return ((uint16_t)data[0] << 8) | data[1];
}

uint64_t md_compact_read64(const uint8_t *data) {
  uint64_t value = 0;
  for (uint8_t n = 0; n < 8; n++) {
    value = (value << 8) | data[n];
  }
  return value;
}

bool md_compact_mask_fits(const uint64_t &mask, uint8_t steps) {
  return steps == 64 || (uint32_t)(mask >> 32) == 0;
}

bool md_compact_wire_mask_fits(const uint8_t *mask, uint8_t steps) {
  return steps == 64 ||
         (mask[0] | mask[1] | mask[2] | mask[3]) == 0;
}

const int8_t *md_compact_lock_row(const MDPattern &pattern, uint16_t row) {
  return pattern.lock_row(row);
}

int8_t *md_compact_lock_row(MDPattern &pattern, uint16_t row) {
  return pattern.lock_row(row);
}

int16_t md_compact_lock_index(const MDPattern &pattern, uint8_t track,
                              uint8_t param) {
  return pattern.paramLocks[track][param];
}

uint8_t md_compact_lock_count(const MDPattern &pattern, uint8_t track,
                              uint8_t param, uint8_t steps) {
  int16_t row = md_compact_lock_index(pattern, track, param);
  if (row < 0 || (uint16_t)row >= MAX_LOCK_ROWS) {
    return 0;
  }
  const int8_t *values = md_compact_lock_row(pattern, (uint16_t)row);
  uint8_t count = 0;
  for (uint8_t step = 0; step < steps; step++) {
    if (values[step] >= 0) {
      count++;
    }
  }
  return count;
}

bool md_compact_step_has_lock(const MDPattern &pattern, uint8_t track,
                              uint8_t step) {
  for (uint8_t param = 0; param < MD_PATTERN_LOCK_SLOTS; param++) {
    int16_t row = pattern.paramLocks[track][param];
    if (row >= 0 && row < MAX_LOCK_ROWS &&
        pattern.lock_row((uint16_t)row)[step] >= 0) {
      return true;
    }
  }
  return false;
}

bool md_compact_header(const SysexView &sysex, uint8_t &steps,
                       uint16_t &used_words) {
  uint16_t record_len = sysex.get_recordLen();
  // Sixteen raw header bytes occupy nineteen bytes after 7-bit packing.
  if (record_len < 32 || sysex.getByte(5) != MD_PATTERN_MESSAGE_ID ||
      sysex.getByte(6) != MD_PATTERN_VERSION_SPSX ||
      sysex.getByte(7) != 1 ||
      !ElektronHelper::checkSysexChecksum(
          sysex, 5, (uint16_t)(record_len - 5))) {
    return false;
  }

  uint16_t declared = ElektronHelper::to16Bit7(
      sysex.getByte(record_len - 2), sysex.getByte(record_len - 1));
  if (declared != (uint16_t)(record_len - 8)) {
    return false;
  }

  ElektronSysexDecoder decoder(sysex, 9);
  uint8_t header[MD_COMPACT_HEADER_BYTES];
  decoder.get(header, sizeof(header));
  for (uint8_t n = 0; n < sizeof(md_compact_magic); n++) {
    if (header[n] != md_compact_magic[n]) {
      return false;
    }
  }
  steps = header[8];
  used_words = md_compact_read16(header + 10);
  if (header[7] != MD_PATTERN_VERSION_SPSX ||
      (steps != 32 && steps != 64) ||
      used_words > MD_COMPACT_MAX_WORDS ||
      header[9] != MD_COMPACT_TRACKS ||
      md_compact_read16(header + 12) !=
          MD_COMPACT_SETTINGS_BYTES +
              MD_COMPACT_TRACKS * MD_COMPACT_TRACK_BYTES ||
      header[14] != 0 || header[15] != 0) {
    return false;
  }

  uint16_t raw_bytes =
      MD_COMPACT_FIXED_BYTES + used_words * 2;
  uint16_t expected_wire = raw_bytes + (raw_bytes + 6) / 7;
  return expected_wire == (uint16_t)(record_len - 13);
}

bool md_compact_settings_valid(const uint8_t *settings, uint8_t steps) {
  uint8_t length = settings[32];
  uint8_t speed = settings[34];
  uint8_t kit = settings[36];
  if (length == 0 || length > steps || settings[28] > 1 ||
      settings[29] > 1 || settings[30] > 1 ||
      settings[31] > 1 || settings[33] > 3 ||
      speed > 7 || settings[35] > 1 ||
      (kit != 0xff && kit >= 64) ||
      !md_compact_wire_mask_fits(settings, steps) ||
      !md_compact_wire_mask_fits(settings + 8, steps) ||
      !md_compact_wire_mask_fits(settings + 16, steps)) {
    return false;
  }
  return true;
}

bool md_compact_build_shape(const MDPattern &pattern, uint8_t steps,
                            MDCompactShape &shape) {
  memset(&shape, 0, sizeof(shape));
  if (pattern.patternLength == 0 || pattern.patternLength > steps ||
      !md_compact_mask_fits(pattern.accentPattern, steps) ||
      !md_compact_mask_fits(pattern.slidePattern, steps) ||
      !md_compact_mask_fits(pattern.swingPattern, steps)) {
    return false;
  }

  uint16_t words = 0;
  for (uint8_t track = 0; track < MD_COMPACT_TRACKS; track++) {
    if (!md_compact_mask_fits(pattern.trigPatterns[track], steps) ||
        !md_compact_mask_fits(pattern.slidePatterns[track], steps) ||
        !md_compact_mask_fits(pattern.accentPatterns[track], steps) ||
        !md_compact_mask_fits(pattern.swingPatterns[track], steps)) {
      return false;
    }
    for (uint8_t param = 0; param < MD_PATTERN_LOCK_SLOTS; param++) {
      uint8_t points =
          md_compact_lock_count(pattern, track, param, steps);
      if (points == 0) {
        continue;
      }
      shape.lanes[track]++;
      words += 1 + (points > steps / 2 ? steps / 2 : points);
    }
    for (uint8_t step = 0; step < steps; step++) {
      if (pattern.ext_microtiming[track][step] != 0) {
        shape.microtiming[track]++;
        words++;
      }
      uint8_t flags = pattern.ext_step_flags[track][step];
      bool disabled = md_compact_step_has_lock(pattern, track, step) &&
                      (flags & 1) == 0;
      if ((flags >> 2) != 0 || (flags & 2) != 0 || disabled) {
        shape.conditions[track]++;
        words++;
      }
    }
  }
  shape.words = words;
  return true;
}

void md_compact_pack8(ElektronDataToSysexEncoder *encoder,
                      uint8_t value) {
  encoder->pack8(value);
}

void md_compact_pack_settings(ElektronDataToSysexEncoder *encoder,
                              const MDPattern &pattern) {
  uint32_t swing = (pattern.swingAmount * 30u + 8192u) >> 14;
  if (swing > 30) {
    swing = 30;
  }
  encoder->pack64(pattern.accentPattern);
  encoder->pack64(pattern.swingPattern);
  encoder->pack64(pattern.slidePattern);
  encoder->pack16(pattern.patternTempo);
  md_compact_pack8(encoder, pattern.accentAmount >> 3);
  md_compact_pack8(encoder, (uint8_t)swing);
  md_compact_pack8(encoder, pattern.accentEditAll ? 1 : 0);
  md_compact_pack8(encoder, pattern.slideEditAll ? 1 : 0);
  md_compact_pack8(encoder, pattern.swingEditAll ? 1 : 0);
  md_compact_pack8(encoder, pattern.ext_scale_mode);
  md_compact_pack8(encoder, pattern.patternLength);
  md_compact_pack8(encoder, pattern.scale);
  md_compact_pack8(encoder, pattern.doubleTempo);
  md_compact_pack8(encoder, pattern.ext_tempo_multiplier);
  md_compact_pack8(encoder, pattern.kit);
  md_compact_pack8(encoder, pattern.ext_chain_row);
  md_compact_pack8(encoder, pattern.ext_chain_loops);
  md_compact_pack8(encoder, pattern.chain_change);
}

void md_compact_pack_track(ElektronDataToSysexEncoder *encoder,
                           const MDPattern &pattern,
                           const MDCompactShape &shape, uint8_t track) {
  encoder->pack64(pattern.trigPatterns[track]);
  encoder->pack64(pattern.slideEditAll ? pattern.slidePattern
                                      : pattern.slidePatterns[track]);
  encoder->pack64(pattern.accentEditAll ? pattern.accentPattern
                                       : pattern.accentPatterns[track]);
  encoder->pack64(pattern.swingEditAll ? pattern.swingPattern
                                      : pattern.swingPatterns[track]);
  md_compact_pack8(encoder, pattern.ext_track_lengths[track]
                                ? pattern.ext_track_lengths[track]
                                : pattern.patternLength);
  md_compact_pack8(encoder, pattern.ext_track_speeds[track] != 0xff
                                ? pattern.ext_track_speeds[track]
                                : pattern.doubleTempo);
  md_compact_pack8(encoder, shape.lanes[track]);
  md_compact_pack8(encoder, shape.microtiming[track]);
  md_compact_pack8(encoder, shape.conditions[track]);
  encoder->fill8(0, 3);
}

void md_compact_pack_words(ElektronDataToSysexEncoder *encoder,
                           const MDPattern &pattern,
                           const MDCompactShape &shape, uint8_t steps) {
  for (uint8_t track = 0; track < MD_COMPACT_TRACKS; track++) {
    for (uint8_t param = 0; param < MD_PATTERN_LOCK_SLOTS; param++) {
      uint8_t points =
          md_compact_lock_count(pattern, track, param, steps);
      if (points == 0) {
        continue;
      }
      bool dense = points > steps / 2;
      uint16_t descriptor =
          param | ((uint16_t)(points - 1) << 6) |
          (dense ? 0x8000 : 0);
      encoder->pack16(descriptor);
      int16_t row = md_compact_lock_index(pattern, track, param);
      const int8_t *values =
          md_compact_lock_row(pattern, (uint16_t)row);
      if (dense) {
        for (uint8_t step = 0; step < steps; step += 2) {
          uint8_t high =
              values[step] >= 0 ? (uint8_t)values[step] : 0xff;
          uint8_t low =
              values[step + 1] >= 0 ? (uint8_t)values[step + 1] : 0xff;
          encoder->pack16(((uint16_t)high << 8) | low);
        }
      } else {
        for (uint8_t step = 0; step < steps; step++) {
          if (values[step] >= 0) {
            encoder->pack16(
                step | ((uint16_t)(uint8_t)values[step] << 6));
          }
        }
      }
    }
    for (uint8_t step = 0; step < steps; step++) {
      int8_t value = pattern.ext_microtiming[track][step];
      if (value != 0) {
        encoder->pack16(
            step | ((uint16_t)(uint8_t)value << 6));
      }
    }
    for (uint8_t step = 0; step < steps; step++) {
      uint8_t flags = pattern.ext_step_flags[track][step];
      uint8_t condition = flags >> 2;
      bool plock = (flags & 2) != 0;
      bool disabled = md_compact_step_has_lock(pattern, track, step) &&
                      (flags & 1) == 0;
      if (condition != 0 || plock || disabled) {
        encoder->pack16(
            step | ((uint16_t)condition << 6) |
            (plock ? 0x1000 : 0) | (disabled ? 0x2000 : 0));
      }
    }
  }
  (void)shape;
}

} // namespace
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
  ext_scale_mode = 1;
  ext_tempo_multiplier = 0;
  ext_chain_row = 0xff;
  ext_chain_loops = 0;
  memset(ext_locks, -1, sizeof(ext_locks));
  memset(ext_lockTracks, -1, sizeof(ext_lockTracks));
  memset(ext_lockParams, -1, sizeof(ext_lockParams));
#endif
}

#if !defined(__AVR__)
namespace {

bool md_lock_row_empty(const MDPattern &pattern, ep_lock_idx_t row) {
  if (row < 0 || row >= MAX_LOCK_ROWS)
    return true;
  const int8_t *values = row < 64
                             ? reinterpret_cast<const int8_t *>(
                                   pattern.locks[row])
                             : pattern.ext_locks[row - 64];
  for (uint8_t step = 0; step < pattern.maxSteps; ++step) {
    if (values[step] != -1)
      return false;
  }
  return true;
}

} // namespace

void MDPattern::clearLockPattern(ep_lock_idx_t row) {
  if (row < 0 || row >= MAX_LOCK_ROWS)
    return;
  const int16_t track = lock_track((uint16_t)row);
  const int16_t param = lock_param((uint16_t)row);
  memset(lock_row((uint16_t)row), -1, maxSteps);
  if (track >= 0 && track < 16 && param >= 0 &&
      param < SPS_PARAMS_PER_TRACK && paramLocks[track][param] == row) {
    paramLocks[track][param] = -1;
  }
  set_lock_track_param((uint16_t)row, -1, -1);
}

ep_lock_idx_t MDPattern::getNextEmptyLock() {
  for (ep_lock_idx_t row = 0; row < MAX_LOCK_ROWS; ++row) {
    if (lock_track((uint16_t)row) == -1 &&
        lock_param((uint16_t)row) == -1) {
      return row;
    }
  }
  return -1;
}

bool MDPattern::addLock(uint8_t track, uint8_t step, uint8_t param,
                        uint8_t value) {
  if (track >= 16 || step >= maxSteps || param >= SPS_PARAMS_PER_TRACK)
    return false;
  ep_lock_idx_t row = getLockIdx(track, param);
  if (row == -1) {
    row = getNextEmptyLock();
    if (row == -1)
      return false;
    setLockIdx(track, param, row);
    set_lock_track_param((uint16_t)row, track, param);
    memset(lock_row((uint16_t)row), -1, maxSteps);
  }
  lock_row((uint16_t)row)[step] = (int8_t)value;
  return true;
}

void MDPattern::clearLock(uint8_t track, uint8_t step, uint8_t param) {
  if (track >= 16 || step >= maxSteps || param >= SPS_PARAMS_PER_TRACK)
    return;
  const ep_lock_idx_t row = getLockIdx(track, param);
  if (row < 0 || row >= MAX_LOCK_ROWS)
    return;
  lock_row((uint16_t)row)[step] = -1;
  if (md_lock_row_empty(*this, row))
    clearLockPattern(row);
}

uint8_t MDPattern::getLock(uint8_t track, uint8_t step, uint8_t param) {
  if (track >= 16 || step >= maxSteps || param >= SPS_PARAMS_PER_TRACK)
    return 255;
  const ep_lock_idx_t row = getLockIdx(track, param);
  if (row < 0 || row >= MAX_LOCK_ROWS)
    return 255;
  return (uint8_t)lock_row((uint16_t)row)[step];
}

void MDPattern::cleanupLocks() {
  for (ep_lock_idx_t row = 0; row < MAX_LOCK_ROWS; ++row) {
    const int16_t track = lock_track((uint16_t)row);
    const int16_t param = lock_param((uint16_t)row);
    if (track < 0 || track >= 16 || param < 0 ||
        param >= SPS_PARAMS_PER_TRACK || md_lock_row_empty(*this, row)) {
      clearLockPattern(row);
    }
  }
}
#endif
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
#if !defined(__AVR__)
bool MDPattern::fromSysex41(const SysexView &sysex) {
  uint8_t steps = 0;
  uint16_t used_words = 0;
  if (!md_compact_header(sysex, steps, used_words)) {
    return false;
  }

  ElektronSysexDecoder decoder(sysex, 9);
  decoder.skip(MD_COMPACT_HEADER_BYTES);
  uint8_t data[MD_COMPACT_SETTINGS_BYTES];
  decoder.get(data, sizeof(data));
  if (!md_compact_settings_valid(data, steps)) {
    return false;
  }

  init();
  version = MD_PATTERN_VERSION_SPSX;
  origPosition = sysex.getByte(8);
  accentPattern = md_compact_read64(data);
  swingPattern = md_compact_read64(data + 8);
  slidePattern = md_compact_read64(data + 16);
  patternTempo = md_compact_read16(data + 24);
  uint8_t accent = data[26] > 15 ? 15 : data[26];
  accentAmount = (accent >> 1) | (accent << 3);
  int8_t swing = (int8_t)data[27];
  if (swing < 0) {
    swing = 0;
  } else if (swing > 30) {
    swing = 30;
  }
  swingAmount = (uint32_t)swing * 16384u / 30u;
  accentEditAll = data[28];
  slideEditAll = data[29];
  swingEditAll = data[30];
  ext_scale_mode = data[31];
  patternLength = data[32];
  scale = data[33];
  doubleTempo = data[34];
  ext_tempo_multiplier = data[35];
  kit = data[36];
  ext_chain_row = data[37];
  ext_chain_loops = data[38];
  chain_change = data[39];
  isExtraPattern = steps == 64;

  MDCompactShape shape;
  memset(&shape, 0, sizeof(shape));
  uint16_t lane_total = 0;
  uint16_t row_count = 0;
  uint16_t cursor = 0;
  for (uint8_t track = 0; track < MD_COMPACT_TRACKS; track++) {
    decoder.get(data, sizeof(data));
    uint8_t length = data[32];
    uint8_t speed = data[33];
    uint8_t lanes = data[34];
    uint8_t microtiming = data[35];
    uint8_t conditions = data[36];
    if (!md_compact_wire_mask_fits(data, steps) ||
        !md_compact_wire_mask_fits(data + 8, steps) ||
        !md_compact_wire_mask_fits(data + 16, steps) ||
        !md_compact_wire_mask_fits(data + 24, steps) ||
        length > steps || (speed != 0xff && speed > 7) ||
        lanes > MD_PATTERN_LOCK_SLOTS ||
        data[37] != 0 || data[38] != 0 || data[39] != 0) {
      goto decode_failed;
    }
    lane_total += lanes;
    if (lane_total > MAX_LOCK_ROWS) {
      goto decode_failed;
    }
    shape.lanes[track] = lanes;
    shape.microtiming[track] = microtiming;
    shape.conditions[track] = conditions;
    trigPatterns[track] = md_compact_read64(data);
    slidePatterns[track] = md_compact_read64(data + 8);
    accentPatterns[track] = md_compact_read64(data + 16);
    swingPatterns[track] = md_compact_read64(data + 24);
    ext_track_lengths[track] =
        length == 0 || length == patternLength ? 0 : length;
    ext_track_speeds[track] =
        speed == 0xff || speed == doubleTempo ? 0xff : speed;
  }

  for (uint8_t track = 0; track < MD_COMPACT_TRACKS; track++) {
    uint8_t previous_param = 0;
    bool have_param = false;
    uint8_t mapping = 0;
    for (uint8_t lane = 0; lane < shape.lanes[track]; lane++) {
      if (cursor >= used_words) {
        goto decode_failed;
      }
      uint16_t descriptor = decoder.gget16();
      cursor++;
      uint8_t param = descriptor & 0x3f;
      uint8_t points = ((descriptor >> 6) & 0x3f) + 1;
      bool dense = (descriptor & 0x8000) != 0;
      uint8_t payload = dense ? steps / 2 : points;
      if ((descriptor & 0x7000) != 0 ||
          param >= MD_PATTERN_LOCK_SLOTS ||
          dense != (points > steps / 2) ||
          (have_param && param <= previous_param) ||
          payload > used_words - cursor) {
        goto decode_failed;
      }
      previous_param = param;
      have_param = true;
      uint16_t row = row_count++;
      paramLocks[track][param] = row;
      set_lock_track_param(row, track, param);
      SET_BIT64(lockPatterns[track], param);
      if (mapping < MD_PATTERN_LOCK_SLOTS) {
        ext_locks_params[track][mapping++] = param + 1;
      }
      int8_t *values = md_compact_lock_row(*this, row);
      memset(values, -1, 64);
      if (dense) {
        uint8_t populated = 0;
        for (uint8_t step = 0; step < steps; step += 2) {
          uint16_t word = decoder.gget16();
          uint8_t high = word >> 8;
          uint8_t low = word;
          if (high != 0xff) {
            if (high > 0x7f) {
              goto decode_failed;
            }
            values[step] = high;
            populated++;
          }
          if (low != 0xff) {
            if (low > 0x7f) {
              goto decode_failed;
            }
            values[step + 1] = low;
            populated++;
          }
        }
        if (populated != points) {
          goto decode_failed;
        }
        cursor += payload;
      } else {
        uint8_t previous_step = 0;
        bool have_step = false;
        for (uint8_t n = 0; n < points; n++) {
          uint16_t word = decoder.gget16();
          uint8_t step = word & 0x3f;
          if ((word & 0xe000) != 0 || step >= steps ||
              (have_step && step <= previous_step)) {
            goto decode_failed;
          }
          previous_step = step;
          have_step = true;
          values[step] = (word >> 6) & 0x7f;
          cursor++;
        }
      }
    }

    for (uint8_t step = 0; step < steps; step++) {
      if (md_compact_step_has_lock(*this, track, step)) {
        ext_step_flags[track][step] = 1;
      }
    }
    if ((uint16_t)(shape.microtiming[track] +
                   shape.conditions[track]) >
        used_words - cursor) {
      goto decode_failed;
    }
    uint8_t previous_step = 0;
    bool have_step = false;
    for (uint8_t n = 0; n < shape.microtiming[track]; n++) {
      uint16_t word = decoder.gget16();
      uint8_t step = word & 0x3f;
      int8_t value = (int8_t)((word >> 6) & 0xff);
      if ((word & 0xc000) != 0 || value == 0 || step >= steps ||
          (have_step && step <= previous_step)) {
        goto decode_failed;
      }
      previous_step = step;
      have_step = true;
      ext_microtiming[track][step] = value;
      cursor++;
    }
    previous_step = 0;
    have_step = false;
    for (uint8_t n = 0; n < shape.conditions[track]; n++) {
      uint16_t word = decoder.gget16();
      uint8_t step = word & 0x3f;
      uint8_t condition = (word >> 6) & 0x3f;
      if ((word & 0xc000) != 0 ||
          (condition == 0 && (word & 0x3000) == 0) ||
          step >= steps || (have_step && step <= previous_step)) {
        goto decode_failed;
      }
      previous_step = step;
      have_step = true;
      uint8_t flags = condition << 2;
      if (word & 0x1000) {
        flags |= 2;
      }
      if ((word & 0x2000) == 0 &&
          md_compact_step_has_lock(*this, track, step)) {
        flags |= 1;
      }
      ext_step_flags[track][step] = flags;
      cursor++;
    }
  }
  if (cursor != used_words) {
    goto decode_failed;
  }
  numRows = row_count;
  numLockedRows = row_count > 255 ? 255 : row_count;
  return true;

decode_failed:
  init();
  return false;
}

uint16_t MDPattern::toSysex41(
    ElektronDataToSysexEncoder *encoder) {
  uint8_t steps = patternLength > 32 ? 64 : 32;
  MDCompactShape shape;
  if (encoder == nullptr ||
      !md_compact_build_shape(*this, steps, shape)) {
    return 0;
  }

  uint16_t body_bytes =
      MD_COMPACT_FIXED_BYTES + shape.words * 2;
  uint16_t wire_bytes =
      body_bytes + (body_bytes + 6) / 7;
  uint16_t full_bytes = 15 + wire_bytes;
  if (full_bytes - 10 > 0x3fff) {
    return 0;
  }

  ElektronHelper::beginSysexEncode(
      encoder, machinedrum_sysex_hdr,
      sizeof(machinedrum_sysex_hdr),
      MD_PATTERN_MESSAGE_ID, MD_PATTERN_VERSION_SPSX,
      origPosition);
  encoder->start7Bit();
  encoder->pack(md_compact_magic, sizeof(md_compact_magic));
  md_compact_pack8(encoder, MD_PATTERN_VERSION_SPSX);
  md_compact_pack8(encoder, steps);
  md_compact_pack8(encoder, MD_COMPACT_TRACKS);
  encoder->pack16(shape.words);
  encoder->pack16(
      MD_COMPACT_SETTINGS_BYTES +
      MD_COMPACT_TRACKS * MD_COMPACT_TRACK_BYTES);
  encoder->pack16(0);
  md_compact_pack_settings(encoder, *this);
  for (uint8_t track = 0; track < MD_COMPACT_TRACKS; track++) {
    md_compact_pack_track(encoder, *this, shape, track);
  }
  md_compact_pack_words(encoder, *this, shape, steps);
  uint16_t encoded = ElektronHelper::finishSysexEncode(encoder);
  return encoded == full_bytes ? encoded : 0;
}
#endif

bool MDPattern::fromSysex(MidiClass *midi) {

  SysexView sysex(midi->midiSysex);
#if !defined(__AVR__)
  if (sysex.get_recordLen() >= 7 &&
      sysex.getByte(6) == MD_PATTERN_VERSION_SPSX) {
    return fromSysex41(sysex);
  }
#endif
  init();
#if defined(__AVR__)
  uint16_t len = sysex.get_recordLen() - 5;
#else
  const uint16_t record_len = sysex.get_recordLen();
  if (record_len < 9) {
    return false;
  }
  uint16_t len = record_len - 5;
#endif
  uint16_t offset = 5;

#if !defined(__AVR__)
  version = sysex.getByte(6);
  bool is_spsx_pat =
      version == MD_PATTERN_VERSION_SPSX_V1;

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

  // accentPattern, slidePattern, swingPattern are 3 consecutive uint64_t
  // members in wire order; coalesce the per-field get32(uint64_t*) calls into
  // one array read (byte-identical 7-bit stream). swingAmount is uint32_t so
  // it stays a separate overload.
  decoder.get32(&accentPattern, 3);

  decoder.get32(&swingAmount);

  decoder.stop7Bit();

  // accentAmount, patternLength, doubleTempo, scale, kit, numLockedRows are
  // 6 consecutive uint8_t members (no padding); coalesce the 6 gget8() reads
  // into one bulk get() over the contiguous run (identical byte stream).
  decoder.get(&accentAmount, 6);
  if (patternLength == 0 || patternLength > maxSteps) {
    return false;
  }

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
  // accentEditAll, slideEditAll, swingEditAll are 3 consecutive uint32_t
  // members; accentPatterns/slidePatterns/swingPatterns are 3 consecutive
  // uint64_t[16] arrays. Both runs are contiguous and in wire order, so
  // coalescing the per-field get32 calls is byte-identical.
  decoder.get32(&accentEditAll, 3);
  decoder.get32(accentPatterns, 48);

  // Build lock tracking for V3 (params 0-23 only)
  // SPSX defers until extension is decoded so high bits are available
#if !defined(__AVR__)
  if (!is_spsx_pat) {
#endif
    numRows = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 24; j++) {
        if (IS_BIT_SET32(lockPatterns[i], j)) {
          if (numRows < 64) {
            paramLocks[i][j] = numRows;
            lockTracks[numRows] = i;
            lockParams[numRows] = j;
            numRows++;
          } else {
            paramLocks[i][j] = -1;
          }
        }
      }
    }
#if !defined(__AVR__)
  }
#endif

  if (isExtraPattern) {
    decoder.start7Bit();
    decoder.get32hi(trigPatterns, 16);
    // accentPattern, slidePattern, swingPattern are 3 consecutive uint64_t
    // members; coalesce their get32hi reads (byte-identical 7-bit stream).
    decoder.get32hi(&accentPattern, 3);

    for (uint8_t i = 0; i < 64; i++) {
      decoder.get(locks[i] + 32, 32);
    }
    // accentPatterns/slidePatterns/swingPatterns are 3 contiguous uint64_t[16]
    // arrays in wire order; coalesce into one get32hi over the 48-elem run.
    decoder.get32hi(accentPatterns, 48);
  }

#if !defined(__AVR__)
  if (is_spsx_pat) {
    decoder.start7Bit();
    decoder.startRLE();

    const uint8_t lock_slot_count =
        mdPatternLockSlotCountForVersion(version);
    for (uint8_t t = 0; t < 16; t++) {
      decoder.get((uint8_t*)ext_microtiming[t], 64);
      decoder.get(ext_step_flags[t], 64);
      decoder.get(ext_locks_params[t], lock_slot_count);
      for (uint8_t slot = 0; slot < lock_slot_count; ++slot) {
        if (ext_locks_params[t][slot] > lock_slot_count) {
          ext_locks_params[t][slot] = 0;
        }
      }
    }

    decoder.get(ext_track_lengths, 16);
    decoder.get(ext_track_speeds, 16);

    // High 32 bits of lockPatterns (params 24-36)
    decoder.get32hi(lockPatterns, 16);
    if (version == MD_PATTERN_VERSION_SPSX_V1) {
      const uint64_t v1_lock_mask =
          (uint64_t(1) << MD_PATTERN_LOCK_SLOTS_V1) - 1;
      for (uint8_t t = 0; t < 16; ++t) {
        lockPatterns[t] &= v1_lock_mask;
      }
    }

    // Expanded lock rows. Wire format mirrors the host (src/host/Midi/MDPattern.cpp):
    // numRows is a little-endian u16, then for rows 64..numRows-1 the encoder
    // emits 32 bytes of low-step locks per row, followed (for 64-step / extra
    // patterns) by 32 bytes of high-step locks per row.
    uint8_t lo = decoder.gget8();
    uint8_t hi = decoder.gget8();
    uint16_t totalRows = (uint16_t)(lo | (hi << 8));
    const uint16_t version_max_rows =
        16u * mdPatternLockSlotCountForVersion(version);
    if (totalRows > version_max_rows) {
      return false;
    }

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

    // Rebuild lock tracking with the negotiated wire format's full range.
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
#if !defined(__AVR__)
  if (MD.supportsCompactPattern41()) {
    cleanupLocks();
    recalculateLockPatterns();
    return toSysex41(encoder);
  }
#endif
#if defined(MDPATTERN_TOSYSEX_ENABLE) || !defined(__AVR__)
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
  uint8_t ver = use_spsx
                    ? MD_PATTERN_VERSION_SPSX_V1
                    : 0x03;
  uint8_t paramLimit = use_spsx ? MD.spsxWireParamCount() : 24;
  const uint64_t wire_param_mask =
      (uint64_t(1) << static_cast<unsigned>(paramLimit)) - 1;
  uint64_t wire_lock_patterns[16];
  for (uint8_t track = 0; track < 16; ++track) {
    wire_lock_patterns[track] = lockPatterns[track] & wire_param_mask;
  }
#else
  uint8_t ver = 0x03;
  uint8_t paramLimit = 24;
#endif
  uint16_t wire_num_rows = 0;
  for (uint8_t track = 0; track < 16; ++track) {
    for (uint8_t param = 0; param < paramLimit; ++param) {
      if (paramLocks[track][param] != -1) {
        ++wire_num_rows;
      }
    }
  }

  ElektronHelper::beginSysexEncode(encoder, machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr), MD_PATTERN_MESSAGE_ID, ver, origPosition);

  encoder->start7Bit();
  encoder->pack32(trigPatterns, 16);
  encoder->reset();
#if !defined(__AVR__)
  encoder->pack32(wire_lock_patterns, 16);
#else
  encoder->pack32(lockPatterns, 16);
#endif
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
  encoder->pack8((wire_num_rows < 255) ? (uint8_t)wire_num_rows : 255);

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
      const uint8_t wire_slot_count = MD_PATTERN_LOCK_SLOTS_V1;
      uint8_t wire_lock_params[MD_PATTERN_LOCK_SLOTS];
      memset(wire_lock_params, 0, sizeof(wire_lock_params));
      for (uint8_t slot = 0; slot < wire_slot_count; ++slot) {
        const uint8_t param = ext_locks_params[t][slot];
        wire_lock_params[slot] = param <= wire_slot_count ? param : 0;
      }
      encoder->pack(wire_lock_params, wire_slot_count);
    }

    encoder->pack(ext_track_lengths, 16);
    encoder->pack(ext_track_speeds, 16);

#if !defined(__AVR__)
    encoder->pack32hi(wire_lock_patterns, 16);
#else
    encoder->pack32hi(lockPatterns, 16);
#endif

    uint16_t out_numRows =
        (wire_num_rows < MAX_LOCK_ROWS) ? wire_num_rows : MAX_LOCK_ROWS;
    // numRows (little-endian u16)
    encoder->pack8(out_numRows & 0xFF);
    encoder->pack8((out_numRows >> 8) & 0xFF);

    // Extra lock rows (64..numRows-1). Symmetric with fromSysex above:
    // first 32 bytes per row (low steps), then if isExtraPattern another
    // 32 bytes per row (high steps).
    if (out_numRows > 64) {
      uint16_t wire_row = 0;
      for (uint8_t track = 0; track < 16; ++track) {
        for (uint8_t param = 0; param < paramLimit; ++param) {
          const int16_t row = paramLocks[track][param];
          if (row == -1) {
            continue;
          }
          if (wire_row >= 64 && wire_row < out_numRows) {
            encoder->pack((const uint8_t*)lock_row((uint16_t)row), 32);
          }
          ++wire_row;
        }
      }
      if (isExtraPattern) {
        wire_row = 0;
        for (uint8_t track = 0; track < 16; ++track) {
          for (uint8_t param = 0; param < paramLimit; ++param) {
            const int16_t row = paramLocks[track][param];
            if (row == -1) {
              continue;
            }
            if (wire_row >= 64 && wire_row < out_numRows) {
              encoder->pack(
                  (const uint8_t*)(lock_row((uint16_t)row) + 32), 32);
            }
            ++wire_row;
          }
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
