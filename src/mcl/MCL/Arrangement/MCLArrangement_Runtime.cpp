#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

void MCLArrangement::resetPlayback() {
  playback_arrangement_idx_ = 0xFF;
  last_tick_q12_ = 0;
  playback_active_mask_ = 0;
  playback_released_mask_ = 0;
  clip_runtime_fade_mask_ = 0;
  playback_active_ = false;
  loop_entered_ = false;
  stored_loop_active_id_ = 0;
  stored_loop_repeats_done_ = 0;
  stored_loop_start_q12_ = 0;
  stored_loop_end_q12_ = 0;
  stored_loop_count_ = 0;
}

void MCLArrangement::resetPlaybackForTransport(bool clearReleasedTracks) {
  uint32_t releasedMask = playback_released_mask_;
  resetPlayback();
  if (!clearReleasedTracks) {
    playback_released_mask_ = releasedMask;
  }
  grid_task.load_queue.init();
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

  GridSlot firstSource = 255;
  GridSlot sourceBase = (GridSlot)(sourceGridBank * GRID_WIDTH);
  for (uint8_t src = 0; src < GRID_WIDTH && src < 16; ++src) {
    GridSlot sourceSlot = (GridSlot)(sourceBase + src);
    if (sourceSlot >= NUM_SLOTS || ((trackMask >> src) & 1u) == 0 ||
        rows[sourceSlot] >= GRID_LENGTH) {
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
    bool privateLoad = row == LOAD_QUEUE_PRIVATE_ROW &&
                       privateSourceIds != nullptr &&
                       privateSourceIds[sourceSlot] != 0;
    if (row < GRID_LENGTH || privateLoad) {
      GridSlot dst = sourceSlot;
      if (!privateLoad && loadOffset < NUM_SLOTS) {
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
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= NUM_SLOTS ||
        clip.row >= GRID_LENGTH) {
      continue;
    }
    if (clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE &&
        !privateSourceCell(clip.sourceId, nullptr, nullptr)) {
      continue;
    }

    uint64_t clipStart = clip.startQ12;
    uint64_t clipEnd = clipStart + clip.durationQ12;
    if (clipEnd < clipStart) {
      clipEnd = 0xFFFFFFFFull;
    }

    const uint32_t bit = (uint32_t)(1ul << clip.track);
    bool matchedLoadedClip =
        clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE
            ? loadedRows[clip.track] == LOAD_QUEUE_PRIVATE_ROW &&
                  loadedPrivateIds[clip.track] == clip.sourceId
            : loadedRows[clip.track] == clip.row &&
                  loadedSources[clip.track] == mclarrfile::clipSourceSlot(clip);
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
  return armed;
}

bool MCLArrangement::releasePlaybackTracks(uint32_t trackMask) {
  if (trackMask == 0) {
    return false;
  }
  uint32_t oldMask = playback_released_mask_;
  playback_released_mask_ |= trackMask;
  playback_active_mask_ &= ~trackMask;
  clip_runtime_fade_mask_ &= ~trackMask;
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
    return;
  }
  clip_runtime_fades_[dst] = fade;
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
    return false;
  }
  *fade = clip_runtime_fades_[dst];
  return true;
}

bool MCLArrangement::seekLoad(uint32_t positionQ12, bool immediate,
                              bool allowPrestartFade,
                              bool clearReleasedTracks) {
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
  return queueClipStarts(positionQ12, endQ12, true, true, queueFlags,
                         !clearReleasedTracks);
}

bool MCLArrangement::queueClipStarts(uint32_t startQ12, uint32_t endQ12,
                                     bool loadActiveAtPosition,
                                     bool clearInactiveTracks,
                                     uint8_t loadQueueFlags,
                                     bool honorReleasedTracks) {
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
    return false;
  }

  ArrangerLoadGroups loadGroups;
  GridRow clearRows[NUM_SLOTS];
  memset(clearRows, 255, sizeof(clearRows));
  bool any = false;
  bool hasPlayableClip = false;
  uint32_t currentActiveMask = 0;
  uint32_t startMask = 0;
  uint32_t nowQ12 = endQ12 > startQ12 ? endQ12 - 1u : startQ12;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!readClipRecord(file, header, clip)) {
      break;
    }
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= NUM_SLOTS ||
        clip.row >= GRID_LENGTH) {
      continue;
    }
    if (clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE &&
        !privateSourceCell(clip.sourceId, nullptr, nullptr)) {
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
    if ((uint64_t)nowQ12 >= clipStart && (uint64_t)nowQ12 < clipEnd) {
      currentActiveMask |= (uint32_t)(1ul << clip.track);
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
    if (q12InRange(clip.startQ12, startQ12, endQ12)) {
      if (addClipLoad(loadGroups, clip)) {
        startMask |= (uint32_t)(1ul << clip.track);
        TrackLoadFadeData fade;
        if (clipFadeAtPosition(clip, clip.startQ12, false, fade)) {
          armRuntimeFade(clip.track, fade);
        }
        any = true;
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
    return false;
  }

  uint32_t clearBaseMask =
      clearInactiveTracks ? (uint32_t)0xFFFFFFFFul : playback_active_mask_;
  uint32_t clearMask = clearBaseMask & ~currentActiveMask;
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
  playback_active_mask_ = currentActiveMask;

  if (any) {
    uint8_t queueMode = LOAD_ARRANG | loadQueueFlags;
    loadGroups.flush(queueMode);
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
  }
  return any;
}

void MCLArrangement::tick() {
  if (!proj.project_loaded || MidiClock.state != MidiClockClass::STARTED) {
    uint32_t releasedMask = playback_released_mask_;
    resetPlayback();
    playback_released_mask_ = releasedMask;
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
                  LOAD_QUEUE_FLAG_IMMEDIATE, true);
}

#endif  // MCL_FEATURE_HOST_ARRANGER
