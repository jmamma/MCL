
#include <Arduino.h>
#include <wiring_private.h>
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
  PORTL |= ~(_BV(PL6));
  XMCRA |= _BV(SRE);
  //  MCUCR |= _BV(SRE);
  //  uint8_t *ptr = 0x2000;
  //  unsigned long i = 0;
  //  for (i = 0; i < 60000; i++) {
    //    ptr[i] = 0;
  //  }
}

void m_init()
{
	// this needs to be called before setup() or some functions won't
	// work there
	sei();
	
	// on the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)
#if defined(TCCR0A) && defined(WGM01)
	sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);
#endif

	// set timer 0 prescale factor to 64
#if defined(__AVR_ATmega128__)
	// CPU specific: different values for the ATmega128
	sbi(TCCR0, CS02);
#elif defined(TCCR0) && defined(CS01) && defined(CS00)
	// this combination is for the standard atmega8
	sbi(TCCR0, CS01);
	sbi(TCCR0, CS00);
#elif defined(TCCR0B) && defined(CS01) && defined(CS00)
	// this combination is for the standard 168/328/1280/2560
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);
#elif defined(TCCR0A) && defined(CS01) && defined(CS00)
	// this combination is for the __AVR_ATmega645__ series
	sbi(TCCR0A, CS01);
	sbi(TCCR0A, CS00);
#else
	#error Timer 0 prescale factor 64 not set correctly
#endif

	// enable timer 0 overflow interrupt
#if defined(TIMSK) && defined(TOIE0)
	sbi(TIMSK, TOIE0);
#elif defined(TIMSK0) && defined(TOIE0)
	sbi(TIMSK0, TOIE0);
#else
	#error	Timer 0 overflow interrupt not set correctly
#endif

	// timers 1 and 2 are used for phase-correct hardware pwm
	// this is better for motors as it ensures an even waveform
	// note, however, that fast pwm mode can achieve a frequency of up
	// 8 MHz (with a 16 MHz clock) at 50% duty cycle

#if defined(TCCR1B) && defined(CS11) && defined(CS10)
	TCCR1B = 0;

	// set timer 1 prescale factor to 64
	sbi(TCCR1B, CS11);
#if F_CPU >= 8000000L
	sbi(TCCR1B, CS10);
#endif
#elif defined(TCCR1) && defined(CS11) && defined(CS10)
	sbi(TCCR1, CS11);
#if F_CPU >= 8000000L
	sbi(TCCR1, CS10);
#endif
#endif
	// put timer 1 in 8-bit phase correct pwm mode
#if defined(TCCR1A) && defined(WGM10)
	sbi(TCCR1A, WGM10);
#endif

	// set timer 2 prescale factor to 64
#if defined(TCCR2) && defined(CS22)
	sbi(TCCR2, CS22);
#elif defined(TCCR2B) && defined(CS22)
	sbi(TCCR2B, CS22);
//#else
	// Timer 2 not finished (may not be present on this CPU)
#endif

	// configure timer 2 for phase correct pwm (8-bit)
#if defined(TCCR2) && defined(WGM20)
	sbi(TCCR2, WGM20);
#elif defined(TCCR2A) && defined(WGM20)
	sbi(TCCR2A, WGM20);
//#else
	// Timer 2 not finished (may not be present on this CPU)
#endif

#if defined(TCCR3B) && defined(CS31) && defined(WGM30)
	sbi(TCCR3B, CS31);		// set timer 3 prescale factor to 64
	sbi(TCCR3B, CS30);
	sbi(TCCR3A, WGM30);		// put timer 3 in 8-bit phase correct pwm mode
#endif

#if defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D) /* beginning of timer4 block for 32U4 and similar */
	sbi(TCCR4B, CS42);		// set timer4 prescale factor to 64
	sbi(TCCR4B, CS41);
	sbi(TCCR4B, CS40);
	sbi(TCCR4D, WGM40);		// put timer 4 in phase- and frequency-correct PWM mode	
	sbi(TCCR4A, PWM4A);		// enable PWM mode for comparator OCR4A
	sbi(TCCR4C, PWM4D);		// enable PWM mode for comparator OCR4D
#else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
#if defined(TCCR4B) && defined(CS41) && defined(WGM40)
	sbi(TCCR4B, CS41);		// set timer 4 prescale factor to 64
	sbi(TCCR4B, CS40);
	sbi(TCCR4A, WGM40);		// put timer 4 in 8-bit phase correct pwm mode
#endif
#endif /* end timer4 block for ATMEGA1280/2560 and similar */	

#if defined(TCCR5B) && defined(CS51) && defined(WGM50)
	sbi(TCCR5B, CS51);		// set timer 5 prescale factor to 64
	sbi(TCCR5B, CS50);
	sbi(TCCR5A, WGM50);		// put timer 5 in 8-bit phase correct pwm mode
#endif

#if defined(ADCSRA)
	// set a2d prescaler so we are inside the desired 50-200 KHz range.
	#if F_CPU >= 16000000 // 16 MHz / 128 = 125 KHz
		sbi(ADCSRA, ADPS2);
		sbi(ADCSRA, ADPS1);
		sbi(ADCSRA, ADPS0);
	#elif F_CPU >= 8000000 // 8 MHz / 64 = 125 KHz
		sbi(ADCSRA, ADPS2);
		sbi(ADCSRA, ADPS1);
		cbi(ADCSRA, ADPS0);
	#elif F_CPU >= 4000000 // 4 MHz / 32 = 125 KHz
		sbi(ADCSRA, ADPS2);
		cbi(ADCSRA, ADPS1);
		sbi(ADCSRA, ADPS0);
	#elif F_CPU >= 2000000 // 2 MHz / 16 = 125 KHz
		sbi(ADCSRA, ADPS2);
		cbi(ADCSRA, ADPS1);
		cbi(ADCSRA, ADPS0);
	#elif F_CPU >= 1000000 // 1 MHz / 8 = 125 KHz
		cbi(ADCSRA, ADPS2);
		sbi(ADCSRA, ADPS1);
		sbi(ADCSRA, ADPS0);
	#else // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the prescaler is 2
		cbi(ADCSRA, ADPS2);
		cbi(ADCSRA, ADPS1);
		sbi(ADCSRA, ADPS0);
	#endif
	// enable a2d conversions
	sbi(ADCSRA, ADEN);
#endif

	// the bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
#if defined(UCSRB)
	UCSRB = 0;
#elif defined(UCSR0B)
	UCSR0B = 0;
#endif
}
uint8_t tcnt2;
void timer_init(void) {
  TCCR0A = _BV(CS01);
  //  TIMSK |= _BV(TOIE0);

  TCCR1A = _BV(WGM10); //  | _BV(COM1A1) | _BV(COM1B1); 
  TCCR1B |= _BV(CS10) | _BV(WGM12); // every cycle
#ifdef MIDIDUINO_MIDI_CLOCK
  TIMSK1 |= _BV(TOIE1);
#endif

 // TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(CS20) | _BV(CS21); // ) | _BV(CS21); // | _BV(COM21);

  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
    TCCR2B &= ~(1<<WGM22);
    TCCR2B |= (1<<CS22)  | (1<<CS20); // Set bits
      
  TIMSK2 &= ~(1<<OCIE2A);
  
    TCCR2B &= ~(1<<CS21); 
      tcnt2 = 131;
      TCNT2 = tcnt2;
        TIMSK2 |= _BV(TOIE2);
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
//  m_init();
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
  isr_usart1(1);
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
 
       TCNT2 = tcnt2; 
  slowclock++;
   if (MidiClock.div96th_counter != MidiClock.div96th_counter_last) {
  MidiClock.div96th_counter_last = MidiClock.div96th_counter;
  isr_usart1(1);  
  MidiClock.callCallbacks();
  }



  
  if (abs(slowclock - lastRunningStatusReset) > 3000) {
    MidiUart.resetRunningStatus();
    lastRunningStatusReset = slowclock;
 }

	MidiUart.tickActiveSense();
	MidiUart2.tickActiveSense();
  
  //  SET_BIT(OUTPUTPORT, OUTPUTPIN);

#ifdef MIDIDUINO_POLL_GUI_IRQ
  gui_poll();
#endif
  //  CLEAR_BIT(OUTPUTPORT, OUTPUTPIN);
}

uint8_t sysexBuf[8144];
MidiClass Midi(&MidiUart, sysexBuf, sizeof(sysexBuf));
uint8_t sysexBuf2[2800];
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
//  if ((MidiClock.mode == MidiClock.EXTERNAL_UART1 ||
//       MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
 //   MidiClock.updateClockInterval();
 // }


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
  //clearLed();
  //clearLed2();

  uint16_t sr = SR165.read16();
  Buttons.clear();
  Buttons.poll(sr >> 8);
  Encoders.poll(sr);
  oldsr = sr;


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

