#include "inttypes.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define R(x) (*(int32_t *)x)

typedef uint8_t byte;
typedef uint32_t uint;

struct unpack_stats {
  uint32_t os_len = 0;
  uint32_t fw_len = 0;
  uint32_t max_seg = 0;
};

unpack_stats fw_stats;

struct load_stats {
  uint32_t length = 0;
  uint32_t starting_address = 0;
};

load_stats dsp_stats;

#define SHL_WITH_0(x) ((uint)(x)*2)
#define SHL_WITH_1(x) ((uint)(x)*2 + 1)
#define SHL_WITH(y, x) ((uint)(x)*2 + (y))
#define LO(x) ((char)x)
#define HI(x) ((char)(x >> 8))
#define HI_BIT0(x) (HI(x) & 1)
#define COPY(dst, src) *dst = *src

byte shift(uint &state, byte *&src) {
  state = SHL_WITH_0(state);
  if (LO(state) == 0) {
    // printf("shift   %02X\n", *src);
    // hint: the last LO bit is shift to HI, was the sync bit.
    // discard, and load mask from the stream
    state = SHL_WITH_1(*src++);
  }
  return HI_BIT0(state);
}

int shift_len(uint &state, byte *&src) {
  int len = 1;
  while (!HI_BIT0(state)) {
    auto bit = shift(state, src);
    len = SHL_WITH(bit, len);
    shift(state, src);
  }
  return len;
}

#pragma warning(disable : 4996)

uint32_t unpack(byte *src, byte *dst) {
  // state is a 8-bit shift register
  uint state = 0;
  uint dst_lookback = 1;
  uint iter = 0;

  byte *orig_src = src;
  byte *orig_dst = dst;
  fw_stats.max_seg = 0;

  while (true) {
    shift(state, src);
    while (!HI_BIT0(state)) {

      ++iter;
      auto idx = shift_len(state, src);

      if (idx != 2) {

        // printf("idx     %02X\n", idx);
        // printf("adj     %02X\n", *src);
        dst_lookback = ((uint)*src++ - 0x300) + idx * 0x100 + 1;
        if (dst_lookback == 0) {
          // printf("iter = %d\n", iter);
          // printf("*src = %02X, idx = %d\n", *(src - 1), idx);
          fw_stats.fw_len = src - orig_src;
          fw_stats.os_len = dst - orig_dst;
          printf("Compressed length: %d\n", fw_stats.fw_len);
          printf("Expanded length: %d\n", fw_stats.os_len);
          printf("Maximum Segment length: %d\n", fw_stats.max_seg);
          return 0;
        }
      }

      auto bit1 = shift(state, src);
      auto bit2 = shift(state, src);
      auto len = SHL_WITH(bit2, bit1);
      if (len == 0) {
        len = shift_len(state, src) + 2;
        // printf("decoded len = %02X\n", len);
      }
      len = len + 1 + (0xd00 < dst_lookback);
      auto target = dst - orig_dst;
      auto source = target - dst_lookback;
      // printf("%08X <- %08X \t [%d]\n", target, source, len);

      fw_stats.max_seg = std::max(fw_stats.max_seg, len);
      // copy from lookback
      for (byte *p = dst - dst_lookback; len != 0; len--) {
        COPY(dst++, p++);
      }

      shift(state, src);
    }
    // plain copy

    // printf("plain   %02X\n", *src);
    COPY(dst++, src++);
  }
}

int main(int argc, char *argv[]) {
  FILE *fp = fopen(argv[1], "rb");
  fseek(fp, 0, SEEK_END);
  long sz_in = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  byte* buf_in = (byte*)malloc(sz_in);
  fread(buf_in, sz_in, 1, fp);
  fclose(fp);
  printf("read %d bytes from input %s\n", sz_in, argv[1]);
  byte* buf_out = (byte*)malloc(8 << 20);
  unpack(buf_in, buf_out);
  return 0;
}
