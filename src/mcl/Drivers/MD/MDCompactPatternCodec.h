#pragma once

#include <inttypes.h>

#if !defined(__AVR__)

class ElektronDataToSysexEncoder;
class MDPattern;
class SysexView;

/*
 * SPS-X pattern 0x41 is the canonical compact pattern body carried by the
 * normal Machinedrum 0x67 envelope. 0x40 remains the established MCL/SPS-X
 * compatibility format implemented by MDPattern itself.
 */
bool md_decode_compact_pattern_41(MDPattern& pattern,
                                  const SysexView& sysex);

uint16_t md_encode_compact_pattern_41(
    MDPattern& pattern,
    ElektronDataToSysexEncoder* encoder);

#endif
