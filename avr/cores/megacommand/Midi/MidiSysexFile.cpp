#include "MidiSysexFile.h"

int MidiSysexFile::readPacket(uint8_t* buf, size_t szbuf) {
  uint8_t *p = buf;
  do {
    *p = read();
  } while (*p++ != 0xF7 && --szbuf > 0);
  if(buf[0] == 0xF0 && p[-1] == 0xF7) {
    return p - buf;
  } else {
    return -1;
  }
}
