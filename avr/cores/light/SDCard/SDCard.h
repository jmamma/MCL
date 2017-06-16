#ifndef SDCARD_H__
#define SDCARD_H__

#include "WProgram.h"
#include <inttypes.h>

extern "C" {
#include "byteordering.hh"
#include "partition.hh"
#include "fat.hh"
#include "fat_config.hh"
#include "sd-reader_config.hh"
#include "sd_raw.hh"
}

class SDCardEntry;

class SDCardClass {
 public:
  struct partition_struct *partition;
  struct fat_fs_struct    *fs;
  bool isInit;

  SDCardClass();
  uint8_t init();
  offset_t getSize();
  offset_t getFree();

  bool findFile(const char *path, struct fat_dir_entry_struct *dir_entry);
  bool findFile(const char *path, SDCardEntry *entry);
  
  bool writeFile(const char *path, const uint8_t *buf, uint8_t len, bool createDir = false);
  int readFile(const char *path, uint8_t *buf, uint8_t len);
  bool deleteFile(const char *path, bool recursive = false);

  bool createDirectory(const char *path, SDCardEntry *entry = NULL);
  int listDirectory(const char *path, SDCardEntry entries[], int maxCount);
};

extern SDCardClass SDCard;

class SDCardEntry {
 public:
  struct fat_dir_entry_struct dir_entry;

  bool exists;
  char dir[128];
  char name[32];

  SDCardEntry();
  SDCardEntry(const char *path);

  bool setPath(const char *path);
  void setFromParentEntry(SDCardEntry *parent);

  bool deleteEntry(bool recursive = false);
  bool deleteFirstEntry();

  /* directory functions */
  bool isDirectory();
  bool findFile(const char *name, struct fat_dir_entry_struct *entry);
  bool findFile(const char *name, SDCardEntry *entry) {
    return findFile(name, &entry->dir_entry);
  }
  int listDirectory(SDCardEntry entries[], int maxCount);

  bool isEmpty();
  bool createSubDirectory(const char *path, struct fat_dir_entry_struct *new_entry);
  bool createSubDirectory(const char *path, SDCardEntry *entry) {
    return createSubDirectory(path, &entry->dir_entry);
  }
};

class SDCardFile : public SDCardEntry {
 public:
  struct fat_file_struct *fd;
  
 SDCardFile(const char *path) : SDCardEntry(path) {
    fd = NULL;
  }

  ~SDCardFile() { close(); }
  bool open(bool create = false);
  void close();
  
  intptr_t read(uint8_t *buf, uint32_t len);
  intptr_t write(const uint8_t *buf, uint32_t len);
  bool seek(int32_t *offset, uint8_t whence);
  
  /* file functions to slurp and write full file */
  int readFile(uint8_t *buf, uint8_t len);
  int writeFile(const uint8_t *buf, uint8_t len);
};

#endif /* SDCARD_H__ */
