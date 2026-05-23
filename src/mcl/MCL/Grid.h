/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "GridRowHeader.h"
#include "MCLSd.h"

#define GRID_VERSION 3000

class ATTR_PACKED() GridHeader {
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

  uint8_t get_slot_model(GridColumn column, GridRow row, bool load);

  uint32_t get_slot_offset(GridColumn column, GridRow row) {
    uint32_t offset = (uint32_t)((column + 1) + (row * (GRID_WIDTH + 1))) * (uint32_t)GRID_SLOT_BYTES;
    return offset;
  }

  uint32_t get_row_header_offset(GridRow row) {
    uint32_t offset = (uint32_t)(0 + (row * (GRID_WIDTH + 1))) * (uint32_t)GRID_SLOT_BYTES;
    return offset;
  }

  bool seek(GridColumn col, GridRow row) {
    return mcl_sd.seek(get_slot_offset(col, row), &file);
  }

  bool seek(GridColumn col, GridRow row, uint16_t offset) {
    return mcl_sd.seek(get_slot_offset(col, row) + offset, &file);
  }

  bool seek_row_header(GridRow row) {
    return mcl_sd.seek(get_row_header_offset(row), &file);
  }

  bool copy_slot(GridColumn s_col, GridRow s_row, GridColumn d_col, GridRow d_row,
                 bool destination_same);
  bool clear_slot(GridColumn column, GridRow row, bool update_header = true);
  bool clear_row(GridRow row);
  bool clear_model(GridColumn column, GridRow row);

  bool read(void *data, size_t len) {
    return mcl_sd.read_data(data, len, &file);
  }

  bool read(void *data, size_t len, GridColumn col, GridRow row) {
    bool ret = seek(col, row);
    if (ret) {
      ret = read(data, len);
    }
    return ret;
  }

  bool write(void *data, size_t len) {
    return mcl_sd.write_data((void *)(data), len, &file);
  }

  bool write(void *data, size_t len, GridColumn col, GridRow row) {
    bool ret = seek(col, row);
    if (ret) {
      ret = write(data, len);
    }
    return ret;
  }

  bool write_row_header(GridRowHeader *row_header, GridRow row) {
    bool ret = seek_row_header(row);
    if (ret) {
      ret = mcl_sd.write_data(row_header->_this(), sizeof(GridRowHeader),
                              &file);
    }
    return ret;
  }

  bool read_row_header(GridRowHeader *row_header, GridRow row) {
    bool ret = seek_row_header(row);
    if (ret) {
      ret = mcl_sd.read_data(row_header->_this(), sizeof(GridRowHeader),
                             &file);
    }
    return ret;
  }

  bool sync() { return file.sync(); }
};
