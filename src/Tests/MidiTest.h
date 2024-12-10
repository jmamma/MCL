// MidiTest.h
#pragma once

#include "Test.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "MidiUart.h"

class MIDITest : public Test {
private:
  // Test message counters
  volatile uint8_t messages_sent = 0;
  volatile uint8_t messages_received = 0;

  // Nested test callback class
  class TestCallback : public MidiCallback {
  private:
    MIDITest *parent;

  public:
    TestCallback(MIDITest *p) : parent(p) {}

    void onNoteOn(uint8_t *msg) {
      DEBUG_PRINTLN("Received Note On:");
      DEBUG_PRINTLN("  Channel: " + String(msg[0] & 0x0F));
      DEBUG_PRINTLN("  Note: " + String(msg[1]));
      DEBUG_PRINTLN("  Velocity: " + String(msg[2]));

      if (msg[0] == (MIDI_NOTE_ON | 0) && msg[1] == 60 && msg[2] == 100) {
        parent->increment_received();
        DEBUG_PRINTLN("  -> Valid test message received");
      } else {
        DEBUG_PRINTLN("  -> Unexpected message values");
      }
    }

    void onCC(uint8_t *msg) {
      DEBUG_PRINTLN("Received CC:");
      DEBUG_PRINTLN("  Channel: " + String(msg[0] & 0x0F));
      DEBUG_PRINTLN("  Controller: " + String(msg[1]));
      DEBUG_PRINTLN("  Value: " + String(msg[2]));

      if (msg[0] == (MIDI_CONTROL_CHANGE | 0) && msg[1] == 7 && msg[2] == 100) {
        parent->increment_received();
        DEBUG_PRINTLN("  -> Valid test message received");
      } else {
        DEBUG_PRINTLN("  -> Unexpected message values");
      }
    }
  };

  // Nested sysex listener class
  class TestSysexListener : public MidiSysexListenerClass {
  private:
    MIDITest *parent;

  public:
    TestSysexListener(MIDITest *p) : MidiSysexListenerClass(), parent(p) {
      ids[0] = 0x00;
      ids[1] = 0x13;
      ids[2] = 0x37;
    }

    virtual void end() {
      DEBUG_PRINTLN("Received SYSEX:");
      DEBUG_PRINTLN("  Length: " + String(sysex->get_recordLen()));
      DEBUG_PRINTLN("  Manufacturer ID: " + String(ids[0], HEX) + " " +
                    String(ids[1], HEX) + " " + String(ids[2], HEX));

      if (sysex->get_recordLen() == 5) {
        uint8_t data1 = sysex->getByte(3);
        uint8_t data2 = sysex->getByte(4);
        DEBUG_PRINTLN("  Data bytes: " + String(data1, HEX) + " " +
                      String(data2, HEX));

        if (data1 == 0x11 && data2 == 0x22) {
          parent->increment_received();
          DEBUG_PRINTLN("  -> Valid test message received");
        } else {
          DEBUG_PRINTLN("  -> Unexpected data values");
        }
      } else {
        DEBUG_PRINTLN("  -> Unexpected message length");
      }
    }
  };

  TestCallback *midi_callback;
  TestSysexListener *sysex_listener;

public:
  MIDITest();
  ~MIDITest();

  void setup() override;
  void run_tests() override;
  void increment_received();

private:
  void reset_counters();
  bool run_note_test();
  bool run_cc_test();
  bool run_sysex_test();
};
;
