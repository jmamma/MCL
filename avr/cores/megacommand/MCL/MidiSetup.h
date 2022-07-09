/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDISETUP_H__
#define MIDISETUP_H__


class MidiSetup {
  public:
  void cfg_clock_recv();
  void cfg_ports(bool boot = false);
};

extern MidiSetup midi_setup;

#endif /* MIDISETUP_H__ */
