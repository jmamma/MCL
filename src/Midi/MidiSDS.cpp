#include "MidiSDS.h"
#include "MidiIDSysex.h"
#include "MidiSDSMessages.h"
#include "MidiSDSSysex.h"
#include "MidiSysexFile.h"
#include "MidiUart.h"
#include "MCLGUI.h"
#include "MD.h"
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
  sendGeneralMessage(MIDI_SDS_NAK);
}

void MidiSDSClass::sendCancelMessage() {
  sendGeneralMessage(MIDI_SDS_CANCEL);
}

void MidiSDSClass::sendWaitMessage() {
  sendGeneralMessage(MIDI_SDS_WAIT);
}

void MidiSDSClass::sendEOFMessage() {
  sendGeneralMessage(MIDI_SDS_EOF);
}

void MidiSDSClass::sendDumpRequest(uint16_t slot) {
  sampleNumber = slot;
  uint8_t data[7] = {0xF0, 0x7E, 0x00, 0x03, 0x00, 0x00, 0xF7};
  data[2] = deviceID;
  data[4] = sampleNumber & 0x7F;
  data[5] = (sampleNumber >> 7) & 0x7F;
  MidiUart.sendRaw(data, 7);
}

uint8_t MidiSDSClass::waitForHandshake() {
  uint8_t rep;
wait:
  rep = waitForMsg();
  if (rep == MIDI_SDS_ACK) {
    hand_shake_state = true;
  } else if (rep == MIDI_SDS_WAIT) {
    goto wait;
  } else {
    // HandShake disabled.
    hand_shake_state = false;
  }
  return rep;
}

uint8_t MidiSDSClass::waitForMsg(uint16_t timeout) {

  volatile uint16_t start_clock = g_clock_ms;
  MidiSDSSysexListener.msgType = 255;
  do {
     handleIncomingMidi();
  } while ((clock_diff(start_clock, g_clock_ms) < timeout) &&
           (MidiSDSSysexListener.msgType == 255));
  return MidiSDSSysexListener.msgType;
}

void MidiSDSClass::cancel() {
  DEBUG_PRINTLN(F("cancelling transmission"));
  wav_file.close();
  state = SDS_READY;
}

static void _setName(const char *filename, uint16_t slot) {
  char name[5];
  int len = strlen(filename);
  int last = len - 1;
  // trim '.wav'
  while (last >= 0 && filename[last] != '.') {
    --last;
  }
  if (last < 0)
    last = len - 1;

  if (last <= 4) {
    for(int i=0;i<last;++i){
      name[i] = filename[i];
      name[last] = 0;
    }
  } else {
    name[0] = filename[0];
    name[1] = filename[1];
    name[2] = filename[last - 2];
    name[3] = filename[last - 1];
    name[4] = 0;
  }

  for(int i=0;i<4;++i) {
    name[i] = toupper(name[i]);
  }

  MD.setSampleName(slot, name);
}

bool MidiSDSClass::sendSyx(const char *filename, uint16_t sample_number) {
  MidiSysexFile file;
  uint8_t buf[256];
  int szbuf;
  bool ret = true;
  uint8_t reply;
  uint32_t fsize;
  int show_progress = 0;
  uint8_t n_retry = 0;
  uint16_t latency_ms = 0;
  if (state != SDS_READY || !file.open(filename, O_READ)) {
    return false;
  }

  MidiSDSSysexListener.setup(&Midi);
  fsize = file.size();
  file.seek(0);

  // 1st packet: sysex request.
  szbuf = file.readPacket(buf, sizeof(buf));
  if (szbuf == -1 || buf[1] != 0x7E || buf[3] != 0x01) {
    ret = false;
    goto cleanup;
  }
  buf[4] = sample_number & 0x7F;
  buf[5] = (sample_number >> 7) & 0x7F;
  state = SDS_SEND;
  MidiUart.sendRaw(buf, szbuf);
  reply = waitForHandshake();
  if (reply == MIDI_SDS_CANCEL) {
    ret = false;
    goto cleanup;
  }

  oled_display.clearDisplay();

  latency_ms = (float) (1000 * sizeof(buf) / ( (float) MidiUart.speed * 0.1f)) + 20;
  DEBUG_PRINTLN("latency");
  DEBUG_PRINTLN(latency_ms);

  while(true) {
    const uint32_t pos = file.position();
    DEBUG_PRINT("pos = ");
    DEBUG_PRINTLN(pos);
    DEBUG_PRINT("fsize = ");
    DEBUG_PRINTLN(fsize);
    if (pos >= fsize) {
      break;
    }
    if (trig_interface.is_key_down(MDX_KEY_NO)) { goto cleanup; }
    if (++show_progress > 10) {
      show_progress = 0;
      mcl_gui.draw_progress("Sending sample", pos * 80 / fsize ,80);
    }
    szbuf = file.readPacket(buf, sizeof(buf));
    if (szbuf == -1) {
      ret = false;
      goto cleanup;
    }
    n_retry = 0;
retry:
    MidiUart.sendRaw(buf, szbuf);
    if (!hand_shake_state) {
      uint16_t myclock = g_clock_ms;
      while (clock_diff(myclock, g_clock_ms) < latency_ms);

    } else if (buf[1] == 0x7E && buf[3] == 0x02) {
      reply = waitForMsg(2000);
      switch (reply) {
        case 255: // nothing came back
          hand_shake_state = false;
          break;
        case MIDI_SDS_WAIT:
          reply = waitForMsg();
          if (reply != MIDI_SDS_ACK) {
            ret = false;
            goto cleanup;
          }
        case MIDI_SDS_ACK:
          break;
        default:
          if(n_retry++ > 3) {
            ret = false;
            goto cleanup;
          }
          goto retry;
      }
    } // otherwise, don't expect ACK reply (maybe MD-specific name-setting command)
  }
  // later packets
cleanup:
  file.close();
  state = SDS_READY;
  MidiSDSSysexListener.cleanup(&Midi);
  return ret;
}

bool MidiSDSClass::sendWav(const char *filename, const char *samplename, uint16_t sample_number, bool show_progress) {
  if (state != SDS_READY) {
    return false;
  }
  if (!wav_file.open(filename, false)) {
    DEBUG_PRINTLN(F("Could not open WAV"));
    return false;
  }
  bool ret = false;
  MidiSDSSysexListener.setup(&Midi);
  packetNumber = 0;
  samplesSoFar = 0;
  sampleNumber = sample_number;
  setSampleRate(wav_file.header.fmt.sampleRate);
  sampleLength =  wav_file.header.get_length();
  sampleFormat = wav_file.header.fmt.bitRate;

  if (wav_file.header.smpl.is_active()) {
    wav_file.header.smpl.to_sds(wav_file.header.fmt, loopType, loopStart, loopEnd);
  } else {
    loopType = SDS_LOOP_OFF;
    loopStart = 0;
    loopEnd = 0;
  }

  packetNumber = 0;
  DEBUG_PRINTLN(F("sending dump"));
  DEBUG_PRINTLN(sampleLength);
  sendDumpHeader();
  uint8_t rep = 0;

  midi_sds.state = SDS_SEND;
  rep = waitForHandshake();
  if (rep == MIDI_SDS_CANCEL) {
    cancel();
    goto fin;
  }
  if (samplename == nullptr) {
  _setName(filename, sample_number);
  }
  else {
  DEBUG_PRINTLN("setting sample name");
  DEBUG_PRINTLN(samplename);
  _setName(samplename, sample_number);
  }
  ret = sendSamples(show_progress);
  fin:
  wav_file.close();
  state = SDS_READY;
  MidiSDSSysexListener.cleanup(&Midi);
  return ret;
}

bool MidiSDSClass::sendSamples(bool show_progress) {
         //MidiUartParent::handle_midi_lock = 0;
  bool ret = false;
  uint8_t midiBytes_per_word = sampleFormat / 7;
  uint8_t bytes_per_word = sampleFormat / 8;
  if (sampleFormat % 8 > 0) {
    bytes_per_word++;
  }
  if (midi_sds.sampleFormat % 7 > 0) {
    midiBytes_per_word++;
  }
  uint32_t sample_offset = (pow(2, sampleFormat) / 2);
  uint32_t num_of_samples = (120 / midiBytes_per_word);
  uint8_t samples[120];
  uint8_t data[120];
  uint8_t n = 0;
  uint8_t byte_count = 0;
  int32_t encode_val = 0;
  int32_t uencode_val = 0;

  uint8_t show_progress_i = 0;

  uint8_t packet = 0;

  uint16_t latency_ms = (float) (1000 * sizeof(data) / ( (float) MidiUart.speed * 0.1f)) + 20;
  DEBUG_PRINTLN("latency");
  DEBUG_PRINTLN(latency_ms);

  uint8_t n_retry = 0;

  for (samplesSoFar = 0; samplesSoFar < midi_sds.sampleLength;
       samplesSoFar += num_of_samples) {
    if (trig_interface.is_key_down(MDX_KEY_NO)) { return false; }
    ++show_progress_i;

    if (show_progress && show_progress_i == 10) {
      show_progress_i = 0;
#ifdef OLED_DISPLAY
      uint32_t progress = samplesSoFar * 80 / midi_sds.sampleLength;
      mcl_gui.draw_progress("Sending sample", progress, 80);
#else
      gfx.display_text("Sending sample", "");
#endif
    }

    ret = wav_file.read_samples(&samples, num_of_samples, samplesSoFar, 0);
    if (!ret) {
      DEBUG_PRINTLN(F("could not read"));
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
      // DEBUG_PRINTLN(byte_count);
      // DEBUG_PRINTLN(n);
      // DEBUG_PRINTLN(num_of_samples);

      for (uint8_t b = 0; b < bytes_per_word; b++) {

        encode_val |= ((int32_t)samples[byte_count]) << (b * 8);
        byte_count++;
      }
      // Convert to unsigned

      // DEBUG_PRINTLN((int16_t)encode_val);

      if (bytes_per_word > 1) {
        encode_val = encode_val + sample_offset;
      } else {
        // encode_val = encode_val;
      }
      // + 1])  << 16);
      //  DEBUG_PRINTLN((uint16_t)( (int16_t)encode_val +
      //  (int16_t)sample_offset));
      uint8_t bits7;
      uint8_t shift;
      for (shift = 0; shift < midiBytes_per_word; shift++) {
        //    DEBUG_PRINTLN(F("shift"));
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

    if (!hand_shake_state) {
      uint16_t myclock = g_clock_ms;
      while (clock_diff(myclock, g_clock_ms) < latency_ms) {};
      sendData(data, n);
    } else {
      uint8_t count = 0;
      uint8_t msgType = 255;
      while ((msgType != MIDI_SDS_ACK) && count < 3) {
        // No message received, assume handshake disabled
        sendData(data, n);
       // msgType = waitForMsg(500);
        msgType = waitForMsg(2000);
        if (msgType == 255) {
          hand_shake_state = false;
          DEBUG_PRINTLN("Reply timeout, switch to no-handshake");
          count = 128;
          // Timeout in reply, switch to no-handshake proto
        }
        if (msgType == MIDI_SDS_CANCEL) {
          DEBUG_PRINTLN("cancel");
          cancel();
          return false;
        }
        if (msgType == MIDI_SDS_NAK) {
          DEBUG_PRINTLN("NAK");
        }
        if (msgType == MIDI_SDS_WAIT) {
          DEBUG_PRINTLN(F("told to wait"));
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

bool MidiSDSClass::recvWav(const char* filename, uint16_t sample_number) {
  bool ret = false;
  if (state != SDS_READY) {
    DEBUG_PRINTLN("SDS in progress aborting");
    return false;
  }
  // init
  MidiSDSSysexListener.setup(&Midi);
  int i = 0;
  uint8_t retries = 3;
  uint8_t m = 255;
  while (retries--) {
    sendDumpRequest(sample_number);

    m = waitForMsg(1000);
    if (MIDI_SDS_DUMPHEADER == m) {
      break;
    }
    DEBUG_PRINTLN("retry sds");
  }
  if (MIDI_SDS_DUMPHEADER != m) {
   DEBUG_PRINTLN("request failed");
  goto recv_fail;
  }
  while(true) {
    if (trig_interface.is_key_down(MDX_KEY_NO)) { goto recv_fail; }
    uint8_t msg = waitForMsg(2000);
    if (msg == 255 || msg == MIDI_SDS_CANCEL)  {
      DEBUG_PRINTLN("sds recv abort");
      goto recv_fail;
    }
    if (midi_sds.state == SDS_READY) {
      DEBUG_PRINTLN("here");
      if (wav_file.file.isOpen()) {
        DEBUG_PRINTLN("wav is open");
        goto recv_fail;
      } else {
        if (SD.exists(filename)) {
          if (!SD.remove(filename)) {
          DEBUG_PRINTLN("could not remove");
          }
        }
        if(!SD.rename(wav_file.filename, filename)) {
          gfx.alert("wav_file rename", "failed :(");
          goto fin;
        }
        DEBUG_PRINTLN("okay");
        ret = true;
        goto fin;
      }
    }
    if (++i < 10) {
      continue;
    }
    i = 0;
#ifdef OLED_DISPLAY
    uint32_t progress = midi_sds.samplesSoFar * 80 / midi_sds.sampleLength;
    mcl_gui.draw_progress("Receiving sample", progress, 80);
#else
    gfx.display_text("Receiving sample", "");
#endif
  }
  recv_fail:
  DEBUG_PRINTLN("Recv fail");
  if (wav_file.file.isOpen()) {
    wav_file.file.remove();
  }
  fin:
  wav_file.close();
  state = SDS_READY;
  MidiSDSSysexListener.cleanup(&Midi);

  return ret;
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
  uint8_t data[127] = {0xF0, 0x7E, deviceID, 0x02, packetNumber};
  uint8_t checksum = 0;
  uint8_t n = 5;
  for (uint8_t i = 1; i < 5; i++)
    checksum ^= data[i];
  for (uint8_t i = 0; i < len; i++) {
    data[n++] = buf[i];
    checksum ^= buf[i];
  }
  for (uint8_t i = len; i < 120; i++)
    data[n++] = 0x00;
  data[n++] = checksum & 0x7F;
  data[n] = 0xF7;
  MidiUart.m_putc(data, 127);
  //DEBUG_PRINT("sending "); DEBUG_PRINT(packetNumber); DEBUG_PRINT(" "); DEBUG_PRINTLN(len);
  return true;
}
MidiSDSClass midi_sds;
