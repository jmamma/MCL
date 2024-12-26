#include "Arduino.h"
#include "MCL/MCL.h"
#include "MidiUart.h"
#include "core.h"
#include "global.h"
#include "pico.h"
#include "platform.h"
#include "irqs.h"
#include "StackMonitor.h"
#include "MidiTest.h"
#include "ISRTiming.h"
#include "oled.h"

#include "PageSelectPage.h"

MIDITest midi_test;


void setup() {
  DEBUG_INIT();
  DEBUG_PRINTLN("debug mode online");
//  StackMonitor::print_stack_info();
  ISRTiming::init();

  GUI_hardware.init();
  MidiUart.initSerial();
  MidiUart2.initSerial();

  setup_irqs();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Explicitly set pins to default state
  pinMode(OLED_CS, OUTPUT);
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_CS,HIGH);
  digitalWrite(OLED_RST,HIGH);

  pinMode(SPI1_MISO_PIN, INPUT);
  pinMode(SPI1_MOSI_PIN, OUTPUT);
  pinMode(SPI1_SCK_PIN, OUTPUT);
  pinMode(SPI1_SS_PIN, OUTPUT);

  // Now reinitialize SPI
  SPI1.setRX(SPI1_MISO_PIN);
  SPI1.setTX(SPI1_MOSI_PIN);
  SPI1.setSCK(SPI1_SCK_PIN);

  mcl.setup();
}

void loop() {
   debugBuffer.flush();
   GUI.loop();
   ISRTiming::print_stats();
}
