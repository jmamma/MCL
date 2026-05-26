/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "Arduino.h"
#include "memory.h"
#include "MCLDefines.h"

#define EXT_TRACKS
#define LFO_TRACKS

constexpr size_t NUM_GRIDS = 2;
constexpr size_t GRID_WIDTH = 16;
constexpr size_t GRID_LENGTH = 128;
constexpr size_t GRID_SLOT_BYTES = 4096;

using GridIndex = uint8_t;
using GridColumn = uint8_t;
using GridRow = uint8_t;
using GridSlot = uint8_t;
using GridSpan = uint8_t;

static_assert(GRID_LENGTH < 255,
              "GridRow must hold all project rows, the clipboard row, and 255 sentinel");
static_assert(GRID_WIDTH * NUM_GRIDS < 255,
              "GridSlot must hold the full logical grid width and 255 sentinel");

constexpr size_t PRJ_NAME_LEN = 14;
constexpr size_t PRJ_PATH_LEN = 64;

constexpr size_t NUM_SLOTS = GRID_WIDTH * NUM_GRIDS;

constexpr size_t NUM_MD_TRACKS = 16;

constexpr size_t NUM_LINKS = 8;

#ifdef EXT_TRACKS
constexpr size_t NUM_A4_TRACKS = 6;
constexpr size_t NUM_A4_SOUND_TRACKS = 4;
constexpr size_t NUM_MNM_TRACKS = 6;
constexpr size_t NUM_EXT_TRACKS = 6;
#else
#pragma message("EXT TRACKS DISABLED")
constexpr size_t NUM_A4_TRACKS = 0;
constexpr size_t NUM_A4_SOUND_TRACKS = 0;
constexpr size_t NUM_MNM_TRACKS = 0;
constexpr size_t NUM_EXT_TRACKS = 0;
#endif


constexpr size_t NUM_INSTRUMENT_TRACKS = (NUM_MD_TRACKS + NUM_EXT_TRACKS);

constexpr size_t NUM_AUX_TRACKS = 3;

constexpr size_t GRIDCHAIN_TRACK_NUM = 10;
constexpr size_t LEGACY_PERF_TRACK_NUM = 11;
constexpr size_t MDFX_TRACK_NUM = 12; //position of MDFX track in grid
constexpr size_t MDLFO_TRACK_NUM = 13; // legacy MD master LFO grid slot
constexpr size_t PERF_TRACK_NUM = 13;
constexpr size_t MDROUTE_TRACK_NUM = 14; //position of MDROUTE track in grid
constexpr size_t MDTEMPO_TRACK_NUM = 15; //position of MDTEMPO track in grid

constexpr size_t NUM_GRID_X_LFO_TRACKS = NUM_MD_TRACKS;
constexpr size_t NUM_GRID_Y_LFO_TRACKS = NUM_EXT_TRACKS;

#if defined(__AVR__)
constexpr uint8_t EXT_NOTE_CLIP_MAX_NOTES = 128;
#else
constexpr uint8_t EXT_NOTE_CLIP_MAX_NOTES = 255;
#endif

constexpr size_t NUM_PERF_PARAMS = 16;
constexpr size_t NUM_SCENES = 8;

constexpr size_t NUM_TRIG_CONDITIONS = 14;
constexpr size_t NUM_LOCKS = 8;
// as of commit  33e243afc758081dc6eb244e42ae61e1e0de09c0
// the track sizes are:
// GridTrack 7
// DeviceTrack 7
// MDTrack 534, plus SeqTrackModData 51, plus native swing storage 9
// ExtTrack 1754
// A4Track 2094
// EmptyTrack 2094
//
// MDLFOTrack 226
// MDRouteTrack 39
// MDFXTrack 43
// MDTempoTrack 11

#if !defined(MEMORY_ALIGN)
#define MEMORY_ALIGN(size) (size)  // for avr, dont align
#endif

constexpr size_t GRID1_TRACK_LEN = MEMORY_ALIGN(597); // MDTrack + SeqTrackModData + swing
#if !defined(__AVR__)
// Non-AVR grid-2 cache slots can carry enhanced MIDI/TBD tracks. Hosted
// builds also need the enlarged slot because native pointers grow several
// MCL structs past the 2128-byte budget that AVR assumes.
constexpr size_t GRID2_TRACK_LEN = MEMORY_ALIGN(GRID_SLOT_BYTES);
#else
constexpr size_t GRID2_TRACK_LEN = MEMORY_ALIGN(2128);
#endif

constexpr size_t DEVICE_TRACK_LEN = MEMORY_ALIGN(7);
constexpr size_t MDLFO_TRACK_LEN = MEMORY_ALIGN(226);
constexpr size_t MDROUTE_TRACK_LEN = MEMORY_ALIGN(39);
constexpr size_t MDFX_TRACK_LEN = MEMORY_ALIGN(43);
constexpr size_t MDTEMPO_TRACK_LEN = MEMORY_ALIGN(11);
constexpr size_t PERF_TRACK_LEN = MEMORY_ALIGN(493);
constexpr size_t GRIDCHAIN_TRACK_LEN = MEMORY_ALIGN(551);

#ifdef EXT_TRACKS
constexpr size_t EMPTY_TRACK_LEN = GRID2_TRACK_LEN - DEVICE_TRACK_LEN;
#else
constexpr size_t EMPTY_TRACK_LEN = GRID1_TRACK_LEN - DEVICE_TRACK_LEN;
#endif

// Total size of main grid cache for MD tracks
constexpr size_t MD_CACHE_LEN = GRID1_TRACK_LEN * NUM_MD_TRACKS;

#ifdef MCL_HAS_SPSX_TRACKS
// SPSX tracks carry SPSMachine (34 params + 2 LFOs) + SeqDataUnion + extras.
// Sized with headroom; static_assert in SPSXTrack.h enforces fit.
constexpr size_t SPSX_TRACK_LEN = MEMORY_ALIGN(1152);
constexpr size_t SPSX_CACHE_LEN = SPSX_TRACK_LEN * NUM_MD_TRACKS;
#endif

#if defined(PLATFORM_TBD)
// DEV1 TBD tracks carry StepSeqTrackData plus the authoritative P4 sound map.
constexpr size_t TBD_TRACK_LEN = MEMORY_ALIGN(2048);
constexpr size_t TBD_CACHE_LEN = TBD_TRACK_LEN * 16;
#endif

// Total size of auxiliary tracks cache
constexpr size_t AUX_CACHE_LEN = GRIDCHAIN_TRACK_LEN +
                                PERF_TRACK_LEN +
                                MDLFO_TRACK_LEN +
                                MDROUTE_TRACK_LEN +
                                MDFX_TRACK_LEN +
                                MDTEMPO_TRACK_LEN;

// Total size of A4 tracks cache
constexpr size_t EXT_CACHE_LEN = GRID2_TRACK_LEN * NUM_EXT_TRACKS;
constexpr size_t FILEBROWSER_CACHE_LEN = 0x2000;

constexpr size_t NUM_FILE_ENTRIES = 256;
constexpr size_t FILE_ENTRY_SIZE = 32;

#ifdef MCL_MEMORY_USE_ARRAYS
/******************************************************************
 * MEMORY LAYOUT: Using array buffers
 ******************************************************************/
// Declare the cache buffers that will be defined in a .cpp file.
extern uint8_t md_cache[MD_CACHE_LEN];
extern uint8_t aux_cache[AUX_CACHE_LEN];
extern uint8_t ext_cache[EXT_CACHE_LEN];
extern uint8_t filebrowser_cache[FILEBROWSER_CACHE_LEN];
#ifdef MCL_HAS_SPSX_TRACKS
extern uint8_t spsx_cache[SPSX_CACHE_LEN];
#endif
#if defined(PLATFORM_TBD)
extern uint8_t tbd_cache[TBD_CACHE_LEN];
#endif

// Declare the start addresses as external constants. They are not `constexpr` because their
// values (the array addresses) are resolved at link-time. We use `uintptr_t` as it is
// the correct integer type for holding pointer values.
extern const uintptr_t BANK1_MD_TRACKS_START;
extern const uintptr_t BANK1_AUX_TRACKS_START;
extern const uintptr_t BANK1_EXT_TRACKS_START;
#ifdef MCL_HAS_SPSX_TRACKS
extern const uintptr_t BANK1_SPSX_TRACKS_START;
#endif
#if defined(PLATFORM_TBD)
extern const uintptr_t BANK1_TBD_TRACKS_START;
#endif

extern const uintptr_t BANK1_GRIDCHAIN_TRACK_START;
extern const uintptr_t BANK1_PERF_TRACK_START;
extern const uintptr_t BANK1_MDLFO_TRACK_START;
extern const uintptr_t BANK1_MDROUTE_TRACK_START;
extern const uintptr_t BANK1_MDFX_TRACK_START;
extern const uintptr_t BANK1_MDTEMPO_TRACK_START;

extern const uintptr_t BANK3_FILE_ENTRIES_START;
extern const uintptr_t BANK3_FILE_ENTRIES_END;

#else
/******************************************************************
 * MEMORY LAYOUT: Using fixed hardware addresses
 ******************************************************************/
// When using fixed addresses, they are known at compile time and can be `constexpr`.
constexpr size_t BANK1_MD_TRACKS_START = (BANK1_SYSEX3_DATA_START + SYSEX3_DATA_LEN);
constexpr size_t BANK1_AUX_TRACKS_START = BANK1_MD_TRACKS_START + MD_CACHE_LEN;
constexpr size_t BANK1_EXT_TRACKS_START = BANK1_AUX_TRACKS_START + AUX_CACHE_LEN;
#if defined(PLATFORM_TBD)
constexpr size_t BANK1_TBD_TRACKS_START = BANK1_EXT_TRACKS_START + EXT_CACHE_LEN;
#endif

// Define track starts as offsets within the contiguous auxiliary region.
constexpr size_t BANK1_GRIDCHAIN_TRACK_START = BANK1_AUX_TRACKS_START;
constexpr size_t BANK1_PERF_TRACK_START = BANK1_GRIDCHAIN_TRACK_START + GRIDCHAIN_TRACK_LEN;
constexpr size_t BANK1_MDLFO_TRACK_START = BANK1_PERF_TRACK_START + PERF_TRACK_LEN;
constexpr size_t BANK1_MDROUTE_TRACK_START = BANK1_MDLFO_TRACK_START + MDLFO_TRACK_LEN;
constexpr size_t BANK1_MDFX_TRACK_START = BANK1_MDROUTE_TRACK_START + MDROUTE_TRACK_LEN;
constexpr size_t BANK1_MDTEMPO_TRACK_START = BANK1_MDFX_TRACK_START + MDFX_TRACK_LEN;

// Bank 3 definitions for file entries.
constexpr size_t BANK3_FILE_ENTRIES_START = (BANK3_START);
constexpr size_t BANK3_FILE_ENTRIES_END = (BANK3_END);
#endif
