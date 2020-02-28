#include "MCL_impl.h"

bool Wav::close(bool write) {
  DEBUG_PRINT_FN();
  bool ret;
  if (write) {
    ret = write_header();
    if (!ret) {
      DEBUG_PRINTLN(F("could not write header"));
      return false;
    }
  }
  headerRead = false;
  return file.close();
}
bool Wav::open(const char *file_name, bool overwrite, uint16_t numChannels,
               uint32_t sampleRate, uint8_t bitRate) {
  DEBUG_PRINT_FN();
  bool ret;
  m_strncpy(&filename, file_name, 16);
  DEBUG_PRINTLN(filename);

  if (file.isOpen()) {
    DEBUG_PRINTLN(F("file already open"));
    file.close();
  }
//  bool create_new = !file.exists(filename);
  bool create_new;

  ret = file.open(file_name, O_RDWR | O_CREAT);
  DEBUG_PRINTLN(file.fileSize());
  if (file.fileSize() > sizeof(header)) {
          create_new = false;
  }
  else {
  create_new = true;
  }
  if (!ret) {
    DEBUG_PRINTLN(F("could not open wave file"));
  }
  if ((overwrite) || create_new) {
    DEBUG_PRINTLN(F("truncating"));
    ret = file.truncate(0);
    if (!ret) {
      DEBUG_PRINTLN(F("truncate failed"));
    }

    header.init_header(numChannels, sampleRate, bitRate);
    ret = write_header();
    headerRead = true;
    if (!ret) {
      DEBUG_PRINTLN(F("Write wave header failed"));
      return false;
    }
    DEBUG_PRINTLN(F("offset"));
    DEBUG_PRINTLN(WAV_DATA_OFFSET);
    DEBUG_PRINTLN(file.fileSize());
    data_offset = WAV_DATA_OFFSET;
  } else if (!headerRead) {

    ret = read_header();
    if (!ret) {
      DEBUG_PRINTLN(F("Could not read header"));
      return false;
    }
    headerRead = true;
  }
  return true;
}

bool Wav::rename(char *new_name) {
  if (!file.rename(&file, new_name)) {
    DEBUG_PRINTLN(F("rename failed"));
  }
  m_strncpy(&filename, new_name, 16);
}

bool Wav::write_header() {
  DEBUG_PRINT_FN();
  bool ret;
  ret = write_data(&header, sizeof(header), 0);
  return ret;
}

bool Wav::read_header() {
  DEBUG_PRINT_FN();
  char header_buf[128];
  bool ret;
  uint32_t read_size;

  if (file.fileSize() < sizeof(header)) {
    return false;
  }
  if (file.fileSize() < sizeof(header_buf)) {
    read_size = file.fileSize();
  } else {
    read_size = sizeof(header_buf);
  }

  ret = read_data(&header_buf, read_size, 0);
  if (ret == false) {
    return false;
  }

  char str_data[6] = "data";
  uint8_t n = 0;
  uint8_t x = 0;
  // find 'data' subchunk
  for (x = 0; x < 128 && n < 4; x++) {
    if (str_data[n] == header_buf[x]) {
      n++;
    } else {
      n = 0;
    }
  }
  if (n == 4) {
    data_offset = x + 4;
  } else {
    return false;
  }

  memcpy(&header, &header_buf, sizeof(header));
  if (header.subchunk1Size > 16) {
    memcpy(&header.subchunk2ID, header_buf + (uint32_t)&((WavHeader*)0)->subchunk2ID + header.subchunk1Size - 16, 8);
    DEBUG_PRINT("Non-standard wave header, adjusting =");
    DEBUG_PRINTLN(header.subchunk2Size);
  }
  if ((header.bitRate > 28) || (header.bitRate < 8)) {
    DEBUG_PRINTLN(F("header bitRate is not valid:"));
    DEBUG_PRINTLN(header.bitRate);
    return false;
  }
  return true;
}

bool Wav::write_data(void *data, uint32_t size, uint32_t position) {
  //  DEBUG_PRINTLN(size);
  //  DEBUG_PRINTLN(position);
  //  DEBUG_PRINTLN(file.fileSize());
  bool ret = false;

  ret = file.seekSet(position);

  if (!ret) {
    DEBUG_PRINTLN(F("could not seek"));
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }

  ret = mcl_sd.write_data(data, size, &file);
  return ret;
}

bool Wav::read_data(void *data, uint32_t size, uint32_t position) {
  // DEBUG_PRINT_FN();

  // DEBUG_PRINTLN(size);
  // DEBUG_PRINTLN(position);
  bool ret;

  ret = file.seekSet(position);
  if (!ret) {
    DEBUG_PRINTLN(F("could not seek"));
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }
  if (!file.isOpen()) {
    DEBUG_PRINTLN(F("file not open"));
    return false;
  }
  ret = mcl_sd.read_data(data, size, &file);
  return ret;
}

bool Wav::write_samples(void *data, uint32_t num_samples,
                        uint32_t sample_offset, uint8_t channel,
                        bool writeheader) {
  //  DEBUG_PRINTLN(channel);
  uint32_t position = channel * (header.subchunk2Size / header.numChannels) +
                      sample_offset * (header.bitRate / 8);

  uint32_t size = num_samples * (header.bitRate / 8);
  // DEBUG_PRINTLN(num_samples);
  //  DEBUG_PRINTLN(header.bitRate);

  // DEBUG_PRINTLN(position);
  // DEBUG_PRINTLN(file.fileSize());
  bool ret = write_data(data, size, position + data_offset);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  uint32_t new_subchunk2Size = (position + size) * header.numChannels;
  // DEBUG_PRINT_FN();
  // DEBUG_PRINTLN(position);
  // DEBUG_PRINTLN(num_samples);
  // DEBUG_PRINTLN(sample_offset);
  /*Sample chunk exceeds size of original data chunk, we must extend*/

  if (new_subchunk2Size > header.subchunk2Size) {
    header.subchunk2Size = new_subchunk2Size;
    header.chunkSize = 36 + header.subchunk2Size;
  }
  if (writeheader) {
    ret = write_header();
  }
  if (!ret) {
    DEBUG_PRINTLN(F("write header failed"));
    return false;
  }
  return ret;
}

bool Wav::read_samples(void *data, uint32_t num_samples, uint32_t sample_index,
                       uint8_t channel) {
  DEBUG_PRINT_FN();
  uint8_t  sample_size = header.bitRate / 8;
  uint8_t  nch_sample_size = sample_size * header.numChannels;
  uint32_t position = sample_index * nch_sample_size;

  uint32_t read_size = num_samples * sample_size;
  uint32_t nch_read_size = read_size * header.numChannels;
  uint32_t new_subchunk2Size = position + nch_read_size;
  // DEBUG_PRINTLN(size);
  // DEBUG_PRINTLN(header.bitRate);
  // DEBUG_PRINTLN(num_samples);
  // DEBUG_PRINTLN(header.subchunk2Size);

  // If requested read size extends past channel size, then truncate read
  if (new_subchunk2Size > header.subchunk2Size) {
    DEBUG_PRINTLN(F("read size is greater than file size, adjusting"));
    read_size = (header.subchunk2Size - position) / header.numChannels;
    nch_read_size = read_size * header.numChannels;
  }

  if (header.numChannels == 1) {
    return read_data(data, read_size, position + data_offset);
  }

  // DEBUG_PRINTLN(file.fileSize());
  // DEBUG_PRINTLN(header.bitRate);
  // DEBUG_PRINTLN(position);
  // DEBUG_PRINTLN(size);
  // DEBUG_PRINTLN(data_offset);

  // Sample data is interleaved.
  // 2-channel interleaved data:
  // [L R] [L R] [L R] ... [L R]    [L R] ... [L R]
  // s0    s1    s2        s{n/2-1} s{n/2}    s{n-1}
  char tmp_buf[80];
  position += data_offset;
  // 16-bit stereo full run = 20(L+R) = 80 bytes
  // 24-bit stereo full run = 13(L+R) = 78 bytes
  uint32_t full_run = ((sizeof(tmp_buf) / header.numChannels) / sample_size) * sample_size * header.numChannels;
  while (read_size > 0) {
    uint32_t current_run = min(full_run, read_size * header.numChannels);
    bool ret = read_data(tmp_buf, current_run, position);
    if (!ret) return false;
    position += current_run;
    read_size -= current_run / header.numChannels;
    for(uint32_t p = channel * sample_size; p < current_run; p += nch_sample_size) {
      memcpy(data, tmp_buf + p, sample_size);
      data = (char*)data + sample_size;
    }
  }
  return true;
}

int16_t Wav::find_peak(uint8_t channel) {
  DEBUG_PRINT_FN();
  int16_t peak_value = 0;
  int16_t current_sample = 0;

  uint8_t bytes_per_word = header.bitRate / 8;
  if (header.bitRate % 8 > 0) {
    bytes_per_word++;
  }
  uint32_t num_of_samples =
      (header.subchunk2Size / header.numChannels) / bytes_per_word;

  int16_t buffer_size = 512;

  int16_t buffer[buffer_size];
  int16_t read_size = buffer_size / bytes_per_word;
  uint32_t sample_val = 0;
  uint32_t sample_max = (pow(2, header.bitRate) / 2);
  bool write_header = false;

  DEBUG_PRINTLN(F("read_size"));
  DEBUG_PRINTLN(read_size);
  DEBUG_PRINTLN(num_of_samples);
  DEBUG_PRINTLN(bytes_per_word);
  DEBUG_PRINTLN(buffer_size);
  for (int32_t n = 0; n < num_of_samples; n += read_size) {
    // Adjust read size if too large
    if (n + read_size > num_of_samples) {
      read_size = num_of_samples - n;
    }
    // Read read_size samples.

    if (!read_samples(buffer, read_size, n, channel)) {
      DEBUG_PRINTLN(F("could not read"));
      return false;
    }
    // Itterate through samples in buffer
    for (uint16_t sample = 0; sample < read_size; sample += 1) {

      current_sample = buffer[sample];
      if (peak_value < abs(current_sample)) { peak_value = current_sample; }
    }
  }
  DEBUG_PRINTLN(F("peak found"));
  DEBUG_PRINTLN(peak_value);
  return peak_value;
}

bool Wav::apply_gain(float gain, uint8_t channel) {
  DEBUG_PRINT_FN();

  uint8_t bytes_per_word = header.bitRate / 8;
  if (header.bitRate % 8 > 0) {
    bytes_per_word++;
  }
  uint32_t num_of_samples =
      (header.subchunk2Size / header.numChannels) / bytes_per_word;

  int16_t buffer_size = 512;

  uint8_t buffer[buffer_size];
  int16_t read_size = buffer_size / bytes_per_word;
  uint32_t sample_val = 0;
  uint32_t sample_max = (pow(2, header.bitRate) / 2);
  bool write_header = false;
  DEBUG_PRINTLN(F("read_size"));
  DEBUG_PRINTLN(read_size);
  DEBUG_PRINTLN(num_of_samples);
  DEBUG_PRINTLN(bytes_per_word);
  DEBUG_PRINTLN(buffer_size);
  DEBUG_PRINTLN(gain);
  for (int32_t n = 0; n < num_of_samples; n += read_size) {
    // Adjust read size if too large
    if (n + read_size > num_of_samples) {
      read_size = num_of_samples - n;
    }
    // Read read_size samples.

    if (!read_samples(buffer, read_size, n, channel)) {
      DEBUG_PRINTLN(F("could not read"));
      return false;
    }
    // Itterate through samples in buffer
    for (uint16_t byte_count = 0; byte_count < read_size * bytes_per_word;
         byte_count += bytes_per_word) {

      sample_val = 0;

      // Decode samples from byte stream;

      for (uint8_t b = 0; b < bytes_per_word; b++) {
        sample_val |= ((int32_t)buffer[byte_count]) << (b * 8);
        byte_count++;
      }
      // For signed formats, we need to remove the "sign" bit before
      // performing multiplication

      if (header.bitRate > 8) {

        bool is_signed = ((uint32_t)1 << header.bitRate - 1) & sample_val;

        if (is_signed) {
          // Signed numbers are stored as 2's complement :'-(
          // To perform any calculations we must first convert them to decimal
          // We also need to shake off any unwanted higher bits if we need to
          // perform a comparator operation.

          sample_val = ~(sample_val - 1);
          sample_val = sample_val << (32 - header.bitRate);
          sample_val = sample_val >> (32 - header.bitRate);
          sample_val &= ~((uint32_t)1 << (header.bitRate - 1));
        }

        // Apply gain adjustment
        sample_val = (uint32_t)((float)sample_val * (float)gain);
        if (sample_val > sample_max) {
          sample_val = sample_max;
        }
        if (is_signed) {
          sample_val = ~(sample_val) + 1;
          sample_val |= ((uint32_t)1 << header.bitRate - 1);
        }
      } else {
        // For 8 bit, convert to signed, then perform mult, then convert back.
        sample_val -= sample_max + 1;
        // Apply gain adjustment
        sample_val = (int32_t)((float)sample_val * (float)gain);
        if (sample_val > sample_max) {
          sample_val = sample_max;
        }
        if (sample_val < -sample_max) {
          sample_val = -sample_max;
        }
        sample_val += sample_max + 1;
      }

      // Check to see if gain adjustment is within range
      // otherwise limit.

      byte_count -= bytes_per_word;
      // Convert modified sample back in to byte stream

      for (uint8_t b = 0; b < bytes_per_word; b++) {
        buffer[byte_count + b] =
            (uint8_t)(sample_val >> (8 * (b))) & (uint8_t)0xFF;
      }
    }
    // Write back the adjusted samples
    if (!write_samples(buffer, read_size, n, channel, write_header)) {
      DEBUG_PRINTLN(F("could not write"));
      return false;
    }
    // loop
  }

  return true;
}
