#pragma once
#include "MCLSd.h"

class MidiSysexFile: public File {
public:

  MidiSysexFile() : File() {}
  // returns -1 if a sysex packet cannot be read.
  int readPacket(uint8_t* buf, size_t szbuf);
};

