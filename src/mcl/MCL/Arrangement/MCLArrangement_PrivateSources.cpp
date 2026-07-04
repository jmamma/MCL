#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"
#include "A4Track.h"
#include "ExtTrack.h"
#include "Grid/MCLActions.h"
#include "Host/SpsHostArrBridge.h"
#include "MDSeqTrack.h"
#include "MDTrack.h"
#include "MidiSeqTrack.h"
#include "MidiTrack.h"
#include "MNMTrack.h"
#include "SPSXSeqTrack.h"
#include "SPSXTrack.h"
#ifdef PLATFORM_TBD
#include "../../Drivers/TBD/TBDTrack.h"
#endif

using namespace mcl_arrangement_internal;

namespace {

#if !defined(__AVR__) && defined(DEBUGMODE)
#define ARR_COPY_TRACE(fmt, ...) DEBUG_PRINT_FN("[arr-copy-mcl] " fmt, ##__VA_ARGS__)
#else
#define ARR_COPY_TRACE(fmt, ...) do { } while (0)
#endif

static const uint32_t kPreviewGridSourceFlag = 0x80000000UL;
static const uint32_t kPreviewGridSourceTrackMask = 0xFFUL;
static const uint32_t kPreviewGridSourceRowMask = 0xFFUL;
static const uint8_t kPreviewGridSourceTrackShift = 8;

uint8_t previewLength(uint8_t length) {
  if (length == 0 || length == 0xFF) {
    return 16;
  }
  return length > 128 ? 128 : length;
}

uint8_t previewSpeed(uint8_t speed, const GridLink &link) {
  if (speed == 0xFF) {
    return link.speed_value();
  }
  return speed;
}

uint64_t mdPreviewTrigMask(const MDSeqTrackData &seq, int8_t *trigTiming,
                           uint8_t maxTrigTiming, uint8_t ticksPerStep) {
  uint64_t mask = 0;
  for (uint8_t step = 0; step < NUM_MD_STEPS && step < 64; step++) {
    if (seq.steps[step].trig) {
      mask |= (1ULL << step);
      if (trigTiming != nullptr && step < maxTrigTiming) {
        trigTiming[step] =
            (int8_t)SeqTrack::microtiming_to_ticks(seq.microtiming[step],
                                                   ticksPerStep);
      }
    }
  }
  return mask;
}

uint64_t stepSeqPreviewTrigMask(const StepSeqTrackData &seq,
                                int8_t *trigTiming, uint8_t maxTrigTiming,
                                uint16_t ticksPerStep) {
  uint64_t mask = 0;
  for (uint8_t step = 0; step < STEPSEQ_NUM_STEPS && step < 64; step++) {
    if (seq.trig_mask & (1ULL << step)) {
      mask |= (1ULL << step);
      if (trigTiming != nullptr && step < maxTrigTiming) {
        trigTiming[step] =
            (int8_t)stepseq_microtiming_to_ticks(seq.microtiming[step],
                                                 ticksPerStep);
      }
    }
  }
  return mask;
}

#if !defined(__AVR__)
static const uint8_t kPreviewNoteFlagTruncated = 1 << 0;
static const uint8_t kPreviewNoteFlagMicrotiming = 1 << 1;

struct PreviewNoteWriter {
  MCLArrangement::PrivateSourcePreviewNote *notes = nullptr;
  uint8_t maxNotes = 0;
  uint8_t count = 0;
  uint8_t noteMin = 0;
  uint8_t noteMax = 0;
  uint8_t flags = 0;
  bool haveNotes = false;

  void add(uint8_t start, uint8_t length, uint8_t note, uint8_t velocity,
           int8_t timing = 0) {
    if (!haveNotes) {
      noteMin = note;
      noteMax = note;
      haveNotes = true;
    } else {
      if (note < noteMin) {
        noteMin = note;
      }
      if (note > noteMax) {
        noteMax = note;
      }
    }

    if (notes == nullptr || count >= maxNotes) {
      flags |= kPreviewNoteFlagTruncated;
      return;
    }

    if (length == 0) {
      length = 1;
    }
    notes[count].start = start;
    notes[count].length = length;
    notes[count].note = note & 0x7F;
    notes[count].velocity = velocity & 0x7F;
    notes[count].timing = timing;
    if (timing != 0) {
      flags |= kPreviewNoteFlagMicrotiming;
    }
    count++;
  }
};

int8_t midiPreviewTimingOffset(uint8_t timing, uint8_t ticksPerStep) {
  int16_t value =
      (int16_t)(((uint32_t)timing * ticksPerStep +
                 (MIDI_SEQ_TIMING_CENTER / 2)) /
                MIDI_SEQ_TIMING_CENTER);
  value -= ticksPerStep;
  if (value < -128) {
    value = -128;
  } else if (value > 127) {
    value = 127;
  }
  return (int8_t)value;
}

uint8_t previewNoteDuration(uint8_t start, uint8_t offStep, uint8_t length,
                            bool haveOff) {
  if (!haveOff || length == 0) {
    return 1;
  }
  uint16_t duration = 0;
  if (offStep > start) {
    duration = offStep - start;
  } else if (offStep < start) {
    duration = (uint16_t)length - start + offStep;
  }
  if (duration == 0) {
    duration = 1;
  }
  return duration > 255 ? 255 : (uint8_t)duration;
}

uint64_t midiPreviewTrigMask(const MidiSeqTrackData &seq, int8_t *trigTiming,
                             uint8_t maxTrigTiming, uint8_t ticksPerStep) {
  uint64_t mask = 0;
  uint8_t length = previewLength(seq.length);
  uint16_t eventCount = seq.used_event_count();
  for (uint8_t step = 0; step < length && step < 64; step++) {
    uint16_t start = 0;
    uint16_t end = 0;
    seq.locate(step, start, end);
    for (uint16_t idx = start; idx < end && idx < eventCount; idx++) {
      const MidiSeqEvent &event = seq.events[idx];
      if (event.type == MIDI_SEQ_EVENT_NOTE_ON) {
        mask |= (1ULL << step);
        if (trigTiming != nullptr && step < maxTrigTiming) {
          trigTiming[step] = midiPreviewTimingOffset(event.timing,
                                                     ticksPerStep);
        }
        break;
      }
    }
  }
  return mask;
}

void midiPreviewNotes(const MidiSeqTrackData &seq, uint8_t ticksPerStep,
                      PreviewNoteWriter &writer) {
  uint8_t length = previewLength(seq.length);
  if (length > MIDI_SEQ_NUM_STEPS) {
    length = MIDI_SEQ_NUM_STEPS;
  }
  uint16_t eventCount = seq.used_event_count();
  for (uint8_t step = 0; step < length; step++) {
    uint16_t start = 0;
    uint16_t end = 0;
    seq.locate(step, start, end);
    if (end > eventCount) {
      end = eventCount;
    }
    for (uint16_t idx = start; idx < end; idx++) {
      const MidiSeqEvent &event = seq.events[idx];
      if (event.type != MIDI_SEQ_EVENT_NOTE_ON) {
        continue;
      }
      uint16_t noteOffIdx = idx;
      uint8_t offStep =
          seq.search_note_off(event.target, step, noteOffIdx, end, length);
      uint8_t duration =
          previewNoteDuration(step, offStep, length, noteOffIdx != 0xFFFF);
      writer.add(step, duration, event.target, (uint8_t)event.value,
                 midiPreviewTimingOffset(event.timing, ticksPerStep));
    }
  }
}

void extPreviewLocate(ExtSeqTrackData &seq, uint8_t step, uint16_t &start,
                      uint16_t &end, uint16_t eventCount);

uint64_t extPreviewTrigMask(ExtSeqTrackData &seq, int8_t *trigTiming,
                            uint8_t maxTrigTiming, uint8_t ticksPerStep) {
  uint64_t mask = 0;
  for (uint8_t step = 0; step < NUM_EXT_STEPS && step < 64; step++) {
    uint8_t bucketCount = seq.event_buckets.get(step);
    if (bucketCount == 0) {
      continue;
    }
    mask |= (1ULL << step);
    if (trigTiming != nullptr && step < maxTrigTiming) {
      uint16_t eventCount = seq.event_count;
      if (eventCount > NUM_EXT_EVENTS) {
        eventCount = NUM_EXT_EVENTS;
      }
      uint16_t start = 0;
      uint16_t end = 0;
      extPreviewLocate(seq, step, start, end, eventCount);
      for (uint16_t idx = start; idx < end; idx++) {
        const ext_event_t &event = seq.events[idx];
        if (!event.is_lock && event.event_on) {
          trigTiming[step] =
              (int8_t)SeqTrack::microtiming_to_ticks(event.micro_timing,
                                                     ticksPerStep);
          break;
        }
      }
    }
  }
  return mask;
}

void extPreviewLocate(ExtSeqTrackData &seq, uint8_t step, uint16_t &start,
                      uint16_t &end, uint16_t eventCount) {
  start = 0;
  for (uint8_t i = 0; i < step; ++i) {
    start += seq.event_buckets.get(i);
  }
  if (start > eventCount) {
    start = eventCount;
  }
  end = start + seq.event_buckets.get(step);
  if (end > eventCount) {
    end = eventCount;
  }
}

uint8_t extPreviewSearchNoteOff(ExtSeqTrackData &seq, uint8_t note,
                                uint8_t step, uint16_t &eventIdx,
                                uint16_t eventEnd, uint8_t length,
                                uint16_t eventCount) {
  if (length == 0) {
    eventIdx = 0xFFFF;
    return step;
  }

  uint8_t curStep = step;
  eventIdx++;
  do {
    for (; eventIdx < eventEnd; eventIdx++) {
      const ext_event_t &event = seq.events[eventIdx];
      if (!event.is_lock && !event.event_on && event.event_value == note) {
        return curStep;
      }
    }

    curStep++;
    if (curStep >= length) {
      curStep = 0;
      eventEnd = 0;
    }
    eventIdx = eventEnd;
    eventEnd += seq.event_buckets.get(curStep);
    if (eventEnd > eventCount) {
      eventEnd = eventCount;
    }
  } while (curStep != step);

  eventIdx = 0xFFFF;
  return step;
}

void extPreviewNotes(ExtSeqTrackData &seq, uint8_t sourceLength,
                     uint8_t ticksPerStep,
                     PreviewNoteWriter &writer) {
  uint8_t length = previewLength(sourceLength);
  if (length > NUM_EXT_STEPS) {
    length = NUM_EXT_STEPS;
  }
  uint16_t eventCount = seq.event_count;
  if (eventCount > NUM_EXT_EVENTS) {
    eventCount = NUM_EXT_EVENTS;
  }
  for (uint8_t step = 0; step < length; step++) {
    uint16_t start = 0;
    uint16_t end = 0;
    extPreviewLocate(seq, step, start, end, eventCount);
    for (uint16_t idx = start; idx < end; idx++) {
      const ext_event_t &event = seq.events[idx];
      if (event.is_lock || !event.event_on) {
        continue;
      }
      uint16_t noteOffIdx = idx;
      uint8_t offStep = extPreviewSearchNoteOff(
          seq, event.event_value, step, noteOffIdx, end, length, eventCount);
      uint8_t duration =
          previewNoteDuration(step, offStep, length, noteOffIdx != 0xFFFF);
      writer.add(step, duration, event.event_value, seq.velocities[step],
                 (int8_t)SeqTrack::microtiming_to_ticks(event.micro_timing,
                                                        ticksPerStep));
    }
  }
}
#endif

bool copyLiveSeqToPrivateTrack(DeviceTrack *track, GridDeviceTrack *gdt,
                               uint8_t trackNumber) {
  if (track == nullptr || gdt == nullptr || gdt->seq_track == nullptr) {
    return false;
  }

  track->link.length = gdt->seq_track->length;
  track->link.set_speed(gdt->seq_track->speed);

  switch (gdt->track_type) {
  case MDSPSX_TRACK_TYPE: {
    auto *spsxTrack = track->as<SPSXTrack>();
    if (spsxTrack == nullptr) {
      return false;
    }
    spsxTrack->get_machine_from_kit(trackNumber);
    SeqTrack::store_mod_data(spsxTrack->seq_storage.mod(), true,
                             trackNumber);
    if (mcl_seq.using_spsx_tracks) {
      auto *seqTrack = static_cast<SPSXSeqTrack *>(gdt->seq_track);
      spsxTrack->seq_storage.seq_version = SPSX_SEQ_VERSION_SPSX;
      memcpy(spsxTrack->seq_storage.seq_data.spsx.data(),
             seqTrack->SPSXSeqTrackData::data(), sizeof(SPSXSeqTrackData));
    } else {
      auto *seqTrack = static_cast<MDSeqTrack *>(gdt->seq_track);
      spsxTrack->seq_storage.seq_version = SPSX_SEQ_VERSION_LEGACY;
      memcpy(spsxTrack->seq_storage.seq_data.legacy.data(), seqTrack->data(),
             sizeof(MDSeqTrackData));
    }
    return true;
  }
  case MD_TRACK_TYPE: {
    auto *mdTrack = track->as<MDTrack>();
    if (mdTrack == nullptr) {
      return false;
    }
    mdTrack->get_machine_from_kit(trackNumber);
    mcl_seq.md_arp_tracks[trackNumber].store_data(&mdTrack->mod_data.arp);
    mcl_seq.md_arp_tracks[trackNumber].store_phase_data(
        mdTrack->mod_data.arp_phase());
    mcl_seq.grid_x_lfo_tracks[trackNumber].store_data(&mdTrack->mod_data.lfo);
    auto *seqTrack = static_cast<MDSeqTrack *>(gdt->seq_track);
    memcpy(mdTrack->seq_data.data(), seqTrack->data(), sizeof(MDSeqTrackData));
    return true;
  }
  case MIDI_TRACK_TYPE: {
    auto *midiTrack = track->as<MidiTrack>();
    if (midiTrack == nullptr) {
      return false;
    }
    auto *seqTrack = static_cast<MidiSeqTrack *>(gdt->seq_track);
    SeqTrack::store_mod_data(midiTrack->seq_data.mod(), false, trackNumber);
    static_cast<MidiSeqTrackData &>(midiTrack->seq_data) = seqTrack->seq_data;
    midiTrack->seq_data.channel = seqTrack->channel();
    midiTrack->seq_data.length = gdt->seq_track->length;
    midiTrack->seq_data.speed = gdt->seq_track->speed;
    return true;
  }
  case EXT_TRACK_TYPE: {
    auto *extTrack = track->as<ExtTrack>();
    if (extTrack == nullptr) {
      return false;
    }
    auto *seqTrack = static_cast<ExtSeqTrack *>(gdt->seq_track);
    SeqTrack::store_mod_data(extTrack->mod_data, false, trackNumber);
    memcpy(&extTrack->seq_data, seqTrack->data(), sizeof(extTrack->seq_data));
    return true;
  }
  case A4_TRACK_TYPE: {
    auto *a4Track = track->as<A4Track>();
    if (a4Track == nullptr) {
      return false;
    }
    auto *seqTrack = static_cast<ExtSeqTrack *>(gdt->seq_track);
    SeqTrack::store_mod_data(a4Track->mod_data, false, trackNumber);
    memcpy(&a4Track->seq_data, seqTrack->data(),
           sizeof(a4Track->seq_data));
    return true;
  }
  case MNM_TRACK_TYPE: {
    auto *mnmTrack = track->as<MNMTrack>();
    if (mnmTrack == nullptr) {
      return false;
    }
    auto *seqTrack = static_cast<ExtSeqTrack *>(gdt->seq_track);
    SeqTrack::store_mod_data(mnmTrack->mod_data, false, trackNumber);
    memcpy(&mnmTrack->seq_data, seqTrack->data(),
           sizeof(mnmTrack->seq_data));
    return true;
  }
  default:
    return false;
  }
}

} // namespace

bool MCLArrangement::createPrivateSourceFromGrid(GridSlot sourceSlot,
                                                 GridRow row,
                                                 GridSlot dstTrack,
                                                 uint32_t *sourceIdOut) {
  if (sourceIdOut != nullptr) {
    *sourceIdOut = 0;
  }
  if (sourceSlot >= NUM_SLOTS || dstTrack >= NUM_SLOTS || row >= GRID_LENGTH ||
      !ensureActive()) {
    return false;
  }

  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(sourceSlot, row);
  if (source == nullptr || !source->is_active()) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, true)) {
    return false;
  }

  uint32_t sourceId = 0;
  GridColumn localCol = 0;
  GridRow localRow = 0;
  uint32_t maxSourceId = (uint32_t)GRID_WIDTH * (uint32_t)GRID_LENGTH;
  for (uint32_t candidate = 1; candidate <= maxSourceId; ++candidate) {
    GridColumn col = 0;
    GridRow privateRow = 0;
    if (!privateSourceCell(candidate, &col, &privateRow)) {
      break;
    }
    EmptyTrack existingScratch;
    DeviceTrack *existing =
        existingScratch.load_from_grid_512(col, privateRow, &privateGrid);
    if (existing == nullptr || !existing->is_active()) {
      sourceId = candidate;
      localCol = col;
      localRow = privateRow;
      break;
    }
  }
  if (sourceId == 0) {
    privateGrid.close_file();
    return false;
  }

  source->on_copy(sourceSlot & 0x0F, dstTrack & 0x0F, false);
  bool stored = source->store_in_grid(localCol, localRow, nullptr, 0, false,
                                      &privateGrid);
  stored = stored && privateGrid.sync();
  privateGrid.close_file();
  if (!stored) {
    ARR_COPY_TRACE("create-private store failed source_slot=%u row=%u dst=%u id=%lu",
                   sourceSlot, row, dstTrack, (unsigned long)sourceId);
    return false;
  }

  if (sourceIdOut != nullptr) {
    *sourceIdOut = sourceId;
  }
  ARR_COPY_TRACE(
      "create-private source_slot=%u row=%u dst=%u source_id=%lu cell=%u:%u",
      sourceSlot, row, dstTrack, (unsigned long)sourceId, localCol, localRow);
  return true;
}

bool MCLArrangement::duplicatePrivateSource(uint32_t sourceId,
                                            GridSlot sourceSlot,
                                            GridSlot dstTrack,
                                            uint32_t *sourceIdOut) {
  if (sourceIdOut != nullptr) {
    *sourceIdOut = 0;
  }
  if (sourceId == 0 || sourceSlot >= NUM_SLOTS || dstTrack >= NUM_SLOTS ||
      !ensureActive()) {
    return false;
  }

  GridColumn sourceCol = 0;
  GridRow sourceRow = 0;
  if (!privateSourceCell(sourceId, &sourceCol, &sourceRow)) {
    return false;
  }

  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if (runtimePrivateSourceId(slot) == sourceId) {
      ARR_COPY_TRACE("duplicate flush-runtime slot=%u source_id=%lu", slot,
                     (unsigned long)sourceId);
      flushRuntimePrivateSource(slot);
      break;
    }
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, true)) {
    return false;
  }

  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(sourceCol, sourceRow,
                                                   &privateGrid);
  if (source == nullptr || !source->is_active()) {
    privateGrid.close_file();
    return false;
  }

  uint32_t newSourceId = 0;
  GridColumn localCol = 0;
  GridRow localRow = 0;
  uint32_t maxSourceId = (uint32_t)GRID_WIDTH * (uint32_t)GRID_LENGTH;
  for (uint32_t candidate = 1; candidate <= maxSourceId; ++candidate) {
    GridColumn col = 0;
    GridRow privateRow = 0;
    if (!privateSourceCell(candidate, &col, &privateRow)) {
      break;
    }
    EmptyTrack existingScratch;
    DeviceTrack *existing =
        existingScratch.load_from_grid_512(col, privateRow, &privateGrid);
    if (existing == nullptr || !existing->is_active()) {
      newSourceId = candidate;
      localCol = col;
      localRow = privateRow;
      break;
    }
  }
  if (newSourceId == 0) {
    privateGrid.close_file();
    return false;
  }

  source->on_copy(sourceSlot & 0x0F, dstTrack & 0x0F, false);
  bool stored = source->store_in_grid(localCol, localRow, nullptr, 0, false,
                                      &privateGrid);
  stored = stored && privateGrid.sync();
  privateGrid.close_file();
  if (!stored) {
    ARR_COPY_TRACE(
        "duplicate store failed old_source_id=%lu source_slot=%u dst=%u new_source_id=%lu",
        (unsigned long)sourceId, sourceSlot, dstTrack,
        (unsigned long)newSourceId);
    return false;
  }

  if (sourceIdOut != nullptr) {
    *sourceIdOut = newSourceId;
  }
  ARR_COPY_TRACE(
      "duplicate old_source_id=%lu source_slot=%u dst=%u new_source_id=%lu cell=%u:%u",
      (unsigned long)sourceId, sourceSlot, dstTrack,
      (unsigned long)newSourceId, localCol, localRow);
  return true;
}

bool MCLArrangement::privateSourcePreview(uint32_t sourceId,
                                          uint8_t *trackType,
                                          uint8_t *length,
                                          uint8_t *speed,
                                          uint64_t *trigMask,
                                          uint8_t *noteCount,
                                          uint8_t *noteMin,
                                          uint8_t *noteMax,
                                          uint8_t *noteFlags,
                                          PrivateSourcePreviewNote *notes,
                                          uint8_t maxNotes,
                                          int8_t *trigTiming,
                                          uint8_t maxTrigTiming) {
  if (trackType != nullptr) {
    *trackType = EMPTY_TRACK_TYPE;
  }
  if (length != nullptr) {
    *length = 16;
  }
  if (speed != nullptr) {
    *speed = SEQ_SPEED_1X;
  }
  if (trigMask != nullptr) {
    *trigMask = 0;
  }
  if (noteCount != nullptr) {
    *noteCount = 0;
  }
  if (noteMin != nullptr) {
    *noteMin = 0;
  }
  if (noteMax != nullptr) {
    *noteMax = 0;
  }
  if (noteFlags != nullptr) {
    *noteFlags = 0;
  }
  if (trigTiming != nullptr) {
    for (uint8_t i = 0; i < maxTrigTiming; i++) {
      trigTiming[i] = 0;
    }
  }

  GridColumn col = 0;
  GridRow row = 0;
  Grid privateGrid;
  bool closePrivateGrid = false;
  if ((sourceId & kPreviewGridSourceFlag) != 0) {
    col = (GridColumn)((sourceId >> kPreviewGridSourceTrackShift) &
                       kPreviewGridSourceTrackMask);
    row = (GridRow)(sourceId & kPreviewGridSourceRowMask);
    if (col >= NUM_SLOTS || row >= GRID_LENGTH) {
      return false;
    }
  } else {
    if (!privateSourceCell(sourceId, &col, &row)) {
      return false;
    }

    if (!openPrivateGrid(privateGrid, false)) {
      return false;
    }
    closePrivateGrid = true;
  }

  EmptyTrack scratch;
  DeviceTrack *source = closePrivateGrid
                            ? scratch.load_from_grid_512(col, row,
                                                         &privateGrid)
                            : scratch.load_from_grid_512(col, row);
  if (source == nullptr || !source->is_active()) {
    if (closePrivateGrid) {
      privateGrid.close_file();
    }
    return false;
  }

  if ((sourceId & kPreviewGridSourceFlag) == 0) {
    for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
      if (runtime_private_source_ids_[slot] != sourceId) {
        continue;
      }
      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(slot);
      if (gdt != nullptr && gdt->seq_track != nullptr &&
          gdt->mem_slot_idx < GRID_WIDTH) {
        copyLiveSeqToPrivateTrack(source, gdt, (uint8_t)(slot & 0x0F));
      }
      break;
    }
  }

  uint8_t outType = source->active;
  uint8_t outLength = previewLength(source->link.length);
  uint8_t outSpeed = source->link.speed_value();
  uint8_t outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
  uint64_t outMask = 0;
#if !defined(__AVR__)
  PreviewNoteWriter noteWriter;
  noteWriter.notes = notes;
  noteWriter.maxNotes = maxNotes;
  bool collectNotes = noteCount != nullptr && notes != nullptr && maxNotes > 0;
#endif

  switch (source->active) {
  case MDSPSX_TRACK_TYPE: {
    auto *spsxTrack = source->as<SPSXTrack>();
    if (spsxTrack == nullptr) {
      break;
    }
    if (spsxTrack->has_spsx_seq()) {
      const auto &seq = spsxTrack->seq_storage.seq_data.spsx;
      outLength = previewLength(seq.track_length != 0 ? seq.track_length
                                                       : source->link.length);
      outSpeed = previewSpeed(seq.track_speed, source->link);
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = stepSeqPreviewTrigMask(seq, trigTiming, maxTrigTiming,
                                       outTicksPerStep);
    } else {
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = mdPreviewTrigMask(spsxTrack->seq_storage.seq_data.legacy,
                                  trigTiming, maxTrigTiming,
                                  outTicksPerStep);
    }
    break;
  }
  case MD_TRACK_TYPE: {
    auto *mdTrack = source->as<MDTrack>();
    if (mdTrack != nullptr) {
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = mdPreviewTrigMask(mdTrack->seq_data, trigTiming,
                                  maxTrigTiming, outTicksPerStep);
    }
    break;
  }
#if !defined(__AVR__)
  case MIDI_TRACK_TYPE: {
    auto *midiTrack = source->as<MidiTrack>();
    if (midiTrack != nullptr) {
      outLength = previewLength(midiTrack->seq_data.length);
      outSpeed = midiTrack->seq_data.speed;
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = midiPreviewTrigMask(midiTrack->seq_data, trigTiming,
                                    maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        midiPreviewNotes(midiTrack->seq_data, outTicksPerStep, noteWriter);
      }
    }
    break;
  }
  case EXT_TRACK_TYPE: {
    auto *extTrack = source->as<ExtTrack>();
    if (extTrack != nullptr) {
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = extPreviewTrigMask(extTrack->seq_data, trigTiming,
                                   maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        extPreviewNotes(extTrack->seq_data, outLength, outTicksPerStep,
                        noteWriter);
      }
    }
    break;
  }
  case A4_TRACK_TYPE: {
    auto *a4Track = source->as<A4Track>();
    if (a4Track != nullptr) {
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = extPreviewTrigMask(a4Track->seq_data, trigTiming,
                                   maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        extPreviewNotes(a4Track->seq_data, outLength, outTicksPerStep,
                        noteWriter);
      }
    }
    break;
  }
  case MNM_TRACK_TYPE: {
    auto *mnmTrack = source->as<MNMTrack>();
    if (mnmTrack != nullptr) {
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = extPreviewTrigMask(mnmTrack->seq_data, trigTiming,
                                   maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        extPreviewNotes(mnmTrack->seq_data, outLength, outTicksPerStep,
                        noteWriter);
      }
    }
    break;
  }
  case A4_MIDI_TRACK_TYPE: {
    auto *a4MidiTrack = source->as<A4MidiTrack>();
    if (a4MidiTrack != nullptr) {
      outLength = previewLength(a4MidiTrack->seq_data.length);
      outSpeed = a4MidiTrack->seq_data.speed;
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = midiPreviewTrigMask(a4MidiTrack->seq_data, trigTiming,
                                    maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        midiPreviewNotes(a4MidiTrack->seq_data, outTicksPerStep, noteWriter);
      }
    }
    break;
  }
  case MNM_MIDI_TRACK_TYPE: {
    auto *mnmMidiTrack = source->as<MNMMidiTrack>();
    if (mnmMidiTrack != nullptr) {
      outLength = previewLength(mnmMidiTrack->seq_data.length);
      outSpeed = mnmMidiTrack->seq_data.speed;
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = midiPreviewTrigMask(mnmMidiTrack->seq_data, trigTiming,
                                    maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        midiPreviewNotes(mnmMidiTrack->seq_data, outTicksPerStep, noteWriter);
      }
    }
    break;
  }
#ifdef PLATFORM_TBD
  case TBD_MIDI_TRACK_TYPE: {
    auto *tbdMidiTrack = source->as<TBDMidiTrack>();
    if (tbdMidiTrack != nullptr) {
      outLength = previewLength(tbdMidiTrack->seq_data.length);
      outSpeed = tbdMidiTrack->seq_data.speed;
      outTicksPerStep = SeqTrack::get_ticks_per_step(outSpeed);
      outMask = midiPreviewTrigMask(tbdMidiTrack->seq_data, trigTiming,
                                    maxTrigTiming, outTicksPerStep);
      if (collectNotes) {
        midiPreviewNotes(tbdMidiTrack->seq_data, outTicksPerStep, noteWriter);
      }
    }
    break;
  }
#endif
#endif
  default:
    break;
  }

  if (closePrivateGrid) {
    privateGrid.close_file();
  }
  if (trackType != nullptr) {
    *trackType = outType;
  }
  if (length != nullptr) {
    *length = outLength;
  }
  if (speed != nullptr) {
    *speed = outSpeed;
  }
  if (trigMask != nullptr) {
    *trigMask = outMask;
  }
#if !defined(__AVR__)
  if (noteCount != nullptr) {
    *noteCount = noteWriter.count;
  }
  if (noteMin != nullptr) {
    *noteMin = noteWriter.haveNotes ? noteWriter.noteMin : 0;
  }
  if (noteMax != nullptr) {
    *noteMax = noteWriter.haveNotes ? noteWriter.noteMax : 0;
  }
  if (noteFlags != nullptr) {
    *noteFlags = noteWriter.flags;
  }
#endif
  return true;
}

bool MCLArrangement::makeClipLocal(uint32_t startQ12, uint32_t durationQ12,
                                   uint8_t track, uint8_t row,
                                   GridSlot expectedSourceSlot,
                                   uint32_t *sourceIdOut) {
  if (sourceIdOut != nullptr) {
    *sourceIdOut = 0;
  }
  if (track >= NUM_SLOTS || row >= GRID_LENGTH || durationQ12 == 0 ||
      !ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  int clipIndex = -1;
  for (uint32_t i = 0; i < clipCount; ++i) {
    mclarrfile::Clip &clip = clips[i];
    if (clip.startQ12 != startQ12 || clip.durationQ12 != durationQ12 ||
        clip.track != track || clip.row != row ||
        clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE) {
      continue;
    }
    GridSlot sourceSlot = (GridSlot)mclarrfile::clipSourceSlot(clip);
    if (expectedSourceSlot < NUM_SLOTS && sourceSlot != expectedSourceSlot) {
      continue;
    }
    clipIndex = (int)i;
    break;
  }
  if (clipIndex < 0) {
    return false;
  }

  mclarrfile::Clip &clip = clips[clipIndex];
  GridSlot sourceSlot = (GridSlot)mclarrfile::clipSourceSlot(clip);
  uint32_t sourceId = 0;
  if (!createPrivateSourceFromGrid(sourceSlot, clip.row, clip.track,
                                   &sourceId)) {
    return false;
  }

  clip.sourceKind = mclarrfile::CLIP_SOURCE_PRIVATE;
  clip.sourceTrack = sourceSlot;
  clip.sourceFlags = mclarrfile::encodePrivateSourceSlot(clip.track);
  clip.sourceReserved = 0;
  clip.sourceId = sourceId;

  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    if (sourceIdOut != nullptr) {
      *sourceIdOut = sourceId;
    }
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::exportPrivateSourceToGrid(uint32_t sourceId,
                                               GridSlot sourceSlot,
                                               GridRow sourceRow,
                                               GridSlot targetSlot,
                                               GridRow targetRow) {
  if (sourceSlot >= NUM_SLOTS || targetSlot >= NUM_SLOTS ||
      sourceRow >= GRID_LENGTH || targetRow >= GRID_LENGTH) {
    return false;
  }

  GridColumn localCol = 0;
  GridRow localRow = 0;
  if (!privateSourceCell(sourceId, &localCol, &localRow)) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, false)) {
    return false;
  }

  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(localCol, localRow,
                                                   &privateGrid);
  privateGrid.close_file();
  if (source == nullptr || !source->is_active()) {
    return false;
  }

  int16_t linkRowOffset = (int16_t)source->link.row - (int16_t)sourceRow;
  int16_t newLinkRow = (int16_t)targetRow + linkRowOffset;
  if (newLinkRow < 0 || newLinkRow >= (int16_t)GRID_LENGTH) {
    newLinkRow = targetRow;
  }
  source->link.row = (uint8_t)newLinkRow;
  source->on_copy(sourceSlot & 0x0F, targetSlot & 0x0F, false);
  bool stored = source->store_in_grid(targetSlot, targetRow, nullptr, 0, false);
  if (!stored) {
    return false;
  }

  GridIndex targetGrid = targetSlot / GRID_WIDTH;
  GridRowHeader header;
  if (proj.read_grid_row_header(&header, targetRow, targetGrid)) {
    header.active = true;
    header.name[0] = '\0';
    proj.write_grid_row_header(&header, targetRow, targetGrid);
  }
  proj.sync_grid(targetGrid);
  return true;
}

void MCLArrangement::beginQueuedPrivateLoads(
    const uint32_t sourceIds[NUM_SLOTS]) {
  if (sourceIds == nullptr) {
    memset(queued_private_source_ids_, 0, sizeof(queued_private_source_ids_));
    return;
  }
  memcpy(queued_private_source_ids_, sourceIds,
         sizeof(queued_private_source_ids_));
}

void MCLArrangement::endQueuedPrivateLoads() {
  memset(queued_private_source_ids_, 0, sizeof(queued_private_source_ids_));
}

uint32_t MCLArrangement::runtimePrivateSourceMask() const {
  uint32_t mask = 0;
  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if (runtime_private_source_ids_[slot] != 0) {
      mask |= (uint32_t)(1ul << slot);
    }
  }
  return mask;
}

uint32_t MCLArrangement::runtimePrivateSourceId(uint8_t dst) const {
  if (dst >= NUM_SLOTS) {
    return 0;
  }
  return runtime_private_source_ids_[dst];
}

void MCLArrangement::setRuntimePrivateSource(uint8_t dst, uint32_t sourceId,
                                             uint8_t sourceSlot) {
  if (dst >= NUM_SLOTS) {
    return;
  }
  uint32_t previousSourceId = runtime_private_source_ids_[dst];
  runtime_private_source_ids_[dst] = sourceId;
  runtime_private_source_slots_[dst] =
      sourceSlot < NUM_SLOTS ? sourceSlot : dst;
  if (dst < 32 && previousSourceId != sourceId) {
    runtime_private_dirty_mask_ &= ~(uint32_t)(1ul << dst);
  }
}

void MCLArrangement::clearRuntimePrivateSource(uint8_t dst) {
  if (dst >= NUM_SLOTS) {
    return;
  }
  if (dst < 32 && (runtime_private_dirty_mask_ & (uint32_t)(1ul << dst)) != 0) {
    flushRuntimePrivateSource(dst);
  }
  runtime_private_source_ids_[dst] = 0;
  runtime_private_source_slots_[dst] = 0;
  if (dst < 32) {
    runtime_private_dirty_mask_ &= ~(uint32_t)(1ul << dst);
  }
}

void MCLArrangement::clearRuntimePrivateSources(uint32_t mask) {
  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if ((mask & (uint32_t)(1ul << slot)) != 0) {
      if ((runtime_private_dirty_mask_ & (uint32_t)(1ul << slot)) != 0) {
        flushRuntimePrivateSource(slot);
      }
      runtime_private_source_ids_[slot] = 0;
      runtime_private_source_slots_[slot] = 0;
    }
  }
  runtime_private_dirty_mask_ &= ~mask;
}

bool MCLArrangement::flushRuntimePrivateSource(uint8_t dst) {
  if (dst >= NUM_SLOTS || runtime_private_source_ids_[dst] == 0) {
    return false;
  }

  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(dst);
  if (gdt == nullptr || gdt->seq_track == nullptr ||
      gdt->mem_slot_idx >= GRID_WIDTH) {
    return false;
  }

  GridColumn localCol = 0;
  GridRow localRow = 0;
  if (!privateSourceCell(runtime_private_source_ids_[dst], &localCol,
                         &localRow)) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, true)) {
    return false;
  }

  GridLink savedLink;
  TrackLoadFadeData savedLoadFade;
  savedLoadFade.init();
  bool haveSavedLink = false;
  EmptyTrack existingScratch;
  DeviceTrack *existing =
      existingScratch.load_from_grid_512(localCol, localRow, &privateGrid);
  if (existing != nullptr && existing->is_active()) {
    savedLink = existing->link;
    haveSavedLink = true;
    const TrackLoadFadeData *existingFade = existing->load_fade_data();
    if (existingFade != nullptr) {
      savedLoadFade = *existingFade;
    }
  }
  if (!haveSavedLink) {
    savedLink.init(localRow);
  }

  EmptyTrack liveScratch;
  DeviceTrack *live =
      liveScratch.load_from_mem(gdt->mem_slot_idx, gdt->track_type);
  if (live == nullptr) {
    live = liveScratch.init_track_type(gdt->track_type);
    if (live == nullptr) {
      privateGrid.close_file();
      return false;
    }
    live->init((uint8_t)(dst & 0x0F), gdt->seq_track);
  }
  live->link = savedLink;
  TrackLoadFadeData *liveFade = live->load_fade_data();
  if (liveFade != nullptr) {
    *liveFade = savedLoadFade;
  }

  uint8_t trackNumber = (uint8_t)(dst & 0x0F);
  uint8_t sourceTrackNumber =
      (uint8_t)(runtime_private_source_slots_[dst] & 0x0F);
  bool copiedSeq = copyLiveSeqToPrivateTrack(live, gdt, trackNumber);
  if (copiedSeq && sourceTrackNumber != trackNumber) {
    live->on_copy(trackNumber, sourceTrackNumber, false);
  }
  bool stored = copiedSeq
                    ? live->write_grid(live->_this(), live->get_store_size(),
                                       localCol, localRow, &privateGrid)
                    : live->store_in_grid(localCol, localRow, gdt->seq_track,
                                          0, false, &privateGrid);
  stored = stored && privateGrid.sync();
  privateGrid.close_file();
  if (stored && dst < 32) {
    runtime_private_dirty_mask_ &= ~(uint32_t)(1ul << dst);
    sps_host_arr_bridge.notifyDirty(
        0xFF, (uint8_t)spsarr::DIRTY_LOCAL_PREVIEW);
  }
  return stored;
}

bool MCLArrangement::markRuntimePrivateSourceEdited(uint8_t dst) {
  if (dst >= NUM_SLOTS || runtime_private_source_ids_[dst] == 0) {
    return false;
  }
  if (dst < 32) {
    runtime_private_dirty_mask_ |= (uint32_t)(1ul << dst);
    sps_host_arr_bridge.notifyDirty(
        0xFF, (uint8_t)spsarr::DIRTY_LOCAL_PREVIEW);
  }
  return true;
}

bool MCLArrangement::flushRuntimePrivateSourceEdits() {
  uint32_t mask = runtime_private_dirty_mask_;
  if (mask == 0) {
    return false;
  }

  bool flushed = false;
  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if ((mask & (uint32_t)(1ul << slot)) == 0) {
      continue;
    }
    flushed = flushRuntimePrivateSource(slot) || flushed;
  }
  return flushed;
}

bool MCLArrangement::loadQueuedPrivateSource(GridSlot sourceSlot, GridRow row,
                                             EmptyTrack &scratch,
                                             DeviceTrack **out,
                                             GridSlot runtimeSlot) {
  if (out != nullptr) {
    *out = nullptr;
  }
  if (row != LOAD_QUEUE_PRIVATE_ROW) {
    return false;
  }
  if (out == nullptr || sourceSlot >= NUM_SLOTS) {
    return true;
  }
  GridSlot runtimeDst = runtimeSlot < NUM_SLOTS ? runtimeSlot : sourceSlot;

  GridColumn localCol = 0;
  GridRow localRow = 0;
  uint32_t sourceId = queued_private_source_ids_[sourceSlot];
  if (!privateSourceCell(sourceId, &localCol, &localRow)) {
    clearRuntimePrivateSource(runtimeDst);
    return true;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, false)) {
    clearRuntimePrivateSource(runtimeDst);
    return true;
  }
  *out = scratch.load_from_grid_512(localCol, localRow, &privateGrid);
  privateGrid.close_file();
  if (*out != nullptr) {
    setRuntimePrivateSource(runtimeDst, sourceId, sourceSlot);
  } else {
    clearRuntimePrivateSource(runtimeDst);
  }
  return true;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
