/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRID_H__
#define GRID_H__

#include "A4Track.h"
#include "GridRowHeader.h"
#include "MCLSd.h"
#include "SdFat.h"

#define GRID_VERSION 3000

#define GRID_LENGTH_270 128
#define GRID_WIDTH_270 20
#define GRID_SLOT_BYTES_270 4096

class Grid_270 {
  public:
  int32_t get_slot_offset(int16_t column, int16_t row);
  int32_t get_row_header_offset(int16_t row);
};

class GridHeader {
public:
  uint32_t version;
  uint8_t id;
  uint32_t hash;
};

class Grid : public GridHeader {
public:
  File file;

  void setup();

  bool new_file(const char *gridname);
  bool new_grid(const char *gridname);
  bool write_header();

  bool open_file(const char *gridname) { return file.open(gridname, O_RDWR); }

  bool close_file() { return file.close(); }

  uint8_t get_slot_model(int column, int row, bool load);

  uint32_t get_slot_offset(int16_t column, int16_t row) {
    uint32_t offset = (int32_t)(GRID_SLOT_BYTES * GRID_WIDTH) +
                      (int32_t)((column + 1) + (row * (GRID_WIDTH + 1))) *
                          (int32_t)GRID_SLOT_BYTES;
    return offset;
  }

  uint32_t get_row_header_offset(int16_t row) {
    uint32_t offset =
        (int32_t)(GRID_SLOT_BYTES * GRID_WIDTH) +
        (int32_t)(0 + (row * (GRID_WIDTH + 1))) * (int32_t)GRID_SLOT_BYTES;
    return offset;
  }

  bool seek(uint8_t col, uint16_t row) {
    return mcl_sd.seek(get_slot_offset(col, row), &file);
  }

  bool seek_row_header(uint16_t row) {
    return mcl_sd.seek(get_row_header_offset(row), &file);
  }

  bool copy_slot(int16_t s_col, int16_t s_row, int16_t d_col, int16_t d_row,
                 bool destination_same);
  bool clear_slot(int16_t column, int16_t row, bool update_header = true);
  bool clear_row(int16_t row);
  bool clear_model(int16_t column, uint16_t row);

  bool read(void *data, size_t len) {
    return mcl_sd.read_data((uint8_t *)(data), len, &file);
  }

  bool read(void *data, size_t len, uint8_t col, uint16_t row) {
    bool ret = seek(col, row);
    if (ret) {
      ret = read(data, len);
    }
    return ret;
  }

  bool write(void *data, size_t len) {
    return mcl_sd.write_data((uint8_t *)(data), len, &file);
  }

  bool write(void *data, size_t len, uint8_t col, uint16_t row) {
    bool ret = seek(col, row);
    if (ret) {
      ret = write((uint8_t *)(data), len);
    }
    return ret;
  }

  bool write_row_header(GridRowHeader *row_header, uint16_t row) {
    bool ret = seek_row_header(row);
    if (ret) {
      ret = mcl_sd.write_data((uint8_t *)(row_header), sizeof(GridRowHeader),
                              &file);
    }
    return ret;
  }

  bool read_row_header(GridRowHeader *row_header, uint16_t row) {
    bool ret = seek_row_header(row);
    if (ret) {
      ret = mcl_sd.read_data((uint8_t *)(row_header), sizeof(GridRowHeader),
                             &file);
    }
    return ret;
  }

  bool sync() { return file.sync(); }
};

#endif /* GRID_H__ */
