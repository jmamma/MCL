// inttypes.h — minimal stub. MCL's only use is the PRI* macros for
// printf-format strings; provide just those, sized for wasm32 (int = 32 bit,
// long long = 64 bit).
#pragma once

#include <stdint.h>

#define PRId8   "d"
#define PRIu8   "u"
#define PRIx8   "x"
#define PRId16  "d"
#define PRIu16  "u"
#define PRIx16  "x"
#define PRId32  "d"
#define PRIu32  "u"
#define PRIx32  "x"
#define PRId64  "lld"
#define PRIu64  "llu"
#define PRIx64  "llx"
