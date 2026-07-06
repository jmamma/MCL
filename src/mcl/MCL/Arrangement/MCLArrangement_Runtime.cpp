#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"
#include "Drivers/MD/MDParams.h"
#include "Grid/MCLActions.h"
#include "GUI/Pages/CommonPages.h"
#include "Host/SpsHostSeqBridge.h"
#include "MCLMemory.h"
#include "MCLSysConfig.h"
#include "Sequencer/SeqTrackUtil.h"
#include "MidiClock.h"

using namespace mcl_arrangement_internal;

namespace {

#if !defined(__AVR__) && defined(DEBUGMODE)
#define ARR_RUNTIME_TRACE(fmt, ...) \
  DEBUG_PRINT_FN("[arr-runtime] " fmt, ##__VA_ARGS__)
#else
#define ARR_RUNTIME_TRACE(fmt, ...)
#endif

static const uint32_t kArrangerBoundaryLookaheadQ12 = 4;

uint8_t automation_curve_phase(uint8_t phase, int8_t curve) {
  uint16_t lin = phase > 127 ? 127 : phase;
  uint16_t expo = (uint16_t)((lin * lin + 63u) / 127u);
  uint16_t inv_base = (uint16_t)(127u - lin);
  uint16_t inv = (uint16_t)(127u - (inv_base * inv_base + 63u) / 127u);
  uint16_t mix = curve < 0 ? (uint16_t)(-curve) : (uint16_t)curve;
  if (mix > 127) {
    mix = 127;
  }
  uint16_t shaped = curve > 0 ? expo : (curve < 0 ? inv : lin);
  uint16_t out =
      (uint16_t)((lin * (127u - mix) + shaped * mix + 63u) / 127u);
  return out > 127 ? 127 : (uint8_t)out;
}

int8_t automation_effective_curve(int8_t curve, uint16_t startValue,
                                  uint16_t endValue) {
  int16_t out = curve;
  if (endValue < startValue) {
    out = -out;
  }
  if (out < -127) {
    out = -127;
  }
  if (out > 127) {
    out = 127;
  }
  return (int8_t)out;
}

uint16_t clamp_automation_value(uint16_t value, uint8_t valueType) {
  if (valueType == mclarrfile::AUTOMATION_VALUE_BOOL) {
    return value != 0 ? 1 : 0;
  }
  if (valueType == mclarrfile::AUTOMATION_VALUE_U14) {
    return value > 16383 ? 16383 : value;
  }
  return value > 127 ? 127 : value;
}

MidiUartClass *automation_uart(uint8_t device) {
  return device == 1 ? &MidiUart2 : &MidiUart;
}

constexpr uint8_t automation_fx_lane() {
  return (uint8_t)(NUM_MD_TRACKS + MDFX_TRACK_NUM);
}

constexpr uint8_t automation_perf_lane() {
  return (uint8_t)(NUM_MD_TRACKS + PERF_TRACK_NUM);
}

constexpr uint8_t automation_route_lane() {
  return (uint8_t)(NUM_MD_TRACKS + MDROUTE_TRACK_NUM);
}

constexpr uint8_t automation_tempo_lane() {
  return (uint8_t)(NUM_MD_TRACKS + MDTEMPO_TRACK_NUM);
}

uint16_t clamp_automation_tempo_raw(uint16_t raw) {
  if (raw < 720) {
    return 720;
  }
  if (raw > 7200) {
    return 7200;
  }
  return raw;
}

uint8_t automation_fx_type(uint8_t targetIndex) {
  switch (targetIndex >> 3) {
    case 0:
      return MD_FX_ECHO;
    case 1:
      return MD_FX_REV;
    case 2:
      return MD_FX_EQ;
    case 3:
      return MD_FX_DYN;
    default:
      return MD_FX_ECHO;
  }
}

PerfEncoder *automation_perf_encoder(uint8_t index) {
  if (index >= NUM_PERF_CONTROLS) {
    return nullptr;
  }
  PerfEncoder *encoder = perf_page.perf_encoders[index];
  if (encoder != nullptr) {
    return encoder;
  }
  switch (index) {
    case 0:
      return &perf_param1;
    case 1:
      return &perf_param2;
    case 2:
      return &perf_param3;
    case 3:
      return &perf_param4;
    default:
      return nullptr;
  }
}

void dispatchAutomationWrite(
    const MCLArrangement::AutomationPendingWrite &write) {
  uint8_t rawTrack = write.track;
  uint16_t value = clamp_automation_value(write.value, write.valueType);
  MidiUartClass *uart = automation_uart(write.device);
  switch (write.targetType) {
    case mclarrfile::AUTOMATION_TARGET_MD_PARAM:
      if (rawTrack == automation_fx_lane()) {
        if (write.targetIndex < 32) {
          MD.setFXParam((uint8_t)(write.targetIndex & 0x07), (uint8_t)value,
                        automation_fx_type(write.targetIndex), false, uart);
        }
        break;
      }
      {
        uint8_t track = rawTrack;
        if (track < NUM_MD_TRACKS &&
            (write.targetIndex < SPS_PARAMS_PER_TRACK ||
             write.targetIndex == MODEL_LEVEL)) {
          MD.setTrackParam(track, write.targetIndex, (uint8_t)value, uart,
                           false);
        }
      }
      break;
    case mclarrfile::AUTOMATION_TARGET_MUTE:
      if (rawTrack < NUM_MD_TRACKS) {
        uint8_t track = rawTrack;
        bool mute = value != 0;
        SeqTrackUtil::set_mute_state(true, track, mute);
        MD.muteTrack(track, mute, uart);
      }
      break;
    case mclarrfile::AUTOMATION_TARGET_FILL:
      if (rawTrack < NUM_MD_TRACKS) {
        uint8_t track = rawTrack;
        bool fill = value != 0;
        DeviceIdx deviceIdx =
            write.device == 1 ? DeviceIdx::Secondary : DeviceIdx::Primary;
        mcl_seq.set_fill_track(deviceIdx, track, fill);
        if (MD.global.baseChannel != 127) {
          MD.sendCC(MD.global.baseChannel + (track >> 2), 68 + (track & 3),
                    fill ? 127 : 0, uart);
        }
      }
      break;
    case mclarrfile::AUTOMATION_TARGET_PERF:
      if (rawTrack == automation_perf_lane() &&
          write.targetIndex < NUM_PERF_CONTROLS) {
        PerfEncoder *encoder = automation_perf_encoder(write.targetIndex);
        if (encoder != nullptr) {
          encoder->setValue((int)value);
          perf_page.perf_id = write.targetIndex;
          perf_page.send_perf_encoder(write.targetIndex, uart);
        }
      }
      break;
    case mclarrfile::AUTOMATION_TARGET_ROUTING:
      if (rawTrack == automation_route_lane() &&
          write.targetIndex < NUM_MD_TRACKS) {
        uint8_t route = value > 6 ? 6 : (uint8_t)value;
        mcl_cfg.routing[write.targetIndex] = route;
        MD.setTrackRouting(write.targetIndex, route);
      }
      break;
    case mclarrfile::AUTOMATION_TARGET_TEMPO:
      if (rawTrack == automation_tempo_lane()) {
        uint16_t rawTempo = clamp_automation_tempo_raw(write.value);
        float bpm = (float)rawTempo / 24.0f;
        mcl_cfg.tempo = bpm;
        MidiClock.setTempo(bpm);
        MD.setTempo(bpm, true);
        sps_host_seq_bridge.notifyActive();
      }
      break;
    default:
      break;
  }
}

}  // namespace

void MCLArrangement::resetPlayback(bool clearPrivateSources) {
  playback_arrangement_idx_ = 0xFF;
  last_tick_q12_ = 0;
  playback_active_mask_ = 0;
  playback_released_mask_ = 0;
  playback_preload_mask_ = 0;
  playback_preclear_mask_ = 0;
  memset(playback_preload_start_q12_, 0, sizeof(playback_preload_start_q12_));
  memset(playback_preclear_end_q12_, 0, sizeof(playback_preclear_end_q12_));
  clip_runtime_fade_mask_ = 0;
  memset(clip_runtime_fade_start_q12_, 0,
         sizeof(clip_runtime_fade_start_q12_));
  if (clearPrivateSources) {
    memset(runtime_private_source_ids_, 0, sizeof(runtime_private_source_ids_));
    memset(runtime_private_source_slots_, 0,
           sizeof(runtime_private_source_slots_));
    runtime_private_dirty_mask_ = 0;
  }
  resetAutomationRuntime();
  playback_active_ = false;
  loop_entered_ = false;
  stored_loop_active_id_ = 0;
  stored_loop_repeats_done_ = 0;
  stored_loop_start_q12_ = 0;
  stored_loop_end_q12_ = 0;
  stored_loop_count_ = 0;
}

void MCLArrangement::resetPlaybackForTransport(bool clearReleasedTracks) {
  flushRuntimePrivateSourceEdits();
  uint32_t releasedMask = playback_released_mask_;
  resetPlayback();
  if (!clearReleasedTracks) {
    playback_released_mask_ = releasedMask;
  }
  grid_task.load_queue.init();
}

void MCLArrangement::reconcilePlaybackAfterEdit(bool clearReleasedTracks) {
  grid_task.load_queue.init();
  mcl_actions.clear_load_fades();
  resetPlayback();
  if (MidiClock.state == MidiClockClass::STARTED) {
    seekLoadCurrentPosition(true, false, clearReleasedTracks);
  }
}

void MCLArrangement::setHostPlaybackSuspended(bool suspended) {
  host_playback_suspended_ = suspended;
  if (suspended) {
    clearLoopRegion();
    resetPlayback(false);
  }
}

void MCLArrangement::setLoopRegion(uint32_t startQ12, uint32_t endQ12) {
  if (endQ12 <= startQ12 ||
      endQ12 - startQ12 < spsarr::kMinArrLoopQ12) {
    clearLoopRegion();
    return;
  }
  loop_enabled_ = true;
  loop_start_q12_ = startQ12;
  loop_end_q12_ = endQ12;
  loop_entered_ = false;
}

void MCLArrangement::clearLoopRegion() {
  loop_enabled_ = false;
  loop_entered_ = false;
  loop_start_q12_ = 0;
  loop_end_q12_ = 0;
}

void MCLArrangement::resetAutomationRuntime() {
  automation_runtime_valid_ = false;
  automation_cache_valid_ = false;
  automation_arrangement_idx_ = 0xFF;
  automation_lane_count_ = 0;
  automation_point_count_ = 0;
  automation_point_offset_ = 0;
  automation_pending_count_ = 0;
  memset(automation_runtime_, 0, sizeof(automation_runtime_));
}

bool MCLArrangement::loadAutomationDirectory() {
  if (automation_cache_valid_ &&
      automation_arrangement_idx_ == mcl_cfg.active_arrangement_idx) {
    return true;
  }

  automation_cache_valid_ = false;
  automation_runtime_valid_ = false;
  automation_lane_count_ = 0;
  automation_point_count_ = 0;
  automation_point_offset_ = 0;
  memset(automation_runtime_, 0, sizeof(automation_runtime_));

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if ((header.flags & mclarrfile::HEADER_HAS_AUTOMATION) == 0) {
    automation_cache_valid_ = true;
    automation_arrangement_idx_ = mcl_cfg.active_arrangement_idx;
    return true;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return false;
  }

  mclarrfile::ChunkDirEntry laneChunk;
  mclarrfile::ChunkDirEntry pointChunk;
  bool ok = findChunk(file, header, mclarrfile::CHUNK_AUTOMATION_LANES,
                      &laneChunk) &&
            laneChunk.itemBytes == sizeof(mclarrfile::AutomationLane) &&
            laneChunk.count <= kRuntimeAutomationLanes;
  if (ok) {
    ok = file.seekSet(laneChunk.offset);
    for (uint32_t i = 0; ok && i < laneChunk.count; ++i) {
      ok = mcl_sd.read_data(&automation_lanes_[i],
                            sizeof(mclarrfile::AutomationLane), &file);
    }
  }
  bool havePoints = findChunk(file, header,
                              mclarrfile::CHUNK_AUTOMATION_POINTS,
                              &pointChunk) &&
                    pointChunk.itemBytes ==
                        sizeof(mclarrfile::AutomationPoint) &&
                    pointChunk.count <= mclarrfile::kMaxAutomationPoints;
  file.close();
  if (!ok) {
    return false;
  }

  automation_lane_count_ = (uint16_t)laneChunk.count;
  automation_point_count_ = havePoints ? pointChunk.count : 0;
  automation_point_offset_ = havePoints ? pointChunk.offset : 0;
  automation_arrangement_idx_ = mcl_cfg.active_arrangement_idx;
  automation_cache_valid_ = true;
  return true;
}

bool MCLArrangement::automationReadPoint(
    File &file, uint32_t pointIndex, mclarrfile::AutomationPoint *out) {
  if (out == nullptr || pointIndex >= automation_point_count_) {
    return false;
  }
  return file.seekSet(automation_point_offset_ +
                      pointIndex * sizeof(mclarrfile::AutomationPoint)) &&
         mcl_sd.read_data(out, sizeof(*out), &file);
}

bool MCLArrangement::automationPrepareLane(File &file, uint16_t laneIndex,
                                           uint32_t positionQ12) {
  if (laneIndex >= automation_lane_count_) {
    return false;
  }
  const mclarrfile::AutomationLane &lane = automation_lanes_[laneIndex];
  AutomationRuntimeLane &rt = automation_runtime_[laneIndex];
  memset(&rt, 0, sizeof(rt));
  if ((lane.flags & mclarrfile::AUTOMATION_LANE_ENABLED) == 0 ||
      lane.pointCount == 0 ||
      lane.pointOffset > automation_point_count_ ||
      lane.pointCount > automation_point_count_ - lane.pointOffset) {
    return false;
  }

  uint32_t end = lane.pointOffset + lane.pointCount;
  for (uint32_t pointIndex = lane.pointOffset; pointIndex < end; ++pointIndex) {
    mclarrfile::AutomationPoint point;
    if (!automationReadPoint(file, pointIndex, &point)) {
      memset(&rt, 0, sizeof(rt));
      return false;
    }
    if (point.q12 <= positionQ12) {
      rt.prev = point;
      rt.have_prev = true;
      rt.next_point_index = pointIndex + 1;
      continue;
    }
    rt.next = point;
    rt.have_next = true;
    rt.next_point_index = pointIndex;
    break;
  }
  if (!rt.have_next && rt.next_point_index < end) {
    mclarrfile::AutomationPoint point;
    if (automationReadPoint(file, rt.next_point_index, &point)) {
      rt.next = point;
      rt.have_next = true;
    }
  }
  rt.active = rt.have_prev || rt.have_next;
  return rt.active;
}

uint16_t MCLArrangement::automationEvaluate(uint16_t laneIndex,
                                            uint32_t positionQ12) const {
  const mclarrfile::AutomationLane &lane = automation_lanes_[laneIndex];
  const AutomationRuntimeLane &rt = automation_runtime_[laneIndex];
  if (!rt.have_prev) {
    return 0;
  }
  uint16_t startValue = clamp_automation_value(rt.prev.value, lane.valueType);
  if (!rt.have_next ||
      rt.prev.interp != mclarrfile::AUTOMATION_INTERP_CURVE ||
      rt.next.q12 <= rt.prev.q12 || positionQ12 <= rt.prev.q12) {
    return startValue;
  }
  if (positionQ12 >= rt.next.q12) {
    return clamp_automation_value(rt.next.value, lane.valueType);
  }
  uint32_t span = rt.next.q12 - rt.prev.q12;
  uint32_t elapsed = positionQ12 - rt.prev.q12;
  uint8_t phase = (uint8_t)((elapsed * 127u) / span);
  uint16_t endValue = clamp_automation_value(rt.next.value, lane.valueType);
  phase = automation_curve_phase(
      phase, automation_effective_curve(rt.prev.curve, startValue, endValue));
  int32_t value = (int32_t)startValue +
                  ((int32_t)endValue - (int32_t)startValue) * phase / 127;
  if (value < 0) {
    value = 0;
  }
  return clamp_automation_value((uint16_t)value, lane.valueType);
}

void MCLArrangement::queueAutomationWrite(uint16_t laneIndex,
                                          uint16_t value) {
  if (laneIndex >= automation_lane_count_) {
    return;
  }
  const mclarrfile::AutomationLane &lane = automation_lanes_[laneIndex];
  AutomationRuntimeLane &rt = automation_runtime_[laneIndex];
  value = clamp_automation_value(value, lane.valueType);
  if (rt.last_sent_valid && rt.last_sent_value == value) {
    return;
  }
  rt.last_sent_valid = true;
  rt.last_sent_value = value;

  for (uint8_t i = 0; i < automation_pending_count_; ++i) {
    AutomationPendingWrite &write = automation_pending_writes_[i];
    if (write.track == lane.track && write.targetType == lane.targetType &&
        write.device == lane.device && write.targetIndex == lane.targetIndex &&
        write.valueType == lane.valueType) {
      write.value = value;
      return;
    }
  }
  if (automation_pending_count_ >= kAutomationPendingWrites) {
    return;
  }
  AutomationPendingWrite &write =
      automation_pending_writes_[automation_pending_count_++];
  write.track = lane.track;
  write.targetType = lane.targetType;
  write.device = lane.device;
  write.targetIndex = lane.targetIndex;
  write.valueType = lane.valueType;
  write.value = value;
}

void MCLArrangement::flushAutomationWrites() {
  for (uint8_t i = 0; i < automation_pending_count_; ++i) {
    dispatchAutomationWrite(automation_pending_writes_[i]);
  }
  automation_pending_count_ = 0;
}

void MCLArrangement::chaseAutomation(uint32_t positionQ12, bool sendValues) {
  automation_pending_count_ = 0;
  if (!loadAutomationDirectory() || automation_lane_count_ == 0 ||
      automation_point_count_ == 0) {
    automation_runtime_valid_ = true;
    return;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return;
  }
  for (uint16_t laneIndex = 0; laneIndex < automation_lane_count_;
       ++laneIndex) {
    const mclarrfile::AutomationLane &lane = automation_lanes_[laneIndex];
    uint32_t trackBit = (uint32_t)(1ul << lane.track);
    if ((playback_released_mask_ & trackBit) != 0) {
      continue;
    }
    if (!automationPrepareLane(file, laneIndex, positionQ12)) {
      continue;
    }
    if (sendValues && automation_runtime_[laneIndex].have_prev) {
      uint16_t value = automationEvaluate(laneIndex, positionQ12);
      queueAutomationWrite(laneIndex, value);
    }
  }
  file.close();
  automation_runtime_valid_ = true;
}

void MCLArrangement::tickAutomation(uint32_t positionQ12) {
  automation_pending_count_ = 0;
  if (!loadAutomationDirectory() || automation_lane_count_ == 0 ||
      automation_point_count_ == 0) {
    return;
  }
  if (!automation_runtime_valid_) {
    chaseAutomation(positionQ12, true);
    return;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return;
  }
  for (uint16_t laneIndex = 0; laneIndex < automation_lane_count_;
       ++laneIndex) {
    const mclarrfile::AutomationLane &lane = automation_lanes_[laneIndex];
    uint32_t trackBit = (uint32_t)(1ul << lane.track);
    if ((playback_released_mask_ & trackBit) != 0) {
      continue;
    }
    AutomationRuntimeLane &rt = automation_runtime_[laneIndex];
    if (!rt.active && !automationPrepareLane(file, laneIndex, positionQ12)) {
      continue;
    }
    uint32_t laneEnd = lane.pointOffset + lane.pointCount;
    while (rt.have_next && rt.next.q12 <= positionQ12) {
      rt.prev = rt.next;
      rt.have_prev = true;
      rt.next_point_index++;
      if (rt.next_point_index < laneEnd) {
        rt.have_next =
            automationReadPoint(file, rt.next_point_index, &rt.next);
      } else {
        rt.have_next = false;
      }
    }
    if (rt.have_prev) {
      uint16_t value = automationEvaluate(laneIndex, positionQ12);
      queueAutomationWrite(laneIndex, value);
    }
  }
  file.close();
}

void MCLArrangement::armClipRuntime(uint8_t dst, const mclarrfile::Clip &clip,
                                    uint16_t elapsedQ12) {
  if (dst >= NUM_SLOTS) {
    return;
  }
  uint32_t bit = (uint32_t)(1ul << dst);
  if ((clip.flags & mclarrfile::CLIP_FADE_OVERRIDE) == 0) {
    clip_runtime_fade_mask_ &= ~bit;
    return;
  }
  TrackLoadFadeData fade = clipFadeData(clip, false);
  if (!fade.enabled()) {
    fade = clipFadeData(clip, true);
  }
  if (!fade.enabled()) {
    clip_runtime_fade_mask_ &= ~bit;
    return;
  }
  fade.set_elapsed_q12(elapsedQ12);
  clip_runtime_fades_[dst] = fade;
  clip_runtime_fade_mask_ |= bit;
}

bool MCLArrangement::armRuntimeForHostLoad(uint32_t positionQ12,
                                           const GridRow rows[NUM_SLOTS],
                                           uint16_t trackMask,
                                           GridSlot loadOffset,
                                           GridIndex sourceGridBank,
                                           const uint32_t
                                               privateSourceIds[NUM_SLOTS]) {
  if (rows == nullptr || !ensureActive()) {
    return false;
  }
  host_playback_suspended_ = false;
  if (sourceGridBank >= NUM_GRIDS) {
    sourceGridBank = 0;
  }

  GridRow loadedRows[NUM_SLOTS];
  memset(loadedRows, 255, sizeof(loadedRows));
  GridSlot loadedSources[NUM_SLOTS];
  memset(loadedSources, 255, sizeof(loadedSources));
  uint32_t loadedPrivateIds[NUM_SLOTS];
  memset(loadedPrivateIds, 0, sizeof(loadedPrivateIds));
  uint32_t loadMask = 0;
  uint32_t clearMask = 0;

  auto isPrivateLoad = [&](GridSlot sourceSlot) {
    return sourceSlot < NUM_SLOTS &&
           rows[sourceSlot] == LOAD_QUEUE_PRIVATE_ROW &&
           privateSourceIds != nullptr && privateSourceIds[sourceSlot] != 0;
  };

  GridSlot firstSource = 255;
  GridSlot sourceBase = (GridSlot)(sourceGridBank * GRID_WIDTH);
  for (uint8_t src = 0; src < GRID_WIDTH && src < 16; ++src) {
    GridSlot sourceSlot = (GridSlot)(sourceBase + src);
    if (sourceSlot >= NUM_SLOTS || ((trackMask >> src) & 1u) == 0 ||
        (rows[sourceSlot] >= GRID_LENGTH && !isPrivateLoad(sourceSlot))) {
      continue;
    }
    firstSource = sourceSlot;
    break;
  }

  for (uint8_t src = 0; src < GRID_WIDTH && src < 16; ++src) {
    if (((trackMask >> src) & 1u) == 0) {
      continue;
    }
    GridSlot sourceSlot = (GridSlot)(sourceBase + src);
    if (sourceSlot >= NUM_SLOTS) {
      continue;
    }
    GridRow row = rows[sourceSlot];
    bool privateLoad = isPrivateLoad(sourceSlot);
    if (row < GRID_LENGTH || privateLoad) {
      GridSlot dst = sourceSlot;
      if (loadOffset < NUM_SLOTS) {
        if (firstSource == 255) {
          continue;
        }
        int mapped = (int)sourceSlot - (int)firstSource + (int)loadOffset;
        if (mapped < 0 || mapped >= (int)NUM_SLOTS) {
          continue;
        }
        dst = (GridSlot)mapped;
      }
      if (dst < NUM_SLOTS) {
        loadedRows[dst] = row;
        loadedSources[dst] = sourceSlot;
        if (privateLoad) {
          loadedPrivateIds[dst] = privateSourceIds[sourceSlot];
        }
        loadMask |= (uint32_t)(1ul << dst);
      }
      continue;
    }
    if (row == LOAD_QUEUE_CLEAR_ROW) {
      clearMask |= (uint32_t)(1ul << sourceSlot);
    }
  }

  const uint32_t touchedMask = loadMask | clearMask;
  clip_runtime_fade_mask_ &= ~touchedMask;
  playback_preload_mask_ &= ~touchedMask;
  playback_preclear_mask_ &= ~touchedMask;
  clearRuntimePrivateSources(touchedMask);
  playback_released_mask_ &= ~touchedMask;
  if (touchedMask == 0) {
    return false;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return false;
  }

  mclarrfile::Header header;
  bool ok = mcl_sd.read_data(&header, sizeof(header), &file) &&
            mclarrfile::validHeader(header) &&
            file.seekSet(header.headerBytes);
  if (!ok) {
    file.close();
    return false;
  }

  bool armed = false;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!readClipRecord(file, header, clip)) {
      break;
    }
    const bool privateClip =
        clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE;
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= NUM_SLOTS ||
        (!privateClip && clip.row >= GRID_LENGTH)) {
      continue;
    }
    if (privateClip && !privateSourceCell(clip.sourceId, nullptr, nullptr)) {
      continue;
    }

    uint64_t clipStart = clip.startQ12;
    uint64_t clipEnd = clipStart + clip.durationQ12;
    if (clipEnd < clipStart) {
      clipEnd = 0xFFFFFFFFull;
    }

    const uint32_t bit = (uint32_t)(1ul << clip.track);
    bool matchedLoadedClip =
        privateClip ? loadedRows[clip.track] == LOAD_QUEUE_PRIVATE_ROW &&
                          loadedPrivateIds[clip.track] == clip.sourceId
                    : loadedRows[clip.track] == clip.row &&
                          loadedSources[clip.track] ==
                              mclarrfile::clipSourceSlot(clip);
    if ((loadMask & bit) != 0 && matchedLoadedClip &&
        (uint64_t)positionQ12 >= clipStart &&
        (uint64_t)positionQ12 < clipEnd) {
      TrackLoadFadeData fade;
      if (clipFadeAtPosition(clip, positionQ12, fade)) {
        armRuntimeFade(clip.track, fade);
        armed = true;
      }
      continue;
    }

    if ((clearMask & bit) != 0 && positionQ12 > 0) {
      uint64_t previous = (uint64_t)positionQ12 - 1u;
      TrackLoadFadeData fade;
      if (previous >= clipStart && previous < clipEnd &&
          clipFadeAtPosition(clip, (uint32_t)previous, true, fade)) {
        armRuntimeFade(clip.track, fade);
        armed = true;
      }
    }
  }
  file.close();

  playback_arrangement_idx_ = mcl_cfg.active_arrangement_idx;
  playback_active_ = true;
  last_tick_q12_ = positionQ12;
  playback_active_mask_ = (playback_active_mask_ & ~clearMask) | loadMask;
  for (uint8_t slot = 0; slot < NUM_SLOTS; ++slot) {
    if (loadedPrivateIds[slot] != 0) {
      ARR_RUNTIME_TRACE(
          "arm host private slot=%u source_id=%lu source_slot=%u pos=%lu load_mask=0x%08lx clear_mask=0x%08lx",
          slot, (unsigned long)loadedPrivateIds[slot], loadedSources[slot],
          (unsigned long)positionQ12, (unsigned long)loadMask,
          (unsigned long)clearMask);
      setRuntimePrivateSource(slot, loadedPrivateIds[slot],
                              loadedSources[slot]);
    }
  }
  return armed;
}

bool MCLArrangement::releasePlaybackTracks(uint32_t trackMask) {
  if (trackMask == 0) {
    return false;
  }
  uint32_t oldMask = playback_released_mask_;
  playback_released_mask_ |= trackMask;
  playback_active_mask_ &= ~trackMask;
  playback_preload_mask_ &= ~trackMask;
  playback_preclear_mask_ &= ~trackMask;
  clip_runtime_fade_mask_ &= ~trackMask;
  clearRuntimePrivateSources(trackMask);
  return playback_released_mask_ != oldMask;
}

void MCLArrangement::armRuntimeFade(uint8_t dst,
                                    const TrackLoadFadeData &fade) {
  if (dst >= NUM_SLOTS) {
    return;
  }
  uint32_t bit = (uint32_t)(1ul << dst);
  if (!fade.enabled()) {
    clip_runtime_fade_mask_ &= ~bit;
    clip_runtime_fade_start_q12_[dst] = 0;
    return;
  }
  clip_runtime_fades_[dst] = fade;
  uint32_t nowQ12 = currentClockQ12();
  uint16_t elapsedQ12 = fade.elapsed_q12();
  clip_runtime_fade_start_q12_[dst] =
      nowQ12 >= elapsedQ12 ? nowQ12 - elapsedQ12 : 0;
  clip_runtime_fade_mask_ |= bit;
}

bool MCLArrangement::applyClipRuntime(uint8_t dst, DeviceTrack *track) {
  if (dst >= NUM_SLOTS || track == nullptr) {
    return false;
  }
  uint32_t bit = (uint32_t)(1ul << dst);
  if ((clip_runtime_fade_mask_ & bit) == 0) {
    return false;
  }
  clip_runtime_fade_mask_ &= ~bit;
  TrackLoadFadeData *fade = track->load_fade_data();
  if (fade == nullptr) {
    clip_runtime_fade_start_q12_[dst] = 0;
    return false;
  }
  TrackLoadFadeData runtimeFade = clip_runtime_fades_[dst];
  uint32_t nowQ12 = currentClockQ12();
  uint32_t fadeStartQ12 = clip_runtime_fade_start_q12_[dst];
  uint32_t elapsedQ12 =
      nowQ12 > fadeStartQ12 ? nowQ12 - fadeStartQ12 : 0;
  if (elapsedQ12 > 0xFFFFu) {
    elapsedQ12 = 0xFFFFu;
  }
  runtimeFade.set_elapsed_q12((uint16_t)elapsedQ12);
  *fade = runtimeFade;
  clip_runtime_fade_start_q12_[dst] = 0;
  return true;
}

bool MCLArrangement::seekLoad(uint32_t positionQ12, bool immediate,
                              bool allowPrestartFade,
                              bool clearReleasedTracks,
                              SeekLoadResult *result) {
  if (result != nullptr) {
    *result = {};
  }
  host_playback_suspended_ = false;
  playback_arrangement_idx_ = mcl_cfg.active_arrangement_idx;
  playback_active_ = true;
  last_tick_q12_ = positionQ12;
  clip_runtime_fade_mask_ = 0;
  if (clearReleasedTracks) {
    playback_released_mask_ = 0;
  }
  loop_entered_ = loop_enabled_ && positionQ12 >= loop_start_q12_ &&
                  positionQ12 < loop_end_q12_;
  uint32_t endQ12 = positionQ12 == 0xFFFFFFFFu ? positionQ12
                                                : positionQ12 + 1u;
  uint8_t queueFlags = immediate ? LOAD_QUEUE_FLAG_IMMEDIATE : 0;
  if (allowPrestartFade) {
    queueFlags |= LOAD_QUEUE_FLAG_PRESTART_FADE;
  }
  bool queued = queueClipStarts(positionQ12, endQ12, true, true, queueFlags,
                                !clearReleasedTracks, 0, result);
  if (result != nullptr) {
    result->queued = queued;
    result->activeMask = playback_active_mask_;
  }
  chaseAutomation(positionQ12, true);
  return queued;
}

bool MCLArrangement::seekLoadCurrentPosition(bool immediate,
                                             bool allowPrestartFade,
                                             bool clearReleasedTracks) {
  return seekLoad(currentClockQ12(), immediate, allowPrestartFade,
                  clearReleasedTracks);
}

bool MCLArrangement::queueClipStarts(uint32_t startQ12, uint32_t endQ12,
                                     bool loadActiveAtPosition,
                                     bool clearInactiveTracks,
                                     uint8_t loadQueueFlags,
                                     bool honorReleasedTracks,
                                     uint32_t boundaryLookaheadQ12,
                                     SeekLoadResult *result) {
  if (result != nullptr) {
    *result = {};
  }
  File file;
  if (!openActive(&file, O_READ)) {
    return false;
  }

  mclarrfile::Header header;
  bool ok = mcl_sd.read_data(&header, sizeof(header), &file) &&
            mclarrfile::validHeader(header);
  if (!ok || header.clipCount == 0 ||
      !file.seekSet(header.headerBytes)) {
    file.close();
    playback_active_mask_ = 0;
    playback_preload_mask_ = 0;
    playback_preclear_mask_ = 0;
    return false;
  }

  ArrangerLoadGroups loadGroups;
  ArrangerLoadGroups preloadGroups;
  GridRow clearRows[NUM_SLOTS];
  memset(clearRows, 255, sizeof(clearRows));
  bool any = false;
  bool hasPlayableClip = false;
  uint32_t currentActiveMask = 0;
  uint32_t startMask = 0;
  uint32_t futureStartMask = 0;
  uint32_t preclearMask = 0;
  uint32_t nowQ12 = endQ12 > startQ12 ? endQ12 - 1u : startQ12;
  uint64_t lookaheadEndQ12 = (uint64_t)nowQ12 + boundaryLookaheadQ12;
  if (lookaheadEndQ12 > 0xFFFFFFFFull) {
    lookaheadEndQ12 = 0xFFFFFFFFull;
  }
  bool allowBoundaryLookahead =
      !loadActiveAtPosition && boundaryLookaheadQ12 > 0;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!readClipRecord(file, header, clip)) {
      break;
    }
    const bool privateClip =
        clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE;
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= NUM_SLOTS ||
        (!privateClip && clip.row >= GRID_LENGTH)) {
      continue;
    }
    if (privateClip && !privateSourceCell(clip.sourceId, nullptr, nullptr)) {
      continue;
    }
    hasPlayableClip = true;
    uint32_t trackBit = (uint32_t)(1ul << clip.track);
    if (honorReleasedTracks && (playback_released_mask_ & trackBit) != 0) {
      continue;
    }
    uint64_t clipStart = clip.startQ12;
    uint64_t clipEnd = clipStart + clip.durationQ12;
    if (clipEnd < clipStart) {
      clipEnd = 0xFFFFFFFFull;
    }
    bool activeNow = (uint64_t)nowQ12 >= clipStart &&
                     (uint64_t)nowQ12 < clipEnd;
    bool startsNow = q12InRange(clip.startQ12, startQ12, endQ12);
    bool hasFadeOverride =
        (clip.flags & mclarrfile::CLIP_FADE_OVERRIDE) != 0;
    bool preloadMatches =
        (playback_preload_mask_ & trackBit) != 0 &&
        playback_preload_start_q12_[clip.track] == clip.startQ12;
    bool preclearMatches =
        clipEnd <= 0xFFFFFFFFull &&
        (playback_preclear_mask_ & trackBit) != 0 &&
        playback_preclear_end_q12_[clip.track] == (uint32_t)clipEnd;
    bool futureStarts =
        allowBoundaryLookahead && !startsNow && !hasFadeOverride &&
        clipStart > nowQ12 && clipStart <= lookaheadEndQ12;
    bool futureEnds =
        allowBoundaryLookahead && !hasFadeOverride &&
        clipEnd > nowQ12 && clipEnd <= lookaheadEndQ12;
    if (activeNow && !preclearMatches) {
      currentActiveMask |= (uint32_t)(1ul << clip.track);
    }
    if (activeNow) {
      if (loadActiveAtPosition) {
        if (addClipLoad(loadGroups, clip)) {
          startMask |= (uint32_t)(1ul << clip.track);
          TrackLoadFadeData fade;
          if (clipFadeAtPosition(clip, nowQ12, fade)) {
            armRuntimeFade(clip.track, fade);
          }
          any = true;
        }
      }
    }
    if (startsNow) {
      playback_preload_mask_ &= ~trackBit;
      playback_preclear_mask_ &= ~trackBit;
      if (preloadMatches) {
        startMask |= (uint32_t)(1ul << clip.track);
      } else {
        if (addClipLoad(loadGroups, clip)) {
          startMask |= (uint32_t)(1ul << clip.track);
          TrackLoadFadeData fade;
          if (clipFadeAtPosition(clip, clip.startQ12, false, fade)) {
            armRuntimeFade(clip.track, fade);
          }
          any = true;
        }
      }
    } else if (futureStarts) {
      if (preloadMatches) {
        startMask |= trackBit;
        futureStartMask |= trackBit;
      } else if ((playback_active_mask_ & trackBit) == 0 &&
                 addClipLoad(preloadGroups, clip)) {
        startMask |= trackBit;
        futureStartMask |= trackBit;
        playback_preload_mask_ |= trackBit;
        playback_preload_start_q12_[clip.track] = clip.startQ12;
        any = true;
      }
    }
    if (futureEnds && (playback_active_mask_ & trackBit) != 0) {
      if (!preclearMatches) {
        preclearMask |= trackBit;
        playback_preclear_mask_ |= trackBit;
        playback_preclear_end_q12_[clip.track] = (uint32_t)clipEnd;
      }
    }
    if ((clip.flags & mclarrfile::CLIP_FADE_OVERRIDE) != 0) {
      TrackLoadFadeData fade = clipFadeData(clip, true);
      if (fade.enabled() && fade.fade_out()) {
        uint64_t fadeDuration =
            fade.duration_q12 < clip.durationQ12 ? fade.duration_q12
                                                 : clip.durationQ12;
        uint64_t fadeStart =
            clipEnd > fadeDuration ? clipEnd - fadeDuration : clipStart;
        if (fadeDuration > 0 && fadeStart != clipStart &&
            fadeStart <= 0xFFFFFFFFull &&
            q12InRange((uint32_t)fadeStart, startQ12, endQ12) &&
            clipFadeAtPosition(clip, (uint32_t)fadeStart, fade)) {
          if (addClipLoad(loadGroups, clip)) {
            startMask |= (uint32_t)(1ul << clip.track);
            armRuntimeFade(clip.track, fade);
            any = true;
          }
        }
      }
    }
  }
  file.close();

  if (!hasPlayableClip) {
    playback_active_mask_ = 0;
    playback_preload_mask_ = 0;
    playback_preclear_mask_ = 0;
    if (clearInactiveTracks) {
      for (uint8_t track = 0; track < NUM_SLOTS; ++track) {
        clearRows[track] = LOAD_QUEUE_CLEAR_ROW;
      }
      grid_task.load_queue.put((uint8_t)(LOAD_ARRANG | loadQueueFlags),
                               clearRows);
      if (result != nullptr) {
        result->queued = true;
        result->clearQueued = true;
        result->activeMask = 0;
      }
      return true;
    }
    if (result != nullptr) {
      result->activeMask = 0;
    }
    return false;
  }

  uint32_t clearBaseMask =
      clearInactiveTracks ? (uint32_t)0xFFFFFFFFul : playback_active_mask_;
  uint32_t clearMask = clearBaseMask & ~currentActiveMask;
  preclearMask &= ~startMask;
  if (preclearMask != 0) {
    clearMask |= preclearMask;
  }
  clearMask &= ~startMask;
  if (honorReleasedTracks) {
    clearMask &= ~playback_released_mask_;
  }
  for (uint8_t track = 0; track < NUM_SLOTS; ++track) {
    if (((clearMask >> track) & 1u) == 0) {
      continue;
    }
    clearRows[track] = LOAD_QUEUE_CLEAR_ROW;
    any = true;
  }
  playback_active_mask_ = (currentActiveMask | futureStartMask) & ~preclearMask;

  if (any) {
    uint8_t queueMode = LOAD_ARRANG | loadQueueFlags;
    bool loadQueued = loadGroups.flush(queueMode);
    if (preloadGroups.flush(
            (uint8_t)(queueMode | LOAD_QUEUE_FLAG_ARRANGER_PRELOAD))) {
      loadQueued = true;
    }
    bool hasClear = false;
    for (uint8_t track = 0; track < NUM_SLOTS; ++track) {
      if (clearRows[track] == LOAD_QUEUE_CLEAR_ROW) {
        hasClear = true;
        break;
      }
    }
    if (hasClear) {
      grid_task.load_queue.put(queueMode, clearRows);
    }
    if (result != nullptr) {
      result->loadQueued = loadQueued;
      result->clearQueued = hasClear;
      result->queued = loadQueued || hasClear;
      result->activeMask = playback_active_mask_;
    }
  } else if (result != nullptr) {
    result->activeMask = playback_active_mask_;
  }
  return any;
}

void MCLArrangement::tick() {
  if (!proj.project_loaded) {
    uint32_t releasedMask = playback_released_mask_;
    flushRuntimePrivateSourceEdits();
    resetPlayback();
    playback_released_mask_ = releasedMask;
    return;
  }
  if (MidiClock.state != MidiClockClass::STARTED) {
    return;
  }
  if (host_playback_suspended_) {
    return;
  }

  const uint8_t activeIdx = mcl_cfg.active_arrangement_idx;
  const uint32_t nowQ12 = currentClockQ12();
  uint32_t startQ12 = nowQ12;
  uint32_t endQ12 = nowQ12 + 1u;

  bool sameArrangement =
      playback_active_ && playback_arrangement_idx_ == activeIdx;
  bool allowLoopTransportSeek = playback_released_mask_ == 0;
  if (allowLoopTransportSeek) {
    // Stored arrangement loops remain active under the temporary UI loop.
    // Check them first so persistent loop wraps are not masked.
    mclarrfile::LoopRegion regions[4];
    uint32_t queryStart = sameArrangement && nowQ12 >= last_tick_q12_
                              ? last_tick_q12_
                              : nowQ12;
    uint32_t queryEnd = nowQ12 + 1u;
    uint16_t regionCount =
        readLoopRegions(queryStart, queryEnd, 0, 4, regions, nullptr, nullptr);
    int activeRegion = -1;
    uint32_t activeWidth = 0xFFFFFFFFu;
    for (uint16_t i = 0; i < regionCount; ++i) {
      const mclarrfile::LoopRegion &region = regions[i];
      if ((region.flags & mclarrfile::LOOP_REGION_ENABLED) == 0 ||
          region.endQ12 <= region.startQ12) {
        continue;
      }
      bool forward = sameArrangement && nowQ12 >= last_tick_q12_;
      bool inside = nowQ12 >= region.startQ12 && nowQ12 < region.endQ12;
      bool crossed = forward && last_tick_q12_ < region.endQ12 &&
                     nowQ12 >= region.endQ12 &&
                     (last_tick_q12_ >= region.startQ12 ||
                      nowQ12 >= region.startQ12);
      if (!inside && !crossed) {
        continue;
      }
      uint32_t width = region.endQ12 - region.startQ12;
      if (activeRegion < 0 || width < activeWidth) {
        activeRegion = i;
        activeWidth = width;
      }
    }

    if (activeRegion >= 0) {
      const mclarrfile::LoopRegion &region = regions[activeRegion];
      uint16_t regionId = region.id != 0 ? region.id : (uint16_t)1;
      if (stored_loop_active_id_ != regionId) {
        stored_loop_active_id_ = regionId;
        stored_loop_repeats_done_ = 0;
      }
      stored_loop_start_q12_ = region.startQ12;
      stored_loop_end_q12_ = region.endQ12;
      stored_loop_count_ = region.repeatCount;
      bool forward = sameArrangement && nowQ12 >= last_tick_q12_;
      bool crossedEnd = forward && last_tick_q12_ < region.endQ12 &&
                        nowQ12 >= region.endQ12;
      bool reachedEnd = nowQ12 >= region.endQ12;
      bool infinite =
          region.repeatCount == mclarrfile::kLoopRegionRepeatInfinite;
      if ((crossedEnd || reachedEnd) &&
          (infinite || stored_loop_repeats_done_ < region.repeatCount)) {
        uint16_t nextRepeatsDone =
            infinite ? stored_loop_repeats_done_
                     : (uint16_t)(stored_loop_repeats_done_ + 1);
        uint32_t tick96 = q12ToHostTick96(region.startQ12);
        MidiClock.set_transport_position(tick96);
        mcl_seq.set_transport_position(tick96);
        sps_host_arr_bridge.notifyArrangementPosition(
            region.startQ12, spsarr::POSITION_NOTIFY_LOOP);
        resetPlaybackForTransport(false);
        stored_loop_active_id_ = regionId;
        stored_loop_repeats_done_ = nextRepeatsDone;
        stored_loop_start_q12_ = region.startQ12;
        stored_loop_end_q12_ = region.endQ12;
        stored_loop_count_ = region.repeatCount;
        seekLoad(region.startQ12, true, false, false);
        return;
      }
    } else if (stored_loop_active_id_ != 0 &&
               stored_loop_end_q12_ > stored_loop_start_q12_) {
      bool forward = sameArrangement && nowQ12 >= last_tick_q12_;
      bool inside = nowQ12 >= stored_loop_start_q12_ &&
                    nowQ12 < stored_loop_end_q12_;
      bool crossedEnd = forward && last_tick_q12_ < stored_loop_end_q12_ &&
                        nowQ12 >= stored_loop_end_q12_;
      bool reachedEnd = nowQ12 >= stored_loop_end_q12_;
      bool infinite =
          stored_loop_count_ == mclarrfile::kLoopRegionRepeatInfinite;
      if ((crossedEnd || reachedEnd) &&
          (infinite || stored_loop_repeats_done_ < stored_loop_count_)) {
        uint16_t nextRepeatsDone =
            infinite ? stored_loop_repeats_done_
                     : (uint16_t)(stored_loop_repeats_done_ + 1);
        uint16_t regionId = stored_loop_active_id_;
        uint32_t startQ12ForLoop = stored_loop_start_q12_;
        uint32_t endQ12ForLoop = stored_loop_end_q12_;
        uint16_t loopCount = stored_loop_count_;
        uint32_t tick96 = q12ToHostTick96(startQ12ForLoop);
        MidiClock.set_transport_position(tick96);
        mcl_seq.set_transport_position(tick96);
        sps_host_arr_bridge.notifyArrangementPosition(
            startQ12ForLoop, spsarr::POSITION_NOTIFY_LOOP);
        resetPlaybackForTransport(false);
        stored_loop_active_id_ = regionId;
        stored_loop_repeats_done_ = nextRepeatsDone;
        stored_loop_start_q12_ = startQ12ForLoop;
        stored_loop_end_q12_ = endQ12ForLoop;
        stored_loop_count_ = loopCount;
        seekLoad(startQ12ForLoop, true, false, false);
        return;
      }
      if (!inside && (nowQ12 < stored_loop_start_q12_ ||
                      nowQ12 >= stored_loop_end_q12_)) {
        stored_loop_active_id_ = 0;
        stored_loop_repeats_done_ = 0;
        stored_loop_start_q12_ = 0;
        stored_loop_end_q12_ = 0;
        stored_loop_count_ = 0;
      }
    } else if (stored_loop_active_id_ != 0) {
      stored_loop_active_id_ = 0;
      stored_loop_repeats_done_ = 0;
      stored_loop_start_q12_ = 0;
      stored_loop_end_q12_ = 0;
      stored_loop_count_ = 0;
    }
  }

  if (allowLoopTransportSeek && loop_enabled_ &&
      loop_end_q12_ > loop_start_q12_) {
    bool insideLoop = nowQ12 >= loop_start_q12_ && nowQ12 < loop_end_q12_;
    if (insideLoop) {
      loop_entered_ = true;
    }
    bool forward = sameArrangement && nowQ12 >= last_tick_q12_;
    bool crossedEnd = forward && last_tick_q12_ < loop_end_q12_ &&
                      nowQ12 >= loop_end_q12_ &&
                      (loop_entered_ || last_tick_q12_ >= loop_start_q12_ ||
                       nowQ12 >= loop_start_q12_);
    bool reachedEnd = loop_entered_ && nowQ12 >= loop_end_q12_;
    if (crossedEnd || reachedEnd) {
      uint32_t tick96 = q12ToHostTick96(loop_start_q12_);
      MidiClock.set_transport_position(tick96);
      mcl_seq.set_transport_position(tick96);
      sps_host_arr_bridge.notifyArrangementPosition(
          loop_start_q12_, spsarr::POSITION_NOTIFY_LOOP);
      resetPlaybackForTransport(false);
      seekLoad(loop_start_q12_, true, false, false);
      return;
    }
    if (!insideLoop && (nowQ12 < loop_start_q12_ || nowQ12 >= loop_end_q12_)) {
      loop_entered_ = false;
    }
  }

  if (sameArrangement) {
    if (nowQ12 == last_tick_q12_) {
      return;
    }
    if (nowQ12 > last_tick_q12_ &&
        nowQ12 - last_tick_q12_ <= kMaxPlaybackCatchupQ12) {
      startQ12 = last_tick_q12_ + 1u;
      endQ12 = nowQ12 + 1u;
    }
  } else {
    playback_active_mask_ = 0;
  }

  playback_active_ = true;
  playback_arrangement_idx_ = activeIdx;
  last_tick_q12_ = nowQ12;
  queueClipStarts(startQ12, endQ12, false, false,
                  LOAD_QUEUE_FLAG_IMMEDIATE, true,
                  kArrangerBoundaryLookaheadQ12);
  tickAutomation(nowQ12);
}

#endif  // MCL_FEATURE_HOST_ARRANGER
