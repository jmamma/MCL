#include "MCLMemory.h"

#ifdef MCL_MEMORY_USE_ARRAYS

// The existing cache array definitions
uint8_t md_cache[MD_CACHE_LEN];
uint8_t aux_cache[AUX_CACHE_LEN];
uint8_t ext_cache[EXT_CACHE_LEN];
uint8_t filebrowser_cache[FILEBROWSER_CACHE_LEN];

// Base addresses of the main cache regions
const uintptr_t BANK1_MD_TRACKS_START = reinterpret_cast<uintptr_t>(md_cache);
const uintptr_t BANK1_AUX_TRACKS_START = reinterpret_cast<uintptr_t>(aux_cache);
const uintptr_t BANK1_EXT_TRACKS_START = reinterpret_cast<uintptr_t>(ext_cache);

// Start addresses of individual tracks within the contiguous aux_cache region
const uintptr_t BANK1_GRIDCHAIN_TRACK_START = BANK1_AUX_TRACKS_START;
const uintptr_t BANK1_PERF_TRACK_START      = BANK1_GRIDCHAIN_TRACK_START + GRIDCHAIN_TRACK_LEN;
const uintptr_t BANK1_MDLFO_TRACK_START     = BANK1_PERF_TRACK_START + PERF_TRACK_LEN;
const uintptr_t BANK1_MDROUTE_TRACK_START   = BANK1_MDLFO_TRACK_START + MDLFO_TRACK_LEN;
const uintptr_t BANK1_MDFX_TRACK_START      = BANK1_MDROUTE_TRACK_START + MDROUTE_TRACK_LEN;
const uintptr_t BANK1_MDTEMPO_TRACK_START   = BANK1_MDFX_TRACK_START + MDFX_TRACK_LEN;

// Address for the file browser cache
const uintptr_t BANK3_FILE_ENTRIES_START = reinterpret_cast<uintptr_t>(filebrowser_cache);
const uintptr_t BANK3_FILE_ENTRIES_END   = BANK3_FILE_ENTRIES_START + FILEBROWSER_CACHE_LEN;

#endif
