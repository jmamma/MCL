/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDICLOCK_H__
#define MIDICLOCK_H__

#include "WProgram.h"
#include <inttypes.h>
#include "Callback.hh"
#include "Vector.hh"
#include "midi-common.hh"

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_clock Midi Clock
 *
 * @{
 **/

class ClockCallback {
};

typedef void (ClockCallback::*midi_clock_callback_ptr_t)(uint32_t count);

class MidiClockClass {
	/**
	 * \addtogroup midi_clock 
	 *
	 * @{
	 **/
	
public:
	volatile uint32_t indiv96th_counter;
	volatile uint32_t outdiv96th_counter;
	
	volatile uint32_t div96th_counter;
	volatile uint32_t div32th_counter;
	volatile uint32_t div16th_counter;
	volatile uint32_t indiv32th_counter;
	volatile uint32_t indiv16th_counter;
	volatile uint8_t mod6_counter;
	volatile uint8_t inmod6_counter;
	volatile uint16_t interval;
	
	volatile uint16_t counter;
	volatile uint16_t rx_phase;
	
	volatile uint16_t rx_last_clock;
	volatile uint16_t rx_clock;
	volatile bool doUpdateClock;
	
	volatile bool useImmediateClock;
	
	volatile bool updateSmaller;
	uint16_t pll_x;
	uint16_t tempo;
	bool transmit;
	bool isInit;

//    volatile uint16_t mcl_clock;
 //   volatile uint16_t mcl_counter;


	volatile enum {
		PAUSED = 0,
		STARTING = 1,
		STARTED = 2,
	} state;
	
#if defined(MIDIDUINO) || defined(HOST_MIDIDUINO)
	
	
	typedef enum {
		OFF = 0,
		INTERNAL,
		EXTERNAL_UART1,
		EXTERNAL_UART2
	} clock_mode_t;
#define INTERNAL_MIDI INTERNAL
#define EXTERNAL_MIDI EXTERNAL_UART1
	
#else  
	typedef enum {
		OFF = 0,
		INTERNAL_MIDI,
		EXTERNAL_UART1,
		EXTERNAL_UART2
	} clock_mode_t;
	// arduino
	
#ifndef BOARD_ID
#define BOARD_ID 0x80
#endif
	
#endif
	
	clock_mode_t mode;
	
	MidiClockClass();
	
	CallbackVector1<ClockCallback,8, uint32_t> on96Callbacks;
	CallbackVector1<ClockCallback,8, uint32_t> on32Callbacks;
	CallbackVector1<ClockCallback,8, uint32_t> on16Callbacks;
	
	void addOn96Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
		on96Callbacks.add(obj, func);
	}
	void removeOn96Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
		on96Callbacks.remove(obj, func);
	}
    void removeOn96Callback(ClockCallback *obj) {
		on96Callbacks.remove(obj);
	}
	
	void addOn32Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
		on32Callbacks.add(obj, func);
	}
	void removeOn32Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
            on32Callbacks.remove(obj, func);
	}
    void removeOn32Callback(ClockCallback *obj) {
		on32Callbacks.remove(obj);
	}
	
	void addOn16Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
		on16Callbacks.add(obj, func);
	}
	void removeOn16Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
		on16Callbacks.remove(obj, func);
	}
	void removeOn16Callback(ClockCallback *obj) {
		on16Callbacks.remove(obj);
	}
	
	void init();
	void handleClock();
	void handleImmediateClock();
	void updateClockInterval();
	void incrementCounters();
	void callCallbacks();
	
	void handleMidiStart();
	void handleMidiContinue();
	void handleMidiStop();
	void handleTimerInt();
	void handleSongPositionPtr(uint8_t *msg);
	void setSongPositionPtr(uint16_t pos);
	
	void start();
	void stop();
	void pause();
	void setTempo(uint16_t tempo);
	uint16_t getTempo();
	
	bool isStarted() {
		return state == STARTED;
	}
	
	/* @} */
};

extern MidiClockClass MidiClock;

/* @} @} */

#endif /* MIDICLOCK_H__ */

