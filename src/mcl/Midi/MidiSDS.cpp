#include "MidiSDS.h"
#include "MidiIDSysex.h"
#include "MidiSDSMessages.h"
#include "MidiSDSSysex.h"
#include "MidiSysexFile.h"
#include "MidiUart.h"
#include "MCLGUI.h"
#include "MD.h"

// ============================================================================
// File Reader Implementations
// ============================================================================

struct SyxReader : SDSFileReader {
  MidiSysexFile file;

  bool open(const char *filename) override {
    return file.open(filename, O_READ);
  }

  void close() override { file.close(); }

  uint32_t size() override { return file.size(); }

  int readPacket(uint8_t *buf, size_t bufsize) override {
    return file.readPacket(buf, bufsize);
  }

  bool hasMorePackets() override { return file.position() < file.size(); }

  uint32_t position() override { return file.position(); }
};

struct WavReader : SDSFileReader {
  Wav *wav;
  uint16_t sample_num;
  bool header_sent;
  uint32_t samples_sent;
  uint32_t total_samples;
  uint8_t bytes_per_word;
  uint8_t midi_bytes_per_word;
  uint32_t sample_offset;
  uint32_t num_samples_per_packet;
  uint8_t sample_format;

  bool open(const char *filename) override {
    if (!wav->open(filename, false))
      return false;
    header_sent = false;
    samples_sent = 0;
    total_samples = wav->header.get_length();

    sample_format = wav->header.fmt.bitRate;
    midi_bytes_per_word = sample_format / 7;
    if (sample_format % 7 > 0)
      midi_bytes_per_word++;

    bytes_per_word = sample_format / 8;
    if (sample_format % 8 > 0)
      bytes_per_word++;

    sample_offset = (pow(2, sample_format) / 2);
    num_samples_per_packet = 120 / midi_bytes_per_word;

    return true;
  }

  void close() override { wav->close(); }

  uint32_t size() override {
    return total_samples * bytes_per_word + 1; // +1 for header
  }

  uint32_t position() override {
    return samples_sent * bytes_per_word + (header_sent ? 1 : 0);
  }

  int buildDumpHeader(uint8_t *buf) {
    buf[0] = 0xF0;
    buf[1] = 0x7E;
    buf[2] = midi_sds.deviceID;
    buf[3] = 0x01;
    buf[4] = sample_num & 0x7F;
    buf[5] = sample_num >> 7;
    buf[6] = sample_format;

    uint32_t samplePeriod;
    midi_sds.setSampleRate(wav->header.fmt.sampleRate);
    samplePeriod = midi_sds.samplePeriod;

      uint32_t loopStart = 0;
    uint32_t loopEnd = 0;
    uint8_t loopType = SDS_LOOP_OFF;
    if (wav->header.smpl.is_active()) {
      wav->header.smpl.to_sds(wav->header.fmt, loopType, loopStart, loopEnd);
    }

    uint32_t vals[4] = {samplePeriod, total_samples, loopStart, loopEnd};
    uint8_t idx = 7;
    for (uint8_t i = 0; i < 4; ++i) {
      uint32_t v = vals[i];
      buf[idx++] = v & 0x7F;
      buf[idx++] = (v >> 7) & 0x7F;
      buf[idx++] = (v >> 14) & 0x7F;
    }
    buf[19] = loopType;
    buf[20] = 0xF7;
    return 21;
  }

  int readPacket(uint8_t *buf, size_t bufsize) override {
    // First packet: send dump header
    if (!header_sent) {
      header_sent = true;
      return buildDumpHeader(buf);
    }

    // Subsequent packets: send data
    if (samples_sent >= total_samples)
      return 0;

    uint8_t samples[120];
    if (!wav->read_samples(&samples, num_samples_per_packet, samples_sent, 0)) {
      return -1;
    }

    uint8_t n = 0, byte_count = 0;
    int32_t encode_val;
    const uint8_t max_n = 120 - midi_bytes_per_word;

    while (n <= max_n && (samples_sent + byte_count / bytes_per_word) < total_samples) {
      // Assemble multi-byte sample value
      uint32_t sample_val = 0;
      for (uint8_t b = 0; b < bytes_per_word; b++) {
        sample_val |= ((uint32_t)samples[byte_count++]) << (b * 8);
      }

      // Convert signed to unsigned (SDS uses 0 as full negative)
      // WAV uses signed, SDS uses unsigned offset
      if (bytes_per_word > 1) {
        sample_val += sample_offset;
      }

      // Pack into 7-bit MIDI bytes, LEFT-JUSTIFIED
      // For 16-bit: byte0 = bits[15:9], byte1 = bits[8:2], byte2 = bits[1:0] << 5
      uint8_t remaining_bits = sample_format;
      for (uint8_t i = 0; i < midi_bytes_per_word - 1; i++) {
        remaining_bits -= 7;
        buf[n++] = (sample_val >> remaining_bits) & 0x7F;
      }
      // Last byte: remaining bits, left-justified in the 7-bit space
      // remaining_bits now contains the number of bits left to encode
      buf[n++] = (sample_val << (7 - remaining_bits)) & 0x7F;
    }

    samples_sent += num_samples_per_packet;
    return n;
  }

  bool hasMorePackets() override {
    return !header_sent || samples_sent < total_samples;
  }
};

// ============================================================================
// Helper Functions
// ============================================================================

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
    for (int i = 0; i < last; ++i) {
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

  for (int i = 0; i < 4; ++i) {
    name[i] = toupper(name[i]);
  }

  MD.setSampleName(slot, name);
}

// ============================================================================
// General MIDI SDS Message Functions
// ============================================================================

void MidiSDSClass::sendGeneralMessage(uint8_t type) {
  uint8_t data[6] = {0xF0, 0x7E, 0x00, 0x00, 0x00, 0xF7};
  data[2] = deviceID;
  data[3] = type;
  data[4] = packetNumber;
  MidiUart.sendRaw(data, 6);
}

void MidiSDSClass::sendAckMessage() { sendGeneralMessage(MIDI_SDS_ACK); }

void MidiSDSClass::sendNakMessage() { sendGeneralMessage(MIDI_SDS_NAK); }

void MidiSDSClass::sendCancelMessage() { sendGeneralMessage(MIDI_SDS_CANCEL); }

void MidiSDSClass::sendWaitMessage() { sendGeneralMessage(MIDI_SDS_WAIT); }

void MidiSDSClass::sendEOFMessage() { sendGeneralMessage(MIDI_SDS_EOF); }

void MidiSDSClass::sendDumpRequest(uint16_t slot) {
  sampleNumber = slot;
  uint8_t data[7] = {0xF0, 0x7E, 0x00, 0x03, 0x00, 0x00, 0xF7};
  data[2] = deviceID;
  data[4] = sampleNumber & 0x7F;
  data[5] = (sampleNumber >> 7) & 0x7F;
  MidiUart.sendRaw(data, 7);
}
bool MidiSDSClass::sendData(uint8_t *buf, uint8_t len) {
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
  return true;
}

void MidiSDSClass::incPacketNumber() {
  packetNumber++;
  if (packetNumber > 0x7F) {
    packetNumber = 0;
  }
}

// ============================================================================
// Handshake and Message Waiting
// ============================================================================

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
  volatile uint16_t start_clock = read_clock_ms();
  MidiSDSSysexListener.msgType = 255;
  do {
    handleIncomingMidi();
  } while ((clock_diff(start_clock, read_clock_ms()) < timeout) &&
           (MidiSDSSysexListener.msgType == 255));
  return MidiSDSSysexListener.msgType;
}

void MidiSDSClass::cancel() {
  DEBUG_PRINTLN(F("cancelling transmission"));
  wav_file.close();
  state = SDS_READY;
}

// ============================================================================
// Consolidated Send File Function
// ============================================================================

bool MidiSDSClass::sendFile(SDSFileReader &reader, const char *filename,
                            uint16_t sample_number, const char *samplename,
                            bool show_progress) {
  uint8_t buf[256];
  int szbuf;
  bool ret = true;
  uint8_t reply;
  uint32_t fsize;
  int show_progress_counter = 0;
  uint16_t latency_ms;

  if (state != SDS_READY || !reader.open(filename)) {
    return false;
  }

  MidiSDSSysexListener.setup(&Midi);
  fsize = reader.size();
  state = SDS_SEND;
  packetNumber = 0;

  // Read and send first packet (header/request)
  szbuf = reader.readPacket(buf, sizeof(buf));
  if (szbuf == -1) {
    ret = false;
    goto cleanup;
  }

  // Validate and update sample number for first packet
  if (buf[1] == 0x7E && (buf[3] == 0x01 || buf[3] == 0x03)) {
    buf[4] = sample_number & 0x7F;
    buf[5] = (sample_number >> 7) & 0x7F;
  }

  MidiUart.sendRaw(buf, szbuf);
  reply = waitForHandshake();
  if (reply == MIDI_SDS_CANCEL) {
    ret = false;
    goto cleanup;
  }

  // Set sample name if provided
  if (samplename) {
    _setName(samplename, sample_number);
  } else if (filename) {
    _setName(filename, sample_number);
  }

  oled_display.clearDisplay();
  latency_ms = (10000UL * sizeof(buf)) / MidiUart.speed + 20;

  DEBUG_PRINTLN("latency");
  DEBUG_PRINTLN(latency_ms);

  // Send remaining packets
  while (reader.hasMorePackets()) {
    const uint32_t pos = reader.position();

    DEBUG_PRINT("pos = ");
    DEBUG_PRINTLN(pos);
    DEBUG_PRINT("fsize = ");
    DEBUG_PRINTLN(fsize);

    if (pos >= fsize)
      break;
    if (key_interface.is_key_down(MDX_KEY_NO)) {
      ret = false;
      goto cleanup;
    }

    if (++show_progress_counter > 10) {
      show_progress_counter = 0;
      if (show_progress) {
        mcl_gui.draw_progress("Sending sample", pos * 80 / fsize, 80);
      }
    }

    szbuf = reader.readPacket(buf, sizeof(buf));
    if (szbuf <= 0) {
      if (szbuf == -1)
        ret = false;
      goto cleanup;
    }

    uint8_t n_retry = 0;
  retry:
    // For data packets (not sysex), wrap in SDS data format
    if (buf[0] != 0xF0) {
      if (!sendData(buf, szbuf)) {
        ret = false;
        goto cleanup;
      }
    } else {
      MidiUart.sendRaw(buf, szbuf);
    }

    if (!hand_shake_state) {
      uint16_t myclock = read_clock_ms();
      while (clock_diff(myclock, read_clock_ms()) < latency_ms)
        ;
    } else if (buf[1] == 0x7E && buf[3] == 0x02) {
      // Expect handshake for data packets
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
        if (n_retry++ > 3) {
          ret = false;
          goto cleanup;
        }
        goto retry;
      }
    }
    incPacketNumber();
  }

cleanup:
  reader.close();
  state = SDS_READY;
  MidiSDSSysexListener.cleanup(&Midi);
  return ret;
}

// ============================================================================
// Public Interface Functions
// ============================================================================

bool MidiSDSClass::sendSyx(const char *filename, uint16_t sample_number) {
  SyxReader reader;
  return sendFile(reader, filename, sample_number, nullptr, true);
}

bool MidiSDSClass::sendWav(const char *filename, const char *samplename,
                           uint16_t sample_number, bool show_progress) {
  WavReader reader;
  reader.wav = &wav_file;
  reader.sample_num = sample_number;
  return sendFile(reader, filename, sample_number, samplename, show_progress);
}

// ============================================================================
// Receive WAV Function
// ============================================================================

bool MidiSDSClass::recvWav(const char *filename, uint16_t sample_number) {
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

  while (true) {
    if (key_interface.is_key_down(MDX_KEY_NO)) {
      goto recv_fail;
    }
    uint8_t msg = waitForMsg(2000);
    if (msg == 255 || msg == MIDI_SDS_CANCEL) {
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
        if (!SD.rename(wav_file.filename, filename)) {
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

MidiSDSClass midi_sds;
