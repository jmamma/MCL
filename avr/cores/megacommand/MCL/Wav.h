/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAV_H__
#define WAV_H__

#include "MCL.h"
#define WAV_DATA_OFFSET 44

class WavHeader {
public:
  // The RIFF chunk descriptor

  char chunkID[4]; //"RIFF";
  uint32_t chunkSize;
  char format[4]; //"WAVE";

  // The "fmt" sub-chunk
  uint8_t subchunk1ID[4];
  uint32_t subchunk1Size;
  uint16_t audioFormat; // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                        // Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t numChannels; // Number of channels 1=Mono 2=Sterio
  uint32_t sampleRate;  // Sampling Frequency in Hz
  uint32_t byteRate;    // bytes per second
  uint16_t blockAlign;  // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitRate;     // Number of bits per sample
  /* "data" sub-chunk */
  uint8_t subchunk2ID[4]; // "data"  string
  uint32_t subchunk2Size; // Sampled data length

  void init_header(uint16_t numChannels_, uint32_t sampleRate_, uint8_t bitRate_) {
    char str_riff[5] = "RIFF";
    char str_wave[5] = "WAVE";
    char str_fmt[5] = "fmt ";
    char str_data[5] = "data";
    for (uint8_t i = 0; i < 4; i++) {
      chunkID[i] = str_riff[i];
      format[i] = str_wave[i];
      subchunk1ID[i] = str_fmt[i];
      subchunk2ID[i] = str_data[i];
    }
    numChannels = numChannels_;
    sampleRate = sampleRate_;
    bitRate = bitRate_;
    subchunk2Size = 0;
    chunkSize = 36 + subchunk2Size;
    subchunk1Size = 16;
    audioFormat = 1; // PCM
    byteRate = sampleRate * numChannels * (bitRate_ / 8);
    blockAlign = numChannels * (bitRate_ / 8);
  }
};

class Wav {
public:
  WavHeader header;
  bool headerRead = false;
  uint32_t data_offset;
  char filename[16];
  File file;
  Wav() {
  }
  bool open(char *file_name, uint16_t numChannels = 1,
            uint32_t sampleRate = 44100, uint8_t bitRate = 16, bool overwrite = false);
  bool close(bool write = false);
  bool write_header();
  bool read_header();
  bool write_data(void *data, uint32_t size, uint32_t position);
  bool read_data(void *data, uint32_t size, uint32_t position);
  bool write_samples(void *data, uint32_t num_samples,
                     uint32_t sample_offset = 0, uint8_t channel = 0, bool writeheader = true);
  bool read_samples(void *data, uint32_t num_samples,
                    uint32_t sample_offset = 0, uint8_t channel = 0);
  bool rename(char *new_name);
};

#endif /* WAV_H__ */
