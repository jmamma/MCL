/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMEMORY_H__
#define MCLMEMORY_H__

#define NUM_MD_TRACKS    16
#define NUM_A4_TRACKS    4
#define NUM_EXT_TRACKS   4
#define NUM_LFO_TRACKS   4
#define NUM_TRACKS (NUM_MD_TRACKS + NUM_A4_TRACKS)
#define NUM_FILE_ENTRIES 1024

#define MD_TRACK_LEN (sizeof(MDTrackLight))
#define A4_TRACK_LEN (sizeof(A4Track))

// 16x MD tracks
#define BANK1_MD_TRACKS_START 0x2200
// 4x A4 tracks
#define BANK1_A4_TRACKS_START (BANK1_MD_TRACKS_START + MD_TRACK_LEN * NUM_MD_TRACKS)
// 1024x FILE entries
#define BANK1_FILE_ENTRIES_START (BANK1_A4_TRACKS_START + A4_TRACK_LEN * NUM_A4_TRACKS)
#define BANK1_FILE_ENTRIES_END (BANK1_FILE_ENTRIES_START + 16 * NUM_FILE_ENTRIES)
#endif /* MCLMEMORY_H__ */
