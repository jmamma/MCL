/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDEVENTS_H__
#define MDEVENTS_H__

class MDMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);

};

class MDEvents {
 public:
 MDMidiEvents midi_events;
 MDEvents() {}
 void setup();
};
extern MDEvents md_events;
#endif /* MDEVENTS_H__ */
