/* Copyright 2026, Justin Mammarella jmamma@gmail.com */

#pragma once

// Platform feature switches for code that is shared between hardware MCL and
// SPS-hosted MCL. Keep feature checks here so AVR builds do not grow because of
// host-only arranger/editor behavior.

#if defined(__AVR__)
#ifndef MCL_FEATURE_HOST_ARRANGER
#define MCL_FEATURE_HOST_ARRANGER 0
#endif
#ifndef MCL_FEATURE_HOST_LOAD_FADE_SEEK
#define MCL_FEATURE_HOST_LOAD_FADE_SEEK 0
#endif
#ifndef MCL_FEATURE_HOST_EXTSTEP_SYNC
#define MCL_FEATURE_HOST_EXTSTEP_SYNC 0
#endif
#ifndef MCL_FEATURE_HOST_GRID_MOVE_UNDO
#define MCL_FEATURE_HOST_GRID_MOVE_UNDO 0
#endif
#ifndef MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
#define MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS 0
#endif
#ifndef MCL_FEATURE_GRID_PRIVATE_LOADS
#define MCL_FEATURE_GRID_PRIVATE_LOADS 0
#endif
#ifndef MCL_FEATURE_GRID_SAVE_QUEUE
#define MCL_FEATURE_GRID_SAVE_QUEUE 0
#endif
#else
#ifndef MCL_FEATURE_HOST_ARRANGER
#define MCL_FEATURE_HOST_ARRANGER 1
#endif
#ifndef MCL_FEATURE_HOST_LOAD_FADE_SEEK
#define MCL_FEATURE_HOST_LOAD_FADE_SEEK 1
#endif
#ifndef MCL_FEATURE_HOST_EXTSTEP_SYNC
#define MCL_FEATURE_HOST_EXTSTEP_SYNC 1
#endif
#ifndef MCL_FEATURE_HOST_GRID_MOVE_UNDO
#define MCL_FEATURE_HOST_GRID_MOVE_UNDO 1
#endif
#ifndef MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
#if defined(PLATFORM_WASM)
#define MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS 1
#else
#define MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS 0
#endif
#endif
#ifndef MCL_FEATURE_GRID_PRIVATE_LOADS
#define MCL_FEATURE_GRID_PRIVATE_LOADS 1
#endif
#ifndef MCL_FEATURE_GRID_SAVE_QUEUE
#define MCL_FEATURE_GRID_SAVE_QUEUE 1
#endif
#endif
