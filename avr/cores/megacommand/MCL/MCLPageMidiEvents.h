/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef MCLPAGEMIDIEVENTS_H__
#define MCLPAGEMIDIEVENTS_H__

#include "MCL.h"

class MCLPageMidiEvents : public MidiCallback {
  public:
  void setup_callbacks();
  void remove_callbacks();
  virtual void onNoteOnCallback_Midi(uint8_t *msg);
  virtual void onNoteOffCallback_Midi(uint8_t *msg);
  virtual void onNoteOnCallback_Midi2(uint8_t *msg);
  virtual void onNoteOffCallback_Midi2(uint8_t *msg);
  virtual void onControlChangeCallback_Midi(uint8_t *msg);
  virtual void onControlChangeCallback_Midi2(uint8_t *msg);
};
#endif /* MCLPAGEMIDIEVENTS_H__ */
