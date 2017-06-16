
/*
 * Copyright (c) 2006-2009 by Roland Riegel <feedback@roland-riegel.de>, Manuel Odendahl <wesen@ruinwesen.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "helpers.h"
#include <string.h>
#include <SDCard.h>

#ifndef NULL
#define NULL 0
#endif

/************************** SDCARD CLASS ***********************************/

SDCardClass::SDCardClass() {
  partition = NULL;
  fs = NULL;
  isInit = false;
}


uint8_t SDCardClass::init() {
  if (isInit)
    return 0;
  
  if (fs != NULL) {
    fat_close(fs);
    fs = NULL;
  }

  if (partition != NULL) {
    partition_close(partition);
    partition = NULL;
  }
  
  if (!sd_raw_init()) {
    return 1;
  }

  partition = partition_open(sd_raw_read, sd_raw_read_interval,
			     sd_raw_write, sd_raw_write_interval,
			     0);

  if (!partition) {
    return 2;
  }

  fs = fat_open(partition);
  if (!fs) {
    return 3;
  }

  isInit = true;

  return 0;
}

offset_t SDCardClass::getSize() {
  if (fs)
    return fat_get_fs_size(fs);
  else
    return 0;
}

offset_t SDCardClass::getFree() {
  if (fs)
    return fat_get_fs_free(fs);
  else
    return 0;
}

bool SDCardClass::findFile(const char *path, struct fat_dir_entry_struct *dir_entry) {
  return fat_get_dir_entry_of_path(SDCard.fs, path, dir_entry);
}

bool SDCardClass::findFile(const char *path, SDCardEntry *entry) {
  return findFile(path, &entry->dir_entry);
}

bool SDCardClass::createDirectory(const char *path, SDCardEntry *entry) {
  SDCardEntry root((char *)"/");
  if (entry == NULL) {
    SDCardEntry bla;
    return root.createSubDirectory(path, &bla);
  } else {
    return root.createSubDirectory(path, entry);
  }
}

bool SDCardClass::deleteFile(const char *path, bool recursive) {
  SDCardEntry file(path);
  return file.deleteEntry(recursive);
}

bool SDCardClass::writeFile(const char *path, const uint8_t *buf, uint8_t len, bool createDir) {
  SDCardFile file(path);
  SDCardEntry parentDir(file.dir);
  if (!parentDir.exists) {
    if (createDir) {
      if (!createDirectory(file.dir)) {
	return false;
      }
      parentDir.setPath(file.dir);
    } else {
      return false;
    }
  }

  return (file.writeFile(buf, len) == len);
}

int SDCardClass::readFile(const char *path, uint8_t *buf, uint8_t len) {
  SDCardFile file(path);
  return file.readFile(buf, len);
}

int SDCardClass::listDirectory(const char *path, SDCardEntry entries[], int maxCount) {
  SDCardEntry entry(path);
  return entry.listDirectory(entries, maxCount);
}

SDCardClass SDCard;

/****************************** SDCARD ENTRY ***********************************/

SDCardEntry::SDCardEntry() {
  exists = false;
  dir[0] = '\0';
  name[0] = '\0';
}

SDCardEntry::SDCardEntry(const char *path) {
  setPath(path);
}

bool SDCardEntry::setPath(const char *path) {
  exists = SDCard.findFile(path, this);

  const char *pos = strrchr(path, '/');
  if (pos != NULL) {
    uint8_t len = MIN(sizeof(dir), (uint16_t)(pos - path));
    if (len == 0) {
      m_strncpy(dir, "/", sizeof(dir));
    } else {
      m_strncpy(dir, path, len);
    }
    m_strncpy(name, pos + 1, sizeof(name));
  } else {
    m_strncpy(dir, path, sizeof(dir));
  }

  return exists;
}

bool SDCardEntry::isDirectory() {
  return exists && dir_entry.attributes & FAT_ATTRIB_DIR;
}

bool SDCardEntry::findFile(const char *name, struct fat_dir_entry_struct *entry) {
  if (!isDirectory())
    return false;

  fat_dir_struct *dd = fat_open_dir(SDCard.fs, &dir_entry);
  if (dd == NULL)
    return false;

  while (fat_read_dir(dd, entry)) {
    if (strcmp(entry->long_name, name) == 0) {
      fat_close_dir(dd);
      return true;
    }
  }

  fat_close_dir(dd);
  return false;
}

int SDCardEntry::listDirectory(SDCardEntry entries[], int maxCount) {
  if (!isDirectory())
    return -1;
  
  int entryCount = 0;

  fat_dir_struct *dd = fat_open_dir(SDCard.fs, &dir_entry);
  if (dd == NULL)
    return -1;
  
  while (fat_read_dir(dd, &entries[entryCount].dir_entry) && (entryCount < maxCount)) {
    if (!strcmp(entries[entryCount].dir_entry.long_name, "."))
      continue;
    entries[entryCount].setFromParentEntry(this);
    entryCount++;
  }
  
  fat_close_dir(dd);
  return entryCount;
}

void SDCardEntry::setFromParentEntry(SDCardEntry *parent) {
  m_strncpy(dir, parent->dir, sizeof(dir));
  m_strnappend(dir, parent->dir_entry.long_name, sizeof(dir));
  
  m_strncpy(name, dir_entry.long_name, sizeof(name));
  exists = true;
}

bool SDCardEntry::createSubDirectory(const char *path, struct fat_dir_entry_struct *new_entry) {
  if (path[0] == '/')
    path++;

  fat_dir_struct *dd = fat_open_dir(SDCard.fs, &dir_entry);
  if (dd == NULL)
    return false;
  
  char subDir[64];
  while (1) {
    const char *pos = strchr(path, '/');
    if (pos == NULL) {
      m_strncpy(subDir, path, sizeof(subDir) - 1);
    } else {
      int len = pos - path;
      memcpy(subDir, path, len);
      subDir[len] = '\0';
      path = pos + 1;
    }
    
    struct fat_dir_entry_struct new_dir_entry;
    
    int result = fat_create_dir(dd, subDir, &new_dir_entry);

    if (result == 0 && strcmp(subDir, new_dir_entry.long_name)) {
      fat_close_dir(dd);
      return false;
    } else {
      memcpy(new_entry, &new_dir_entry, sizeof(new_dir_entry));
      fat_close_dir(dd);
      dd = fat_open_dir(SDCard.fs, &new_dir_entry);

      if (dd == NULL) {
	return false;
      } else {
	if (pos == NULL) {
	  sd_raw_sync();
	  fat_close_dir(dd);
	  return true;
	}
      }
    }
  }
}

/* recursively delete entries */
bool SDCardEntry::deleteFirstEntry() {
  if (!isDirectory())
    return false;


  fat_dir_struct *dd = fat_open_dir(SDCard.fs, &dir_entry);
  if (dd == NULL)
    return false;
  
  SDCardEntry entry;
  uint8_t ret;
 again:
  ret = fat_read_dir(dd, &entry.dir_entry);
  if (!strcmp(entry.dir_entry.long_name, ".") ||
      !strcmp(entry.dir_entry.long_name, "..")) {
    goto again;
  }
  fat_close_dir(dd);

  if (!ret) {
    return false;
  }
  entry.setFromParentEntry(this);
  return entry.deleteEntry(true);
}

bool SDCardEntry::deleteEntry(bool recursive) {
  if (!exists)
    return true;
  
  if (!isDirectory()) {
    uint8_t ret = fat_delete_file(SDCard.fs, &dir_entry);
    if (ret) {
      exists = false;
    }
    return ret;
  } else {
    while (!isEmpty()) {
      if (!deleteFirstEntry()) {
	return false;
      }
    }
    uint8_t ret = fat_delete_file(SDCard.fs, &dir_entry);
    if (ret) {
      exists = false;
    }
    return ret;
  }
}

bool SDCardEntry::isEmpty() {
  if (!isDirectory()) {
    return false;
  }

  fat_dir_struct *dd = fat_open_dir(SDCard.fs, &dir_entry);
  if (dd == NULL)
    return -1;

  uint8_t cnt = 0;
  struct fat_dir_entry_struct entry;
  uint8_t ret;
  while ((ret = fat_read_dir(dd, &entry))) {
    if (!strcmp(entry.long_name, ".") ||
	!strcmp(entry.long_name, "..")) {
      continue;
    } else {
      cnt++;
      break;
    }
  }
  fat_close_dir(dd);
  return (cnt == 0);
}

/******************* SD CARD *****************************/

bool SDCardFile::open(bool create) {
  if (!exists) {
    if (create) {
      SDCardEntry parentDir(dir);
      if (!parentDir.exists)
	return false;
      
      fat_dir_struct *dd = fat_open_dir(SDCard.fs, &parentDir.dir_entry);
      if (!fat_create_file(dd, name, &dir_entry)
	  && strcmp(name, dir_entry.long_name)) {
	fat_close_dir(dd);
	return false;
      }
      fat_close_dir(dd);
    } else {
      return false;
    }
  }
  
  fd = fat_open_file(SDCard.fs, &dir_entry);
  if (fd != NULL)
    return true;
  else
    return false;
}

void SDCardFile::close() {
  if (fd != NULL) {
    fat_close_file(fd);
    sd_raw_sync();
    fd = NULL;
  }
}

intptr_t SDCardFile::read(uint8_t *buf, uint32_t len) {
  if (fd == NULL)
    return -1;

  return fat_read_file(fd, buf, len);
}

intptr_t SDCardFile::write(const uint8_t *buf, uint32_t len) {
  if (fd == NULL)
    return -1;

  return fat_write_file(fd, buf, len);
}

bool SDCardFile::seek(int32_t *offset, uint8_t whence) {
  if (fd == NULL)
    return -1;

  if (fat_seek_file(fd, offset, whence))
    return true;
  else
    return false;
}

int SDCardFile::readFile(uint8_t *buf, uint8_t len) {
  if (!open())
    return -1;
  intptr_t ret = read(buf, len);
  close();
  return ret;
}

int SDCardFile::writeFile(const uint8_t *buf, uint8_t len) {
  if (!open(true))
    return -1;
  intptr_t ret = write(buf, len);
  close();
  return ret;
}
