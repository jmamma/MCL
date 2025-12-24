#pragma once

// SdFat.h stub for desktop builds

#include <stdint.h>
#include <stdio.h>

// SDFAT_FILE_TYPE is not > 1 on desktop
#define SDFAT_FILE_TYPE 1

class SdCard {
public:
    bool isBusy() { return false; }
    uint32_t sectorCount() { return 0; }
    void setDedicatedSpi(bool) {}
};

// Global card instance
inline SdCard _dummyCard;
inline SdCard* getCard() { return &_dummyCard; }

class File {
public:
    bool open(const char* path, int mode = 0) { return false; }
    bool close() { return true; }
    int read() { return -1; }
    int read(void* buf, size_t count) { return 0; }
    size_t write(uint8_t b) { return 0; }
    size_t write(const uint8_t* buf, size_t size) { return 0; }
    bool seek(uint32_t pos) { return false; }
    uint32_t position() { return 0; }
    uint32_t size() { return 0; }
    bool isOpen() { return false; }
    bool isDir() { return false; }
    operator bool() { return false; }
    bool sync() { return false; }
    bool truncate(uint32_t size) { return false; }
};

class SdFat {
public:
    bool begin(uint8_t cs = 0, uint32_t speed = 0) { return false; }
    File open(const char* path, int mode = 0) { return File(); }
    bool exists(const char* path) { return false; }
    bool mkdir(const char* path) { return false; }
    bool remove(const char* path) { return false; }
    bool rmdir(const char* path) { return false; }
    bool rename(const char* oldPath, const char* newPath) { return false; }
protected:
    SdCard* card() { return &_dummyCard; }
};

#define O_READ 0x01
#define O_WRITE 0x02
#define O_RDWR 0x03
#define O_CREAT 0x10
#define O_TRUNC 0x20
#define O_APPEND 0x40

#define FILE_READ O_READ
#define FILE_WRITE (O_WRITE | O_CREAT | O_TRUNC)
