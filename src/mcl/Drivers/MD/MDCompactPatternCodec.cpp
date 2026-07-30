#include "MDCompactPatternCodec.h"

#if !defined(__AVR__)

#include "Elektron.h"
#include "ElektronDataEncoder.h"
#include "MDMessages.h"
#include "MDPattern.h"
#include "MidiSysex.h"

#include <string.h>

namespace {

constexpr uint8_t kSchema = 0x41;
constexpr uint8_t kSubversion = 0x01;
constexpr uint8_t kSteps = 64;
constexpr uint8_t kTracks = 16;
constexpr uint8_t kParameters = 37;
constexpr uint16_t kWireHeaderBytes = 16;
constexpr uint16_t kSettingsBytes = 40;
constexpr uint16_t kTrackBytes = 40;
constexpr uint16_t kBodyHeaderBytes =
    kSettingsBytes + kTracks * kTrackBytes;
constexpr uint16_t kFixedBytes =
    kWireHeaderBytes + kBodyHeaderBytes;
constexpr uint8_t kMagic[7]{
    'S', 'P', 'S', 'X', 'P', 'T', 'N'};

struct TrackShape {
  uint8_t lanes;
  uint8_t microtiming;
  uint8_t conditions;
};

struct PatternShape {
  TrackShape tracks[kTracks];
  uint16_t used_words;
};

uint16_t read_be16(const uint8_t* bytes) {
  return static_cast<uint16_t>(
      (static_cast<uint16_t>(bytes[0]) << 8) | bytes[1]);
}

uint64_t read_be64(const uint8_t* bytes) {
  uint64_t value = 0;
  for (uint8_t index = 0; index < 8; ++index)
    value = (value << 8) | bytes[index];
  return value;
}

class BodyReader {
public:
  BodyReader(const SysexView& sysex, uint16_t wire_bytes)
      : sysex_(sysex), wire_bytes_(wire_bytes) {}

  bool rewind() {
    wire_cursor_ = 0;
    decoded_ = 0;
    group_remaining_ = 0;
    group_bit_ = 0;
    group_msb_ = 0;
    return true;
  }

  bool byte(uint8_t* value) {
    if (value == nullptr)
      return false;
    if (group_remaining_ == 0) {
      if (wire_cursor_ >= wire_bytes_)
        return false;
      group_msb_ = sysex_.getByte(
          static_cast<uint16_t>(9 + wire_cursor_++));
      if ((group_msb_ & 0x80) != 0)
        return false;
      const uint16_t remaining =
          static_cast<uint16_t>(wire_bytes_ - wire_cursor_);
      group_remaining_ =
          remaining < 7 ? static_cast<uint8_t>(remaining) : 7;
      group_bit_ = 0;
      if (group_remaining_ == 0)
        return false;
    }
    if (wire_cursor_ >= wire_bytes_)
      return false;
    const uint8_t low = sysex_.getByte(
        static_cast<uint16_t>(9 + wire_cursor_++));
    if ((low & 0x80) != 0)
      return false;
    *value = static_cast<uint8_t>(
        low |
        (((group_msb_ >> (6 - group_bit_)) & 1u) << 7));
    ++group_bit_;
    --group_remaining_;
    ++decoded_;
    return true;
  }

  bool bytes(uint8_t* values, uint16_t count) {
    if (values == nullptr)
      return false;
    for (uint16_t index = 0; index < count; ++index) {
      if (!byte(values + index))
        return false;
    }
    return true;
  }

  bool be16(uint16_t* value) {
    uint8_t bytes[2];
    if (value == nullptr || !this->bytes(bytes, sizeof(bytes)))
      return false;
    *value = read_be16(bytes);
    return true;
  }

  uint16_t decoded() const { return decoded_; }
  uint16_t consumed_wire() const { return wire_cursor_; }

private:
  const SysexView& sysex_;
  uint16_t wire_bytes_;
  uint16_t wire_cursor_ = 0;
  uint16_t decoded_ = 0;
  uint8_t group_remaining_ = 0;
  uint8_t group_bit_ = 0;
  uint8_t group_msb_ = 0;
};

class BodyWriter {
public:
  explicit BodyWriter(ElektronDataToSysexEncoder* encoder)
      : encoder_(encoder) {}

  bool byte(uint8_t value) {
    if (encoder_ == nullptr)
      return false;
    encoder_->pack8(value);
    ++written_;
    return true;
  }

  bool bytes(const uint8_t* values, uint16_t count) {
    if (values == nullptr)
      return false;
    for (uint16_t index = 0; index < count; ++index) {
      if (!byte(values[index]))
        return false;
    }
    return true;
  }

  bool be16(uint16_t value) {
    return byte(static_cast<uint8_t>(value >> 8)) &&
           byte(static_cast<uint8_t>(value));
  }

  bool be64(uint64_t value) {
    for (uint8_t index = 0; index < 8; ++index) {
      if (!byte(static_cast<uint8_t>(
              value >> ((7 - index) * 8)))) {
        return false;
      }
    }
    return true;
  }

  uint16_t written() const { return written_; }

private:
  ElektronDataToSysexEncoder* encoder_;
  uint16_t written_ = 0;
};

bool mask_fits(uint64_t mask, uint8_t steps) {
  return steps == 64 || (steps == 32 && (mask >> 32) == 0);
}

bool valid_settings(const uint8_t* bytes, uint8_t steps) {
  const uint64_t accent = read_be64(bytes);
  const uint64_t swing = read_be64(bytes + 8);
  const uint64_t slide = read_be64(bytes + 16);
  const uint8_t accent_edit_all = bytes[28];
  const uint8_t slide_edit_all = bytes[29];
  const uint8_t swing_edit_all = bytes[30];
  const uint8_t scale_mode = bytes[31];
  const uint8_t length = bytes[32];
  const uint8_t scale = bytes[33];
  const uint8_t speed = bytes[34];
  const uint8_t multiplier = bytes[35];
  const uint8_t kit = bytes[36];
  return length <= steps && scale <= 3 && speed <= 7 &&
         multiplier <= 1 && (kit == 0xff || kit < 64) &&
         scale_mode <= 1 && accent_edit_all <= 1 &&
         slide_edit_all <= 1 && swing_edit_all <= 1 &&
         mask_fits(accent, steps) &&
         mask_fits(swing, steps) &&
         mask_fits(slide, steps);
}

bool valid_track(const uint8_t* bytes, uint8_t steps,
                 TrackShape* shape) {
  if (shape == nullptr ||
      !mask_fits(read_be64(bytes), steps) ||
      !mask_fits(read_be64(bytes + 8), steps) ||
      !mask_fits(read_be64(bytes + 16), steps) ||
      !mask_fits(read_be64(bytes + 24), steps) ||
      bytes[32] > steps ||
      (bytes[33] != 0xff && bytes[33] > 7) ||
      bytes[34] > kParameters ||
      bytes[37] != 0 || bytes[38] != 0 || bytes[39] != 0) {
    return false;
  }
  shape->lanes = bytes[34];
  shape->microtiming = bytes[35];
  shape->conditions = bytes[36];
  return true;
}

bool lane_canonical(uint16_t descriptor) {
  return (descriptor & 0x7000u) == 0;
}

uint8_t lane_parameter(uint16_t descriptor) {
  return static_cast<uint8_t>(descriptor & 0x3fu);
}

uint8_t lane_points(uint16_t descriptor) {
  return static_cast<uint8_t>(((descriptor >> 6) & 0x3fu) + 1);
}

bool lane_dense(uint16_t descriptor) {
  return (descriptor & 0x8000u) != 0;
}

uint8_t point_step(uint16_t word) {
  return static_cast<uint8_t>(word & 0x3fu);
}

bool validate_words(BodyReader& input,
                    const PatternShape& shape,
                    uint8_t steps) {
  uint16_t cursor = 0;
  for (uint8_t track = 0; track < kTracks; ++track) {
    uint8_t previous_parameter = 0;
    bool have_parameter = false;
    for (uint8_t lane = 0;
         lane < shape.tracks[track].lanes; ++lane) {
      uint16_t descriptor = 0;
      if (cursor >= shape.used_words ||
          !input.be16(&descriptor)) {
        return false;
      }
      ++cursor;
      const uint8_t parameter = lane_parameter(descriptor);
      const uint8_t points = lane_points(descriptor);
      const bool dense = lane_dense(descriptor);
      const uint8_t payload_words =
          dense ? static_cast<uint8_t>(steps / 2) : points;
      if (!lane_canonical(descriptor) ||
          parameter >= kParameters || points > steps ||
          dense != (points > steps / 2) ||
          (have_parameter && parameter <= previous_parameter) ||
          static_cast<uint32_t>(cursor) + payload_words >
              shape.used_words) {
        return false;
      }
      previous_parameter = parameter;
      have_parameter = true;

      if (dense) {
        uint8_t populated = 0;
        for (uint8_t word = 0; word < payload_words; ++word) {
          uint16_t encoded = 0;
          if (!input.be16(&encoded))
            return false;
          const uint8_t high = static_cast<uint8_t>(encoded >> 8);
          const uint8_t low = static_cast<uint8_t>(encoded);
          if (high != 0xff) {
            if (high > 0x7f) return false;
            ++populated;
          }
          if (low != 0xff) {
            if (low > 0x7f) return false;
            ++populated;
          }
        }
        if (populated != points)
          return false;
        cursor = static_cast<uint16_t>(cursor + payload_words);
      } else {
        uint8_t previous_step = 0;
        bool have_step = false;
        for (uint8_t point = 0; point < points; ++point) {
          uint16_t encoded = 0;
          if (!input.be16(&encoded))
            return false;
          const uint8_t step = point_step(encoded);
          if ((encoded & 0xe000u) != 0 || step >= steps ||
              (have_step && step <= previous_step)) {
            return false;
          }
          previous_step = step;
          have_step = true;
          ++cursor;
        }
      }
    }

    uint8_t previous_step = 0;
    bool have_step = false;
    for (uint8_t index = 0;
         index < shape.tracks[track].microtiming; ++index) {
      uint16_t encoded = 0;
      if (cursor >= shape.used_words ||
          !input.be16(&encoded)) {
        return false;
      }
      const uint8_t step = point_step(encoded);
      const int8_t value =
          static_cast<int8_t>((encoded >> 6) & 0xffu);
      if ((encoded & 0xc000u) != 0 || value == 0 ||
          step >= steps ||
          (have_step && step <= previous_step)) {
        return false;
      }
      previous_step = step;
      have_step = true;
      ++cursor;
    }

    previous_step = 0;
    have_step = false;
    for (uint8_t index = 0;
         index < shape.tracks[track].conditions; ++index) {
      uint16_t encoded = 0;
      if (cursor >= shape.used_words ||
          !input.be16(&encoded)) {
        return false;
      }
      const uint8_t step = point_step(encoded);
      const uint8_t condition =
          static_cast<uint8_t>((encoded >> 6) & 0x3fu);
      if ((encoded & 0xc000u) != 0 || step >= steps ||
          (condition == 0 &&
           (encoded & 0x3000u) == 0) ||
          (have_step && step <= previous_step)) {
        return false;
      }
      previous_step = step;
      have_step = true;
      ++cursor;
    }
  }
  return cursor == shape.used_words;
}

bool read_and_validate(BodyReader& input,
                       uint16_t decoded_bytes,
                       PatternShape* shape,
                       uint8_t* steps_out) {
  if (shape == nullptr || steps_out == nullptr)
    return false;
  input.rewind();
  uint8_t header[kWireHeaderBytes];
  if (!input.bytes(header, sizeof(header)))
    return false;
  for (uint8_t index = 0; index < sizeof(kMagic); ++index) {
    if (header[index] != kMagic[index])
      return false;
  }
  const uint8_t steps = header[8];
  const uint16_t used_words = read_be16(header + 10);
  const uint32_t required =
      static_cast<uint32_t>(kFixedBytes) +
      static_cast<uint32_t>(used_words) * 2u;
  if (header[7] != kSchema ||
      (steps != 32 && steps != 64) ||
      header[9] != kTracks ||
      read_be16(header + 12) != kBodyHeaderBytes ||
      header[14] != 0 || header[15] != 0 ||
      required != decoded_bytes) {
    return false;
  }

  uint8_t settings[kSettingsBytes];
  if (!input.bytes(settings, sizeof(settings)) ||
      !valid_settings(settings, steps)) {
    return false;
  }
  PatternShape parsed{};
  parsed.used_words = used_words;
  uint8_t track_bytes[kTrackBytes];
  uint16_t minimum_words = 0;
  for (uint8_t track = 0; track < kTracks; ++track) {
    if (!input.bytes(track_bytes, sizeof(track_bytes)) ||
        !valid_track(track_bytes, steps,
                     &parsed.tracks[track])) {
      return false;
    }
    minimum_words = static_cast<uint16_t>(
        minimum_words + parsed.tracks[track].lanes +
        parsed.tracks[track].microtiming +
        parsed.tracks[track].conditions);
  }
  if (minimum_words > used_words ||
      !validate_words(input, parsed, steps) ||
      input.decoded() != decoded_bytes ||
      input.consumed_wire() == 0) {
    return false;
  }
  *shape = parsed;
  *steps_out = steps;
  return true;
}

uint8_t lock_count(const MDPattern& pattern,
                   uint8_t track, uint8_t parameter,
                   uint8_t steps) {
  const int16_t row = pattern.paramLocks[track][parameter];
  if (row < 0 || row >= MAX_LOCK_ROWS)
    return 0;
  const int8_t* values =
      row < 64 ? reinterpret_cast<const int8_t*>(
                     pattern.locks[row])
               : pattern.ext_locks[row - 64];
  uint8_t count = 0;
  for (uint8_t step = 0; step < steps; ++step) {
    if (values[step] >= 0)
      ++count;
  }
  return count;
}

bool step_has_lock(const MDPattern& pattern,
                   uint8_t track, uint8_t step) {
  for (uint8_t parameter = 0;
       parameter < kParameters; ++parameter) {
    const int16_t row = pattern.paramLocks[track][parameter];
    if (row >= 0 && row < MAX_LOCK_ROWS &&
        pattern.lock_row(static_cast<uint16_t>(row))[step] >= 0) {
      return true;
    }
  }
  return false;
}

bool build_shape(const MDPattern& pattern, PatternShape* shape) {
  if (shape == nullptr ||
      (pattern.patternLength != 32 &&
       pattern.patternLength != 64 &&
       (pattern.patternLength == 0 ||
        pattern.patternLength > 64))) {
    return false;
  }
  PatternShape result{};
  uint32_t words = 0;
  const uint8_t steps =
      pattern.patternLength > 32 ? 64 : 32;
  for (uint8_t track = 0; track < kTracks; ++track) {
    TrackShape& track_shape = result.tracks[track];
    for (uint8_t parameter = 0;
         parameter < kParameters; ++parameter) {
      const uint8_t points =
          lock_count(pattern, track, parameter, steps);
      if (points == 0)
        continue;
      ++track_shape.lanes;
      words += 1u + (points > steps / 2
                         ? steps / 2
                         : points);
    }
    for (uint8_t step = 0; step < steps; ++step) {
      if (pattern.ext_microtiming[track][step] != 0)
        ++track_shape.microtiming;
      const uint8_t flags =
          pattern.ext_step_flags[track][step];
      const bool disabled =
          step_has_lock(pattern, track, step) &&
          (flags & 1u) == 0;
      if ((flags >> 2) != 0 ||
          (flags & 2u) != 0 || disabled) {
        ++track_shape.conditions;
      }
    }
    words += track_shape.microtiming +
             track_shape.conditions;
  }
  if (words > 0xffffu)
    return false;
  result.used_words = static_cast<uint16_t>(words);
  *shape = result;
  return true;
}

bool write_settings(BodyWriter& output,
                    const MDPattern& pattern) {
  int swing = static_cast<int>(
      (static_cast<uint64_t>(pattern.swingAmount) * 30u +
       8192u) /
      16384u);
  if (swing < 0) swing = 0;
  if (swing > 30) swing = 30;
  return output.be64(pattern.accentPattern) &&
         output.be64(pattern.swingPattern) &&
         output.be64(pattern.slidePattern) &&
         output.be16(pattern.patternTempo) &&
         output.byte(static_cast<uint8_t>(
             pattern.accentAmount >> 3)) &&
         output.byte(static_cast<uint8_t>(swing)) &&
         output.byte(pattern.accentEditAll ? 1 : 0) &&
         output.byte(pattern.slideEditAll ? 1 : 0) &&
         output.byte(pattern.swingEditAll ? 1 : 0) &&
         output.byte(pattern.ext_scale_mode) &&
         output.byte(pattern.patternLength) &&
         output.byte(pattern.scale) &&
         output.byte(pattern.doubleTempo) &&
         output.byte(pattern.ext_tempo_multiplier) &&
         output.byte(pattern.kit) &&
         output.byte(pattern.ext_chain_row) &&
         output.byte(pattern.ext_chain_loops) &&
         output.byte(pattern.chain_change);
}

bool write_track(BodyWriter& output,
                 const MDPattern& pattern,
                 const PatternShape& shape,
                 uint8_t track) {
  const uint64_t slide = pattern.slideEditAll
      ? pattern.slidePattern : pattern.slidePatterns[track];
  const uint64_t accent = pattern.accentEditAll
      ? pattern.accentPattern : pattern.accentPatterns[track];
  const uint64_t swing = pattern.swingEditAll
      ? pattern.swingPattern : pattern.swingPatterns[track];
  const uint8_t length =
      pattern.ext_track_lengths[track] != 0
          ? pattern.ext_track_lengths[track]
          : pattern.patternLength;
  const uint8_t speed =
      pattern.ext_track_speeds[track] != 0xff
          ? pattern.ext_track_speeds[track]
          : pattern.doubleTempo;
  return output.be64(pattern.trigPatterns[track]) &&
         output.be64(slide) &&
         output.be64(accent) &&
         output.be64(swing) &&
         output.byte(length) && output.byte(speed) &&
         output.byte(shape.tracks[track].lanes) &&
         output.byte(shape.tracks[track].microtiming) &&
         output.byte(shape.tracks[track].conditions) &&
         output.byte(0) && output.byte(0) && output.byte(0);
}

bool write_words(BodyWriter& output,
                 const MDPattern& pattern,
                 const PatternShape& shape,
                 uint8_t steps) {
  uint16_t words = 0;
  for (uint8_t track = 0; track < kTracks; ++track) {
    for (uint8_t parameter = 0;
         parameter < kParameters; ++parameter) {
      const uint8_t points =
          lock_count(pattern, track, parameter, steps);
      if (points == 0)
        continue;
      const bool dense = points > steps / 2;
      const uint16_t descriptor = static_cast<uint16_t>(
          parameter |
          (static_cast<uint16_t>(points - 1) << 6) |
          (dense ? 0x8000u : 0u));
      if (!output.be16(descriptor))
        return false;
      ++words;
      const int16_t row =
          pattern.paramLocks[track][parameter];
      const int8_t* values =
          pattern.lock_row(static_cast<uint16_t>(row));
      if (dense) {
        for (uint8_t step = 0; step < steps; step += 2) {
          const uint8_t high = values[step] >= 0
              ? static_cast<uint8_t>(values[step]) : 0xff;
          const uint8_t low = values[step + 1] >= 0
              ? static_cast<uint8_t>(values[step + 1]) : 0xff;
          if (!output.be16(static_cast<uint16_t>(
                  (static_cast<uint16_t>(high) << 8) | low))) {
            return false;
          }
          ++words;
        }
      } else {
        for (uint8_t step = 0; step < steps; ++step) {
          if (values[step] < 0)
            continue;
          const uint16_t encoded = static_cast<uint16_t>(
              step |
              (static_cast<uint16_t>(
                   static_cast<uint8_t>(values[step])) << 6));
          if (!output.be16(encoded))
            return false;
          ++words;
        }
      }
    }
    for (uint8_t step = 0; step < steps; ++step) {
      const int8_t value =
          pattern.ext_microtiming[track][step];
      if (value == 0)
        continue;
      const uint16_t encoded = static_cast<uint16_t>(
          step |
          (static_cast<uint16_t>(
               static_cast<uint8_t>(value)) << 6));
      if (!output.be16(encoded))
        return false;
      ++words;
    }
    for (uint8_t step = 0; step < steps; ++step) {
      const uint8_t flags =
          pattern.ext_step_flags[track][step];
      const uint8_t condition =
          static_cast<uint8_t>(flags >> 2);
      const bool plock = (flags & 2u) != 0;
      const bool disabled =
          step_has_lock(pattern, track, step) &&
          (flags & 1u) == 0;
      if (condition == 0 && !plock && !disabled)
        continue;
      const uint16_t encoded = static_cast<uint16_t>(
          step |
          (static_cast<uint16_t>(condition) << 6) |
          (plock ? 0x1000u : 0u) |
          (disabled ? 0x2000u : 0u));
      if (!output.be16(encoded))
        return false;
      ++words;
    }
  }
  return words == shape.used_words;
}

bool apply_body(BodyReader& input,
                MDPattern& pattern,
                const PatternShape& shape,
                uint8_t steps) {
  input.rewind();
  uint8_t header[kWireHeaderBytes];
  uint8_t settings[kSettingsBytes];
  if (!input.bytes(header, sizeof(header)) ||
      !input.bytes(settings, sizeof(settings))) {
    return false;
  }

  pattern.init();
  pattern.version = kSchema;
  pattern.origPosition = 0;
  pattern.accentPattern = read_be64(settings);
  pattern.swingPattern = read_be64(settings + 8);
  pattern.slidePattern = read_be64(settings + 16);
  pattern.patternTempo = read_be16(settings + 24);
  const uint8_t accent = settings[26] > 15 ? 15 : settings[26];
  pattern.accentAmount = static_cast<uint8_t>(
      (accent >> 1) | (accent << 3));
  int swing = static_cast<int8_t>(settings[27]);
  if (swing < 0) swing = 0;
  if (swing > 30) swing = 30;
  pattern.swingAmount = static_cast<uint32_t>(
      swing * 16384 / 30);
  pattern.accentEditAll = settings[28];
  pattern.slideEditAll = settings[29];
  pattern.swingEditAll = settings[30];
  pattern.ext_scale_mode = settings[31];
  pattern.patternLength = settings[32];
  pattern.scale = settings[33];
  pattern.doubleTempo = settings[34];
  pattern.ext_tempo_multiplier = settings[35];
  pattern.kit = settings[36];
  pattern.ext_chain_row = settings[37];
  pattern.ext_chain_loops = settings[38];
  pattern.chain_change = settings[39];
  pattern.isExtraPattern = steps == 64;

  uint8_t track_bytes[kTrackBytes];
  for (uint8_t track = 0; track < kTracks; ++track) {
    if (!input.bytes(track_bytes, sizeof(track_bytes)))
      return false;
    pattern.trigPatterns[track] = read_be64(track_bytes);
    pattern.slidePatterns[track] = read_be64(track_bytes + 8);
    pattern.accentPatterns[track] = read_be64(track_bytes + 16);
    pattern.swingPatterns[track] = read_be64(track_bytes + 24);
    const uint8_t length = track_bytes[32];
    const uint8_t speed = track_bytes[33];
    pattern.ext_track_lengths[track] =
        length == pattern.patternLength ? 0 : length;
    pattern.ext_track_speeds[track] =
        speed == pattern.doubleTempo ? 0xff : speed;
  }

  pattern.numRows = 0;
  for (uint8_t track = 0; track < kTracks; ++track) {
    uint8_t mapping = 0;
    for (uint8_t lane = 0;
         lane < shape.tracks[track].lanes; ++lane) {
      uint16_t descriptor = 0;
      if (!input.be16(&descriptor) ||
          pattern.numRows >= MAX_LOCK_ROWS) {
        return false;
      }
      const uint8_t parameter = lane_parameter(descriptor);
      const uint8_t points = lane_points(descriptor);
      const bool dense = lane_dense(descriptor);
      const uint16_t row = pattern.numRows++;
      pattern.paramLocks[track][parameter] =
          static_cast<int16_t>(row);
      pattern.set_lock_track_param(
          row, track, parameter);
      SET_BIT64(pattern.lockPatterns[track], parameter);
      if (mapping < MD_PATTERN_LOCK_SLOTS)
        pattern.ext_locks_params[track][mapping++] =
            static_cast<uint8_t>(parameter + 1);
      int8_t* values = pattern.lock_row(row);
      memset(values, -1, 64);
      if (dense) {
        for (uint8_t step = 0; step < steps; step += 2) {
          uint16_t encoded = 0;
          if (!input.be16(&encoded))
            return false;
          const uint8_t high =
              static_cast<uint8_t>(encoded >> 8);
          const uint8_t low = static_cast<uint8_t>(encoded);
          if (high != 0xff)
            values[step] = static_cast<int8_t>(high);
          if (low != 0xff)
            values[step + 1] = static_cast<int8_t>(low);
        }
      } else {
        for (uint8_t point = 0; point < points; ++point) {
          uint16_t encoded = 0;
          if (!input.be16(&encoded))
            return false;
          values[point_step(encoded)] = static_cast<int8_t>(
              (encoded >> 6) & 0x7fu);
        }
      }
    }

    for (uint8_t step = 0; step < steps; ++step) {
      if (step_has_lock(pattern, track, step))
        pattern.ext_step_flags[track][step] |= 1u;
    }
    for (uint8_t index = 0;
         index < shape.tracks[track].microtiming; ++index) {
      uint16_t encoded = 0;
      if (!input.be16(&encoded))
        return false;
      pattern.ext_microtiming[track][point_step(encoded)] =
          static_cast<int8_t>((encoded >> 6) & 0xffu);
    }
    for (uint8_t index = 0;
         index < shape.tracks[track].conditions; ++index) {
      uint16_t encoded = 0;
      if (!input.be16(&encoded))
        return false;
      const uint8_t step = point_step(encoded);
      uint8_t flags = static_cast<uint8_t>(
          ((encoded >> 6) & 0x3fu) << 2);
      if ((encoded & 0x1000u) != 0)
        flags |= 2u;
      if ((encoded & 0x2000u) == 0 &&
          step_has_lock(pattern, track, step)) {
        flags |= 1u;
      }
      pattern.ext_step_flags[track][step] = flags;
    }
  }
  pattern.numLockedRows = static_cast<uint8_t>(
      pattern.numRows > 255 ? 255 : pattern.numRows);
  return input.decoded() ==
         static_cast<uint16_t>(
             kFixedBytes + shape.used_words * 2u);
}

} // namespace

bool md_decode_compact_pattern_41(MDPattern& pattern,
                                  const SysexView& sysex) {
  const uint16_t record_len = sysex.get_recordLen();
  if (record_len < 13 || sysex.getByte(5) != MD_PATTERN_MESSAGE_ID ||
      sysex.getByte(6) != kSchema ||
      sysex.getByte(7) != kSubversion ||
      !ElektronHelper::checkSysexChecksum(
          sysex, 5, static_cast<uint16_t>(record_len - 5))) {
    return false;
  }
  const uint16_t declared = ElektronHelper::to16Bit7(
      sysex.getByte(static_cast<uint16_t>(record_len - 2)),
      sysex.getByte(static_cast<uint16_t>(record_len - 1)));
  if (declared != static_cast<uint16_t>(record_len - 8))
    return false;

  const uint16_t wire_bytes =
      static_cast<uint16_t>(record_len - 13);
  BodyReader input(sysex, wire_bytes);
  uint8_t first_header[kWireHeaderBytes];
  input.rewind();
  if (!input.bytes(first_header, sizeof(first_header)))
    return false;
  const uint16_t used_words = read_be16(first_header + 10);
  const uint32_t decoded_bytes =
      static_cast<uint32_t>(kFixedBytes) +
      static_cast<uint32_t>(used_words) * 2u;
  const uint32_t expected_wire =
      decoded_bytes + (decoded_bytes + 6u) / 7u;
  if (decoded_bytes > 0xffffu ||
      expected_wire != wire_bytes) {
    return false;
  }

  PatternShape shape{};
  uint8_t steps = 0;
  if (!read_and_validate(
          input, static_cast<uint16_t>(decoded_bytes),
          &shape, &steps) ||
      !apply_body(input, pattern, shape, steps)) {
    return false;
  }
  pattern.origPosition = sysex.getByte(8);
  return true;
}

uint16_t md_encode_compact_pattern_41(
    MDPattern& pattern,
    ElektronDataToSysexEncoder* encoder) {
  PatternShape shape{};
  if (encoder == nullptr || !build_shape(pattern, &shape))
    return 0;
  const uint32_t body_bytes =
      static_cast<uint32_t>(kFixedBytes) +
      static_cast<uint32_t>(shape.used_words) * 2u;
  const uint32_t wire_bytes =
      body_bytes + (body_bytes + 6u) / 7u;
  const uint32_t full_bytes = 15u + wire_bytes;
  if (body_bytes > 0xffffu || full_bytes > 0xffffu ||
      full_bytes < 10u || full_bytes - 10u > 0x3fffu)
    return 0;

  ElektronHelper::beginSysexEncode(
      encoder, machinedrum_sysex_hdr,
      sizeof(machinedrum_sysex_hdr),
      MD_PATTERN_MESSAGE_ID, kSchema,
      pattern.origPosition);
  encoder->start7Bit();
  BodyWriter output(encoder);
  if (!output.bytes(kMagic, sizeof(kMagic)) ||
      !output.byte(kSchema) ||
      !output.byte(pattern.patternLength > 32 ? 64 : 32) ||
      !output.byte(kTracks) ||
      !output.be16(shape.used_words) ||
      !output.be16(kBodyHeaderBytes) ||
      !output.byte(0) || !output.byte(0) ||
      !write_settings(output, pattern)) {
    return 0;
  }
  for (uint8_t track = 0; track < kTracks; ++track) {
    if (!write_track(output, pattern, shape, track))
      return 0;
  }
  if (!write_words(
          output, pattern, shape,
          pattern.patternLength > 32 ? 64 : 32) ||
      output.written() != body_bytes) {
    return 0;
  }
  const uint16_t encoded =
      ElektronHelper::finishSysexEncode(encoder);
  return encoded == full_bytes ? encoded : 0;
}

#endif
