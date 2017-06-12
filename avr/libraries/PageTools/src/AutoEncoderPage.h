#ifndef AUTOMDPAGE_H__
#define AUTOMDPAGE_H__

// possibly clever specializition on CCEncoder to avoid duplicaiton of code
// XXX
// read up in template books

// template specialization of handleEvent

#include <GUI.h>
#include <CCHandler.h>

template <typename EncoderType> 
class AutoEncoderPage : public EncoderPage, public ClockCallback {
 public:
  EncoderType realEncoders[4];
  RecordingEncoder<64> recEncoders[4];

  bool muted;

  void on32Callback(uint32_t counter);
  void startRecording();
  void stopRecording();
  void clearRecording();
  void clearRecording(uint8_t i);
  virtual void setup();

  void autoLearnLast4();

  virtual bool handleEvent(gui_event_t *event);
};

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::on32Callback(uint32_t counter) {
  if (muted)
    return;
  for (uint8_t i = 0; i < 4; i++) {
    recEncoders[i].playback(counter & 0xFF);
  }
}

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::startRecording() {
  for (uint8_t i = 0; i < 4; i++) {
    recEncoders[i].startRecording();
  }
}

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::stopRecording() {
  for (uint8_t i = 0; i < 4; i++) {
    recEncoders[i].stopRecording();
  }
}

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::clearRecording() {
  for (uint8_t i = 0; i < 4; i++) {
    recEncoders[i].clearRecording();
  }
}

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::clearRecording(uint8_t i) {
  recEncoders[i].clearRecording();
}

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::setup() {
  muted = false;
  for (uint8_t i = 0; i < 4; i++) {
    realEncoders[i].setName("___");
    recEncoders[i].initRecordingEncoder(&realEncoders[i]);
    encoders[i] = &recEncoders[i];
    ccHandler.addEncoder(&realEncoders[i]);
    // XXX set CC to nothing
  }
  MidiClock.addOn32Callback(this, (midi_clock_callback_ptr_t)&AutoEncoderPage<EncoderType>::on32Callback);
}

template <typename EncoderType>
void AutoEncoderPage<EncoderType>::autoLearnLast4() {
  int8_t ccAssigned[4] = { -1, -1, -1, -1 };
  int8_t encoderAssigned[4] = { -1, -1, -1, -1 };
  incoming_cc_t ccs[4];

  uint8_t count = ccHandler.incomingCCs.size();
  for (uint8_t i = 0; i < count; i++) {
    ccHandler.incomingCCs.getCopy(i, &ccs[i]);
    incoming_cc_t *cc = &ccs[i];
    for (uint8_t j = 0; j < 4; j++) {
      if ((realEncoders[j].getCC() == cc->cc) &&
					(realEncoders[j].getChannel() == cc->channel)) {
				ccAssigned[i] = j;
				encoderAssigned[j] = i;
				break;
      }
    }
  }

  for (uint8_t i = 0; i < count; i++) {
    incoming_cc_t *cc = &ccs[i];
    if (ccAssigned[i] != -1) {
      if ((realEncoders[ccAssigned[i]].getChannel() != cc->channel) &&
					(realEncoders[ccAssigned[i]].getCC() != cc->cc)) {
				realEncoders[ccAssigned[i]].initCCEncoder(cc->channel, cc->cc);
				realEncoders[ccAssigned[i]].setValue(cc->value);
				clearRecording(ccAssigned[i]);
      }
    } else {
      for (uint8_t j = 0; j < 4; j++) {
				if (encoderAssigned[j] == -1) {
					ccAssigned[i] = j;
					encoderAssigned[j] = i;
					realEncoders[ccAssigned[i]].initCCEncoder(cc->channel, cc->cc);
					realEncoders[ccAssigned[i]].setValue(cc->value);
					clearRecording(ccAssigned[i]);
					break;
				}
      }
    }
  }
}

template <typename EncoderType>
bool AutoEncoderPage<EncoderType>::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    autoLearnLast4();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    startRecording();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    stopRecording();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4) || EVENT_RELEASED(event, Buttons.BUTTON4)) {
    return true;
  }
  if (BUTTON_DOWN(Buttons.BUTTON4)) {
    for (uint8_t i = Buttons.ENCODER1; i <= Buttons.ENCODER4; i++) {
      if (EVENT_PRESSED(event, i)) {
				GUI.setLine(GUI.LINE1);
				GUI.flash_string_fill("CLEAR");
				GUI.setLine(GUI.LINE2);
				GUI.flash_put_value(0, i);
				clearRecording(i);
      }
    }
  }
  return false;
}
				      

#endif /* AUTOMDPAGE_H__ */
