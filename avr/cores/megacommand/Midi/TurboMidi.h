/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef TURBOMIDI_H__
#define TURBOMIDI_H__

#include "WProgram.h"

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_turbomidi TurboMidi
 *
 * @{
 **/

#ifdef AVR

#define TURBOMIDI_SPEED_REQUEST 0x10
#define TURBOMIDI_SPEED_ANSWER 0x11
#define TURBOMIDI_SPEED_NEGOTIATION_MASTER 0x12
#define TURBOMIDI_SPEED_ACK_SLAVE 0x13
#define TURBOMIDI_SPEED_TEST_MASTER 0x14
#define TURBOMIDI_SPEED_RESULT_SLAVE 0x15
#define TURBOMIDI_SPEED_TEST_MASTER_2 0x16
#define TURBOMIDI_SPEED_RESULT_SLAVE_2 0x17

#define TURBOMIDI_SPEED_1x 0
#define TURBOMIDI_SPEED_2x 1
#define TURBOMIDI_SPEED_3_33x 2
#define TURBOMIDI_SPEED_4x 3
#define TURBOMIDI_SPEED_5x 4
#define TURBOMIDI_SPEED_6_66x 5
#define TURBOMIDI_SPEED_8x 6
#define TURBOMIDI_SPEED_10x 7
#define TURBOMIDI_SPEED_13_3x 8
#define TURBOMIDI_SPEED_16x 9
#define TURBOMIDI_SPEED_20x 10

class TurboMidiSysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup midi_turbomidi
   *
   * @{
   **/
public:
  TurboMidiSysexListenerClass()
      : MidiSysexListenerClass() {
    ids[0] = 0x00;
    ids[1] = 0x20;
    ids[2] = 0x3c;
    currentSpeed = TURBOMIDI_SPEED_1x;
    state = tm_state_normal;
  };

  bool isGenericMessage;

  virtual void start() { isGenericMessage = false; }
  virtual void end_immediate();

  void setup(MidiClass *midi_) { sysex = &(midi_->midiSysex); sysex->addSysexListener(this); }

  void setupTurboMidiSlave();

  bool sendSpeedRequest();
  void sendSpeedNegotiationRequest(uint8_t speed1, uint8_t speed2);
  void sendSpeedTest1(uint8_t speed1);
  void sendSpeedTest2(uint8_t speed2);

  void sendSpeedAnswer();
  void sendSpeedNegotiationAck();
  void sendSpeedTest1Result();
  void sendSpeedTest2Result();
  void midiSpeedPush(uint8_t push_speed);
  bool startTurboMidi();
  void stopTurboMidi();

  static uint32_t tmSpeeds[12];

  uint8_t currentSpeed;
  uint8_t slave_speed1;
  uint8_t slave_speed2;

  void setSpeed(uint8_t speed);

  uint16_t slaveSpeeds;
  uint16_t certifiedSlaveSpeeds;

  static const uint16_t speeds =
      _BV(TURBOMIDI_SPEED_1x) | _BV(TURBOMIDI_SPEED_2x) |
      _BV(TURBOMIDI_SPEED_3_33x) | _BV(TURBOMIDI_SPEED_4x)
      //		| _BV(TURBOMIDI_SPEED_5x)
      //		| _BV(TURBOMIDI_SPEED_6_66x)
      //		| _BV(TURBOMIDI_SPEED_8x)
      ;

  static const uint16_t certifiedSpeeds =
      _BV(TURBOMIDI_SPEED_1x) | _BV(TURBOMIDI_SPEED_2x) |
      _BV(TURBOMIDI_SPEED_3_33x) | _BV(TURBOMIDI_SPEED_4x)
      //		| _BV(TURBOMIDI_SPEED_5x)
      //		| _BV(TURBOMIDI_SPEED_6_66x)
      //		| _BV(TURBOMIDI_SPEED_8x)
      ;

  typedef enum {
    tm_state_normal = 0,

    /* master states */
    tm_master_wait_req_answer,
    tm_master_req_answer_recvd,
    tm_master_wait_speed_ack,
    tm_master_speed_ack_recvd,
    tm_master_wait_test_1,
    tm_master_test_1_recvd,
    tm_master_wait_test_2,
    tm_master_test_2_recvd,
    tm_master_ok,

    /* slave states */
    tm_slave_req_answer_sent,
    tm_slave_wait_speed_neg,
    tm_slave_speed_ack_sent,
    tm_slave_wait_test_1,
    tm_slave_test_1_sent,
    tm_slave_wait_test_2,
    tm_slave_test_2_sent,
    tm_slave_ok,

  } tm_state_t;

  tm_state_t state;

  bool blockForState(tm_state_t state, uint16_t timeout = 3000);

#ifdef HOST_MIDIDUINO
  virtual ~TurboMidiSysexListenerClass() {}
#endif

  /* @} */
};

extern TurboMidiSysexListenerClass TurboMidi;

#endif

/* @} @} */

#endif /* TURBOMIDI_H__ */
