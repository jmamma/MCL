#include "MidiSDS.h"
#include "MidiIDSysex.h"
#include "MidiSDSMessages.h"
#include "MidiSDSSysex.h"
#include "MidiSysexFile.h"
#include "MidiUart.h"
#include "MCLGUI.h"
#include "GUI_hardware.h"
#include "platform.h"
#include "../Drivers/MD/MD.h"

namespace {
constexpr uint8_t kProgressUpdateInterval = 10;
constexpr uint8_t kMaxHandshakeRetries = 3;

inline void wait_for_latency(uint16_t latency_ms) {
  if (!latency_ms) {
    return;
  }
  uint16_t start = read_clock_ms();
  while (clock_diff(start, read_clock_ms()) < latency_ms) {
    platform_wait_poll();
  }
}

inline uint16_t calculate_latency(uint32_t uart_speed, size_t packet_bytes) {
  if (uart_speed == 0) {
    return 0;
  }
  return static_cast<uint16_t>((10000UL * packet_bytes) / uart_speed + 20);
}

inline bool packet_requires_data_ack(const uint8_t *buf, uint16_t len) {
  if (len == 0) {
    return false;
  }
  if (buf[0] != 0xF0) {
    return true;
  }
  return len > 4 && buf[1] == 0x7E && buf[3] == 0x02;
}

inline void write_sds_21(uint8_t *buf, uint32_t value) {
  buf[0] = value & 0x7F;
  buf[1] = (value >> 7) & 0x7F;
  buf[2] = (value >> 14) & 0x7F;
}

inline void update_progress(bool show_progress, uint8_t &counter, uint32_t pos,
                            uint32_t total) {
  if (++counter <= kProgressUpdateInterval) {
    return;
  }
  counter = 0;
  if (!show_progress || total == 0) {
    return;
  }
  uint32_t progress = pos >= total ? 80 : (pos * 80 / total);
  mcl_gui.draw_progress("Sending sample", progress, 80);
}

#if defined(__AVR__)
// SDS sends one WAV channel to the MD; keep this packet-sized path local so AVR
// does not pull in Wav::read_samples' generic all-channel reader.
bool __attribute__((noinline))
read_wav_packet_channel0(Wav &wav, uint8_t *data, uint8_t num_samples,
                         uint32_t sample_index) {
  uint8_t sample_size = (wav.header.fmt.bitRate + 7) >> 3;

  uint8_t channels = wav.header.fmt.numChannels;
  if (channels == 0) {
    return false;
  }

  uint8_t frame_size = sample_size * channels;
  uint32_t position = sample_index * frame_size;
  uint16_t read_size = num_samples * sample_size;
  uint16_t frame_read_size = read_size * channels;

  if (position >= wav.header.data.chunk_size) {
    return true;
  }
  uint32_t available = wav.header.data.chunk_size - position;
  if (frame_read_size > available) {
    read_size = available / channels;
    frame_read_size = read_size * channels;
  }

  if (channels == 1) {
    return wav.read_data(data, read_size, position + wav.data_offset);
  }

  char tmp_buf[80];
  uint8_t full_run = (sizeof(tmp_buf) / frame_size) * frame_size;
  if (full_run == 0) {
    return false;
  }
  position += wav.data_offset;
  while (frame_read_size > 0) {
    uint8_t current_run = min((uint16_t)full_run, frame_read_size);
    if (!wav.read_data(tmp_buf, current_run, position)) {
      return false;
    }
    position += current_run;
    frame_read_size -= current_run;
    for (uint8_t p = 0; p < current_run; p += frame_size) {
      memcpy(data, tmp_buf + p, sample_size);
      data += sample_size;
    }
  }
  return true;
}
#endif
} // namespace

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
  bool header_sent;
  uint32_t samples_sent;
  uint32_t total_samples;
  uint8_t bytes_per_word;
  uint8_t midi_bytes_per_word;
  uint32_t sample_offset;
  uint8_t num_samples_per_packet;
  uint8_t sample_format;

  bool open(const char *filename) override {
    if (!wav->open(filename, false))
      return false;
    header_sent = false;
    samples_sent = 0;
    total_samples = wav->header.get_length();

    sample_format = wav->header.fmt.bitRate;
    midi_bytes_per_word = (sample_format + 6) / 7;
    bytes_per_word = (sample_format + 7) >> 3;

    sample_offset = midi_sds_sample_midpoint(sample_format);
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
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = sample_format;

    uint32_t samplePeriod;
    midi_sds.setSampleRate(wav->header.fmt.sampleRate);
    samplePeriod = midi_sds.samplePeriod;

      uint32_t loopStart = 0;
    uint32_t loopEnd = 0;
    uint8_t loopType = SDS_LOOP_OFF;
    if (wav->header.smpl.is_active()) {
      wav->header.smpl.to_sds(loopType, loopStart, loopEnd);
      uint16_t block_align = wav->header.fmt.blockAlign;
      if ((loopEnd > total_samples) && (block_align > 1)) {
        loopStart /= block_align;
        loopEnd /= block_align;
      }
    }

    write_sds_21(buf + 7, samplePeriod);
    write_sds_21(buf + 10, total_samples);
    write_sds_21(buf + 13, loopStart);
    write_sds_21(buf + 16, loopEnd);
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

    uint8_t samples_to_send = num_samples_per_packet;
    uint32_t samples_left = total_samples - samples_sent;
    if (samples_left < samples_to_send) {
      samples_to_send = (uint8_t)samples_left;
    }

    uint8_t samples[120];
    bool read_ok;
#if defined(__AVR__)
    read_ok = read_wav_packet_channel0(*wav, samples, samples_to_send,
                                       samples_sent);
#else
    read_ok = wav->read_samples(&samples, samples_to_send, samples_sent, 0);
#endif
    if (!read_ok) {
      return -1;
    }

    uint8_t n = 0, byte_count = 0;
    for (uint8_t sample = 0; sample < samples_to_send; sample++) {
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

    samples_sent += samples_to_send;
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
  char name[5] = {};
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
    }
  } else {
    name[0] = filename[0];
    name[1] = filename[1];
    name[2] = filename[last - 2];
    name[3] = filename[last - 1];
    name[4] = 0;
  }

  m_toupper(name);

  MD.setSampleName(slot, name);
}

static inline void set_sample_name_if_needed(const char *samplename,
                                             const char *filename,
                                             uint16_t slot) {
  const char *source = (samplename && samplename[0]) ? samplename : filename;
  if (source) {
    _setName(source, slot);
  }
}

// ============================================================================
// General MIDI SDS Message Functions
// ============================================================================

void MidiSDSClass::sendGeneralMessage(uint8_t type) {
  uint8_t data[6] = {0xF0, 0x7E, deviceID, type, packetNumber, 0xF7};
  MD.uart->sendRaw(data, 6);
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
  MD.uart->sendRaw(data, 7);
}
bool MidiSDSClass::sendData(uint8_t *buf, uint8_t len) {
  if (len > 120)
    return false;
  uint8_t data[127];
  data[0] = 0xF0;
  data[1] = 0x7E;
  data[2] = deviceID;
  data[3] = 0x02;
  data[4] = packetNumber;
  uint8_t checksum = 0x7E ^ deviceID ^ 0x02 ^ packetNumber;
  uint8_t n = 5;
  for (uint8_t i = 0; i < len; i++) {
    data[n++] = buf[i];
    checksum ^= buf[i];
  }
  while (n < 125)
    data[n++] = 0x00;
  data[n++] = checksum & 0x7F;
  data[n] = 0xF7;
  MD.uart->m_putc(data, 127);
  return true;
}

void MidiSDSClass::incPacketNumber() {
  packetNumber = (packetNumber + 1) & 0x7F;
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
    platform_wait_poll();
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

bool MidiSDSClass::transmitPacket(uint8_t *buf, uint16_t len) {
  if (len == 0) {
    return false;
  }
  if (buf[0] != 0xF0) {
    if (len > 120) {
      return false;
    }
    return sendData(buf, (uint8_t)len);
  }
  MD.uart->sendRaw(buf, len);
  return true;
}

MidiSDSClass::AckResult MidiSDSClass::awaitDataAck(uint16_t latency_ms) {
  if (!hand_shake_state) {
    wait_for_latency(latency_ms);
    return AckResult::Ok;
  }

  uint8_t reply = waitForMsg(2000);
  if (reply == 255) {
    hand_shake_state = false;
    return AckResult::Ok;
  }

  if (reply == MIDI_SDS_WAIT) {
    reply = waitForMsg();
  }

  if (reply == MIDI_SDS_ACK) {
    return AckResult::Ok;
  }
  if (reply == MIDI_SDS_CANCEL) {
    return AckResult::Cancel;
  }
  return AckResult::Retry;
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
  uint8_t show_progress_counter = 0;
  uint16_t latency_ms;

  if (state != SDS_READY || !reader.open(filename)) {
    return false;
  }

  MidiSDSSysexListener.setup(MD.midi);
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

  if (!transmitPacket(buf, szbuf)) {
    ret = false;
    goto cleanup;
  }

  reply = waitForHandshake();
  if (reply == MIDI_SDS_CANCEL) {
    ret = false;
    goto cleanup;
  }

  // Set sample name if provided
  set_sample_name_if_needed(samplename, filename, sample_number);

  oled_display.clearDisplay();
  latency_ms = calculate_latency(MD.uart->speed, sizeof(buf));

  // Send remaining packets
  while (reader.hasMorePackets()) {
    const uint32_t pos = reader.position();
    if (pos >= fsize) {
      break;
    }

    if (key_interface.is_key_down(MDX_KEY_NO)) {
      ret = false;
      goto cleanup;
    }

    update_progress(show_progress, show_progress_counter, pos, fsize);

    szbuf = reader.readPacket(buf, sizeof(buf));
    if (szbuf <= 0) {
      if (szbuf == -1)
        ret = false;
      goto cleanup;
    }

    uint8_t retries = 0;
    const uint16_t packet_len = (uint16_t)szbuf;
    const bool needs_data_ack = packet_requires_data_ack(buf, packet_len);
    while (true) {
      if (!transmitPacket(buf, packet_len)) {
        ret = false;
        goto cleanup;
      }
      AckResult ack = AckResult::Ok;
      if (needs_data_ack) {
        ack = awaitDataAck(latency_ms);
      } else if (!hand_shake_state) {
        wait_for_latency(latency_ms);
      }
      if (ack == AckResult::Ok) {
        incPacketNumber();
        break;
      }
      if (ack == AckResult::Cancel || ++retries > kMaxHandshakeRetries) {
        ret = false;
        goto cleanup;
      }
    }
  }

cleanup:
  reader.close();
  state = SDS_READY;
  MidiSDSSysexListener.cleanup(MD.midi);
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
  MidiSDSSysexListener.setup(MD.midi);
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
    if (key_interface.is_key_down(MDX_KEY_NO) || BUTTON_DOWN(Buttons.BUTTON1)) {
      sendCancelMessage();
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
        SD.remove(filename);
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
  MidiSDSSysexListener.cleanup(MD.midi);
  return ret;
}

MidiSDSClass midi_sds;
