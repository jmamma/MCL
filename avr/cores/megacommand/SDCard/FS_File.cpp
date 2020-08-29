#include "FS_FileSystem.h"
#include "FS_File.h"

bool FS_File::seek(uintptr_t pos) {
    if (entry.active == FS_ENTRY_DISABLED) { return 0; }
    if (pos < entry.size) {
      cur_position = pos;
      return true;
    }
    else {
      return false;
    }
  }
  
  void FS_File::init() {
    close();
  }

  uint32_t FS_File::read(uint8_t* buffer, uintptr_t length) {
    if (entry.active == FS_ENTRY_DISABLED) { return 0; }
    if ((sd_raw_read(entry.offset + cur_position, buffer, length) == 1) && (cur_position + length < entry.size)) {
    cur_position += length;
    return length;
    }
    else {
    return 0;
    }
  }
  
  uint32_t FS_File::write(uint8_t* buffer, uintptr_t length) {
    if (entry.active == FS_ENTRY_DISABLED) { return 0; }

    if ((sd_raw_write(entry.offset + cur_position, buffer, length) == 1) && (cur_position + length < entry.size)) {
    cur_position += length;
    return length;
    }
    else {
    return 0;
    }
  }

  bool FS_File::open(const char *path, uint32_t size) {
    if (int x = file_system.open(path, size, &entry) >= 0) {
      i = x;
      return true; 
    }
    else {
      return false;
    }
  }
  bool FS_File::close() {
    cur_position = 0;
    entry.active = FS_ENTRY_DISABLED; 
    return true;
  }

  bool FS_File::del() {
    if (entry.active == FS_ENTRY_DISABLED) { return 0; }
    file_system.del_entry(i);    
    
  
  }

