
/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "MidiClock.h"
#include "midi-common.hh"
#include "helpers.h"
#include "MidiUartParent.hh"

// #include "MidiUart.h"

// #define DEBUG_MIDI_CLOCK 0

MidiClockClass::MidiClockClass() {
	init();
	mode = OFF;
	setTempo(120);
	transmit = false;
	useImmediateClock = true;
	
}
//global uint_32 mcl_16counter;
void MidiClockClass::init() {
	state = PAUSED;
	counter = 10000;
	rx_clock = rx_last_clock = 0;
	outdiv96th_counter = 0;
	div96th_counter_last = div96th_counter = indiv96th_counter = 0;
	div32th_counter = indiv32th_counter = 0;
	div16th_counter = indiv16th_counter = 0;
	mod6_counter = inmod6_counter = 0;
    mod3_counter = 0;
    pll_x = 200;
	isInit = false;



    //mcl_16counter = 0;
    //mcl_counter = 0;
    //mcl_clock = read_slowclock();;
}


uint16_t midi_clock_diff(uint16_t old_clock, uint16_t new_clock) {
	if (new_clock >= old_clock)
		return new_clock - old_clock;
	else
		return new_clock + (65535 - old_clock);
}

void MidiClockClass::handleMidiStart() {
	if (transmit) {
		MidiUart.sendRaw(MIDI_START);
	    MidiUart2.sendRaw(MIDI_START);
    }
    init();
	state = STARTING;

	
	onMidiStartCallbacks.call(div96th_counter);
}

void MidiClockClass::handleMidiStop() {
	state = PAUSED;
	if (transmit) {
		MidiUart.sendRaw(MIDI_STOP);
        MidiUart2.sendRaw(MIDI_STOP);
    }
    init();

	onMidiStopCallbacks.call(div96th_counter);
}

void MidiClockClass::handleMidiContinue() {
	if (transmit) {
		MidiUart.sendRaw(MIDI_CONTINUE);
        MidiUart2.sendRaw(MIDI_CONTINUE);
    }
    state = STARTING;
	
	onMidiContinueCallbacks.call(div96th_counter);

	counter = 10000;
	rx_clock = rx_last_clock = 0;
	isInit = false;
}

void MidiClockClass::start() {
	if (mode == INTERNAL_MIDI) {
		init();
		state = STARTED;
		if (transmit)
			MidiUart.sendRaw(MIDI_START);
	}
}

void MidiClockClass::stop() {
	if (mode == INTERNAL_MIDI) {
		state = PAUSED;
		if (transmit)
			MidiUart.sendRaw(MIDI_STOP);
	}
}

void MidiClockClass::pause() {
	if (mode == INTERNAL_MIDI) {
		if (state == PAUSED) {
			start();
		} else {
			stop();
		}
	}
}

void MidiClockClass::setTempo(uint16_t _tempo) {
	USE_LOCK();
	SET_LOCK();
	tempo = _tempo;
	//  interval = 62500 / (tempo * 24 / 60);
	interval = (uint32_t)((uint32_t)156250 / tempo) - 16;
	CLEAR_LOCK();
}

void MidiClockClass::handleSongPositionPtr(uint8_t *msg) {
/*
    if (mode == EXTERNAL || mode == EXTERNAL_UART2) {
		uint16_t ptr = (msg[1] & 0x7F) | ((msg[2] & 0x7F) << 7);
		setSongPositionPtr(ptr);
	}
*/
}


void MidiClockClass::setSongPositionPtr(uint16_t pos) {
/*
    if (transmit) {
		uint8_t msg[3] = { MIDI_SONG_POSITION_PTR, 0, 0 };
		msg[1] = pos & 0x7F;
		msg[2] = (pos >> 7) & 0x7F;
		MidiUart.sendRaw(msg, 3);
	}
	
	USE_LOCK();
	SET_LOCK();
	outdiv96th_counter = div96th_counter = indiv96th_counter = pos * 6;
	div32th_counter = indiv32th_counter = pos * 2;
	div16th_counter = indiv16th_counter = pos;
	
	on16Callbacks.call(div16th_counter);
	on32Callbacks.call(div32th_counter);
	
	mod6_counter = inmod6_counter = 0;
	CLEAR_LOCK();
*/
    }

void MidiClockClass::updateClockInterval() {
	if (useImmediateClock)
		return;
	/*
	USE_LOCK();
	
	if (state == STARTED) {
		
		SET_LOCK();
		uint16_t _interval = interval;
		uint16_t _rx_clock = rx_clock;
		uint16_t _rx_last_clock = rx_last_clock;
		CLEAR_LOCK();
		
		uint16_t diff_rx = midi_clock_diff(_rx_last_clock, _rx_clock);
		
		if (diff_rx < 0x80)
			diff_rx = 0x80;
		
		
		uint16_t new_interval = 0;
		
#ifdef DEBUG_MIDI_CLOCK
		//    GUI.setLine(GUI.LINE2);
		//    GUI.put_value16(1, diff_rx);
		//    GUI.put_value16(2, _interval);
#endif
		
		if (!isInit) {
			new_interval = diff_rx;
			isInit = true;
		} else {
			uint32_t bla =
			(((uint32_t)_interval * (uint32_t)pll_x) +
			 (uint32_t)(256 - pll_x) * (uint32_t)diff_rx);
			//      uint8_t rem = bla & 0xFF;
			if (bla > 0xffff) {
				// XXX stop
				// clock weg
			}
			bla >>= 8;
			new_interval = bla;
			//  if (rem > 190) // omg voodoo to correct discarded precision induced drift
			//	new_interval++;
		}
		
		if (new_interval > 0x4FFF) {
			new_interval = 0x4FFF;
		} else if (new_interval < 0x80) {
			new_interval = 0x80;
		}
		
		
		SET_LOCK();
		interval = new_interval;
		CLEAR_LOCK();
	}
    */
}

void MidiClockClass::incrementCounters() {
	if (state == STARTING && (mode == INTERNAL_MIDI || useImmediateClock)) {
		state = STARTED;
        MidiClock.callCallbacks();
	} else if (state == STARTED) {
		div96th_counter++;
		mod6_counter++;
		mod3_counter++;
        if (mod3_counter == 3) {
        mod3_counter = 0;
        }
        if (mod6_counter == 6) {
			mod6_counter = 0;
			div16th_counter++;
			div32th_counter++;
		} else if (mod6_counter == 3) {
			div32th_counter++;
		}
	}
}

void MidiClockClass::callCallbacks() {
	if (state != STARTED)
		return;
	
    //Moved MidiClock callbacks to Main Loop

    	
	static bool inCallback = false;
	if (inCallback) {
		return;
	} else {
		inCallback = true;
	}
	
#ifndef HOST_MIDIDUINO
	sei();
#endif 
    // HOST_MIDIDUINO/
	
	   on96Callbacks.call(div96th_counter);
	
	if (mod6_counter == 0) {
		on16Callbacks.call(div16th_counter);
		on32Callbacks.call(div32th_counter);
	    //mcl_16counter++;
    }
	if (mod6_counter == 3) {
		on32Callbacks.call(div32th_counter);
	}
	
	inCallback = false;

}

void MidiClockClass::handleImmediateClock() {
	uint8_t _mod6_counter = mod6_counter;
	
	if ((transmit) ) {
//       MidiUart.putc(0xF8);
        MidiUart2.m_putc_immediate(0xF8);
		MidiUart.m_putc_immediate(0xF8);
    }
	incrementCounters();
	if (div16th_counter % 4 == 0) {
		setLed();
	} else {
		clearLed();
	}

    //callCallbacks();
}

/* in interrupt on receiving 0xF8 */
void MidiClockClass::handleClock() {
	
	if (useImmediateClock) {
		handleImmediateClock();
		return;
	} 
	
	//  setLed();
	/*
	rx_phase = counter;
#ifdef HOST_MIDIDUINO
	uint16_t my_clock = read_clock();
#else
	uint16_t my_clock = clock;
#endif
	
	rx_last_clock = rx_clock;
	rx_clock = my_clock;
	
	if (state == STARTING) {
		counter = 0;
		state = STARTED;
	} else {
		indiv96th_counter++;
		inmod6_counter++;
		if (inmod6_counter == 6) {
			inmod6_counter = 0;
			indiv16th_counter++;
			indiv32th_counter++;
		} else if (inmod6_counter == 3) {
			indiv32th_counter++;
		}
	}
	
	if (mode == EXTERNAL_MIDI || mode == EXTERNAL_UART2) {
		if (div96th_counter < indiv96th_counter) {
			updateSmaller = true;
			uint16_t phase_add = rx_phase / 4;
			if (phase_add == 0) {
				//	phase_add = rx_phase;
			}
			if (counter > phase_add) {
				counter -= phase_add;
				
			}
		} else {
			updateSmaller = false;
			if (interval > rx_phase) {
				uint16_t phase_add = (interval - rx_phase) / 4;
				if (phase_add == 0) {
					//	  phase_add = (interval - rx_phase);
				}
				counter += phase_add;
			}
		}
	}
	
	//  clearLed();
	*/
}
/* in interrupt on timer */
void MidiClockClass::handleTimerInt()  {
	if (useImmediateClock)
		return;
/*	
	//  setLed2();
	
	static bool inIRQ = false;
	
	//  sei();
#ifdef HOST_MIDIDUINO
	// on the host, always trigger 
	counter = 0;
#endif
	
	if (counter == 0) {
		counter = interval;
		
#ifndef HOST_MIDIDUINO
		sei();
#endif
		
		if (inIRQ) {
			//      setLed2();
			return;
		} else {
			//      clearLed2();
			inIRQ = true;
		}
		
		incrementCounters();
        
	if (state == STARTED) {
			if (transmit) {
				int len = (div96th_counter - outdiv96th_counter);
   			for (int i = 0; i < len; i++) {
					MidiUart.putc_immediate(MIDI_CLOCK);
					outdiv96th_counter++;
				}
			}
			
			if (mode == EXTERNAL_MIDI || mode == EXTERNAL_UART2) {
				if ((div96th_counter < indiv96th_counter) ||
					(div96th_counter > (indiv96th_counter + 1))) {
					div96th_counter = indiv96th_counter;
					div32th_counter = indiv32th_counter;
					div16th_counter = indiv16th_counter;
					mod6_counter = inmod6_counter;
				}
			}
			
			callCallbacks();
			
			inIRQ = false;
		}
	} else {
		counter--;
	}
*/
}

MidiClockClass MidiClock;
