#include "WProgram.h"
#include "helpers.h"
#include "MNMParams.hh"
#include "MNMSysex.hh"
#include "MNMMessages.hh"

MNMSysexListenerClass MNMSysexListener;

void MNMSysexListenerClass::start() {
  isMNMEncodedMessage = false;
  isMNMMessage = false;
  msgLen = 0;
  msgCksum = 0;
  sysexCirc.clear();
}

void MNMSysexListenerClass::handleByte(uint8_t byte) {
  if (MidiSysex.len == 3) {
    isMNMEncodedMessage = false;
    if (byte == 0x03) {
      isMNMMessage = true;
    } else {
      isMNMMessage = false;
    }
    return;
  }
  
  if (isMNMMessage) {
    if (MidiSysex.len == sizeof(monomachine_sysex_hdr)) {
      msgType = byte;
      switch (byte) {
      case MNM_STATUS_RESPONSE_ID:
				// MidiSysex.startRecord();
				break;
	
      case MNM_GLOBAL_MESSAGE_ID:
      case MNM_KIT_MESSAGE_ID:
      case MNM_SONG_MESSAGE_ID:
				//				MidiSysex.resetRecord();
				isMNMEncodedMessage = false;
				break;

      case MNM_PATTERN_MESSAGE_ID:
				isMNMEncodedMessage = false;
				break;
      }
    }

    if (isMNMEncodedMessage) {
      if (MidiSysex.len >= sizeof(monomachine_sysex_hdr)) {
				if (MidiSysex.len == 9) {
					encoder.init(DATA_ENCODER_INIT(MidiSysex.recordBuf + MidiSysex.recordLen,
																				 MidiSysex.maxRecordLen - MidiSysex.recordLen));
				}
				if (MidiSysex.len < 9) {
					if (MidiSysex.len == 8) {
						msgCksum = byte;
						msgLen++;
					}
					MidiSysex.recordByte(byte);
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

void MNMSysexListenerClass::end() {
  if (!isMNMMessage)
    return;

  if (isMNMEncodedMessage) {
    uint16_t len = encoder.finish();
    //    printf("%x\n", len);
    if (len > 0) {
      MidiSysex.recordLen += len;
    }
    msgCksum &= 0x3FFF;
    uint16_t realCksum = ElektronHelper::to16Bit7(sysexCirc.get(3), sysexCirc.get(2));
    uint16_t realLen = ElektronHelper::to16Bit7(sysexCirc.get(1), sysexCirc.get(0));
    if ((msgLen + 4) != realLen) {
#ifdef HOST_MIDIDUINO
      fprintf(stderr, "wrong message len, %d should be %d\n", (msgLen + 4), realLen);
#endif
      return;
    }
    if (msgCksum != realCksum) {
#ifdef HOST_MIDIDUINO
      fprintf(stderr, "wrong message cksum, 0x%x should be 0x%x\n", msgCksum, realCksum);
#endif
      return;
    }
  }

  switch (msgType) {
  case MNM_STATUS_RESPONSE_ID:
    onStatusResponseCallbacks.call(MidiSysex.data[6], MidiSysex.data[7]);
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

void MNMSysexListenerClass::setup() {
  MidiSysex.addSysexListener(this);
}
