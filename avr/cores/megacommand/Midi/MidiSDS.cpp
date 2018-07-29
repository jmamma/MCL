#include "WProgram.h"

#include "Midi.h"
#include "MidiSDS.hh"
#include "MidiSDSMessages.hh"
#include "MidiSDSSysex.hh"

void MidiSDSClass::sendGeneralMessage(uint8_t type) {
  uint8_t data[6] = {0xF0, 0x7E, 0x00, 0x00, 0x00, 0xF7};
  data[2] = deviceID;
  data[3] = type;
  data[4] = packetNumber;
  MidiUart.sendRaw(data, 6);
  // for (uint8_t n = 0; n < 6; n++) {
  // MidiUart.m_putc(data[n]);
  // }
}

void MidiSDSClass::sendAckMessage() {
  //  DEBUG_PRINT_FN();
  sendGeneralMessage(MIDI_SDS_ACK);
}

void MidiSDSClass::sendNakMessage() {
  DEBUG_PRINT_FN();
  sendGeneralMessage(MIDI_SDS_NAK);
}

void MidiSDSClass::sendCancelMessage() {
  DEBUG_PRINT_FN();
  sendGeneralMessage(MIDI_SDS_CANCEL);
}

void MidiSDSClass::sendWaitMessage() {
  DEBUG_PRINT_FN();
  sendGeneralMessage(MIDI_SDS_WAIT);
}

void MidiSDSClass::sendEOFMessage() {
  DEBUG_PRINT_FN();
  sendGeneralMessage(MIDI_SDS_EOF);
}

void MidiSDSClass::sendDumpRequest(uint16_t slot) {
  DEBUG_PRINT_FN();

  sampleNumber = slot;
  uint8_t data[7] = {0xF0, 0x7E, 0x00, 0x03, 0x00, 0x00, 0xF7};
  data[2] = deviceID;
  data[4] = sampleNumber & 0x7F;
  data[5] = (sampleNumber >> 7) & 0x7F;
  MidiUart.sendRaw(data, 7);
}
uint8_t MidiSDSClass::waitForMsg(uint16_t timeout) {

  MidiSDSSysexListener.msgType = 255;

  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;
  do {
    current_clock = read_slowclock();

    handleIncomingMidi();
    // GUI.display();
  } while ((clock_diff(start_clock, current_clock) < timeout) &&
           (MidiSDSSysexListener.msgType == 255));
  return MidiSDSSysexListener.msgType;
}
void MidiSDSClass::cancel() {
  DEBUG_PRINTLN("cancelling transmission");
  midi_sds.state = 255;
  wav_file.close();
}
bool MidiSDSClass::sendWav(char *filename, uint16_t sample_number,
                           uint8_t loop_type, uint32_t loop_start,
                           uint32_t loop_end) {
  if (!wav_file.open(filename, false)) {
    DEBUG_PRINTLN("Could not open WAV");
    return false;
  }
  packetNumber = 0;
  samplesSoFar = 0;
  sampleNumber = sample_number;
  setSampleRate(wav_file.header.sampleRate);
  sampleLength = (wav_file.header.subchunk2Size / wav_file.header.numChannels) /
                 (wav_file.header.bitRate / 8);
  sampleFormat = wav_file.header.bitRate;
  loopType = loop_type;
  loopStart = loop_start;
  loopEnd = loop_end;
  packetNumber = 0;
  DEBUG_PRINTLN("sending dump");
  DEBUG_PRINTLN(sampleLength);
  sendDumpHeader();
  uint8_t rep = 0;

  midi_sds.state = SDS_SEND;
  if (rep = waitForMsg() == MIDI_SDS_ACK) {
    handShake = true;
  } else if (rep == MIDI_SDS_CANCEL) {
    cancel();
    wav_file.close();
    return false;
  }
  // HandShake disabled.
  bool ret = sendSamples();
  wav_file.close();
  return ret;
}
bool MidiSDSClass::sendSamples() {
  bool ret = false;
  uint8_t midiBytes_per_word = sampleFormat / 7;
  uint8_t bytes_per_word = sampleFormat / 8;
  if (sampleFormat % 8 > 0) {
    bytes_per_word++;
  }
  if (midi_sds.sampleFormat % 7 > 0) {
    midiBytes_per_word++;
  }
  uint32_t sample_offset = (pow(2, sampleFormat) / 2) + 1;
  uint32_t num_of_samples = (120 / midiBytes_per_word);
  uint8_t samples[120];
  uint8_t data[120];
  uint8_t n = 0;
  uint8_t byte_count = 0;
  int32_t encode_val = 0;
  int32_t uencode_val = 0;

  for (samplesSoFar = 0; samplesSoFar < midi_sds.sampleLength;
       samplesSoFar += num_of_samples) {
    // DEBUG_PRINTLN("NUM OF SAMPLES");
    // DEBUG_PRINTLN(num_of_samples);
    ret = wav_file.read_samples(&samples, num_of_samples, samplesSoFar, 0);
 //   DEBUG_PRINTLN(samplesSoFar);
    if (!ret) {
      DEBUG_PRINTLN("could not read");
      return ret;
    }
    byte_count = 0;

    for (n = 0;
         (n <= 120 - midiBytes_per_word) &&
         ((samplesSoFar + byte_count / bytes_per_word) < midi_sds.sampleLength);
         n += midiBytes_per_word) {
      encode_val = 0;
      uencode_val = 0;
      // Shift the value in to b, byte values for wav file.
      //  DEBUG_PRINTLN(byte_count);
      //  DEBUG_PRINTLN(n);
      //   DEBUG_PRINTLN(num_of_samples);

      for (uint8_t b = 0; b < bytes_per_word; b++) {

        encode_val |= ((int32_t)samples[byte_count]) << (b * 8);
        byte_count++;
      }
      // Convert to unsigned

     // DEBUG_PRINTLN((int16_t)encode_val);

      if (bytes_per_word > 1) {
        encode_val = encode_val + sample_offset;
      } else {
        encode_val = encode_val;
      }
      // + 1])  << 16);
      //  DEBUG_PRINTLN((uint16_t)( (int16_t)encode_val +
      //  (int16_t)sample_offset));
      uint8_t bits7;
      uint8_t shift;
      for (shift = 0; shift < midiBytes_per_word; shift++) {
        //    DEBUG_PRINTLN("shift");
        //   DEBUG_PRINTLN(shift + n);
        bits7 = encode_val >> (sampleFormat - (7 * (shift + 1)));
        data[n + shift] = (uint8_t)0x7F & bits7;
      }

      //  DEBUG_PRINTLN(shift + n);
      //  DEBUG_PRINTLN("end shift");
      //    bits7 = encode_val >> (sampleFormat - (7 * (shift + 1)));
      bits7 = encode_val << (8 - shift);
      data[n + shift - 1] = 0x7F & bits7;
    }
    if (!handShake) {

      delay(200);
      sendData(data, n);
    } else {
      uint8_t count = 0;
      uint8_t msgType = 255;
      while ((msgType != MIDI_SDS_ACK) && count < 3) {
        // No message received, assume handshake disabled

        sendData(data, n);
        msgType = waitForMsg(20);
        if (msgType == 255) {
          handShake = false;
          count = 128;
          // Timeout in reply, switch to no-handshake proto
        }
        if (msgType == MIDI_SDS_WAIT) {
          DEBUG_PRINTLN("told to wait");
          msgType = waitForMsg();
          if (msgType != MIDI_SDS_ACK) {
            cancel();
            return false;
          }
        }
        // Nak received. resend last packet.
        count++;
      }
    }
    incPacketNumber();
  }
//  DEBUG_PRINTLN(samplesSoFar);
  return true;
}
void MidiSDSClass::incPacketNumber() {
  packetNumber++;

  if (midi_sds.packetNumber > 0x7F) {
    midi_sds.packetNumber = 0;
  }
}
void MidiSDSClass::sendDumpHeader() {
  uint8_t data[21] = {0xF0, 0x7E, 0x00, 0x1,  0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7};
  data[2] = deviceID;
  data[4] = (sampleNumber & 0x7F);
  data[5] = (sampleNumber >> 7) & 0x7F;
  data[6] = sampleFormat;

  data[7] = (samplePeriod)&0x7F;
  data[8] = (samplePeriod >> 7) & 0x7F;
  data[9] = (samplePeriod >> 14) & 0x7F;

  data[10] = (sampleLength)&0x7F;
  data[11] = (sampleLength >> 7) & 0x7F;
  data[12] = (sampleLength >> 14) & 0x7F;

  data[13] = (loopStart)&0x7F;
  data[14] = (loopStart >> 7) & 0x7F;
  data[15] = (loopStart >> 14) & 0x7F;

  data[16] = (loopEnd)&0x7F;
  data[17] = (loopEnd >> 7) & 0x7F;
  data[18] = (loopEnd >> 14) & 0x7F;

  data[19] = loopType;

  MidiUart.sendRaw(data, 21);
}

bool MidiSDSClass::sendData(uint8_t *buf, uint8_t len) {
  // DEBUG_PRINT_FN();
  // DEBUG_PRINTLN(len);
  if (len > 120)
    return false;
  uint8_t data[5] = {0xF0, 0x7E, 0x00, 0x02, 0x00};
  data[2] = deviceID;
  data[4] = packetNumber;
  uint8_t checksum = 0;
  MidiUart.sendRaw(data, 5);
  for (int i = 1; i < 5; i++)
    checksum ^= data[i];
  for (int i = 0; i < len; i++) {
    MidiUart.m_putc(buf[i]);
    checksum ^= buf[i];
    if (buf[i] > 0x7F) {
      DEBUG_PRINTLN("crap");
    }
  }
  for (int i = len; i < 120; i++)
    MidiUart.m_putc(0x00);
  MidiUart.m_putc(checksum & 0x7F);
  MidiUart.m_putc(0xF7);
  return true;
}
MidiSDSClass midi_sds;
