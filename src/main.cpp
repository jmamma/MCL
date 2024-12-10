#include "Arduino.h"
#include "pico.h"
#include "core.h"
#include "MCL/MCL.h"
#include "platform.h"
#include "global.h"
#include "MidiUart.h"
#include "timers.h"

#include "MidiTest.h"

MIDITest midi_test;

void setup() {
    DEBUG_INIT();
    delay(2000);
    DEBUG_PRINTLN("debug mode online");
    // Setup UART0 interrupt
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq_handler);
    // Setup UART1 interrupt
    irq_set_exclusive_handler(UART1_IRQ, uart1_irq_handler);
    MidiUart.initSerial();
    MidiUart2.initSerial();

    setup_timers();
}

void loop() {
    midi_test.setup();
    midi_test.run_tests();
    if (midi_test.is_successful()) {
        DEBUG_PRINTLN("All tests completed successfully!");
    } else {
        DEBUG_PRINTLN("Some tests failed!");
    }

}
