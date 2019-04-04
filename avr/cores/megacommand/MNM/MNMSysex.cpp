#include "MNMMessages.hh"
#include "MNMParams.hh"
#include "MNMSysex.hh"
#include "WProgram.h"
#include "helpers.h"

MNMSysexListenerClass MNMSysexListener;

void MNMSysexListenerClass::start() {
  isMNMEncodedMessage = false;
  isMNMMessage = false;
  msgLen = 0;
  msgCksum = 0;
  sysexCirc.clear();
}

void MNMSysexListenerClass::handleByte(uint8_t byte) {
  if (sysex->len == 3) {
    isMNMEncodedMessage = false;
    if (byte == 0x03) {
      isMNMMessage = true;
    } else {
      isMNMMessage = false;
    }
    return;
  }

  if (isMNMMessage) {
    if (sysex->len == sizeof(monomachine_sysex_hdr)) {
      msgType = byte;
      switch (byte) {
      case MNM_STATUS_RESPONSE_ID:
        // sysex->startRecord();
        break;

      case MNM_GLOBAL_MESSAGE_ID:
      case MNM_KIT_MESSAGE_ID:
      case MNM_SONG_MESSAGE_ID:
        //				sysex->resetRecord();
        isMNMEncodedMessage = false;
        break;

      case MNM_PATTERN_MESSAGE_ID:
        isMNMEncodedMessage = false;
        break;
      }
    }

    if (isMNMEncodedMessage) {
      if (sysex->len >= sizeof(monomachine_sysex_hdr)) {
        if (sysex->len == 9) {
          encoder.init(
              DATA_ENCODER_INIT(midi, sysex->recordLen, sysex->maxRecordLen - sysex->recordLen));
        }
        if (sysex->len < 9) {
          if (sysex->len == 8) {
            msgCksum = byte;
            msgLen++;
          }
          sysex->recordByte(byte);
        } else {
          if (sysexCirc.size() == 4 && byte != 0xF7) {
            uint8_t c = sysexCirc.get(3);
            msgCksum += c;
            msgLen++;
            //	    printf("_pack: %x, byte %x\n", c, byte);
            encoder.pack8(c);
          }
          sysexCirc.put(byte);
        }
      }
    } else {
    }
  }
}

void MNMSysexListenerClass::end_immediate() {
  if (!isMNMMessage)
    return;

  if (isMNMEncodedMessage) {
    uint16_t len = encoder.finish();
    //    printf("%x\n", len);
    if (len > 0) {
      sysex->recordLen += len;
    }
    msgCksum &= 0x3FFF;
    uint16_t realCksum =
        ElektronHelper::to16Bit7(sysexCirc.get(3), sysexCirc.get(2));
    uint16_t realLen =
        ElektronHelper::to16Bit7(sysexCirc.get(1), sysexCirc.get(0));
    if ((msgLen + 4) != realLen) {
#ifdef HOST_MIDIDUINO
      fprintf(stderr, "wrong message len, %d should be %d\n", (msgLen + 4),
              realLen);
#endif
      return;
    }
    if (msgCksum != realCksum) {
#ifdef HOST_MIDIDUINO
      fprintf(stderr, "wrong message cksum, 0x%x should be 0x%x\n", msgCksum,
              realCksum);
#endif
      return;
    }
  }

  switch (msgType) {
  case MNM_STATUS_RESPONSE_ID:
    onStatusResponseCallbacks.call(sysex->getByte(6), sysex->getByte(7));
    break;

  case MNM_GLOBAL_MESSAGE_ID:
    onGlobalMessageCallbacks.call();
    break;

  case MNM_KIT_MESSAGE_ID:
    onKitMessageCallbacks.call();
    break;

  case MNM_PATTERN_MESSAGE_ID:
    onPatternMessageCallbacks.call();
    break;

  case MNM_SONG_MESSAGE_ID:
    onSongMessageCallbacks.call();
    break;
  }
}

void MNMSysexListenerClass::setup(MidiClass *_midi) {
  sysex = &(_midi->midiSysex);
  midi = _midi;
  sysex->addSysexListener(this);
}
