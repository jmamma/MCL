// SdFat.h — desktop_common shim. Matches the subset of SdFat MCL actually
// uses (see grep over src/mcl/MCL/MCLSd.cpp, Project.cpp).
//
// The interface stores opaque int32_t handles; the .cpp implementation in
// either platform/desktop/ (std::filesystem-backed) or platform/wasm/
// (host-import-backed) translates them. Keeping the header free of std::
// types lets the same SdFat.h compile under wasm32 where libc++ isn't
// available.
#pragma once

#include "Arduino.h"

#include <stdint.h>
#include <stddef.h>

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

#ifdef __cplusplus

// Set the directory that MCL's "/" paths resolve against. Called from JUCE
// host code (via mcl_set_sd_root in desktop_entry.h). Idempotent.
void mcl_desktop_set_sd_root(const char* abs_path);

class SdCard {
public:
    void setDedicatedSpi(bool /*on*/) {}
};

// FsFile (typedef'd to File in MCLSd.h) — both a file handle and a directory
// iterator depending on what was opened. The private state is just two
// platform-opaque int32 handles plus state bits; the .cpp implementation
// is responsible for mapping handle → file/directory.
class FsFile {
public:
    FsFile()  = default;
    ~FsFile() { close(); }

    // Non-copyable; movable so APIs that return FsFile-by-value work.
    FsFile(const FsFile&)            = delete;
    FsFile& operator=(const FsFile&) = delete;
    FsFile(FsFile&& other) noexcept            { *this = (FsFile&&)other; }
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

    // The implementation accesses these directly. Public so the per-platform
    // .cpp doesn't need friend declarations across translation units.
    int32_t handle_  = -1;   // file fd OR directory iterator handle
    char    name_[64] = {0}; // last seen filename (for getName)
    bool    open_    = false;
    bool    is_dir_  = false;
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

#endif  // __cplusplus
