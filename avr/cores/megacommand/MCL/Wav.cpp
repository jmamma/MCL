#include "MCL.h"
#include "Wav.h"

bool Wav::close(bool write) {
  DEBUG_PRINT_FN();
  bool ret;
  if (write) {
    ret = write_header();
    if (!ret) {
      DEBUG_PRINTLN("could not write header");
      return false;
    }
  }
  headerRead = false;
  return file.close();
}

bool Wav::open(char *file_name, bool write, uint16_t numChannels,
               uint32_t sampleRate, uint8_t bitRate, bool loop) {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(filename);
  uint8_t file_mode = write ? (O_WRITE | O_CREAT) : (O_READ);
  m_strncpy(filename, file_name, 16);
  headerRead = false;

  if (file.isOpen()) {
    DEBUG_PRINTLN("file already open");
    goto failed;
  }

  if (!file.open(file_name, file_mode)) {
    DEBUG_PRINTLN("could not open wave file");
    goto failed;
  }

  DEBUG_PRINTLN(file.fileSize());
  if (write) {
    DEBUG_PRINTLN("truncating");
    if (!file.truncate(0)) {
      DEBUG_PRINTLN("truncate failed");
      goto failed;
    }

    if (loop) {
      header.init(numChannels, sampleRate, bitRate, SDS_LOOP_FORWARD, 0, 0);
    } else {
      header.init(numChannels, sampleRate, bitRate);
    }
    if (!write_header()) {
      DEBUG_PRINTLN("Write wave header failed");
      goto failed;
    }

    headerRead = true;
    DEBUG_DUMP(data_offset);
    DEBUG_DUMP(file.fileSize());
  } else {
    if (!read_header()) {
      DEBUG_PRINTLN("Could not read header");
      goto failed;
    }
    headerRead = true;
  }
  return true;
failed:
  file.close();
  return false;
}

bool Wav::rename(char *new_name) {
  if (!file.rename(&file, new_name)) {
    DEBUG_PRINTLN("rename failed");
  }
  m_strncpy(&filename, new_name, 16);
}

/// write layout:
/// RIFF head
/// fmt
/// smpl (optional)
/// data
bool Wav::write_header() {
  DEBUG_PRINT_FN();
  uint32_t chunk_offset = 12;
  // write RIFF chunk header
  if (!write_data(&header, chunk_offset, 0)) {
    return false;
  }
  if (!write_data(&header.fmt, header.fmt.total_len(), chunk_offset)) {
    return false;
  }
  chunk_offset += header.fmt.total_len();
  DEBUG_DUMP(header.fmt.total_len());
  DEBUG_DUMP(chunk_offset);
  if (header.smpl.is_active()) {
    if (!write_data(&header.smpl, header.smpl.total_len(), chunk_offset)) {
      return false;
    }
    chunk_offset += header.smpl.total_len();
    DEBUG_DUMP(header.smpl.total_len());
    DEBUG_DUMP(chunk_offset);
  }
  if (!write_data(&header.data, sizeof(datachunk_t), chunk_offset)) {
    return false;
  }
  chunk_offset += sizeof(datachunk_t);

  data_offset = chunk_offset;
  DEBUG_DUMP(data_offset);
  return true;
}

bool Wav::read_header() {
  DEBUG_PRINT_FN();
  char header_buf[72];
  uint32_t file_size = file.fileSize();
  uint32_t chunk_offset = 12;
  chunk_t *pchunk = (chunk_t *)header_buf;

  if (!read_data(&header, chunk_offset, 0)) {
    return false;
  }
  // check if it is a wav file
  if (!header.check(file_size)) {
    return false;
  }
  // deactivate all chunks
  header.init();

  // parse the subchunks
  while (chunk_offset < file_size) {
    uint32_t read_size = min(sizeof(header_buf), file_size - chunk_offset);
    if (!read_data(&header_buf, read_size, chunk_offset)) {
      return false;
    }
    if (pchunk->is<fmtchunk_t>()) {
      header.fmt = *(fmtchunk_t *)pchunk;
      DEBUG_PRINTLN("parse fmt");
    } else if (pchunk->is<datachunk_t>()) {
      header.data = *(datachunk_t *)pchunk;
      data_offset = chunk_offset + sizeof(datachunk_t);
      DEBUG_PRINTLN("parse data");
      DEBUG_DUMP(data_offset);
    } else if (pchunk->is<smplchunk_t>()) {
      header.smpl = *(smplchunk_t *)pchunk;
      smpl_offset = chunk_offset + sizeof(smplchunk_t) - sizeof(loop_t);
      DEBUG_PRINTLN("parse smpl");
      DEBUG_DUMP(smpl_offset);
    } else {
      break;
    }
    chunk_offset += pchunk->total_len();
  }

  // required subchunks
  if (!header.fmt.is_active() || !header.data.is_active()) {
    return false;
  }

  if ((header.fmt.bitRate > 28) || (header.fmt.bitRate < 8)) {
    DEBUG_PRINTLN("header bitRate is not valid:");
    DEBUG_PRINTLN(header.fmt.bitRate);
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
    DEBUG_PRINTLN("could not seek");
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
    DEBUG_PRINTLN("could not seek");
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }
  if (!file.isOpen()) {
    DEBUG_PRINTLN("file not open");
    return false;
  }
  ret = mcl_sd.read_data(data, size, &file);
  return ret;
}

bool Wav::write_samples(void *data, uint32_t num_samples,
                        uint32_t sample_offset, uint8_t channel,
                        bool writeheader) {
  //  DEBUG_PRINTLN(channel);
  uint32_t position =
      channel * (header.data.chunk_size / header.fmt.numChannels) +
      sample_offset * (header.fmt.bitRate / 8);

  uint32_t size = num_samples * (header.fmt.bitRate / 8);
  // DEBUG_PRINTLN(num_samples);
  //  DEBUG_PRINTLN(header.bitRate);

  // DEBUG_PRINTLN(position);
  // DEBUG_PRINTLN(file.fileSize());
  bool ret = write_data(data, size, position + data_offset);

  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }
  uint32_t new_subchunk2Size = (position + size) * header.fmt.numChannels;
  // DEBUG_PRINT_FN();
  // DEBUG_PRINTLN(position);
  // DEBUG_PRINTLN(num_samples);
  // DEBUG_PRINTLN(sample_offset);
  /*Sample chunk exceeds size of original data chunk, we must extend*/

  if (new_subchunk2Size > header.data.chunk_size) {
    header.data.chunk_size = new_subchunk2Size;
    header.chunk_size = header.total_len() - sizeof(chunk_t);
  }
  if (writeheader) {
    ret = write_header();
  }
  if (!ret) {
    DEBUG_PRINTLN("write header failed");
    return false;
  }
  return ret;
}

bool Wav::read_samples(void *data, uint32_t num_samples, uint32_t sample_index,
                       uint8_t channel) {
  uint8_t sample_size = header.fmt.bitRate / 8;
  if (header.fmt.bitRate % 8 > 0) {
    sample_size++;
  }

  uint8_t nch_sample_size = sample_size * header.fmt.numChannels;
  uint32_t position = sample_index * nch_sample_size;

  uint32_t read_size = num_samples * sample_size;
  uint32_t nch_read_size = read_size * header.fmt.numChannels;
  uint32_t new_subchunk2Size = position + nch_read_size;
  // DEBUG_PRINTLN(size);
  // DEBUG_PRINTLN(header.bitRate);
  // DEBUG_PRINTLN(num_samples);
  // DEBUG_PRINTLN(header.subchunk2Size);

  // If requested read size extends past channel size, then truncate read
  if (new_subchunk2Size > header.data.chunk_size) {
    DEBUG_PRINTLN("read size is greater than file size, adjusting");
    read_size = (header.data.chunk_size - position) / header.fmt.numChannels;
    nch_read_size = read_size * header.fmt.numChannels;
  }

  if (header.fmt.numChannels == 1) {
    return read_data(data, read_size, position + data_offset);
  }
  // Read all channels, if channel exceeds number of channels
  if (channel >= header.fmt.numChannels) {
    return read_data(data, nch_read_size, position + data_offset);
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
  uint32_t full_run =
      ((sizeof(tmp_buf) / header.fmt.numChannels) / sample_size) * sample_size *
      header.fmt.numChannels;
  while (read_size > 0) {
    uint32_t current_run = min(full_run, read_size * header.fmt.numChannels);
    bool ret = read_data(tmp_buf, current_run, position);
    if (!ret)
      return false;
    position += current_run;
    read_size -= current_run / header.fmt.numChannels;
    for (uint32_t p = channel * sample_size; p < current_run;
         p += nch_sample_size) {
      memcpy(data, tmp_buf + p, sample_size);
      data = (char *)data + sample_size;
    }
  }
  return true;
}

int32_t Wav::find_peak(uint8_t channel, uint32_t num_samples,
                       uint32_t sample_index) {
  wav_sample_t min_sample;
  wav_sample_t max_sample;
  find_peaks(channel, num_samples, sample_index, &max_sample, &min_sample);
  if (abs(min_sample.val) > max_sample.val) {
    return abs(min_sample.val);
  }
  return max_sample.val;
}

void Wav::find_peaks(uint8_t channel, uint32_t num_samples,
                     uint32_t sample_index, wav_sample_t *max_sample,
                     wav_sample_t *min_sample) {
  DEBUG_PRINT_FN();
  max_sample->val = 0;
  min_sample->val = 0;

  int16_t min_sample16;
  int16_t max_sample16;

  int32_t sample_val = 0;
  int16_t sample_val16 = 0;

  uint8_t sample_size = header.fmt.bitRate / 8;
  if (header.fmt.bitRate % 8 > 0) {
    sample_size++;
  }
  uint32_t num_of_samples;

  if (num_samples > 0) {
    num_of_samples = num_samples;
  }

  else {
    uint32_t num_of_samples =
        (header.data.chunk_size / header.fmt.numChannels) / sample_size;
  }

  int16_t buffer_size = 1024;

  uint8_t buffer[buffer_size];

  int16_t read_size = buffer_size / sample_size;
  int32_t sample_max = (pow(2, header.fmt.bitRate) / 2);

  DEBUG_PRINTLN("read_size");
  DEBUG_PRINTLN(read_size);
  DEBUG_PRINTLN(num_of_samples);
  DEBUG_PRINTLN(sample_size);
  DEBUG_PRINTLN(buffer_size);

  uint8_t word_offset = 4 - sample_size;

  for (int32_t n = sample_index; n < sample_index + num_of_samples;
       n += read_size) {
    // Adjust read size if too large
    if (n + read_size > sample_index + num_of_samples) {
      read_size = sample_index + num_of_samples - n;
    }
    // Read read_size samples.
    if (!read_samples(buffer, read_size, n, channel)) {
      DEBUG_PRINTLN("could not read");
      return false;
    }
    // Itterate through samples in buffer
    uint16_t byte_count = 0;

    switch (sample_size) {
    //16bit - fast mode.
    case 2:

      for (uint16_t sample = 0; sample < read_size; sample += 1) {
        sample_val16 = ((int16_t*)&buffer)[byte_count++];
        if (sample_val16 < min_sample16) {
          min_sample16 = sample_val16;
          min_sample->pos = sample + sample_index;
        }
        if (sample_val16 > max_sample16) {
          max_sample16 = sample_val16;
          max_sample->pos = sample + sample_index;
        }
      }
      break;

    default:

      for (uint16_t sample = 0; sample < read_size; sample += 1) {
        // Move byte stream in to 32bit MSBs.
        for (uint8_t b = 0; b < sample_size; b++) {
          ((uint8_t *)&sample_val)[b + word_offset] = buffer[byte_count++];
        }
        // Down shift to preserve sign.
        sample_val = sample_val >> (word_offset * 8);

        if (sample_val < min_sample->val) {
          min_sample->val = sample_val;
          min_sample->pos = sample + sample_index;
        }
        if (sample_val > max_sample->val) {
          max_sample->val = sample_val;
          max_sample->pos = sample + sample_index;
        }
      }
    }
  }

  if (sample_size == 2) {
  min_sample->val = (int32_t) min_sample16;
  max_sample->val = (int32_t) max_sample16;
  }
}

bool Wav::apply_gain(float gain, uint8_t channel) {
  DEBUG_PRINT_FN();

  uint8_t sample_size = header.fmt.bitRate / 8;
  if (header.fmt.bitRate % 8 > 0) {
    sample_size++;
  }

  uint32_t num_of_samples =
      (header.data.chunk_size / header.fmt.numChannels) / sample_size;

  int16_t buffer_size = 512;

  uint8_t buffer[buffer_size];
  int16_t read_size = buffer_size / sample_size;
  uint32_t sample_val = 0;
  uint32_t sample_max = (pow(2, header.fmt.bitRate) / 2);
  bool write_header = false;
  DEBUG_PRINTLN("read_size");
  DEBUG_PRINTLN(read_size);
  DEBUG_PRINTLN(num_of_samples);
  DEBUG_PRINTLN(sample_size);
  DEBUG_PRINTLN(buffer_size);
  DEBUG_PRINTLN(gain);
  for (int32_t n = 0; n < num_of_samples; n += read_size) {
    // Adjust read size if too large
    if (n + read_size > num_of_samples) {
      read_size = num_of_samples - n;
    }
    // Read read_size samples.

    if (!read_samples(buffer, read_size, n, channel)) {
      DEBUG_PRINTLN("could not read");
      return false;
    }
    // Itterate through samples in buffer
    for (uint16_t byte_count = 0; byte_count < read_size * sample_size;
         byte_count += sample_size) {

      sample_val = 0;

      // Decode samples from byte stream;

      for (uint8_t b = 0; b < sample_size; b++) {
        sample_val |= ((int32_t)buffer[byte_count]) << (b * 8);
        byte_count++;
      }
      // For signed formats, we need to remove the "sign" bit before
      // performing multiplication

      if (header.fmt.bitRate > 8) {

        bool is_signed = ((uint32_t)1 << header.fmt.bitRate - 1) & sample_val;

        if (is_signed) {
          // Signed numbers are stored as 2's complement :'-(
          // To perform any calculations we must first convert them to decimal
          // We also need to shake off any unwanted higher bits if we need to
          // perform a comparator operation.

          sample_val = ~(sample_val - 1);
          sample_val = sample_val << (32 - header.fmt.bitRate);
          sample_val = sample_val >> (32 - header.fmt.bitRate);
          sample_val &= ~((uint32_t)1 << (header.fmt.bitRate - 1));
        }

        // Apply gain adjustment
        sample_val = (uint32_t)((float)sample_val * (float)gain);
        if (sample_val > sample_max) {
          sample_val = sample_max;
        }
        if (is_signed) {
          sample_val = ~(sample_val) + 1;
          sample_val |= ((uint32_t)1 << header.fmt.bitRate - 1);
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

      byte_count -= sample_size;
      // Convert modified sample back in to byte stream

      for (uint8_t b = 0; b < sample_size; b++) {
        buffer[byte_count + b] =
            (uint8_t)(sample_val >> (8 * (b))) & (uint8_t)0xFF;
      }
    }
    // Write back the adjusted samples
    if (!write_samples(buffer, read_size, n, channel, write_header)) {
      DEBUG_PRINTLN("could not write");
      return false;
    }
    // loop
  }

  return true;
}
