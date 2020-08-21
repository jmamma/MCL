/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMEMORY_H__
#define MCLMEMORY_H__

#ifdef MEGACOMMAND
#define EXT_TRACKS
#define LFO_TRACKS
#endif

#define NUM_GRIDS 2
#define GRID_WIDTH 16
#define GRID_LENGTH 128
#define GRID_SLOT_BYTES 4096

#define NUM_SLOTS GRID_WIDTH * NUM_GRIDS

#define NUM_MD_TRACKS    16UL

#ifdef EXT_TRACKS
#define NUM_A4_TRACKS    6UL
#else
#pragma message("EXT TRACKS DISABLED")
#define NUM_A4_TRACKS    0UL
#endif

#define NUM_EXT_TRACKS   NUM_A4_TRACKS

#define NUM_LFO_TRACKS   2UL
#define NUM_TRACKS (NUM_MD_TRACKS + NUM_A4_TRACKS)
#define NUM_FILE_ENTRIES 256UL

#include "MDSeqTrack.h"
#include "ExtSeqTrack.h"
#include "MD.h"

#define MD_TRACK_LEN (sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine))
#define A4_TRACK_LEN (sizeof(GridTrack) + sizeof(ExtSeqTrackData) + sizeof(A4Sound))

//Use these to produce compiler errors that probes the sizes!
//template<int X> struct __WOW;
//#pragma message("MD_TRACK_LEN = 520")
//#pragma message("A4_TRACK_LEN = 1752")


#ifdef EXT_TRACKS
#define EMPTY_TRACK_LEN A4_TRACK_LEN
#else
#define EMPTY_TRACK_LEN MD_TRACK_LEN
#endif

// 16x MD tracks
// GRID1 tracks start at 0x9330
#define BANK1_MD_TRACKS_START (BANK1_SYSEX2_DATA_START + SYSEX2_DATA_LEN)

// 6x A4 tracks
// GRID2 tracks start at 0xB190
#define BANK1_A4_TRACKS_START (BANK1_MD_TRACKS_START + MD_TRACK_LEN * NUM_MD_TRACKS)

// 256x file entries (16 bytes each)
// Start at 0xECB8
#define BANK1_FILE_ENTRIES_START (BANK1_A4_TRACKS_START + A4_TRACK_LEN * NUM_A4_TRACKS)
#define BANK1_FILE_ENTRIES_END (BANK1_FILE_ENTRIES_START + 16UL * NUM_FILE_ENTRIES)

// At 0xFCB8

#endif /* MCLMEMORY_H__ */
