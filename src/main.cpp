#include "Arduino.h"
#include "MCL/MCL.h"
#include "MidiUart.h"
#include "core.h"
#include "global.h"
#include "pico.h"
#include "platform.h"
#include "timers.h"

#include "StackMonitor.h"
#include "MidiTest.h"

MIDITest midi_test;


void setup() {
  DEBUG_INIT();
  sleep_ms(2000);
  DEBUG_PRINTLN("debug mode online");

  StackMonitor::print_stack_info();

  MidiUart.initSerial();
  MidiUart2.initSerial();

  setup_timers();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  std::vector<uint32_t> baud_rates = {
    31250,      // Standard MIDI baud rate
    31250 * 2,  // 2x
    31250 * 4,  // 4x
    31250 * 8,  // 8x
    31250 * 10, // 10x
  };
  midi_test.setup();

  for (uint32_t baud_rate : baud_rates) {
    DEBUG_PRINT("Testing baud rate: ");
    // Set the new baud rate
    DEBUG_PRINTLN(baud_rate);

    MidiUart.set_speed(baud_rate);
    MidiUart2.set_speed(baud_rate);

    sleep_ms(500);

    // Run the tests
    midi_test.run_tests();
    sleep_ms(500);
    // Check the results
    if (midi_test.is_successful()) {
      DEBUG_PRINTLN("All tests completed successfully at this baud rate!");
    } else {
      DEBUG_PRINTLN("Some tests failed at this baud rate!");
    }
    DEBUG_PRINTLN(""); // Add a separator for clarity
    debugBuffer.flush();
  }

}

void loop() {
   debugBuffer.flush();
}
