
#include "A4Messages.hh"
#include "A4Params.hh"
#include "Elektron.hh"
#include "helpers.h"

#include "A4.h"

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

bool A4Sound::fromSysex(uint8_t *data, uint16_t len) {
  if (len != (415 - 10 - 0)) {
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
  decoder.skip(2);
  decoder.get(payload, sizeof(payload));
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
  if (workSpace) {
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
