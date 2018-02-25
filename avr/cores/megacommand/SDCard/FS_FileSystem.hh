#ifndef FS_FILESYTEM_H__
#define FS_FILESYTEM_H__


extern "C" {
#include "sd_raw.h"
#include "sd_raw_config.h"
}
#include "helpers.h"
#include "string.h"
//Let's define a simple binary pattern to identify to filesystem of 10101010
#define FS_ID 170
#define FS_VERSION 100
#define FS_ENTRY_DISABLED 0
#define FS_ENTRY_DELETED 3
#define FS_ENTRY_ACTIVE 1
#define FS_MAX_ENTRIES 10000
#define FS_BLOCK_SIZE 512
#define FS_HEADER_RESERVED 512
#define FS_ENTRY_SIZE 256
#define FS_MIN_FILE_SIZE 262144 //0.25megabyte, to mitigate small file lock-in as file space is fragmented.
#define FS_TABLE_RESERVED (FS_MAX_ENTRIES * FS_ENTRY_SIZE)
#define FS_ENABLED 1
#define FS_DISABLED 0
#define FS_FILE_NAME_LENGTH 16
// FS_FileSystemLayout
//
// 0    Header
// 512  FS_FileEntry1
// 512 + 256**n  FS_FileEntryn

class FS_FileSystemHeader {
  public:
  uint8_t check;
  uint8_t version;
  uint16_t number_of_entries;
  uint32_t fs_size;
  uint32_t fs_allocated;
  uint32_t fs_used;
  uint32_t file_table_reserved; 
};

class FS_FileEntry {
  public:
  uint8_t active; 
  char path[FS_FILE_NAME_LENGTH];
  offset_t offset;
  uint32_t size;
};

class FS_FileSystem {
  public:
  FS_FileSystemHeader header;
  uint8_t active;
  sd_raw_info sdcard_info;
  //constructor
  virtual void init();
  virtual bool write_header();
  virtual bool read_header();
  virtual bool check_header();
  virtual void format();
  virtual void del_entry(int i);
  virtual void reset_entry(int i);
  virtual int list_entries(char** buf, int n);
  virtual bool write_entry(int i, FS_FileEntry *temp);
  virtual bool read_entry(int i, FS_FileEntry *temp);
  virtual int open(const char *path, uint32_t size, FS_FileEntry *fileentry);
  virtual int find(const char *path);
  virtual bool create(const char *path, uint32_t size, FS_FileEntry *entry);
  FS_FileSystem() {
  init();
  }
  

};

extern FS_FileSystem file_system;

#endif /* SIMPLEFS_H__ */
