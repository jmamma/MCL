#include "WProgram.h"
#include "MD.h"
#include "helpers.h"

#include "MidiUartParent.hh"

uint8_t machinedrum_sysex_hdr[5] = {
	0x00,
	0x20,
	0x3c,
	0x02,
	0x00
};


uint8_t MDClass::noteToTrack(uint8_t pitch) {
	uint8_t i;
	if (MD.loadedGlobal) {
		for (i = 0; i < sizeof(MD.global.drumMapping); i++) {
			if (pitch == MD.global.drumMapping[i])
				return i;
		}
		return 128;
	} else {
		return 128;
	}
}

uint8_t standardDrumMapping[16] = {
	36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62
};

MDClass::MDClass()  {
	currentGlobal = -1;
	currentKit = -1;
	currentPattern = -1;
	global.baseChannel = 0;
	for (int i = 0; i < 16; i++) {
		global.drumMapping[i] = standardDrumMapping[i];
	}
	loadedKit = loadedGlobal = false;
}

void MDClass::parseCC(uint8_t channel, uint8_t cc, uint8_t *track, uint8_t *param) {
	if ((channel >= global.baseChannel) && (channel < (global.baseChannel + 4))) {
		channel -= global.baseChannel;
		*track = channel * 4;
		if (cc >= 96) {
			*track += 3;
			*param = cc - 96;
		} else if (cc >= 72) {
			*track += 2;
			*param = cc - 72;
		} else if (cc >= 40) {
			*track += 1;
			*param = cc - 40;
		} else if (cc >= 16) {
			*param = cc - 16;
		} else if (cc >= 12) {
			*track += (cc - 12);
			*param = 32; // MUTE
		} else if (cc >= 8) {
			*track += (cc - 8);
			*param = 33; // LEV
		}
	} else {
		*track = 255;
	}
}

void MDClass::sendRequest(uint8_t type, uint8_t param) {
	MidiUart.putc(0xF0);
	MidiUart.sendRaw(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
	MidiUart.putc(type);
	MidiUart.putc(param);
	MidiUart.putc(0xF7);
}

void MDClass::triggerTrack(uint8_t track, uint8_t velocity) {
	if (global.drumMapping[track] != -1 && global.baseChannel != 127) {
		MidiUart.sendNoteOn(global.baseChannel, global.drumMapping[track], velocity);
	}
}

void MDClass::setTrackParam(uint8_t track, uint8_t param, uint8_t value) {
	if (global.baseChannel > 15)
		return;
	if ((track > 15) || (param > 33))
		return;
	
	uint8_t channel = track >> 2;
	uint8_t b = track & 3;
	uint8_t cc = 0;
	if (param == 32) { // MUTE
		cc = 12 + b;
	} else if (param == 33) { // LEV
		cc = 8 + b;
	} else {
		cc = param;
		if (b < 2) {
			cc += 16 + b * 24;
		} else {
			cc += 24 + b * 24;
		}
	}
	MidiUart.sendCC(channel + global.baseChannel, cc, value);
}

//  0x5E, 0x5D, 0x5F, 0x60

void MDClass::sendSysex(uint8_t *bytes, uint8_t cnt) {
	MidiUart.putc(0xF0);
	MidiUart.sendRaw(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
	MidiUart.sendRaw(bytes, cnt);
	MidiUart.putc(0xf7);
}

void MDClass::sendFXParam(uint8_t param, uint8_t value, uint8_t type) {
	uint8_t data[3] = { type, param, value };
	MD.sendSysex(data, 3);
}

void MDClass::setEchoParam(uint8_t param, uint8_t value) {
	sendFXParam(param, value, MD_SET_RHYTHM_ECHO_PARAM_ID);
}

void MDClass::setReverbParam(uint8_t param, uint8_t value) {
	sendFXParam(param, value, MD_SET_GATE_BOX_PARAM_ID);
}

void MDClass::setEQParam(uint8_t param, uint8_t value) {
	sendFXParam(param, value, MD_SET_EQ_PARAM_ID);
}

void MDClass::setCompressorParam(uint8_t param, uint8_t value) {
	sendFXParam(param, value, MD_SET_DYNAMIX_PARAM_ID);
}

/*** tunings ***/


uint8_t MDClass::trackGetCCPitch(uint8_t track, uint8_t cc, int8_t *offset) {
	tuning_t const *tuning = getModelTuning(kit.models[track]);
	
	if (tuning == NULL)
		return 128;
	
	uint8_t i;
	int8_t off = 0;
	for (i = 0; i < tuning->len; i++) {
		uint8_t ccStored = pgm_read_byte(&tuning->tuning[i]);
		off = ccStored - cc;
		if (ccStored >= cc) {
			if (offset != NULL) {
				*offset = off;
			}
			if (off <= tuning->offset)
				return i + tuning->base;
			else 
				return 128;
		}
	}
	off = ABS(pgm_read_byte(&tuning->tuning[tuning->len - 1]) - cc);
	if (offset != NULL)
		*offset = off;
	if (off <= tuning->offset)
		return i + tuning->base;
	else
		return 128;
}

uint8_t MDClass::trackGetPitch(uint8_t track, uint8_t pitch) {
	tuning_t const *tuning = getModelTuning(kit.models[track]);
	
	if (tuning == NULL)
		return 128;
	
	uint8_t base = tuning->base;
	uint8_t len = tuning->len;
	
	if ((pitch < base) || (pitch >= (base + len))) {
		return 128;
	}
	
	return pgm_read_byte(&tuning->tuning[pitch - base]);
}

void MDClass::sendNoteOn(uint8_t track, uint8_t pitch, uint8_t velocity) {
	uint8_t realPitch = trackGetPitch(track, pitch);
	if (realPitch == 128)
		return;
	setTrackParam(track, 0, realPitch);
	//  setTrackParam(track, 0, realPitch);
	//  delay(20);
	triggerTrack(track, velocity);
	//  delay(20);
	//  setTrackParam(track, 0, realPitch - 10);
	//  triggerTrack(track, velocity);
}

void MDClass::sliceTrack32(uint8_t track, uint8_t from, uint8_t to, bool correct) {
	uint8_t pfrom, pto;
	if (from > to) {
		pfrom = MIN(127, from * 4 + 1);
		pto = MIN(127, to * 4);
	} else {
		pfrom = MIN(127, from * 4);
		pto = MIN(127, to * 4);
		if (correct && pfrom >= 64)
			pfrom++;
	}
	setTrackParam(track, 4, pfrom);
	setTrackParam(track, 5, pto);
	triggerTrack(track, 127);
}

void MDClass::sliceTrack16(uint8_t track, uint8_t from, uint8_t to) {
	if (from > to) {
		setTrackParam(track, 4, MIN(127, from * 8 + 1));
		setTrackParam(track, 5, MIN(127, to * 8));
	} else {
		setTrackParam(track, 4, MIN(127, from * 8));
		setTrackParam(track, 5, MIN(127, to * 8));
	}
	triggerTrack(track, 100);
}

bool MDClass::isMelodicTrack(uint8_t track) {
	return (getModelTuning(kit.models[track]) != NULL);
}

void MDClass::setLFOParam(uint8_t track, uint8_t param, uint8_t value) {
	uint8_t data[3] = { 0x62, track << 3 | param, value };
	MD.sendSysex(data, countof(data));
}

void MDClass::setLFO(uint8_t track, MDLFO *lfo) {
	setLFOParam(track, 0, lfo->destinationTrack);
	setLFOParam(track, 1, lfo->destinationParam);
	setLFOParam(track, 2, lfo->shape1);
	setLFOParam(track, 3, lfo->shape2);
	setLFOParam(track, 4, lfo->type);
	setLFOParam(track, 5, lfo->speed);
	setLFOParam(track, 6, lfo->depth);
	setLFOParam(track, 7, lfo->mix);
}

void MDClass::mapMidiNote(uint8_t pitch, uint8_t track) {
	uint8_t data[3] = { 0x5a, pitch, track };
	MD.sendSysex(data, countof(data));
}

void MDClass::resetMidiMap() {
	uint8_t data[1] = { 0x64 };
	MD.sendSysex(data, countof(data));
}

void MDClass::setTrackRouting(uint8_t track, uint8_t output) {
	uint8_t data[3] = { 0x5c, track, output };
	MD.sendSysex(data, countof(data));
}

void MDClass::setTempo(uint16_t tempo) {
	uint8_t data[3] = { 0x61, tempo >> 7, tempo & 0x7F };
	MD.sendSysex(data, countof(data));
}

void MDClass::setTrigGroup(uint8_t srcTrack, uint8_t trigTrack) {
	uint8_t data[3] = { 0x65, srcTrack, trigTrack };
	MD.sendSysex(data, countof(data));
}

void MDClass::setMuteGroup(uint8_t srcTrack, uint8_t muteTrack) {
	uint8_t data[3] = { 0x66, srcTrack, muteTrack };
	MD.sendSysex(data, countof(data));
}

void MDClass::saveCurrentKit(uint8_t pos) {
	uint8_t data[2] = { 0x59, pos & 0x7F };
	MD.sendSysex(data, countof(data));
}

void MDClass::assignMachine(uint8_t track, uint8_t model, uint8_t init) {
	uint8_t data[] = { 0x5B, track, model, 0x00, init };
	if (model >= 128) {
		data[2] = (model - 128);
		data[3] = 0x01;
	} else {
		data[2] = model;
		data[3] = 0x00;
	}
	MD.sendSysex(data, countof(data));
}

void MDClass::setMachine(uint8_t track, MDMachine *machine) {
	assignMachine(track, machine->model);
	for (uint8_t i = 0; i < 24; i++) {
		setTrackParam(track, i, machine->params[i]);
	}
	setLFO(track, &machine->lfo);
}

void MDClass::muteTrack(uint8_t track, bool mute) {
	if (global.baseChannel == 127)
		return;
	
	uint8_t channel = track >> 2;
	uint8_t b = track & 3;
    uint8_t cc = 12 + b;
	MidiUart.sendCC(channel + global.baseChannel, cc, mute ? 1 : 0);
}

void MDClass::setStatus(uint8_t id, uint8_t value) {
	uint8_t data[] = { 0x71, id & 0x7F, value & 0x7F };
	MD.sendSysex(data, countof(data));
}

void MDClass::loadGlobal(uint8_t id) {
	setStatus(1, id);
}

void MDClass::loadKit(uint8_t kit) {
	setStatus(2, kit);
}

void MDClass::loadPattern(uint8_t pattern) {
	setStatus(4, pattern);
}

void MDClass::loadSong(uint8_t song) {
	setStatus(8, song);
}


void MDClass::setSequencerMode(uint8_t mode) {
	setStatus(16, mode);
}

void MDClass::setLockMode(uint8_t mode) {
	setStatus(32, mode);
}

void MDClass::getPatternName(uint8_t pattern, char str[5]) {
	uint8_t bank = pattern / 16;
	uint8_t num = pattern % 16 + 1;
	str[0] = 'A' + bank;
	str[1] = '0' + (num / 10);
	str[2] = '0' + (num % 10);
	str[3] = ' ';
	str[4] = 0;
}

void MDClass::requestKit(uint8_t kit) {
	sendRequest(MD_KIT_REQUEST_ID, kit);
}

void MDClass::requestPattern(uint8_t pattern) {
	sendRequest(MD_PATTERN_REQUEST_ID, pattern);
}

void MDClass::requestSong(uint8_t song) {
	sendRequest(MD_SONG_REQUEST_ID, song);
}

void MDClass::requestGlobal(uint8_t global) {
	sendRequest(MD_GLOBAL_REQUEST_ID, global);
}

bool MDClass::checkParamSettings() {
	if (loadedGlobal) {
		return (MD.global.baseChannel <= 12);
	} else {
		return false;
	}
}

bool MDClass::checkTriggerSettings() {
	return false;
}

bool MDClass::checkClockSettings() {
	return false;
}

bool MDClass::waitBlocking(MDBlockCurrentStatusCallback *cb, uint16_t timeout) {	
	uint16_t start_clock = read_slowclock();
	uint16_t current_clock = start_clock;
	do {
		current_clock = read_slowclock();
       
       //MCL Code, trying to replicate main loop
       
        if ((MidiClock.mode == MidiClock.EXTERNAL ||
                                 MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
              MidiClock.updateClockInterval();
         }
        handleIncomingMidi();
		GUI.display();
	} while ((clock_diff(start_clock, current_clock) < timeout) && !cb->received);
	return cb->received;
}


uint8_t MDClass::getBlockingStatus(uint8_t type, uint16_t timeout) {
	MDBlockCurrentStatusCallback cb(type);
	
	MDSysexListener.addOnStatusResponseCallback(&cb, (md_status_callback_ptr_t)&MDBlockCurrentStatusCallback::onStatusResponseCallback);
	MD.sendRequest(MD_STATUS_REQUEST_ID, type);
	
	bool ret = waitBlocking(&cb, timeout);
	
	MDSysexListener.removeOnStatusResponseCallback(&cb);
	
	return cb.value;
}

bool MDClass::getBlockingKit(uint8_t kit, uint16_t timeout) {
	MDBlockCurrentStatusCallback cb;
	MDSysexListener.addOnKitMessageCallback(&cb,
											(md_callback_ptr_t)&MDBlockCurrentStatusCallback::onSysexReceived);
	MD.requestKit(kit);
	bool ret = waitBlocking(&cb, timeout);
	MDSysexListener.removeOnKitMessageCallback(&cb);
	
	return ret;
}

bool MDClass::getBlockingPattern(uint8_t pattern, uint16_t timeout) {
	MDBlockCurrentStatusCallback cb;
	MDSysexListener.addOnPatternMessageCallback(&cb,
												(md_callback_ptr_t)&MDBlockCurrentStatusCallback::onSysexReceived);
	MD.requestPattern(pattern);
	bool ret = waitBlocking(&cb, timeout);
	MDSysexListener.removeOnPatternMessageCallback(&cb);
	
	return ret;
}

bool MDClass::getBlockingGlobal(uint8_t global, uint16_t timeout) {
	MDBlockCurrentStatusCallback cb;
	MDSysexListener.addOnGlobalMessageCallback(&cb,
											   (md_callback_ptr_t)&MDBlockCurrentStatusCallback::onSysexReceived);
	MD.requestGlobal(global);
	bool ret = waitBlocking(&cb, timeout);
	MDSysexListener.removeOnGlobalMessageCallback(&cb);
	
	return ret;
}

uint8_t MDClass::getCurrentTrack(uint16_t timeout) {
	uint8_t value = getBlockingStatus(0x22, timeout);
	if (value == 255) {
		return 255;
	} else {
		MD.currentTrack = value;
		return value;
	}
}
uint8_t MDClass::getCurrentKit(uint16_t timeout) {
	uint8_t value = getBlockingStatus(MD_CURRENT_KIT_REQUEST, timeout);
	if (value == 255) {
		return 255;
	} else {
		MD.currentKit = value;
		return value;
	}
}

uint8_t MDClass::getCurrentPattern(uint16_t timeout) {
	uint8_t value = getBlockingStatus(MD_CURRENT_PATTERN_REQUEST, timeout);
	if (value == 255) {
		return 255;
	} else {
		MD.currentPattern = value;
		return value;
	}
}


MDClass MD;
