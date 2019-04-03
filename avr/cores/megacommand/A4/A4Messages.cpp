
#include "A4Messages.hh"
#include "A4Params.hh"
#include "Elektron.hh"
#include "helpers.h"

#include "A4.h"

#include <array>

bool A4Global::fromSysex(uint8_t *data, uint16_t len) {
  if (len != 0xC4 - 6) {
    //		printf("wrong length\n");
    // wrong length
    return false;
  }

  if (!ElektronHelper::checkSysexChecksum(data, len)) {
    //		printf("wrong checksum\n");
    return false;
  }

  //	origPosition = data[3];
  ElektronSysexDecoder decoder(DATA_ENCODER_INIT(data + 4, len - 4));

  return true;
}

uint16_t A4Global::toSysex(uint8_t *data, uint16_t len) {
  ElektronDataToSysexEncoder encoder(DATA_ENCODER_INIT(data, len));
  return toSysex(encoder);
  if (len < 0xC5)
    return 0;
}

uint16_t A4Global::toSysex(ElektronDataToSysexEncoder &encoder) {
  encoder.stop7Bit();
  encoder.begin();
  encoder.pack(a4_sysex_hdr, sizeof(a4_sysex_hdr));
  encoder.pack8(A4_GLOBAL_MESSAGE_ID);
  encoder.pack8(0x05); // version
  encoder.pack8(0x01); // revision

  encoder.startChecksum();
  // encoder.pack8(origPosition);

  uint16_t enclen = encoder.finish();
  encoder.finishChecksum();

  return enclen + 5;
}

/**
 * 0xF0 [8B prologue] [1B ORIGPOS] 
 * <begin checksum> [8B header] <begin 7bit enc> 
 * {1B BEGIN SOUND} {4B 32-TAGS} {15B + nul NAME}
 * <begin parameters>
 */

static constexpr std::array<uint8_t, 8> a4sound_prologue { 0x00, 0x20, 0x3c, 0x06, 0x00, 0x53, 0x01, 0x01 };
static constexpr std::array<uint8_t, 8> a4sound_header   { 0x78, 0x3e, 0x6f, 0x3a, 0x3a, 0x00, 0x00, 0x00 };

static constexpr size_t a4sound_sysex_len = 415 - 2; // 2 for sysex frame
static constexpr size_t a4sound_origpos_idx = sizeof(a4sound_prologue);
static constexpr size_t a4sound_checksum_startidx = a4sound_origpos_idx + 1;
static constexpr size_t a4sound_encoding_startidx = a4sound_checksum_startidx + sizeof(a4sound_header);

///  !Note, sysex frame wrappers are not included in [data..data+len)
///  !Note, first effective payload byte is origPosition @ 0x08.
///  !Note, checksum starts at 0x09 without origPosition.
bool A4Sound::fromSysex(uint8_t *data, uint16_t len) {
  if (len != a4sound_sysex_len) {
    GUI.setLine(GUI.LINE1);
    GUI.flash_strings_fill("WRONG LEN", "");
    GUI.setLine(GUI.LINE2);

    GUI.put_value16(0, len);
    //  GUI.put_value16(8, 384 - 10 - 2 -2 -1);

    return false;
  }

  if (!ElektronHelper::checkSysexChecksumAnalog(
    data + a4sound_checksum_startidx,
    len - a4sound_checksum_startidx)) {
    GUI.flash_strings_fill("WRONG CKSUM", "");
    return false;
  }

  origPosition = data[a4sound_origpos_idx];

  ElektronSysexDecoder decoder(
    DATA_ENCODER_INIT(
      data + a4sound_encoding_startidx,
      len - a4sound_encoding_startidx - 4));
  decoder.skip(1); // skip sound header 0x05
  decoder.get(tags);
  decoder.get(name);
  decoder.get(osc[0].tuning);
  decoder.get(osc[0].fine);
  decoder.get(osc[1].tuning);
  decoder.get(osc[1].fine);
  decoder.get(osc[0].detune);
  decoder.get(osc[1].detune);
  decoder.get(osc[0].keytrack);
  decoder.get(osc[1].keytrack);
  // hold on...
  return true;
}

uint16_t A4Sound::toSysex() {
  ElektronDataToSysexEncoder encoder(&MidiUart2);
  return toSysex(encoder);
}
uint16_t A4Sound::toSysex(uint8_t *data, uint16_t len) {
  ElektronDataToSysexEncoder encoder(DATA_ENCODER_INIT(data, len));
  if (len < 0xC5)
    return 0;
  return toSysex(encoder);
}

uint16_t A4Sound::toSysex(ElektronDataToSysexEncoder &encoder) {
  encoder.stop7Bit();
  encoder.begin();
  encoder.pack(a4_sysex_hdr, sizeof(a4_sysex_hdr));
  if (!soundpool) {
    encoder.pack8(A4_SOUNDX_MESSAGE_ID);
  } else {
    encoder.pack8(A4_SOUND_MESSAGE_ID);
  }
  encoder.pack(a4_sysex_proto_version, sizeof(a4_sysex_proto_version));
  encoder.pack8(origPosition);

  encoder.startChecksum();
  encoder.pack8(0x78); // unknown
  encoder.pack8(0x3E); // unknown
  encoder.pack((uint8_t *)&payload, sizeof(payload));
  uint16_t enclen = encoder.finish();
  encoder.finishChecksum();

  return enclen + 5;
}

bool A4Kit::fromSysex(uint8_t *data, uint16_t len) {
  if (len != (2679 - 10 - 0)) {
    GUI.setLine(GUI.LINE1);
    GUI.flash_strings_fill("WRONG LEN", "");
    GUI.setLine(GUI.LINE2);

    GUI.put_value16(0, len);
    //  GUI.put_value16(8, 384 - 10 - 2 -2 -1);

    return false;
  }

  if (!ElektronHelper::checkSysexChecksumAnalog(data + 1, len - 1)) {
    GUI.flash_strings_fill("WRONG CKSUM", "");
    return false;
  }
  // origPosition = data[3];

  ElektronSysexDecoder decoder(DATA_ENCODER_INIT(data, len - 4));
  decoder.stop7Bit();
  decoder.get8(&origPosition);
  //    decoder.skip(2);
  decoder.get((uint8_t *)&payload_start, sizeof(payload_start));

  for (uint8_t i = 0; i < 4; i++) {
    decoder.get((uint8_t *)&sounds[i].payload, sizeof(sounds[i].payload));
    sounds[i].origPosition = i;
  }
  decoder.get((uint8_t *)&payload_end, sizeof(payload_end));

  return true;
}

uint16_t A4Kit::toSysex() {
  ElektronDataToSysexEncoder encoder(&MidiUart2);
  return toSysex(encoder);
}
uint16_t A4Kit::toSysex(uint8_t *data, uint16_t len) {
  ElektronDataToSysexEncoder encoder(DATA_ENCODER_INIT(data, len));
  if (len < 0xC5)
    return 0;
  return toSysex(encoder);
}

uint16_t A4Kit::toSysex(ElektronDataToSysexEncoder &encoder) {
  encoder.stop7Bit();
  encoder.begin();
  encoder.pack(a4_sysex_hdr, sizeof(a4_sysex_hdr));
  if (workSpace) {
    encoder.pack8(A4_KITX_MESSAGE_ID);
  } else {
    encoder.pack8(A4_KIT_MESSAGE_ID);
  }
  encoder.pack(a4_sysex_proto_version, sizeof(a4_sysex_proto_version));
  encoder.pack8(origPosition);

  encoder.startChecksum();
  encoder.pack(payload_start, sizeof(payload_start)); // unknow
  for (uint8_t i = 0; i < 4; i++) {

    encoder.pack(sounds[i].payload, sizeof(sounds[i].payload));
  }
  encoder.pack(payload_end, sizeof(payload_end));
  uint16_t enclen = encoder.finish();
  encoder.finishChecksum();

  return enclen + 5;
}
