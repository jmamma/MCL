#include "Arduino.h"
#include "pico.h"
#include "core.h"
#include "MCL/MCL.h"
#include "platform.h"
#include "global.h"
#include "MidiUart.h"

#define DEBUG_MODE 1

void setup() {
/*
    // Setup UART0 interrupt
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq_handler);
    // Setup UART1 interrupt
    irq_set_exclusive_handler(UART1_IRQ, uart1_irq_handler);

    MidiUart.initSerial();
    MidiUart2.initSerial();
*/
    DEBUG_INIT();
    DEBUG_PRINTLN("debug mode online")
}

void loop() {
   DEBUG_PRINTLN("debu");
   if (g_ms_ticks >= 1000) {
    g_ms_ticks = 0;
    DEBUG_PRINTLN("timer");
   }
}
