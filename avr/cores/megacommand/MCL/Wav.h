/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAV_H__
#define WAV_H__

#include "MCLSd.h"

// ref: http://www.piclist.com/techref/io/serial/midi/wave.html

typedef char chunkid_t[4];

struct chunk_t {
  chunkid_t chunk_id;
  uint32_t chunk_size;

  /// returns total chunk length.
  uint32_t total_len() const { return chunk_size + sizeof(chunk_t); }

  /// returns extra data length in the chunk.
  template <typename T> uint32_t ex_len() const {
    return total_len() - sizeof(T);
  }

  /// basic chunk initialization with extra data length
  /// set exlen = 0 if no extra data (i.e. fixed-length chunk)
  template <typename T> void activate(uint32_t exlen) {
    memcpy(chunk_id, T::id, sizeof(chunkid_t));
    chunk_size = sizeof(T) - sizeof(chunk_t) + exlen;
  }

  /// clear the chunk id and length, mark the current chunk inactive.
  void deactivate() {
    *(uint32_t *)chunk_id = 0;
    chunk_size = 0;
  }

  /// check the chunk type
  template <typename T> bool is() {
    return 0 == memcmp(chunk_id, T::id, sizeof(chunkid_t));
  }

  bool is_active() const { return *(uint32_t *)chunk_id != 0; }
};

struct fmtchunk_t : public chunk_t {
  uint16_t audioFormat; // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                        // Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t numChannels; // Number of channels 1=Mono 2=Sterio
  uint32_t sampleRate;  // Sampling Frequency in Hz
  uint32_t byteRate;    // bytes per second
  uint16_t blockAlign;  // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitRate;     // Number of bits per sample

  void init(const uint16_t &nchannel, const uint32_t &smplrate,
            const uint8_t &bitrate) {
    numChannels = nchannel;
    sampleRate = smplrate;
    bitRate = bitrate;
    audioFormat = 1; // PCM
    byteRate = sampleRate * nchannel * (bitrate / 8);
    blockAlign = nchannel * (bitrate / 8);

    activate<fmtchunk_t>(0);
  }

  static constexpr char *id = "fmt ";
};

struct datachunk_t : public chunk_t {
  uint8_t data[0];

  static constexpr char *id = "data";

  void init() { activate<datachunk_t>(0); }
};

struct loop_t {
  uint32_t dwIdentifier;
  uint32_t dwType;
  uint32_t dwStart;
  uint32_t dwEnd;
  uint32_t dwFraction;
  uint32_t dwPlayCount;
};

struct wav_sample_t {
  int32_t val;
  uint32_t pos;
};

struct smplchunk_t : public chunk_t {
  uint32_t dwManufacturer;
  uint32_t dwProduct;
  uint32_t dwSamplePeriod;
  uint32_t dwMIDIUnityNote;
  uint32_t dwMIDIPitchFraction;
  uint32_t dwSMPTEFormat;
  uint32_t dwSMPTEOffset;
  uint32_t cSampleLoops;
  uint32_t cbSamplerData;
  loop_t loops[1]; // keep at least 1 loop in-place

  void init(const fmtchunk_t &fmt, uint8_t SDS_loop_type,
            uint32_t SDS_loop_start, uint32_t SDS_loop_end) {
    activate<smplchunk_t>(0);
    dwManufacturer = 0;
    memcpy(&dwProduct, "MCL ", 4);
    dwSamplePeriod = 0;
    dwMIDIUnityNote = 60; // middle C
    dwMIDIPitchFraction = 0;
    dwSMPTEFormat = 0;
    dwSMPTEOffset = 0;
    cSampleLoops = 1;
    cbSamplerData = 0;

    loops[0].dwIdentifier = 0;
    loops[0].dwType = SDS_loop_type;
    loops[0].dwStart = SDS_loop_start * fmt.numChannels * (fmt.bitRate / 8);
    loops[0].dwEnd = SDS_loop_end * fmt.numChannels * (fmt.bitRate / 8);
    loops[0].dwFraction = 0;
    loops[0].dwPlayCount = 0;
  }

  void to_sds(const fmtchunk_t &fmt, uint8_t &SDS_loop_type,
              uint32_t &SDS_loop_start, uint32_t &SDS_loop_end) {
    // only activate SDS looping if we're not dealing with chain/slices
    if (cSampleLoops != 1) {
      return;
    }
    if (loops[0].dwType > 1) { // neither forward nor fw-bw
      return;
    }

    SDS_loop_type = loops[0].dwType;
    SDS_loop_start =
        loops[0].dwStart;          // /  fmt.numChannels / (fmt.bitRate / 8);
    SDS_loop_end = loops[0].dwEnd; // / fmt.numChannels / (fmt.bitRate / 8);

    DEBUG_DUMP(loops[0].dwStart);
    DEBUG_DUMP(loops[0].dwEnd);
    DEBUG_DUMP(fmt.numChannels);
    DEBUG_DUMP(fmt.bitRate);
    DEBUG_DUMP(SDS_loop_start);
    DEBUG_DUMP(SDS_loop_end);
  }

  static constexpr char *id = "smpl";
};

// ref: http://soundfile.sapp.org/doc/WaveFormat/
struct WavHeader {
  // The RIFF chunk descriptor

  chunkid_t chunk_id; // "RIFF"
  uint32_t chunk_size;
  char format[4]; //"WAVE";

  fmtchunk_t fmt;
  datachunk_t data;
  smplchunk_t smpl;

  uint32_t total_len() const {
    uint32_t sz = 12;
    sz += fmt.total_len();
    sz += data.total_len();
    if (smpl.is_active()) {
      sz += smpl.total_len();
    }
    return sz;
  }

  void _fill_chunkinfo() {
    memcpy(chunk_id, "RIFF", 4);
    memcpy(format, "WAVE", 4);
    chunk_size = total_len() - sizeof(chunk_t);
  }

  void init(uint16_t numChannels, uint32_t sampleRate, uint8_t bitRate,
            uint8_t SDS_loop_type, uint32_t SDS_loop_start,
            uint32_t SDS_loop_end) {
    DEBUG_PRINT_FN();
    fmt.init(numChannels, sampleRate, bitRate);
    data.init();
    smpl.init(fmt, SDS_loop_type, SDS_loop_start, SDS_loop_end);
    _fill_chunkinfo();
  }

  void init(uint16_t numChannels, uint32_t sampleRate, uint8_t bitRate) {
    fmt.init(numChannels, sampleRate, bitRate);
    data.init();
    smpl.deactivate();
    _fill_chunkinfo();
  }

  /// deactivates all subchunks
  void init() {
    fmt.deactivate();
    data.deactivate();
    smpl.deactivate();
    _fill_chunkinfo();
  }

  bool check(uint32_t filesize) const {
    if (memcmp(chunk_id, "RIFF", 4)) {
      return false;
    }
    if (memcmp(format, "WAVE", 4)) {
      return false;
    }
    if (filesize != chunk_size + 8) {
      return false;
    }
    return true;
  }
};

class Wav {
public:
  WavHeader header;
  bool headerRead = false;
  uint32_t data_offset;
  uint32_t smpl_offset;
  char filename[16];
  File file;
  Wav() {}
  bool open(char *file_name, bool write = false, uint16_t numChannels = 1,
            uint32_t sampleRate = 44100, uint8_t bitRate = 16,
            bool loop = false);
  bool close(bool write = false);
  bool write_header();
  bool read_header();
  bool write_data(void *data, uint32_t size, uint32_t position);
  bool read_data(void *data, uint32_t size, uint32_t position);
  bool write_samples(void *data, uint32_t num_samples,
                     uint32_t sample_offset = 0, uint8_t channel = 0,
                     bool writeheader = true);
  bool read_samples(void *data, uint32_t num_samples,
                    uint32_t sample_offset = 0, uint8_t channel = 0);
  bool rename(char *new_name);
  int32_t find_peak(uint8_t channel = 0, uint32_t num_samples = 0,
                    uint32_t sample_index = 0);
  void find_peaks(uint32_t num_samples = 0, uint32_t sample_index = 0,
                  wav_sample_t *c0_max_sample = NULL,
                  wav_sample_t *c0_min_sample = NULL,
                  wav_sample_t *c1_max_sample = NULL,
                  wav_sample_t *c1_min_sample = NULL);
  bool apply_gain(float gain, uint8_t channel = 0);
};

#endif /* WAV_H__ */
