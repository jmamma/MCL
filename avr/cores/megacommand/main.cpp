#define IS_ISR_ROUTINE

#include <Arduino.h>
#include <WProgram.h>
#include <wiring_private.h>
//#include <Events.h>
extern "C" {
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
}

//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1305.h>

#define OLED_CLK 52
#define OLED_MOSI 51

// Used for software or hardware SPI
#define OLED_CS 42
#define OLED_DC 44
#define OLED_RESET 38

#ifdef OLED_DISPLAY
Adafruit_SSD1305 oled_display(OLED_DC, OLED_RESET, OLED_CS);
#endif
volatile uint8_t MidiUartParent::handle_midi_lock = 0;
// extern MidiClockClass MidiClock;
// extern volatile uint16_t clock = 0;
// extern volatile uint16_t slowclock = 0;

void my_init_ram(void) __attribute__((naked)) __attribute__((used))
__attribute__((section(".init3")));

void my_init_ram(void) {
// Set PL6 as output
//
#ifdef MEGACOMMAND
  DDRL |= _BV(PL6);
  PORTL &= ~(_BV(PL6));
#else
  DDRB |= _BV(PB0);
  PORTB &= ~(_BV(PB0));
#endif
  XMCRA |= _BV(SRE);
  //  MCUCR |= _BV(SRE);
  //  uint8_t *ptr = 0x2000;
  //  unsigned long i = 0;
  //  for (i = 0; i < 60000; i++) {
  //    ptr[i] = 0;
  //  }
}

uint8_t tcnt2;

void timer_init(void) {
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
#error Timer 0 overflow interrupt not set correctly
#endif

  //  TCCR0A = _BV(CS01);
  //   TIMSK |= _BV(TOIE0);

  TCCR1A = _BV(WGM10);              //  | _BV(COM1A1) | _BV(COM1B1);
  TCCR1B |= _BV(CS10) | _BV(WGM12); // every cycle
#ifdef MIDIDUINO_MIDI_CLOCK
//  TIMSK1 |= _BV(TOIE1);
#endif

  // http://www.arduinoslovakia.eu/application/timer-calculator
  // Microcontroller: ATmega2560
  // Created: 2017-10-28T08:18:15.310Z

  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // 5000 Hz (16000000/((49+1)*64))
  OCR1A = 49;
  // CTC
  TCCR1B |= (1 << WGM12);
  // Prescaler 64
  TCCR1B |= (1 << CS11) | (1 << CS10);
// Output Compare Match A Interrupt Enable
#ifdef MEGACOMMAND
  TIMSK1 |= (1 << OCIE1A);
#else
  TIMSK |= (1 << OCIE1A);
#endif
  // TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(CS20) | _BV(CS21); // ) | _BV(CS21);
  // // | _BV(COM21);

  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;
  // 1000 Hz (16000000/((249+1)*64))
  OCR3A = 249;
  // CTC
  TCCR3B |= (1 << WGM32);
  // Prescaler 64
  TCCR3B |= (1 << CS31) | (1 << CS30);
// Output Compare Match A Interrupt Enable
#ifdef MEGACOMMAND
  TIMSK3 |= (1 << OCIE3A);
#else
  ETIMSK |= (1 << OCIE3A);
#endif
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

  // Enable External SRAM
  MCUCR = _BV(SRE);

  // activate lever converter
  SET_BIT(DDRD, PD4);
  SET_BIT(PORTD, PD4);

  DDRE |= _BV(PE4) | _BV(PE5);

  timer_init();
  //  m_init();
}

void (*jump_to_boot)(void) = (void (*)(void))0xFF11;

void start_bootloader(void) {
  cli();
  eeprom_write_word(START_MAIN_APP_ADDR, 0);
  wdt_enable(WDTO_30MS);
  while (1) {
  };
}

void setup();
void loop();
void handleGui();

#define PHASE_FACTOR 16
static inline uint32_t phase_mult(uint32_t val) {
  return (val * PHASE_FACTOR) >> 8;
}

bool in_irq = false;

ISR(TIMER1_COMPA_vect) {

  select_bank(0);

  clock++;
  MidiClock.div192th_countdown++;
  if (MidiClock.state == 2) {
    if (MidiClock.div192th_countdown >= MidiClock.div192_time) {
      if (MidiClock.div192th_counter != MidiClock.div192th_counter_last) {
        MidiClock.increment192Counter();
        MidiClock.div192th_countdown = 0;
        MidiClock.div192th_counter_last = MidiClock.div192th_counter;
        MidiClock.callCallbacks();
      }
    }
  }

  if (!MidiUartParent::handle_midi_lock)  {
   uint8_t _irqlock_tmp = SREG;
   MidiUartParent::handle_midi_lock = 1;
   sei();
   handleIncomingMidi();
   SREG = _irqlock_tmp;
   MidiUartParent::handle_midi_lock = 0;
  }
}

// XXX CMP to have better time

static uint16_t oldsr = 0;
volatile uint8_t *rand_ptr = 0;

ALWAYS_INLINE() void gui_poll() {
  static bool inGui = false;
  if (inGui) {
    return;
  } else {
    inGui = true;
  }

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
#define OUTPUTDDR DDRD
#define OUTPUTPIN PD0

// extern uint16_t myvar;
uint16_t minuteclock = 0;

ISR(TIMER3_COMPA_vect) {

  select_bank(0);

  slowclock++;
  minuteclock++;
  if (minuteclock == 60000) {
    minuteclock = 0;
    clock_minutes++;
  }
  if (abs(slowclock - lastRunningStatusReset) > 3000) {
    MidiUart.resetRunningStatus();
    lastRunningStatusReset = slowclock;
  }

  MidiUart.tickActiveSense();
  MidiUart2.tickActiveSense();
  sei();
#ifdef MIDIDUINO_POLL_GUI_IRQ
  gui_poll();
#endif
}

bool enable_clock_callbacks = true;


void handleIncomingMidi() {
  while (Midi.midiSysex.avail()) {
    Midi.sysexEnd(Midi.midiSysex.msg_rd);
    Midi.midiSysex.get_next_msg();
  }
  while (Midi2.midiSysex.avail()) {
    Midi2.sysexEnd(Midi2.midiSysex.msg_rd);
    Midi2.midiSysex.get_next_msg();
  }
  while (MidiUart.avail()) {
    Midi.handleByte(MidiUart.m_getc());
  }
  // Disable non realtime midi
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
  //handleIncomingMidi();
  if (callLoop) {
    GUI.loop();
  }
}

void setupEventHandlers();
void setupMidiCallbacks();
// void setupClockCallbacks();
int main(void) {
  delay(100);
  init();
  // clearLed();
  // clearLed2();

  uint16_t sr = SR165.read16();
  Buttons.clear();
  Buttons.poll(sr >> 8);
  Encoders.poll(sr);
  oldsr = sr;

  OUTPUTDDR |= _BV(OUTPUTPIN);
  setupEventHandlers();
  setupMidiCallbacks();
  //	setupClockCallbacks();
  sei();

  DEBUG_INIT();

// Set SD card select HIGH before initialising OLED.
#ifdef MEGACOMMAND
  PORTB |= (1 << PB0);
#else
  PORTE |= (1 << PE7);
#endif
  setup();
  for (;;) {
    loop();
    __mainInnerLoop();
  }
  return 0;
}

void handleGui() { pollEventGUI(); }
