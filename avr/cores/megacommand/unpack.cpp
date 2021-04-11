#include "unpack.h"

#define SHL_WITH_0(x) ((uint16_t)(x)*2)
#define SHL_WITH_1(x) ((uint16_t)(x)*2 + 1)
#define SHL_WITH(y, x) ((uint16_t)(x)*2 + (y))
#define LO(x) ((unsigned char)x)
#define HI(x) ((unsigned char)(x >> 8))
#define HI_BIT0(x) (HI(x) & 1)
#define COPY(dst, src) *dst = *src

static byte shift(uint16_t &state, byte *&src) {
  state = SHL_WITH_0(state);
  if (LO(state) == 0) {
    state = SHL_WITH_1(*src++);
  }
  return HI_BIT0(state);
}

int16_t shift_len(uint16_t &state, byte *&src) {
  int16_t len = 1;
  while (!HI_BIT0(state)) {
    auto bit = shift(state, src);
    len = SHL_WITH(bit, len);
    shift(state, src);
  }
  return len;
}

size_t unpack(uint8_t* src, uint8_t* dst) {
  uint16_t state = 0;
  uint16_t dst_lookback = 1;

  src = src + 8;
  while (true) {
    shift(state, src);
    while (!HI_BIT0(state)) {
      auto idx = shift_len(state, src);
      if (idx != 2) {
        dst_lookback = ((uint16_t)*src++ - 0x300) + idx * 0x100 + 1;
        if (dst_lookback == 0) {
          return 0;
        }
      }

      auto bit1 = shift(state, src);
      auto bit2 = shift(state, src);
      auto len = SHL_WITH(bit2, bit1);
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
    COPY(dst++, src++);
  }
}
