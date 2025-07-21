#include "Arduino.h"
#include "MCL.h"
#include "MidiUart.h"
#include "global.h"
#include "platform.h"
#include "irqs.h"
#include "StackMonitor.h"
#include "MidiTest.h"
#include "ISRTiming.h"
#include "oled.h"
#include "SdFat.h"
#include "PageSelectPage.h"
#include "SdFat.h"

void setup() {
  DEBUG_INIT();
  DEBUG_PRINTLN("debug mode online");
  GUI_hardware.init();
  delay(2000);
#ifndef DEBUGMODE
  MidiUartUSB.init();
  #ifdef RUNNING_STATUS_OUT
  MidiUartUSB.running_status_out = 0;
  #endif
#endif
  MidiUart.init();
  MidiUart2.init();
  setup_irqs();

#if !defined(PLATFORM_TBD) && !defined(__AVR__)
  picow_init();
#endif
  mcl.setup();
  GUI.init();
}

void loop() {
   debugBuffer.flush();
   GUI.loop();
   ISRTiming::print_stats();
}
