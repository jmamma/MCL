#ifndef FS_FILE_H__
#define FS_FILE_H__

#include "FS_FileSystem.hh"

class FS_File {
  public:
  FS_FileEntry entry;
  uint32_t cur_position;
  int i;
  FS_File() {
  init();
  }  
  virtual bool seek(uintptr_t pos);
  
  virtual void init();

  virtual uint32_t read(uint8_t* buffer, uintptr_t length);
  virtual uint32_t write(uint8_t* buffer, uintptr_t length);

  virtual bool open(const char *path, uint32_t size);
  virtual bool close();
  virtual bool del();

};

#endif
