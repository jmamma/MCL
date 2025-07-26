#include "unpack.h"

#define SHL_WITH_0(x) ((uint16_t)(x)*2)
#define SHL_WITH_1(x) ((uint16_t)(x)*2 + 1)
#define SHL32_WITH(y, x) ((int32_t)(x)*2 + (y))
#define LO(x) ((unsigned char)x)
#define HI(x) ((unsigned char)(x >> 8))
#define HI_BIT0(x) (HI(x) & 1)
#define COPY(dst, src) *dst = *src

static byte shift(uint16_t &state, byte *&src) {
  state = SHL_WITH_0(state);
  if (LO(state) == 0) {
    state = SHL_WITH_1(pgm_read_byte(src));
    ++src;
  }
  return HI_BIT0(state);
}

int32_t shift_len(uint16_t &state, byte *&src) {
  int32_t len = 1;
  while (!HI_BIT0(state)) {
    auto bit = shift(state, src);
    len = SHL32_WITH(bit, len);
    shift(state, src);
  }
  return len;
}

size_t unpack(uint8_t* src, uint8_t* dst) {
  uint16_t state = 0;
  uint16_t dst_lookback = 1;
  uint8_t* orig_src = src;
  uint8_t* orig_dst = dst;

  while (true) {
    shift(state, src);
    while (!HI_BIT0(state)) {
      auto idx = shift_len(state, src);
      if (idx != 2) {
        uint16_t v = pgm_read_byte(src);
        ++src;
        dst_lookback = (v - 0x300) + idx * 0x100 + 1;
        if (dst_lookback == 0) {
          size_t src_len = src - orig_src;
          size_t dst_len = dst - orig_dst;
          return dst_len;
        }
      }

      auto bit1 = shift(state, src);
      auto bit2 = shift(state, src);
      auto len = SHL32_WITH(bit2, bit1);
      if (len == 0) {
        len = shift_len(state, src) + 2;
      }
      len = len + 1 + (0xd00 < dst_lookback);

      // copy from lookback
      for (byte *p = dst - dst_lookback; len != 0; len--) {
        COPY(dst++, p++);
      }

      shift(state, src);
    }
    *dst++ = pgm_read_byte(src);
    ++src;
  }
}
