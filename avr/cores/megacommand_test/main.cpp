\
#include <Arduino.h>
//#include "../../Libraries/Midi/src/Midi.h"
//#include <../../Libraries/Midi/src/MidiClock.h>

//#include <../../Libraries/CommonTools/src/helpers.h>
#include <WProgram.h>
//#include <Events.hh>
extern "C" {
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
}
//extern MidiClockClass MidiClock;
//extern volatile uint16_t clock = 0;
//extern volatile uint16_t slowclock = 0;
void my_init_ram (void) __attribute__ ((naked))	__attribute__ ((used))  __attribute__ ((section (".init3")));

void my_init_ram (void) {
  //Set PL6 as output
  //
  DDRL |= _BV(PL6);
   PORTL = 0;
  XMCRA |= _BV(SRE);
  //  MCUCR |= _BV(SRE);
  //  uint8_t *ptr = 0x2000;
  //  unsigned long i = 0;
  //  for (i = 0; i < 60000; i++) {
    //    ptr[i] = 0;
  //  }
}

void timer_init(void) {
  TCCR0A = _BV(CS01);
  //  TIMSK |= _BV(TOIE0);
  
  TCCR1A = _BV(WGM10); //  | _BV(COM1A1) | _BV(COM1B1); 
  TCCR1B |= _BV(CS10) | _BV(WGM12); // every cycle
#ifdef MIDIDUINO_MIDI_CLOCK
  TIMSK1 |= _BV(TOIE1);
#endif

  TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(CS20) | _BV(CS21); // ) | _BV(CS21); // | _BV(COM21);
  TIMSK1 |= _BV(TOIE2);
}

void init(void) {
  /** Disable watchdog. **/
  wdt_disable();
  //  wdt_enable(WDTO_15MS);

 // Configure Port C as 8 channels of output. disable pullup resistors.
  DDRC = 0xFF;
  PORTC = 0x00;
  
  /* move interrupts to bootloader section */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(SRE);

  // activate lever converter
  SET_BIT(DDRD, PD4);
  SET_BIT(PORTD, PD4);

  // activate background pwm
  TCCR3B = _BV(WGM32) | _BV(CS30);
  TCCR3A = _BV(WGM30) | _BV(COM3A1);
  OCR3A = 160;

  DDRE |= _BV(PE4) | _BV(PE5);
  //  DDRB |= _BV(PB0);
  //  DDRC |= _BV(PC3);

  timer_init();
}


void (*jump_to_boot)(void) = (void(*)(void))0xFF11;

void start_bootloader(void) {
  cli();
  eeprom_write_word(START_MAIN_APP_ADDR, 0);
	wdt_enable(WDTO_30MS); 
	while(1) {};
}

void setup();
void loop();
void handleGui();

#define PHASE_FACTOR 16
static inline uint32_t phase_mult(uint32_t val) {
  return (val * PHASE_FACTOR) >> 8;
}


ISR(TIMER1_OVF_vect) {

  clock++;
#ifdef MIDIDUINO_MIDI_CLOCK
//  if (MidiClock.state == MidiClock.STARTED) {
 //   MidiClock.handleTimerInt();
 // }
#endif

  //  clearLed2();
}

// XXX CMP to have better time

static uint16_t oldsr = 0;

void gui_poll() {
  static bool inGui = false;
  if (inGui) { 
    return;
  } else {
    inGui = true;
  }
  sei(); // reentrant interrupt

  uint16_t sr = SR165.read16();
  if (sr != oldsr) {
    Buttons.clear();
    Buttons.poll(sr >> 8);
    Encoders.poll(sr);
    oldsr = sr;
    pollEventGUI();
  }
  inGui = false;
}

uint16_t lastRunningStatusReset = 0;

#define OUTPUTPORT PORTD
#define OUTPUTDDR  DDRD
#define OUTPUTPIN PD0

//extern uint16_t myvar;
ISR(TIMER2_OVF_vect) {
 slowclock++;
  if (abs(slowclock - lastRunningStatusReset) > 3000) {
    MidiUart.resetRunningStatus();
    lastRunningStatusReset = slowclock;
 }

	MidiUart.tickActiveSense();
	//MidiUart2.tickActiveSense();
  
  //  SET_BIT(OUTPUTPORT, OUTPUTPIN);

#ifdef MIDIDUINO_POLL_GUI_IRQ
  gui_poll();
#endif
  //  CLEAR_BIT(OUTPUTPORT, OUTPUTPIN);
}

uint8_t sysexBuf[8192];
MidiClass Midi(&MidiUart, sysexBuf, sizeof(sysexBuf));
uint8_t sysexBuf2[512];
MidiClass Midi2(&MidiUart2, sysexBuf2, sizeof(sysexBuf2));

void handleIncomingMidi() {
  while (MidiUart.avail()) {
    Midi.handleByte(MidiUart.m_getc());
  }
 //Disable non realtime midi 
  while (MidiUart2.avail()) {
    Midi2.handleByte(MidiUart2.m_getc());
  }
}

void __mainInnerLoop(bool callLoop) {
  //  SET_BIT(OUTPUTPORT, OUTPUTPIN);
  //  setLed2();
  if ((MidiClock.mode == MidiClock.EXTERNAL_UART1 ||
       MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
    MidiClock.updateClockInterval();
  }

  //  CLEAR_BIT(OUTPUTPORT, OUTPUTPIN);
  handleIncomingMidi();
  
  if (callLoop) {
    GUI.loop();
  }
}

void setupEventHandlers();
void setupMidiCallbacks();
//void setupClockCallbacks();

int main(void) {
  delay(100);
  init();
  clearLed();
  clearLed2();

  uint16_t sr = SR165.read16();
  Buttons.clear();
  Buttons.poll(sr >> 8);
  Encoders.poll(sr);
  oldsr = sr;

  MidiSysex.addSysexListener(&MididuinoSysexListener);

  OUTPUTDDR |= _BV(OUTPUTPIN);
  setup();
	setupEventHandlers();
	setupMidiCallbacks();
//	setupClockCallbacks();
  sei();

  for (;;) {
    __mainInnerLoop();
  }
  return 0;
}

void handleGui() {
  pollEventGUI();
}

