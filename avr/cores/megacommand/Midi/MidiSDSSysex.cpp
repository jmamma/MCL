#include "WProgram.h"

#include "MCLSd.h"
#include "math.h"
#include "MidiSDSMessages.h"
#include "MidiSDSSysex.h"
#include "helpers.h"
MidiSDSSysexListenerClass MidiSDSSysexListener;

void MidiSDSSysexListenerClass::start() {

  msgType = 255;
  isSDSMessage = false;
}

void MidiSDSSysexListenerClass::handleByte(uint8_t byte) {}

#define ELEKTRON_ID 0x3C
#define MD_ID 0x02
#define MD_SDS_NAME 0x73

void MidiSDSSysexListenerClass::end() {
  if (sysex->getByte(0) == 0x7E) {
    isSDSMessage = true;
  } else {
    isSDSMessage = false;
    return;
  }

  msgType = sysex->getByte(2);
  switch (msgType) {

    case MIDI_SDS_DATAPACKET:
    if (midi_sds.state != SDS_REC) {
      DEBUG_PRINTLN(F("not in sds rec mode"));
      midi_sds.sendCancelMessage();
      midi_sds.cancel();
      return;
    }
    data_packet();

    break;
  }
}
void MidiSDSSysexListenerClass::end_immediate() {

  if ((sysex->getByte(2) == ELEKTRON_ID) && (sysex->getByte(3) == MD_ID) &&
      (sysex->getByte(5) == MD_SDS_NAME)) {
    sds_slot = sysex->getByte(6);
    sds_name[0] = sysex->getByte(7);
    sds_name[1] = sysex->getByte(8);
    sds_name[2] = sysex->getByte(9);
    sds_name[3] = sysex->getByte(10);
    sds_name_rec = true;
    DEBUG_PRINTLN(F("sample name received"));

    return;
  }

  if (sysex->getByte(0) == 0x7E) {
    isSDSMessage = true;
  } else {
    isSDSMessage = false;
    return;
  }

  msgType = sysex->getByte(2);

  switch (msgType) {
  case MIDI_SDS_DUMPHEADER:

    if (midi_sds.state != SDS_READY) {
      DEBUG_PRINTLN(F("header received, not in ready"));
      return;
    }
    midi_sds.state = SDS_REC;

    dump_header();

    break;


          case MIDI_SDS_DUMPREQUEST:
    dump_request();
    break;

  case MIDI_SDS_ACK:
    ack();
    break;

  case MIDI_SDS_NAK:
    nak();
    break;

  case MIDI_SDS_CANCEL:
    cancel();
    break;

  case MIDI_SDS_WAIT:

    wait();
    break;

  case MIDI_SDS_EOF:
    eof();
    break;
  }
}
void MidiSDSSysexListenerClass::wait() {}
void MidiSDSSysexListenerClass::eof() {}

void MidiSDSSysexListenerClass::cancel() {}

void MidiSDSSysexListenerClass::nak() {}

void MidiSDSSysexListenerClass::ack() {}
void MidiSDSSysexListenerClass::dump_request() {}

void MidiSDSSysexListenerClass::dump_header() {
  sds_name_rec = false;
  uint8_t i = 3;

  midi_sds.sampleNumber = sysex->getByte(i++);
  midi_sds.sampleNumber |= ((uint32_t)sysex->getByte(i++) << 7);

  midi_sds.sampleFormat = sysex->getByte(i++);

  midi_sds.samplePeriod = sysex->getByte(i++);
  midi_sds.samplePeriod |= ((uint32_t)sysex->getByte(i++) << 7);
  midi_sds.samplePeriod |= ((uint32_t)sysex->getByte(i++) << 14);

  // SampleLength in words;
  DEBUG_PRINTLN(sysex->getByte(i));
  midi_sds.sampleLength = (uint32_t)(sysex->getByte(i++));
  midi_sds.sampleLength |= ((uint32_t)sysex->getByte(i++) << 7);
  midi_sds.sampleLength |= ((uint32_t)sysex->getByte(i++) << 14);

  DEBUG_PRINTLN(midi_sds.sampleLength);
  uint32_t sampleRate = 1000000000 / midi_sds.samplePeriod + 1;

  DEBUG_PRINTLN(F("MidiSDS DumpHeader Received"));
  DEBUG_PRINTLN(midi_sds.sampleNumber);
  DEBUG_PRINTLN(midi_sds.sampleFormat);
  DEBUG_PRINTLN(midi_sds.samplePeriod);
  DEBUG_PRINTLN(sampleRate);
  char my_string[16] = "___.wav";

  my_string[0] = (midi_sds.sampleNumber % 1000) / 100 + '0';
  my_string[1] = (midi_sds.sampleNumber % 100) / 10 + '0';
  my_string[2] = (midi_sds.sampleNumber % 10) + '0';
  if ((midi_sds.sampleFormat != 8) && (midi_sds.sampleFormat != 16) &&
      (midi_sds.sampleFormat != 24)) {
    midi_sds.sendNakMessage();
    return;
  }
  bool overwrite = true;
  if (!midi_sds.wav_file.open(my_string, overwrite, 1, sampleRate,
                              midi_sds.sampleFormat)) {
    midi_sds.sendCancelMessage();
    midi_sds.cancel();
  }
  midi_sds.samplesSoFar = 0;
  midi_sds.packetNumber = 0;

  //  midi_sds.sample_offset = (pow(2, midi_sds.sampleFormat) / 2) + 1;

  midi_sds.sample_offset = (pow(2, midi_sds.sampleFormat) / 2);
  midi_sds.midiBytes_per_word = midi_sds.sampleFormat / 7;
  midi_sds.bytes_per_word = midi_sds.sampleFormat / 8;
  if (midi_sds.sampleFormat % 8 > 0) {
    midi_sds.bytes_per_word++;
  }
  if (midi_sds.sampleFormat % 7 > 0) {
    midi_sds.midiBytes_per_word++;
  }
  // temp_file.open("temp_file.sds", FILE_WRITE | O_CREAT);
  ///  temp_file.close();
  if (midi_sds.use_hand_shake) { midi_sds.sendAckMessage(); }
}

void MidiSDSSysexListenerClass::data_packet() {
  uint8_t checksum = 0;
  uint8_t packetNumber = sysex->getByte(3);
  if (packetNumber != midi_sds.packetNumber) {
    DEBUG_PRINTLN(F("Packet received out of order"));
    midi_sds.sendNakMessage();
    return;
  }
  for (uint16_t b = 0; b < sysex->get_recordLen() - 1; b++) {
    checksum ^= sysex->getByte(b);
  }
  if ((sysex->get_recordLen() == 125) &&
      (sysex->getByte(sysex->get_recordLen() - 1) == checksum)) {
    // 120 byte data stream divided in to m words.
    // 7bits per data midi data byte.
    // For an 8bit sample (smallest bit rate), 2 midi data bytes are required.
    // Therefore a maximum of 60 samples can be transfered with a single data
    // packet.
    // temp_file.open("temp_file.sds", FILE_WRITE | O_APPEND);
    //    if (temp_file.write(&MidiSysex, MidiSysex.len) != MidiSysex.len) {
    //    DEBUG_PRINTLN(F("could not write"));
    //}
    // temp_file.close();
    // midi_sds.sendAckMessage();
    // midi_sds.packetNumber += 1;
    //  if (midi_sds.packetNumber > 0x7F) {
    //       midi_sds.packetNumber = 0;
    //    }
    midi_sds.sendWaitMessage();

    uint8_t samples[120];

    uint32_t num_of_samples = 0;
    uint8_t byte_count = 0;

    uint32_t decode_val = 0;
    int16_t signed_val = 0;
    for (uint8_t n = 0;
         n <= 120 - midi_sds.midiBytes_per_word &&
         (num_of_samples + midi_sds.samplesSoFar) < midi_sds.sampleLength;
         n = n + midi_sds.midiBytes_per_word) {
      decode_val = 0;
      signed_val = 0;
      // Decode 7 bit midiBytes in to value
      uint8_t shift;
      //  DEBUG_PRINTLN(F("MIDI_BIN"));
      for (shift = 0; shift < midi_sds.midiBytes_per_word; shift++) {

        decode_val = decode_val << 7;
        decode_val |= (int32_t)(0x7F & sysex->getByte(n + 4 + shift));
        //  Serial.println(sysex->getByte(n + 4 + shift), HEX);
      }
      // Remove 0 padding.
      decode_val = decode_val >> (8 - shift);
      // For bit depth greater than 8 we need to convert the sample
      // from unsigned to signed by subtracting offset

      if (midi_sds.bytes_per_word > 1) {
        signed_val = decode_val - midi_sds.sample_offset;
        // decode_val -= midi_sds.sample_offset;
      } else {
        signed_val = decode_val;
      }
      // Shift the value in to b, byte values for wav file.
      //DEBUG_PRINTLN(" ");
      //DEBUG_PRINT(decode_val);
      //DEBUG_PRINT(",");
      //DEBUG_PRINT(signed_val);
      for (uint8_t b = 0; b < midi_sds.bytes_per_word; b++) {
        samples[byte_count + b] = (uint8_t)((signed_val >> (8 * (b))) & 0xFF);
      }

      byte_count += midi_sds.bytes_per_word;
      num_of_samples++;
    }

    bool ret = false;
    bool write_header = false;
    if (midi_sds.wav_file.write_samples(
            &samples, num_of_samples, midi_sds.samplesSoFar, 0, write_header)) {
      midi_sds.samplesSoFar += num_of_samples;
      if (midi_sds.use_hand_shake) { midi_sds.sendAckMessage(); }
      midi_sds.incPacketNumber();
    } else {
      DEBUG_PRINTLN(F("error writing sds to SDCard"));
      midi_sds.sendCancelMessage();
      midi_sds.state = SDS_READY;
    }
    // DEBUG_PRINT(" ");
    if (midi_sds.samplesSoFar == midi_sds.sampleLength) {
      DEBUG_PRINTLN(F("Sample receive finished"));
      DEBUG_PRINTLN(midi_sds.wav_file.header.subchunk2Size);
      bool write_header = true;
      midi_sds.wav_file.close(write_header);
      if (sds_name_rec) {
        midi_sds.wav_file.rename(sds_name);
      }
      midi_sds.state = SDS_READY;
    }

  } else {
    DEBUG_PRINTLN(F("sds packet checksum error"));
    DEBUG_PRINTLN(midi_sds.packetNumber);
    DEBUG_PRINTLN(sysex->get_recordLen());
    DEBUG_PRINTLN(sysex->getByte(sysex->get_recordLen() - 1));
    DEBUG_PRINT(F(" "));
    DEBUG_PRINT(checksum);
    midi_sds.sendNakMessage();
  }
}

void MidiSDSSysexListenerClass::setup(MidiClass *_midi) { sysex = &(_midi->midiSysex); sysex->addSysexListener(this); }
