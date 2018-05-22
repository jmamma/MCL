#include "Wav.h"
#include "MCL.h"

bool Wav::write_header() {
  wave_file.seekSet(0);
  return mcl_sd.write_data(&header, sizeof(header), &wave_file);
}

bool Wav::read_header() {
  wave_file.seekSet(0);
  return mcl_sd.read_data(&header, sizeof(header), &wave_file);
}

bool Wav::write_data(void *data, uint32_t size, uint32_t position) {
  wave_file.seekSet(position);
  return mcl_sd.write_data(data, size, &wave_file);
}

bool Wav::read_data(void *data, uint32_t size, uint32_t position) {
  wave_file.seekSet(position);
  return mcl_sd.read_data(data, size, &wave_file);
}

bool Wav::write_samples(void *data, uint32_t num_samples,
                        uint32_t sample_offset, uint8_t channel) {

  uint32_t position = sizeof(header) +
                      channel * (header.subchunk2Size / header.numChannels) +
                      sample_offset * (header.bitRate / 8);

  uint32_t size = num_samples * (header.bitRate / 8);

  bool ret = write_data(data, size, position);

  if (!ret) {
    return false;
  }
  uint32_t new_subchunk2Size = (sample_offset + size) * header.numChannels;

  /*Sample chunk exceeds size of original data chunk, we must extend*/

  if (new_subchunk2Size > header.subchunk2Size) {
    header.subchunk2Size = new_subchunk2Size;
  }
  return write_header();
}

bool Wav::read_samples(void *data, uint32_t num_samples, uint32_t sample_offset, uint8_t channel) {
  uint32_t position = sizeof(header) +
                      channel * (header.subchunk2Size / header.numChannels) +
                      sample_offset * (header.bitRate / 8);
  uint32_t size = num_samples * (header.bitRate / 8);

  uint32_t new_subchunk2Size =
      sample_offset + num_samples * (header.bitRate / 8) * header.numChannels;

  //If requested read size extends past channel size, then truncate read
  if (sample_offset + size > header.subchunk2Size / header.numChannels) {
    size = (header.subchunk2Size / header.numChannels) - sample_offset;
  }
  return read_data(data, size, position);
}
