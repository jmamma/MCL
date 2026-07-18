// SdFat.cpp — wasm implementation of the SdFat shim declared in
// desktop_common/SdFat.h. Every file/dir op goes through a host import;
// the host decides where MCL's logical "SD card" lives.
//
// FsFile::handle_ is the int32 returned by host_fs_open / host_fs_dir_open.
// The host owns the actual fd table; we just shuttle bytes.
#include "SdFat.h"
#include "host_imports.h"

#include <string.h>

// mcl_desktop_set_sd_root keeps its name across platforms for API
// continuity; on wasm it's a no-op because the host already knows where
// the SD root is — it doesn't ask wasm.
void mcl_desktop_set_sd_root(const char* /*abs_path*/) {}

// ---------------- FsFile -------------------------------------------------

FsFile& FsFile::operator=(FsFile&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        memcpy(name_, other.name_, sizeof(name_));
        memcpy(path_, other.path_, sizeof(path_));
        open_   = other.open_;
        is_dir_ = other.is_dir_;
        other.handle_ = -1;
        other.open_   = false;
        other.is_dir_ = false;
        other.path_[0] = '\0';
    }
    return *this;
}

static void store_name(char dst[64], const char* src) {
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n > 63) n = 63;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void store_path(char dst[128], const char* src) {
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n > 127) n = 127;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static const char* basename_of(const char* path) {
    if (!path) return "";
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') last = p + 1;
    }
    return last;
}

bool FsFile::open(const char* path, uint8_t mode) {
    close();
    if (!path) return false;

    if (host_fs_is_dir(path)) {
        int32_t h = host_fs_dir_open(path);
        if (h < 0) return false;
        handle_ = h;
        is_dir_ = true;
        open_   = true;
        store_name(name_, basename_of(path));
        store_path(path_, path);
        return true;
    }

    int32_t fd = host_fs_open(path, mode);
    if (fd < 0) return false;
    handle_ = fd;
    is_dir_ = false;
    open_   = true;
    store_name(name_, basename_of(path));
    store_path(path_, path);
    return true;
}

bool FsFile::openNext(FsFile* dir, uint8_t mode) {
    if (!dir || !dir->open_ || !dir->is_dir_) return false;

    char child[64];
    int got = host_fs_dir_next(dir->handle_, child, sizeof(child));
    if (got <= 0) return false;

    close();
    char child_path[192];
    if (dir->path_[0]) {
        size_t parent_len = strlen(dir->path_);
        if (parent_len >= sizeof(child_path)) return false;
        memcpy(child_path, dir->path_, parent_len);
        size_t pos = parent_len;
        if (pos > 0 && child_path[pos - 1] != '/') {
            if (pos + 1 >= sizeof(child_path)) return false;
            child_path[pos++] = '/';
        }
        size_t child_len = strlen(child);
        if (pos + child_len >= sizeof(child_path)) return false;
        memcpy(child_path + pos, child, child_len + 1);
    } else {
        store_path(child_path, child);
    }

    if (host_fs_is_dir(child_path)) {
        int32_t h = host_fs_dir_open(child_path);
        if (h < 0) return false;
        handle_ = h;
        is_dir_ = true;
        open_   = true;
        store_name(name_, child);
        store_path(path_, child_path);
        return true;
    }

    int32_t fd = host_fs_open(child_path, mode);
    if (fd >= 0) {
        handle_ = fd;
        is_dir_ = false;
        open_   = true;
        store_name(name_, child);
        store_path(path_, child_path);
        return true;
    }
    return false;
}

bool FsFile::close() {
    if (open_) {
        if (is_dir_) host_fs_dir_close(handle_);
        else         host_fs_close(handle_);
    }
    handle_ = -1;
    open_   = false;
    is_dir_ = false;
    name_[0] = '\0';
    path_[0] = '\0';
    return true;
}

int FsFile::read(void* buf, size_t n) {
    if (!open_ || is_dir_) return -1;
    return host_fs_read(handle_, buf, (int32_t)n);
}

int FsFile::write(const void* buf, size_t n) {
    if (!open_ || is_dir_) return -1;
    return host_fs_write(handle_, buf, (int32_t)n);
}

bool FsFile::seekSet(uint32_t pos) {
    if (!open_ || is_dir_) return false;
    return host_fs_seek(handle_, (int32_t)pos) >= 0;
}

uint32_t FsFile::curPosition() const {
    if (!open_ || is_dir_) return 0;
    int32_t p = host_fs_tell(handle_);
    return p < 0 ? 0 : (uint32_t)p;
}

uint32_t FsFile::fileSize() const {
    if (!open_ || is_dir_) return 0;
    int32_t sz = host_fs_size(handle_);
    return sz < 0 ? 0 : (uint32_t)sz;
}

bool FsFile::rewind() {
    if (!open_) return false;
    if (is_dir_) return host_fs_dir_rewind(handle_) >= 0;
    return seekSet(0);
}

size_t FsFile::getName(char* dst, size_t cap) const {
    if (!dst || cap == 0) return 0;
    size_t n = strlen(name_);
    if (n >= cap) n = cap - 1;
    memcpy(dst, name_, n);
    dst[n] = '\0';
    return n;
}

bool FsFile::sync() {
    if (!open_ || is_dir_) return false;
    return host_fs_sync(handle_) >= 0;
}

bool FsFile::truncate(uint32_t length) {
    if (!open_ || is_dir_) return false;
    return host_fs_truncate(handle_, (int32_t)length) >= 0;
}

bool FsFile::remove() {
    if (!open_ || is_dir_) return false;

    // The host may not permit unlinking an open file. Close the handle without
    // clearing path_ first, then remove the same logical path.
    host_fs_close(handle_);
    handle_ = -1;
    open_ = false;
    bool removed = host_fs_remove(path_) >= 0;
    is_dir_ = false;
    name_[0] = '\0';
    path_[0] = '\0';
    return removed;
}

bool FsFile::preAllocate(uint32_t length) {
    if (!open_ || is_dir_) return false;
    return host_fs_truncate(handle_, (int32_t)length) >= 0;
}

// ---------------- SdFat --------------------------------------------------

bool SdFat::exists(const char* path)                           { return host_fs_exists(path) != 0; }
bool SdFat::mkdir(const char* path, bool recursive)            { return host_fs_mkdir(path, recursive ? 1 : 0) >= 0; }
bool SdFat::rmdir(const char* path)                            { return host_fs_rmdir(path) >= 0; }
bool SdFat::remove(const char* path)                           { return host_fs_remove(path) >= 0; }
bool SdFat::rename(const char* oldName, const char* newName)   { return host_fs_rename(oldName, newName) >= 0; }
bool SdFat::chdir(const char* path)                            { return host_fs_chdir(path) >= 0; }
