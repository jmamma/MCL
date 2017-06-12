/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDISYSEX_H__
#define MIDISYSEX_H__

#include <inttypes.h>

#ifndef SYSEX_BUF_SIZE
#define SYSEX_BUF_SIZE 1024
#endif

class MidiSysexClass;

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_sysex Midi Sysex
 *
 * @{
 **/

class MidiSysexListenerClass {
	/**
	 * \addtogroup midi_sysex 
	 *
	 * @{
	 **/
	
 public:
  uint8_t ids[3];
	MidiSysexClass *sysex;

  MidiSysexListenerClass(MidiSysexClass *_sysex = NULL) {
		sysex = _sysex;
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
  };
  
  virtual void start() {
  }
  virtual void abort() {
  }
  virtual void end() {
  }
  virtual void handleByte(uint8_t byte) {
  }

#ifdef HOST_MIDIDUINO
  virtual ~MidiSysexListenerClass() {
  }
#endif

	/* @} */
};

#define NUM_SYSEX_SLAVES 4

class MidiSysexClass {
	/**
	 * \addtogroup midi_sysex 
	 *
	 * @{
	 **/
	
 protected:
  bool aborted;
  bool recording;
  uint8_t recvIds[3];
  bool sysexLongId;

 public:
  void startRecord(uint8_t *buf = NULL, uint16_t maxLen = 0);
  void stopRecord();
  
  void resetRecord(uint8_t *buf = NULL, uint16_t maxLen = 0);
  bool recordByte(uint8_t c);

  uint16_t max_len;
  uint16_t recordLen;
  uint8_t *data;
  uint8_t *recordBuf;
  uint16_t maxRecordLen;

  uint16_t len;

  MidiSysexListenerClass *listeners[NUM_SYSEX_SLAVES];
  
  MidiSysexClass(uint8_t *_data, uint16_t size) {
    data = _data;
    max_len = size;
    len = 0;
    aborted = false;
    recording = false;
    recordBuf = NULL;
    maxRecordLen = 0;
    sysexLongId = false;
  }

  void initSysexListeners() {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++)
      listeners[i] = NULL;
  }
  bool addSysexListener(MidiSysexListenerClass *listener) {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == NULL || listeners[i] == listener) {
				listeners[i] = listener;
				listener->sysex = this;
				return true;
      }
    }
    return false;
  }
  void removeSysexListener(MidiSysexListenerClass *listener) {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == listener)
	listeners[i] = NULL;
    }
  }
  bool isListenerActive(MidiSysexListenerClass *listener);

  void reset();
  
  void start();
  void abort();
  void end();
  void handleByte(uint8_t byte);

	/* @} */
};

class MididuinoSysexListenerClass : public MidiSysexListenerClass {
	/**
	 * \addtogroup midi_sysex 
	 *
	 * @{
	 **/
	
 public:
  MididuinoSysexListenerClass();
  virtual void handleByte(uint8_t byte);

#ifdef HOST_MIDIDUINO
  virtual ~MididuinoSysexListenerClass() {
  }
#endif

	/* @} */
};

// extern MidiSysexClass MidiSysex;
// extern MidiSysexClass MidiSysex2;
#define MidiSysex Midi.midiSysex
#define MidiSysex2 Midi2.midiSysex
extern MididuinoSysexListenerClass MididuinoSysexListener;

/* @} @} */

#endif /* MIDISYSEX_H__ */
