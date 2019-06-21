/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMEMORY_H__
#define MCLMEMORY_H__

#define NUM_MD_TRACKS    16UL

#ifdef EXT_TRACKS
#define NUM_A4_TRACKS    4UL
#else
#define NUM_A4_TRACKS    0UL
#endif

#define NUM_EXT_TRACKS   NUM_A4_TRACKS

#define NUM_LFO_TRACKS   4UL
#define NUM_TRACKS (NUM_MD_TRACKS + NUM_A4_TRACKS)
#define NUM_FILE_ENTRIES 256UL

//#define MD_TRACK_LEN (sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine))
//#define A4_TRACK_LEN (sizeof(GridTrack) + sizeof(ExtSeqTrackData) + sizeof(A4Sound))

//Use these to produce compiler errors that probes the sizes!
//template<int X> struct __WOW;
//__WOW<sizeof(MDTrackLight)> szmd;
//__WOW<sizeof(A4Track)> sza4;
#pragma message("MD_TRACK_LEN = 501")
#pragma message("A4_TRACK_LEN = 1742")


#define MD_TRACK_LEN (sizeof(MDTrackLight))
#define A4_TRACK_LEN (sizeof(A4Track))

#ifdef EXT_TRACKS
#define EMPTY_TRACK_LEN A4_TRACK_LEN
#else
#define EMPTY_TRACK_LEN MD_TRACK_LEN
#endif

// 16x MD tracks
#define BANK1_MD_TRACKS_START (BANK1_SYSEX2_DATA_START + SYSEX2_DATA_LEN)

// 4x A4 tracks
#define BANK1_A4_TRACKS_START (BANK1_MD_TRACKS_START + MD_TRACK_LEN * NUM_MD_TRACKS)
// 1024x FILE entries
#define BANK1_FILE_ENTRIES_START (BANK1_A4_TRACKS_START + A4_TRACK_LEN * NUM_A4_TRACKS)
#define BANK1_FILE_ENTRIES_END (BANK1_FILE_ENTRIES_START + 16UL * NUM_FILE_ENTRIES)


#pragma message (VAR_NAME_VALUE(NUM_MD_TRACKS))
#pragma message (VAR_NAME_VALUE(NUM_A4_TRACKS))
#pragma message (VAR_NAME_VALUE(NUM_EXT_TRACKS))
#pragma message (VAR_NAME_VALUE(NUM_LFO_TRACKS))
#pragma message (VAR_NAME_VALUE(NUM_TRACKS))
#pragma message (VAR_NAME_VALUE(BANK1_MD_TRACKS_START))
#pragma message (VAR_NAME_VALUE(BANK1_A4_TRACKS_START))
#pragma message (VAR_NAME_VALUE(BANK1_FILE_ENTRIES_START))
#pragma message (VAR_NAME_VALUE(BANK1_FILE_ENTRIES_END))

#endif /* MCLMEMORY_H__ */
