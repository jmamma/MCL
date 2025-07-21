#include "MCLMemory.h"

#ifdef MCL_MEMORY_USE_ARRAYS

// Buffer declarations using the length constants
uint8_t md_cache[MD_CACHE_LEN];
uint8_t aux_cache[AUX_CACHE_LEN];
uint8_t ext_cache[EXT_CACHE_LEN];
uint8_t filebrowser_cache[FILEBROWSER_CACHE_LEN];

#endif
