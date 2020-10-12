/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMEMORY_H__
#define MCLMEMORY_H__

#ifdef MEGACOMMAND
#define EXT_TRACKS
#define LFO_TRACKS
#endif

constexpr size_t NUM_GRIDS = 2;
constexpr size_t GRID_WIDTH = 16;
constexpr size_t GRID_LENGTH = 128;
constexpr size_t GRID_SLOT_BYTES = 4096;

constexpr size_t NUM_SLOTS = GRID_WIDTH * NUM_GRIDS;

constexpr size_t NUM_MD_TRACKS = 16;

#ifdef EXT_TRACKS
constexpr size_t NUM_A4_TRACKS = 6;
constexpr size_t NUM_A4_SOUND_TRACKS = 4;
constexpr size_t NUM_MNM_TRACKS = 6;
constexpr size_t NUM_EXT_TRACKS = 6;
constexpr size_t NUM_AUX_TRACKS = 4;
#else
#pragma message("EXT TRACKS DISABLED")
constexpr size_t NUM_A4_TRACKS = 0;
constexpr size_t NUM_A4_SOUND_TRACKS = 0;
constexpr size_t NUM_MNM_TRACKS = 0;
constexpr size_t NUM_EXT_TRACKS = 0;
#endif

constexpr size_t NUM_FX_TRACKS = 2;
constexpr size_t MDFX_TRACK_NUM = 13; //position of MDFX track in grid
constexpr size_t MDROUTE_TRACK_NUM = 14; //position of MDROUTE track in grid
constexpr size_t MDTEMPO_TRACK_NUM = 15; //position of MDTEMPO track in grid

constexpr size_t NUM_LFO_TRACKS = 2;
constexpr size_t NUM_TRACKS = (NUM_MD_TRACKS + NUM_EXT_TRACKS);
constexpr size_t NUM_FILE_ENTRIES = 256;

// as of commit  33e243afc758081dc6eb244e42ae61e1e0de09c0
// the track sizes are:
// MDTrack 534
// A4Track 1957
// DeviceTrack 7
// EmptyTrack 1957
// FXTrack (TODO) 43

// So we manually allocate the following BANK1 memory regions, with a little bit of headroom:

constexpr size_t GRID1_TRACK_LEN = 534;
constexpr size_t GRID2_TRACK_LEN = 1957;
constexpr size_t FX_TRACK_LEN = 43;
constexpr size_t DEVICE_TRACK_LEN = 7;

//Use these to produce compiler errors that probes the sizes!
template<uint32_t X> struct __SIZE_PROBE;


#ifdef EXT_TRACKS
constexpr size_t EMPTY_TRACK_LEN = GRID2_TRACK_LEN - DEVICE_TRACK_LEN;
#else
constexpr size_t EMPTY_TRACK_LEN = GRID1_TRACK_LEN - DEVICE_TRACK_LEN;
#endif

// 16x MD tracks
// GRID1 tracks start at 0x6B60
constexpr size_t BANK1_MD_TRACKS_START = BANK1_SYSEX2_DATA_START + SYSEX2_DATA_LEN;
// AUX tracks start at 0x8CC0
constexpr size_t BANK1_AUX_TRACKS_START = BANK1_MD_TRACKS_START + GRID1_TRACK_LEN * NUM_MD_TRACKS;
// 6x A4 tracks
// GRID2 tracks start at 0x8D16
constexpr size_t BANK1_A4_TRACKS_START = BANK1_AUX_TRACKS_START + FX_TRACK_LEN * NUM_FX_TRACKS;
 
// 256x file entries (16 bytes each)
// Start at 0xBAF4
constexpr size_t BANK1_FILE_ENTRIES_START = (BANK1_A4_TRACKS_START + GRID2_TRACK_LEN * NUM_A4_TRACKS);
constexpr size_t BANK1_FILE_ENTRIES_END = (BANK1_FILE_ENTRIES_START + 16 * NUM_FILE_ENTRIES);

// At 0xCAF4

#endif /* MCLMEMORY_H__ */
