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

    MidiUart.initSerial();
    MidiUart2.initSerial();
    DEBUG_PRINTLN("here");
   setup_timers();
   pinMode(LED_BUILTIN, OUTPUT);
//   digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    delay(200);
    midi_test.setup();
    midi_test.run_tests();
    if (midi_test.is_successful()) {
        DEBUG_PRINTLN("All tests completed successfully!");
    } else {
        DEBUG_PRINTLN("Some tests failed!");
    }
}
