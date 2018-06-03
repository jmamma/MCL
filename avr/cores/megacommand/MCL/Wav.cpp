#include "MCL.h"
#include "Wav.h"

bool Wav::close(bool write) {
  if (write) { write_header(); }
  file.close();
}
bool Wav::open(char *file_name, uint16_t numChannels, uint32_t sampleRate,
               uint8_t bitRate, bool overwrite) {
  DEBUG_PRINT_FN();
  bool ret;
  m_strncpy(&filename, file_name, 16);
  DEBUG_PRINTLN(file_name);
  DEBUG_PRINTLN(file.fileSize());

  if (file.isOpen()) {
  DEBUG_PRINTLN("file already open");
  file.close();
  }
  ret = file.open(file_name, O_RDWR | O_CREAT);
  if (!ret) {
    DEBUG_PRINTLN("could not open wave file");
  }
  if (overwrite) {

    ret = file.truncate(0);
    if (!ret) {
      DEBUG_PRINTLN("truncate failed");
    }

    header.init_header(numChannels, sampleRate, bitRate);
    ret = write_header();
    headerRead = true;
    if (!ret) {
      DEBUG_PRINTLN("Write wave header failed");
      return false;
    }
    DEBUG_PRINTLN("offset");
    DEBUG_PRINTLN(WAV_DATA_OFFSET);
    DEBUG_PRINTLN(file.fileSize());
    data_offset = WAV_DATA_OFFSET;
  } else if (!headerRead) {

    ret = read_header();
    if (!ret) {
      DEBUG_PRINTLN("Could not read header");
      return false;
    }
    headerRead = true;
  }
  return true;
}

bool Wav::rename(char *new_name) {
  if (!file.rename(&file, new_name)) {
  DEBUG_PRINTLN("rename failed");
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
    read_size = sizeof(header);
  }

  ret = read_data(&header_buf, read_size, 0);
  if (ret == false) {
    return false;
  }

  char str_data[6] = "data";
  uint8_t n = 0;
  uint8_t x = 0;
  for (x = 0; x < 128 && n < 3; x++) {
    if (str_data[n] == header_buf[x]) {
      n++;
    } else {
      n = 0;
    }
  }
  if (n == 3) {
    data_offset = x + 1 + 4;
  } else {
    return false;
  }

  m_memcpy(&header, &header_buf, sizeof(header));
  if ((header.bitRate > 28) || (header.bitRate < 8)) {
   DEBUG_PRINTLN("header bitRate is not valid:");
   DEBUG_PRINTLN(header.bitRate);
     return false;
  }
  return true;
}

bool Wav::write_data(void *data, uint32_t size, uint32_t position) {
  DEBUG_PRINT_FN();
  //  DEBUG_PRINTLN(size);
  //  DEBUG_PRINTLN(position);
  //  DEBUG_PRINTLN(file.fileSize());
  bool ret = false;

  ret = file.seekSet(position);

  if (!ret) {
    Serial.println(SD.card()->errorCode(), HEX);
    DEBUG_PRINTLN("could not seek");
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }

  ret = mcl_sd.write_data(data, size, &file);
  return ret;
}

bool Wav::read_data(void *data, uint32_t size, uint32_t position) {
   //DEBUG_PRINT_FN();

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
                      channel * (header.subchunk2Size / header.numChannels) +
                      sample_offset * (header.bitRate / 8);

  uint32_t size = num_samples * (header.bitRate / 8);
  // DEBUG_PRINTLN(num_samples);
  //  DEBUG_PRINTLN(header.bitRate);

  // DEBUG_PRINTLN(position);
  // DEBUG_PRINTLN(file.fileSize());
  bool ret = write_data(data, size, position + data_offset);

  if (!ret) {
    DEBUG_PRINTLN("write failed");
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
    DEBUG_PRINTLN("write header failed");
    return false;
  }
  return ret;
}

bool Wav::read_samples(void *data, uint32_t num_samples, uint32_t sample_offset,
                       uint8_t channel) {
//  DEBUG_PRINT_FN();
  uint32_t position = channel * (header.subchunk2Size / header.numChannels) +
                      sample_offset * (header.bitRate / 8);
  
 //   DEBUG_PRINTLN(num_samples);
  uint32_t size = num_samples * (header.bitRate / 8);
//DEBUG_PRINTLN(size);
//DEBUG_PRINTLN(header.bitRate);
  uint32_t new_subchunk2Size = (position + size) * header.numChannels;
  
//  DEBUG_PRINTLN(header.subchunk2Size);
  // If requested read size extends past channel size, then truncate read
  if (position + size > header.subchunk2Size / header.numChannels) {
     DEBUG_PRINTLN("read size is greater than file size, adjusting");
     size = (header.subchunk2Size / header.numChannels) - position;
  }
//  DEBUG_PRINTLN(file.fileSize());
   // DEBUG_PRINTLN(header.bitRate);
  //  DEBUG_PRINTLN(position);
   // DEBUG_PRINTLN(size);
//  DEBUG_PRINTLN(data_offset);
  bool ret = read_data(data, size, position + data_offset);
  return ret;
}
