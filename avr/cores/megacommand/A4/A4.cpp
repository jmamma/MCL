#include "WProgram.h"
#include "A4.h"
#include "helpers.h"

#include "MidiUartParent.hh"

uint8_t a4_sysex_hdr[5] = {
	0x00,
	0x20,
	0x3c,
	0x06,
	0x00
};

uint8_t a4_sysex_proto_version[2] = {
    0x01,
    0x01
};

uint8_t a4_sysex_ftr[4] {
    0x00,
    0x00,
    0x00,
    0x05
};

A4Class::A4Class()  {
}
void A4Class::sendRequest(uint8_t type, uint8_t param) {
    USE_LOCK();
    SET_LOCK();
	MidiUart2.m_putc(0xF0);
	MidiUart2.sendRaw(a4_sysex_hdr, sizeof(a4_sysex_hdr));
	MidiUart2.m_putc(type); 
	MidiUart2.sendRaw(a4_sysex_proto_version, sizeof(a4_sysex_proto_version));
    MidiUart2.m_putc(param);
	MidiUart2.sendRaw(a4_sysex_ftr, sizeof(a4_sysex_ftr));
	MidiUart2.m_putc(0xF7);
    CLEAR_LOCK();
}

void A4Class::sendSysex(uint8_t *bytes, uint8_t cnt) {
        USE_LOCK();
        SET_LOCK();
	MidiUart2.m_putc(0xF0);
	MidiUart2.sendRaw(a4_sysex_hdr, sizeof(a4_sysex_hdr));
	MidiUart2.sendRaw(bytes, cnt);
	MidiUart2.m_putc(0xf7);
    CLEAR_LOCK();
}

void A4Class::requestKit(uint8_t kit) {
	sendRequest(A4_KIT_REQUEST_ID, kit);
}

void A4Class::requestKitX(uint8_t kit) {
	sendRequest(A4_KITX_REQUEST_ID, kit);
}

void A4Class::requestSound(uint8_t sound) {
	sendRequest(A4_SOUND_REQUEST_ID, sound);
}
void A4Class::requestSoundX(uint8_t sound) {
	sendRequest(A4_SOUNDX_REQUEST_ID, sound);
}

void A4Class::requestSong(uint8_t song) {
	sendRequest(A4_SONG_REQUEST_ID, song);
}


void A4Class::requestSongX(uint8_t song) {
	sendRequest(A4_SONGX_REQUEST_ID, song);
}


void A4Class::requestPattern(uint8_t pattern) {
	sendRequest(A4_PATTERN_REQUEST_ID, pattern);
}


void A4Class::requestPatternX(uint8_t pattern) {
	sendRequest(A4_PATTERNX_REQUEST_ID, pattern);
}


void A4Class::requestSettings(uint8_t setting) {
	sendRequest(A4_SETTINGS_REQUEST_ID, setting);
}

void A4Class::requestSettingsX(uint8_t setting) {
	sendRequest(A4_SETTINGSX_REQUEST_ID, setting);
}


void A4Class::requestGlobal(uint8_t global) {
	sendRequest(A4_GLOBAL_REQUEST_ID, global);
}

void A4Class::requestGlobalX(uint8_t global) {
	sendRequest(A4_GLOBALX_REQUEST_ID, global);
}


bool A4Class::waitBlocking(A4BlockCurrentStatusCallback *cb, uint16_t timeout) {	
	uint16_t start_clock = read_slowclock();
	uint16_t current_clock = start_clock;
	do {
		current_clock = read_slowclock();
       
       //MCL Code, trying to replicate main loop
       
        if ((MidiClock.mode == MidiClock.EXTERNAL_UART1 ||
                                 MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
              MidiClock.updateClockInterval();
         }
        handleIncomingMidi();
		GUI.display();
	} while ((clock_diff(start_clock, current_clock) < timeout) && !cb->received);
	return cb->received;
}

bool A4Class::getBlockingKit(uint8_t kit, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnKitMessageCallback(&cb,
											(A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestKit(kit);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnKitMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingPattern(uint8_t pattern, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnPatternMessageCallback(&cb,
												(A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestPattern(pattern);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnPatternMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingGlobal(uint8_t global, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnGlobalMessageCallback(&cb,
											   (A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestGlobal(global);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnGlobalMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingSound(uint8_t sound, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnSoundMessageCallback(&cb,
												(A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestSound(sound);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnSoundMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingSettings(uint8_t settings, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnSettingsMessageCallback(&cb,
											   (A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestSettings(settings);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnSettingsMessageCallback(&cb);
	
	return ret;
}


bool A4Class::getBlockingKitX(uint8_t kit, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnKitMessageCallback(&cb,
											(A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestKitX(kit);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnKitMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingPatternX(uint8_t pattern, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnPatternMessageCallback(&cb,
												(A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestPatternX(pattern);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnPatternMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingGlobalX(uint8_t global, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnGlobalMessageCallback(&cb,
											   (A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestGlobalX(global);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnGlobalMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingSoundX(uint8_t sound, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnSoundMessageCallback(&cb,
												(A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestSoundX(sound);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnSoundMessageCallback(&cb);
	
	return ret;
}

bool A4Class::getBlockingSettingsX(uint8_t settings, uint16_t timeout) {
	A4BlockCurrentStatusCallback cb;
	A4SysexListener.addOnSettingsMessageCallback(&cb,
											   (A4_callback_ptr_t)&A4BlockCurrentStatusCallback::onSysexReceived);
	Analog4.requestSettingsX(settings);
	bool ret = waitBlocking(&cb, timeout);
	A4SysexListener.removeOnSettingsMessageCallback(&cb);
	
	return ret;
}

A4Class Analog4;
