/* Justin Mammarella jmamma@gmail.com 2026 */

#pragma once

/*
 * Single source of truth for MCL feature gates.
 *
 * Two kinds of macros are defined here:
 *
 *   1. Semantic feature flags (MCL_HAS_*) that describe WHAT the platform
 *      can do, rather than WHICH platform it is. Source code should
 *      prefer these for conditional compilation when the gated logic is
 *      really about capability presence.
 *
 *   2. The legacy platform macros (__AVR__, PLATFORM_TBD, EXT_TRACKS,
 *      LFO_TRACKS) stay as-is — they are still used widely, and some
 *      genuinely express platform-only concerns (TBD UI, AVR memory
 *      layout). The feature flags below are derived from them.
 *
 * When adding a new conditional, ask: is the gate about a capability
 * (use MCL_HAS_*) or about a platform-specific concern (use the legacy
 * macro). If a feature appears on more than one platform / config, the
 * semantic flag is the right choice.
 */

#include "platform.h"

/*
 * Device capability layer (DeviceMixerCapability and friends with virtual
 * dispatch) — present on every non-AVR build. On AVR, drivers use direct
 * calls into MD / ext tracks.
 */
#if !defined(__AVR__)
#define MCL_HAS_DEVICE_CAPABILITIES 1
#endif

/*
 * Generic ext-step tracks: secondary devices can host a generic
 * SeqExtStepTrackApi that wraps either ExtSeqTrack (A4/MNM/Generic) or
 * MidiSeqTrack (TBD). On AVR there is only ExtSeqTrack with no runtime
 * dispatch.
 */
#if !defined(__AVR__)
#define MCL_HAS_GENERIC_EXT_STEP_TRACKS 1
#endif

/*
 * SPSX track storage (extended MD track + per-track sound) — non-AVR
 * only. The legacy MDSeqTrack stays available everywhere.
 */
#if !defined(__AVR__)
#define MCL_HAS_SPSX_TRACKS 1
#endif

/*
 * TBD driver: P4 sound integration and multi-page UI overlays. Platform-only.
 */
#ifdef PLATFORM_TBD
#define MCL_HAS_TBD_DRIVER 1
#endif

/*
 * Extended panel input: a platform supplies the TBD-style physical button
 * surface (cluster buttons, transport row, arrows, trigs), and MCL maps it
 * into MDX command / note events before the active page handles input.
 */
#if defined(PLATFORM_TBD) || defined(PLATFORM_DESKTOP) || defined(PLATFORM_WASM)
#define MCL_HAS_EXTENDED_PANEL_INPUT 1
#endif

/*
 * MidiSeqTrack (enhanced generic MIDI sequencer with 14-bit locks) —
 * available on non-AVR builds. TBD layers P4 sound integration on top.
 */
#if !defined(__AVR__)
#define MCL_HAS_MIDI_SEQ_TRACKS 1
#endif

/*
 * P4 sound data (TbdP4SoundData, descriptor tables, lock-param
 * routing) — TBD only.
 */
#ifdef PLATFORM_TBD
#define MCL_HAS_P4_SOUND 1
#endif
