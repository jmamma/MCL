/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "Elektron.hh"
#include "MDMessages.hh"
#include "helpers.h"
#include "MDParams.hh"

#include "MD.h"
uint8_t lfo_statestore[31];


bool MDGlobal::fromSysex(uint8_t *data, uint16_t len) {
	if (len != 0xC4 - 6)  {
		//		printf("wrong length\n");
		// wrong length 
		return false;
	}
	
	if (!ElektronHelper::checkSysexChecksum(data, len)) {
		//		printf("wrong checksum\n");
		return false;
	}
	
	origPosition = data[3];
	ElektronSysexDecoder decoder(DATA_ENCODER_INIT(data + 4, len - 4));
	decoder.stop7Bit();
	decoder.get(drumRouting, 16);
	
	decoder.start7Bit();
	decoder.get(keyMap, 128);
	decoder.stop7Bit();
	
	decoder.get8(&baseChannel);
	decoder.get8(&unused);
	decoder.get16(&tempo);
	decoder.getb(&extendedMode);
	
	uint8_t byte = 0;
	decoder.get8(&byte);
	clockIn = IS_BIT_SET(byte, 0);
	transportIn = IS_BIT_SET(byte, 4);
	clockOut = IS_BIT_SET(byte, 5);
	transportOut = IS_BIT_SET(byte, 6);
	decoder.getb(&localOn);
	
	decoder.get(&drumLeft, 12);
	
	for (int i = 0; i < 128; i++) {
		if (keyMap[i] < 16) {
			drumMapping[keyMap[i]] = i;
		}
	}
	
	return true;
}

uint16_t MDGlobal::toSysex(uint8_t *data, uint16_t len) {
	ElektronDataToSysexEncoder encoder(DATA_ENCODER_INIT(data, len));
	return toSysex(encoder);
	if (len < 0xC5)
		return 0;
}

uint16_t MDGlobal::toSysex(ElektronDataToSysexEncoder &encoder) {
	encoder.stop7Bit();
	encoder.pack8(0xF0);
	encoder.pack(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
	encoder.pack8(MD_GLOBAL_MESSAGE_ID);
	encoder.pack8(0x05); // version
	encoder.pack8(0x01); // revision
	
	encoder.startChecksum();
	encoder.pack8(origPosition);
	
	encoder.pack(drumRouting, 16);
	
	encoder.start7Bit();
	encoder.pack(keyMap, 128);
	encoder.stop7Bit();
	
	encoder.pack8(baseChannel);
	encoder.pack8(unused);
	encoder.pack16(tempo);
	encoder.packb(extendedMode);
	
	uint8_t byte = 0;
//  clockIn = false;
//  transportIn = true;
//  clockOut = true;
//  transportOut = true;
    if (clockIn)
//        byte = byte + 1;
        SET_BIT(byte, 0);
	if (!transportIn)
 //       byte = byte + 8;
	SET_BIT(byte, 4);
    if (clockOut)
 //      byte = byte + 16;
		SET_BIT(byte, 5);
	if (transportOut)
//        byte = byte + 32;
		SET_BIT(byte, 6);
	encoder.pack8(byte);
	encoder.packb(localOn);
	
	encoder.pack(&drumLeft, 12);
	
	uint16_t enclen = encoder.finish();
	encoder.finishChecksum();
	
	return enclen + 5;
}

bool MDKitShort::fromSysex(uint8_t *data, uint16_t len) {
	if (len != (0x4d1 - 7)) {
		//    GUI.flash_strings_fill("WRONG LEN", "");
		//    GUI.setLine(GUI.LINE2);
		//    GUI.flash_put_value16(0, len);
		return false;
	}
	
	if (!ElektronHelper::checkSysexChecksum(data, len)) {
		return false;
	}
	
	origPosition = data[3];
	ElektronSysexDecoder decoder(DATA_ENCODER_INIT(data + 4, len - 4));
	decoder.stop7Bit();
	decoder.get((uint8_t *)name, 16);
	name[16] = '\0';
	
	decoder.skip(24 * 16 + 16);
	decoder.start7Bit();
	decoder.get32(models, 16);
	
	return true;
}

bool MDKit::fromSysex(uint8_t *data, uint16_t len) {
	if (len != (0x4d1 - 7)) {
		GUI.flash_strings_fill("WRONG LEN", "");
		GUI.setLine(GUI.LINE2);
		GUI.flash_put_value16(0, len);
		return false;
	}
	
	if (!ElektronHelper::checkSysexChecksum(data, len)) {
		GUI.flash_strings_fill("WRONG CKSUM", "");
		return false;
	}
	
	origPosition = data[3];
	
	ElektronSysexDecoder decoder(DATA_ENCODER_INIT(data + 4, len - 4));
	GUI.setLine(GUI.LINE2);
	decoder.stop7Bit();
	decoder.get((uint8_t *)name, 16);
	name[16] = '\0';
	
	decoder.get((uint8_t *)params, 16 * 24);
	decoder.get(levels, 16);
	
	decoder.start7Bit();
	decoder.get32(models, 16);
	decoder.stop7Bit(); // reset 7 bit
	decoder.start7Bit();
    //uint8_t lfo_out;

	for (uint8_t i = 0; i < 16; i++) {
        		decoder.get((uint8_t *)&lfos[i], 5);
        decoder.get((uint8_t *)&lfo_statestore, 31);

	//	decoder.get((uint8_t *)&lfos[i], 5);
     //   for (int j = 0; j < 31; j++) {
      //      decoder.get((uint8_t *)&lfo_out,1);
       //                      LCD.goLine(0);
        //    LCD.putnumber(j);
         //                    LCD.goLine(1);
          //  LCD.putnumber((int)lfo_out);
           // delay(400);

//        }

	}
  //  GUI.flash_put_value(0, lfo_out);

	decoder.stop7Bit();
	
	decoder.get(reverb, 8);
	decoder.get(delay, 8);
	decoder.get(eq, 8);
	decoder.get(dynamics, 8);
	
	decoder.start7Bit();
	decoder.get(trigGroups, 16);
	decoder.get(muteGroups, 16);
	
	return true;
}

uint16_t MDKit::toSysex(uint8_t *data, uint16_t len) {
	ElektronDataToSysexEncoder encoder(DATA_ENCODER_INIT(data, len));
	if (len < 0xC5)
		return 0;
	return toSysex(encoder);
}

uint16_t MDKit::toSysex(ElektronDataToSysexEncoder &encoder) {
	encoder.stop7Bit();
	encoder.pack8(0xF0);
	encoder.pack(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
	encoder.pack8(MD_KIT_MESSAGE_ID);
	encoder.pack8(0x04); // version
	encoder.pack8(0x01); // revision
	
	encoder.startChecksum();
	encoder.pack8(origPosition);
	
	encoder.pack((uint8_t *)name, 16);
	name[16] = '\0';
	
	encoder.pack((uint8_t *)params, 16 * 24);
	encoder.pack(levels, 16);
	
	encoder.start7Bit();
	encoder.pack32(models, 16);
	encoder.stop7Bit();
	encoder.start7Bit();
	for (uint8_t i = 0; i < 16; i++) {
//        encoder.pack((uint8_t *)&lfos[i], 36);

		encoder.pack((uint8_t *)&lfos[i], 5);
        encoder.pack((uint8_t *)&lfo_statestore[i], 31);

	}
	encoder.stop7Bit();
	
	encoder.pack(reverb, 8);
	encoder.pack(delay, 8);
	encoder.pack(eq, 8);
	encoder.pack(dynamics, 8);
	
	encoder.start7Bit();
	encoder.pack(trigGroups, 16);
	encoder.pack(muteGroups, 16);
	
	uint16_t enclen = encoder.finish();
	encoder.finishChecksum();
	
	return enclen + 5;
}

uint8_t swapNumber(uint8_t num, uint8_t a, uint8_t b) {
	if (num == a) {
		return b;
	} else if (num == b) {
		return a;
	} else {
		return num;
	}
}

void MDKit::swapTracks(uint8_t srcTrack, uint8_t dstTrack) {
	uint8_t _params[24];
	uint8_t _level;
	uint32_t _model;
	MDLFO _lfo;
	uint8_t _trigGroup;
	uint8_t _muteGroup;
	
	/* adjust lfo destination, as well as trig and mute groups. */
#if 0
	for (uint8_t i = 0; i < 16; i++) {
		lfos[i].destinationTrack = swapNumber(lfos[i].destinationTrack, srcTrack, dstTrack);
		trigGroups[i] = swapNumber(trigGroups[i], srcTrack, dstTrack);
		muteGroups[i] = swapNumber(muteGroups[i], srcTrack, dstTrack);
		if (models[i] == CTR_8P_MODEL) {
			for (uint8_t j = 8; j < 24; j += 2) {
				params[i][j] = swapNumber(params[i][j], srcTrack, dstTrack);
			}
		}
	}
#endif
	
	/* swap params */
	m_memcpy(_params, params[srcTrack], 24);
	m_memcpy(params[srcTrack], params[dstTrack], 24);
	m_memcpy(params[dstTrack], _params, 24);
	
	m_memcpy(&_lfo, &lfos[srcTrack], sizeof(_lfo));
	m_memcpy(&lfos[srcTrack], &lfos[dstTrack], sizeof(_lfo));
	m_memcpy(&lfos[dstTrack], &_lfo, sizeof(_lfo));
	
	/* swap level, model, trig group, mute group */
	_level = levels[srcTrack];
	levels[srcTrack] = levels[dstTrack];
	levels[dstTrack] = _level;
	
	_model = models[srcTrack];
	models[srcTrack] = models[dstTrack];
	models[dstTrack] = _model;
	
	_trigGroup = trigGroups[srcTrack];
	trigGroups[srcTrack] = trigGroups[dstTrack];
	trigGroups[dstTrack] = _trigGroup;
	
	_muteGroup = muteGroups[srcTrack];
	muteGroups[srcTrack] = muteGroups[dstTrack];
	muteGroups[dstTrack] = _muteGroup;
}

bool MDSong::fromSysex(uint8_t *data, uint16_t len) {
	if (len < 0x1a - 7)
		return false;
	
	if (!ElektronHelper::checkSysexChecksum(data, len)) {
		return false;
	}
	
	numRows = (len - (0x1A - 7)) / 12;
	
	origPosition = data[3];
	ElektronSysexDecoder decoder(DATA_ENCODER_INIT(data + 4, len - 4));
	decoder.stop7Bit();
	decoder.get((uint8_t *)name, 16);
	name[16] = '\0';
	
	for (int i = 0; i < numRows; i++) {
		decoder.start7Bit();
		decoder.get((uint8_t *)&rows[i], 4);
		decoder.get16(&rows[i].mutes);
		decoder.get16(&rows[i].tempo);
		decoder.get(&rows[i].startPosition, 2);
		decoder.stop7Bit();
	}
	
	return true;
}

uint16_t MDSong::toSysex(uint8_t *data, uint16_t len) {
	ElektronDataToSysexEncoder encoder(DATA_ENCODER_INIT(data, len));
	if (len < (uint16_t)(0x1F + numRows * 12 ))
		return 0;
	
	return toSysex(encoder);
}

uint16_t MDSong::toSysex(ElektronDataToSysexEncoder &encoder) {
	encoder.stop7Bit();
	encoder.pack8(0xF0);
	encoder.pack(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
	encoder.pack8(MD_PATTERN_MESSAGE_ID);
	encoder.pack8(0x04); // version
	encoder.pack8(0x01); // revision
	
	encoder.startChecksum();
	encoder.pack8(origPosition);
	encoder.pack((uint8_t *)name, 16);
	
	for (uint8_t i = 0; i < numRows; i++) {
		encoder.start7Bit();
		encoder.pack((uint8_t *)&rows[i].pattern, 4);
		encoder.pack16(rows[i].mutes);
		encoder.pack16(rows[i].tempo);
		encoder.pack(&rows[i].startPosition, 2);
		encoder.stop7Bit();
	}
	
	uint16_t enclen = encoder.finish();
	encoder.finishChecksum();
	
	return enclen + 5;
}
