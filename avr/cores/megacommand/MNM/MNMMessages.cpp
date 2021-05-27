#include "MCL_impl.h"
#ifdef HOST_MIDIDUINO
#include <stdio.h>
#endif

bool MNMGlobal::fromSysex(MidiClass *midi) {
  uint16_t offset = 5;
  uint16_t len = midi->midiSysex.get_recordLen() - 5;

  //DEBUG_PRINT_FN();

  if (!ElektronHelper::checkSysexChecksum(midi, offset, len)) {
    return false;
  }

  //DEBUG_PRINT_FN();
  //for(int i=0;i<midi->midiSysex.get_recordLen(); ++i) {
    //DEBUG_PRINTLN(midi->midiSysex.getByte(i));
  //}

  origPosition = midi->midiSysex.getByte(offset+3);
  MNMSysexDecoder decoder(midi, offset + 4);

  decoder.get(&autotrackChannel, 5);
  /*
          autotrackChannel = udata[1];
          baseChannel = udata[2];
          channelSpan = udata[3];
          multitrigChannel = udata[4];
          multimapChannel = udata[5];
  */

  uint8_t byte = 0;
  decoder.get8(&byte);

  clockIn = IS_BIT_SET(byte, 0);
  clockOut = IS_BIT_SET(byte, 5);
  ctrlIn = IS_BIT_SET(byte, 4);
  ctrlOut = IS_BIT_SET(byte, 6);

  decoder.get8(&transportIn);
  decoder.get8(&sequencerOut);
  decoder.get8(&arpOut);
  decoder.get8(&keyboardOut);
  decoder.get8(&transportOut);
  decoder.get8(&midiClockOut);
  decoder.get8(&pgmChangeOut);

  decoder.get(&note, 5); // note, gate, sense, minvel, maxvel

  decoder.get(midiMachineChannels, 6);
  decoder.get((uint8_t *)ccDestinations, 6 * 4);
  decoder.get(midiSeqLegato, 6);
  decoder.get(legato, 6);

  decoder.get(mapRange, 32 * 6);
  decoder.get8(&globalRouting);
  decoder.getb(&pgmChangeIn);
  decoder.get(unused, 6);
  decoder.get32(&baseFreq);

  return true;
}

bool MNMGlobal::fromSysex(uint8_t* buf, uint16_t len) {
  // XXX not implemented
  return false;
}

uint16_t MNMGlobal::toSysex(ElektronDataToSysexEncoder *encoder) {

  encoder->stop7Bit();
  encoder->begin();
  encoder->pack(monomachine_sysex_hdr, sizeof(monomachine_sysex_hdr));
  encoder->pack8(MNM_GLOBAL_MESSAGE_ID);
  encoder->pack8(0x03); // version
  encoder->pack8(0x01); // revision
  encoder->startChecksum();
  encoder->pack8(origPosition);
  encoder->start7Bit();

  encoder->pack(&autotrackChannel, 5);

  uint8_t byte = 0;
  if (clockIn)
    SET_BIT(byte, 0);
  if (ctrlIn)
    SET_BIT(byte, 4);
  if (clockOut)
    SET_BIT(byte, 5);
  if (ctrlOut)
    SET_BIT(byte, 6);
  encoder->pack8(byte);

  encoder->pack8(transportIn);
  encoder->pack8(sequencerOut);
  encoder->pack8(arpOut);
  encoder->pack8(keyboardOut);
  encoder->pack8(transportOut);
  encoder->pack8(midiClockOut);
  encoder->pack8(pgmChangeOut);

  encoder->pack(&note, 5);

  encoder->pack(midiMachineChannels, 6);
  encoder->pack((uint8_t *)ccDestinations, 6 * 4);
  encoder->pack(midiSeqLegato, 6);
  encoder->pack(legato, 6);

  encoder->pack(mapRange, 32 * 6);
  encoder->pack8(globalRouting);
  encoder->packb(pgmChangeIn);
  encoder->pack(unused, 6);
  encoder->pack32(baseFreq);

  uint16_t enclen = encoder->finish();
  encoder->finishChecksum();

  return enclen + 5;
}

uint16_t MNMGlobal::toSysex(uint8_t *sysex, uint16_t len) {
  // TODO not implemented
  return 0;
}

bool MNMKit::fromSysex(MidiClass *midi) {
  DEBUG_PRINT_FN();
  uint16_t offset = 5;
  uint16_t len = midi->midiSysex.get_recordLen() - 5;

  if (!ElektronHelper::checkSysexChecksum(midi, offset, len)) {
#ifdef MIDIDUINO
    GUI.flash_strings_fill("WRONG CHECK", "");
#endif
    return false;
  }

  origPosition = midi->midiSysex.getByte(3 + offset);
  MNMSysexDecoder decoder(midi, offset + 4);
  decoder.get((uint8_t *)name, 11);
  name[11] = '\0';

  decoder.get(levels, 6);
  decoder.get((uint8_t *)parameters, 6 * 72);
  decoder.get(models, 6);
  decoder.get(types, 6);
  decoder.get8((uint8_t*)&patchBusIn); // unused byte
  decoder.get16(&patchBusIn);
  decoder.get8(&mirrorLR);
  decoder.get8(&mirrorUD);

  decoder.get((uint8_t *)destPages, 6 * 6 * 2);
  decoder.get((uint8_t *)destParams, 6 * 6 * 2);
  decoder.get((uint8_t *)destRanges, 6 * 6 * 2);
  decoder.get8(&lpKeyTrack);
  decoder.get8(&hpKeyTrack);

  decoder.get8(&trigPortamento);
  decoder.get(trigTracks, 6);
  decoder.get8(&trigLegatoAmp);
  decoder.get8(&trigLegatoFilter);
  decoder.get8(&trigLegatoLFO);

  decoder.get8(&commonMultimode);
  uint8_t byte;
  decoder.get8(&byte);
  if (byte == 0) {
    commonTiming = 0;
  } else {
    commonTiming = 1 << (1 - byte);
  }
  decoder.get8(&splitKey);
  decoder.get8(&splitRange);
  DEBUG_PRINTLN(F("mnm kit okay"));
  return true;
}

bool MNMKit::fromSysex(uint8_t *data, uint16_t len) {
  if (!ElektronHelper::checkSysexChecksum(data, len)) {
#ifdef MIDIDUINO
    GUI.flash_strings_fill("WRONG CHECK", "");
#endif
    return false;
  }

  origPosition = data[3];
  MNMSysexDecoder decoder(data + 4);
  decoder.get((uint8_t *)name, 16);
  name[16] = '\0';

  decoder.get(levels, 6);
  decoder.get((uint8_t *)parameters, 6 * 72);
  decoder.get(models, 6);
  decoder.get(types, 6);
  decoder.get16(&patchBusIn);
  decoder.get8(&mirrorLR);
  decoder.get8(&mirrorUD);

  decoder.get((uint8_t *)destPages, 6 * 6 * 2);
  decoder.get((uint8_t *)destParams, 6 * 6 * 2);
  decoder.get((uint8_t *)destRanges, 6 * 6 * 2);
  decoder.get8(&lpKeyTrack);
  decoder.get8(&hpKeyTrack);

  decoder.get8(&trigPortamento);
  decoder.get(trigTracks, 6);
  decoder.get8(&trigLegatoAmp);
  decoder.get8(&trigLegatoFilter);
  decoder.get8(&trigLegatoLFO);

  decoder.get8(&commonMultimode);
  uint8_t byte;
  decoder.get8(&byte);
  if (byte == 0) {
    commonTiming = 0;
  } else {
    commonTiming = 1 << (1 - byte);
  }
  decoder.get8(&splitKey);
  decoder.get8(&splitRange);

  return true;
}

uint16_t MNMKit::toSysex(uint8_t *data, uint16_t len) {
  MNMDataToSysexEncoder encoder(data);
  return toSysex(&encoder);
}

uint16_t MNMKit::toSysex(ElektronDataToSysexEncoder *encoder) {
  if ((MidiClock.state == 2) && (MNM.midi->uart->speed > 62500)) {
    encoder->throttle = true;
  }
  encoder->stop7Bit();
  encoder->begin();
  encoder->pack(monomachine_sysex_hdr, sizeof(monomachine_sysex_hdr));
  encoder->pack8(MNM_KIT_MESSAGE_ID);
  encoder->pack8(0x02); // version
  encoder->pack8(0x01); // revision

  encoder->startChecksum();
  encoder->pack8(origPosition);
  encoder->start7Bit();

  encoder->pack((uint8_t *)name, 11);
  name[11] = '\0';

  encoder->pack(levels, 6);
  encoder->pack((uint8_t *)parameters, 6 * 72);
  encoder->pack(models, 6);
  encoder->pack(types, 6);
  encoder->pack8(0); // unused
  encoder->pack16(patchBusIn);
  encoder->pack8(mirrorLR);
  encoder->pack8(mirrorUD);

  encoder->pack((uint8_t *)destPages, 6 * 6 * 2);
  encoder->pack((uint8_t *)destParams, 6 * 6 * 2);
  encoder->pack((uint8_t *)destRanges, 6 * 6 * 2);
  encoder->pack8(lpKeyTrack);
  encoder->pack8(hpKeyTrack);

  encoder->pack8(trigPortamento);
  encoder->pack(trigTracks, 6);
  encoder->pack8(trigLegatoAmp);
  encoder->pack8(trigLegatoFilter);
  encoder->pack8(trigLegatoLFO);

  encoder->pack8(commonMultimode);
  uint8_t byte = 0;
  for (uint8_t j = 0; j < 6; j++) {
    if (IS_BIT_SET(commonTiming, j)) {
      byte = (j + 1);
      break;
    }
  }
  encoder->pack8(byte);
  encoder->pack8(splitKey);
  encoder->pack8(splitRange);

  uint16_t enclen = encoder->finish();
  encoder->finishChecksum();

  return enclen + 5;
}

bool MNMSong::fromSysex(uint8_t *data, uint16_t len) {
  if (!ElektronHelper::checkSysexChecksum(data, len)) {
    return false;
  }

  //	origPosition = data[3];
  MNMSysexDecoder decoder(data + 4);

  return false;
}

bool MNMSong::fromSysex(MidiClass *midi) {
  uint16_t offset = 0;
  uint16_t len = midi->midiSysex.get_recordLen();

  if (!ElektronHelper::checkSysexChecksum(midi, offset, len)) {
    return false;
  }

  //	origPosition = data[3];
  MNMSysexDecoder decoder(midi, offset + 4);

  return false;
}

uint16_t MNMSong::toSysex(uint8_t *sysex, uint16_t len) {
  MNMDataToSysexEncoder encoder(sysex);
  return toSysex(&encoder);
}

uint16_t MNMSong::toSysex(ElektronDataToSysexEncoder *encoder) {
  encoder->stop7Bit();
  encoder->begin();
  encoder->pack(monomachine_sysex_hdr, sizeof(monomachine_sysex_hdr));
  encoder->pack8(MNM_KIT_MESSAGE_ID);
  encoder->pack8(0x02); // version
  encoder->pack8(0x01); // revision

  encoder->startChecksum();
  //	encoder->pack8(origPosition);
  encoder->start7Bit();

  return 0;
}
