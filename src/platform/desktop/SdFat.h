// SdFat.h — desktop shim. Matches the subset of SdFat MCL actually uses
// (see grep over src/mcl/MCL/MCLSd.cpp, Project.cpp). Backed by std::filesystem
// and std::fstream. The "SD root" is the directory all "/foo" paths resolve
// against — set via mcl_desktop_set_sd_root(); defaults to "./mcl_sd".
#pragma once

#include "Arduino.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

// File-open mode flags. Match SdFat values where MCL passes them around as
// integers (MCLSd.cpp:122 O_RDWR, 254 O_READ, 257 O_RDWR | O_CREAT | O_EXCL).
#ifndef O_READ
#define O_READ   0x01
#endif
#ifndef O_WRITE
#define O_WRITE  0x02
#endif
#ifndef O_RDWR
#define O_RDWR   (O_READ | O_WRITE)
#endif
#ifndef O_CREAT
#define O_CREAT  0x04
#endif
#ifndef O_EXCL
#define O_EXCL   0x08
#endif
#ifndef O_APPEND
#define O_APPEND 0x10
#endif
#ifndef O_TRUNC
#define O_TRUNC  0x20
#endif

// SDFAT_FILE_TYPE > 1 selects the modern FsFile typedef used by
// MCL/src/mcl/MCL/MCLSd.h:13 `typedef FsFile File;`.
#ifndef SDFAT_FILE_TYPE
#define SDFAT_FILE_TYPE 2
#endif

// Set the directory that MCL's "/" paths resolve against. Called from JUCE
// host code (via mcl_set_sd_root in desktop_entry.h). Idempotent.
void mcl_desktop_set_sd_root(const char* abs_path);

// Internal helper: resolve an MCL-relative path (starting with '/' or a
// relative name) against the current SD root + chdir state.
std::filesystem::path mcl_desktop_resolve_sd_path(const char* path);

class SdCard {
public:
    void setDedicatedSpi(bool /*on*/) {}
};

// FsFile (typedef'd to File in MCLSd.h) — both a file handle and a directory
// iterator depending on what was opened.
class FsFile {
public:
    FsFile()  = default;
    ~FsFile() { close(); }

    // Non-copyable; movable so APIs that return FsFile-by-value work.
    FsFile(const FsFile&)            = delete;
    FsFile& operator=(const FsFile&) = delete;
    FsFile(FsFile&&) noexcept;
    FsFile& operator=(FsFile&&) noexcept;

    // Returns true on success. Mode is OR-of O_* flags. Defaults to O_READ
    // when not specified — matches SdFat's File::open(path) overload.
    bool open(const char* path, uint8_t mode = O_READ);

    // Iterate the next entry in a directory previously opened with O_READ.
    bool openNext(FsFile* dir, uint8_t mode);

    // Returns true to match MCL's expectation (e.g. Grid::close_file).
    bool close();

    int  read (void* buf, size_t n);
    int  write(const void* buf, size_t n);
    int  read()              { uint8_t b; return read(&b, 1) == 1 ? b : -1; }
    int  write(uint8_t b)    { return write(&b, 1); }

    bool seekSet(uint32_t pos);
    bool seek(uint32_t pos)              { return seekSet(pos); }
    uint32_t curPosition() const;
    // SdFat exposes position() and curPosition() as synonyms.
    uint32_t position() const { return curPosition(); }
    uint32_t fileSize() const;
    // SdFat exposes both fileSize() and size() as synonyms.
    uint32_t size() const { return fileSize(); }
    // isOpen() is the modern SdFat name for what `operator bool()` returns.
    bool isOpen() const { return open_; }

    bool isDirectory() const { return is_dir_; }
    bool isFile()      const { return open_ && !is_dir_; }

    bool rewind();
    size_t getName(char* dst, size_t cap) const;

    bool sync();
    bool truncate(uint32_t length);
    bool remove();
    bool preAllocate(uint32_t length);

    explicit operator bool() const { return open_; }

private:
    std::filesystem::path        path_;
    std::unique_ptr<std::fstream> stream_;

    bool                         open_   = false;
    bool                         is_dir_ = false;
    bool                         dir_iter_initialised_ = false;
    std::filesystem::directory_iterator dir_it_;
};

class SdFat {
public:
    bool begin(...)              { return true; }
    bool exists(const char* path);
    bool mkdir(const char* path, bool recursive = false);
    bool rmdir(const char* path);
    bool remove(const char* path);
    bool rename(const char* oldName, const char* newName);
    bool chdir(const char* path);
    SdCard* card() { return &card_; }

private:
    SdCard card_;
};
