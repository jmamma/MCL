#include "MidiTest.h"
#include "global.h"

MIDITest::MIDITest() : Test("MIDI Loopback Test") {
  midi_callback = new TestCallback(this);
  sysex_listener = new TestSysexListener(this);
}

MIDITest::~MIDITest() {
  delete midi_callback;
  delete sysex_listener;
}

void MIDITest::setup() {
  Midi2.addOnNoteOnCallback(midi_callback,
                            (midi_callback_ptr_t)&TestCallback::onNoteOn);
  Midi2.addOnControlChangeCallback(midi_callback,
                                   (midi_callback_ptr_t)&TestCallback::onCC);
  MidiSysex2.addSysexListener(sysex_listener);
  DEBUG_PRINTLN("MIDI callbacks registered");

  reset_counters();
  DEBUG_PRINTLN("Test counters reset");
}

void MIDITest::cleanup() {
  Midi2.removeOnNoteOnCallback(midi_callback,
                            (midi_callback_ptr_t)&TestCallback::onNoteOn);
  Midi2.removeOnControlChangeCallback(midi_callback,
                                   (midi_callback_ptr_t)&TestCallback::onCC);
  MidiSysex2.removeSysexListener(sysex_listener);
  DEBUG_PRINTLN("MIDI callbacks registered");

  reset_counters();
  DEBUG_PRINTLN("Test counters reset");
}


void MIDITest::run_tests() {
  log_test_start(get_name());

  bool note_test = run_note_test();
  bool cc_test = run_cc_test();
  bool sysex_test = run_sysex_test();

  DEBUG_PRINTLN("\n=== Final Test Results ===");
  log_test_result("Note Test", note_test);
  log_test_result("CC Test", cc_test);
  log_test_result("SYSEX Test", sysex_test);

  test_success = note_test && cc_test && sysex_test;
  test_complete = true;

  DEBUG_PRINTLN(test_success ? "\nAll tests PASSED!" : "\nSome tests FAILED!");
}

void MIDITest::increment_received() { messages_received++; }

void MIDITest::reset_counters() {
  messages_sent = 0;
  messages_received = 0;
  test_complete = false;
  test_success = false;
}

bool MIDITest::run_note_test() {
  log_test_start("Note Test");
  reset_counters();

  DEBUG_PRINTLN("Sending Note On message:");
  DEBUG_PRINTLN("  Channel: 1");
  DEBUG_PRINTLN("  Note: 60 (Middle C)");
  DEBUG_PRINTLN("  Velocity: 100");
  MidiUart2.sendNoteOn(0, 60, 100);
  messages_sent++;

  DEBUG_PRINTLN("Waiting for processing...");
  sleep_ms(10);

  DEBUG_PRINTLN("Processing received messages...");

  bool success = messages_received == messages_sent;
  DEBUG_PRINTLN("Messages sent: " + String(messages_sent));
  DEBUG_PRINTLN("Messages received: " + String(messages_received));

  return success;
}

bool MIDITest::run_cc_test() {
  log_test_start("CC Test");
  reset_counters();

  DEBUG_PRINTLN("Sending CC message:");
  DEBUG_PRINTLN("  Channel: 1");
  DEBUG_PRINTLN("  Controller: 7 (Volume)");
  DEBUG_PRINTLN("  Value: 100");
  MidiUart2.sendCC(0, 7, 100);
  messages_sent++;

  DEBUG_PRINTLN("Waiting for processing...");
  sleep_ms(10);

  DEBUG_PRINTLN("Processing received messages...");

  bool success = messages_received == messages_sent;
  DEBUG_PRINTLN("Messages sent: " + String(messages_sent));
  DEBUG_PRINTLN("Messages received: " + String(messages_received));

  return success;
}

bool MIDITest::run_sysex_test() {
  log_test_start("SYSEX Test");
  reset_counters();

  uint8_t sysex_data[] = {0x00, 0x13, 0x37, 0x11, 0x22};

  DEBUG_PRINTLN("Sending SYSEX message:");
  DEBUG_PRINTLN("  Manufacturer ID: 00 13 37");
  DEBUG_PRINTLN("  Data bytes: 11 22");
  MidiUart2.sendSysex(sysex_data, sizeof(sysex_data));
  messages_sent++;

  DEBUG_PRINTLN("Waiting for processing...");
  sleep_ms(100);

  DEBUG_PRINTLN("Processing received messages...");

  bool success = messages_received == messages_sent;
  DEBUG_PRINTLN("Messages sent: " + String(messages_sent));
  DEBUG_PRINTLN("Messages received: " + String(messages_received));

  return success;
}
