/**
 * Shared SPS private SysEx framing helpers.
 *
 * Protocol-specific headers provide identity bytes and command/body enums.
 * This file only owns the reusable 8->7 packing, checksum, frame
 * build/parse, and little-endian scalar helpers.
 */
#ifndef SPS_WIRE_PROTOCOL_H
#define SPS_WIRE_PROTOCOL_H

#include <stdint.h>

namespace spswire {

static const uint8_t kSysexStart = 0xF0;
static const uint8_t kSysexEnd = 0xF7;
static const uint16_t kFrameMinLen = 7;

struct Parsed {
    uint8_t cmd;
    uint8_t tag;
    const uint8_t* body7;
    uint16_t body7len;
};

inline uint16_t pack7(const uint8_t* in, uint16_t n, uint8_t* out) {
    uint16_t retlen = 0;
    uint16_t cnt7 = 0;
    out[0] = 0;
    for (uint16_t cnt = 0; cnt < n; cnt++) {
        uint8_t c = in[cnt] & 0x7F;
        uint8_t msb = (uint8_t)(in[cnt] >> 7);
        out[0] |= (uint8_t)(msb << (6 - cnt7));
        out[1 + cnt7] = c;
        if (cnt7++ == 6) {
            out += 8;
            retlen += 8;
            out[0] = 0;
            cnt7 = 0;
        }
    }
    return (uint16_t)(retlen + cnt7 + (cnt7 != 0 ? 1 : 0));
}

inline uint16_t unpack7(const uint8_t* in, uint16_t encLen, uint8_t* out,
                        uint16_t outCap) {
    uint16_t cnt2 = 0;
    uint16_t bits = 0;
    for (uint16_t cnt = 0; cnt < encLen; cnt++) {
        if ((cnt % 8) == 0) {
            bits = in[cnt];
        } else {
            bits <<= 1;
            if (cnt2 < outCap)
                out[cnt2] = (uint8_t)(in[cnt] | (bits & 0x80));
            cnt2++;
        }
    }
    return cnt2 < outCap ? cnt2 : outCap;
}

inline uint16_t pack7Size(uint16_t n) {
    return (uint16_t)(((n + 6) / 7) * 8);
}

inline uint16_t unpack7Size(uint16_t encLen) {
    uint16_t full = (uint16_t)((encLen / 8) * 7);
    uint16_t rem = (uint16_t)(encLen % 8);
    return (uint16_t)(full + (rem ? (rem - 1) : 0));
}

inline uint16_t checksum(const uint8_t* p, uint16_t n) {
    uint16_t s = 0;
    for (uint16_t i = 0; i < n; i++)
        s = (uint16_t)(s + p[i]);
    return (uint16_t)(s & 0x3FFF);
}

inline uint16_t buildFrame(uint8_t mfr, uint8_t sub0, uint8_t sub1,
                           uint8_t cmd, uint8_t tag,
                           const uint8_t* body8, uint16_t body8len,
                           uint8_t* out, uint16_t outcap) {
    uint16_t need =
        (uint16_t)(1 + 3 + 2 + pack7Size(body8len) + 2 + 1);
    if (need > outcap)
        return 0;
    uint16_t i = 0;
    out[i++] = kSysexStart;
    out[i++] = mfr;
    out[i++] = sub0;
    out[i++] = sub1;
    out[i++] = cmd;
    out[i++] = tag;
    uint16_t enc = (body8 && body8len) ? pack7(body8, body8len, out + i) : 0;
    i = (uint16_t)(i + enc);
    uint16_t cks = checksum(out + 4, (uint16_t)(2 + enc));
    out[i++] = (uint8_t)((cks >> 7) & 0x7F);
    out[i++] = (uint8_t)(cks & 0x7F);
    out[i++] = kSysexEnd;
    return i;
}

inline bool parseFrame(uint8_t mfr, uint8_t sub0, uint8_t sub1,
                       const uint8_t* msg, uint16_t len, Parsed& out) {
    if (len < kFrameMinLen)
        return false;
    if (msg[0] != mfr || msg[1] != sub0 || msg[2] != sub1)
        return false;
    uint16_t cks = (uint16_t)(((uint16_t)msg[len - 2] << 7) | msg[len - 1]);
    if (checksum(msg + 3, (uint16_t)(len - 2 - 3)) != cks)
        return false;
    out.cmd = msg[3];
    out.tag = msg[4];
    out.body7 = msg + 5;
    out.body7len = (uint16_t)(len - 7);
    return true;
}

inline uint16_t decodeBody(const Parsed& p, uint8_t* body8, uint16_t cap) {
    return unpack7(p.body7, p.body7len, body8, cap);
}

inline void putU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}
inline uint16_t getU16(const uint8_t* p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
inline void putU32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}
inline uint32_t getU32(const uint8_t* p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
inline void putU64(uint8_t* p, uint64_t v) {
    for (int i = 0; i < 8; i++)
        p[i] = (uint8_t)((v >> (8 * i)) & 0xFF);
}
inline uint64_t getU64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= ((uint64_t)p[i]) << (8 * i);
    return v;
}

}  // namespace spswire

#endif  // SPS_WIRE_PROTOCOL_H
