// SdFat.cpp — desktop std::filesystem-backed implementation of the SdFat
// shim declared in desktop_common/SdFat.h. The header is platform-agnostic
// (int32_t handle slots); this file maps those handles to std::fstream /
// std::filesystem::directory_iterator behind a small handle table.
#include "SdFat.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <system_error>

namespace fs = std::filesystem;

namespace {

// ---- Path resolution -----------------------------------------------------

fs::path& sd_root() {
    static fs::path root = fs::current_path() / "mcl_sd";
    return root;
}
fs::path& sd_cwd() {
    static fs::path cwd = "/";
    return cwd;
}

fs::path resolve_sd_path(const char* path) {
    if (!path || !*path) {
        return sd_root() / sd_cwd().relative_path();
    }
    fs::path p(path);
    if (p.is_absolute() || path[0] == '/') {
        std::string s = (path[0] == '/') ? std::string(path + 1) : p.string();
        return sd_root() / s;
    }
    return sd_root() / sd_cwd().relative_path() / p;
}

// ---- Handle tables -------------------------------------------------------
//
// FsFile holds a single int32_t handle_ that maps to either a file entry
// (file_table) or a directory iterator (dir_table) depending on is_dir_.
// Handles are dense, positive ints assigned at open() time; -1 = invalid.

struct FileEntry {
    fs::path                       path;
    std::unique_ptr<std::fstream>  stream;
};
struct DirEntry {
    fs::path                       path;
    fs::directory_iterator         it;
    bool                           started = false;
};

std::unordered_map<int32_t, FileEntry>& file_table() {
    static std::unordered_map<int32_t, FileEntry> t;
    return t;
}
std::unordered_map<int32_t, DirEntry>& dir_table() {
    static std::unordered_map<int32_t, DirEntry> t;
    return t;
}

int32_t next_handle() {
    static int32_t n = 1;
    return n++;
}

}  // namespace

void mcl_desktop_set_sd_root(const char* abs_path) {
    if (!abs_path || !*abs_path) return;
    sd_root() = fs::path(abs_path);
    std::error_code ec;
    fs::create_directories(sd_root(), ec);
    sd_cwd() = "/";
}

// ---------------- FsFile -------------------------------------------------

FsFile& FsFile::operator=(FsFile&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        std::memcpy(name_, other.name_, sizeof(name_));
        std::memcpy(path_, other.path_, sizeof(path_));
        open_   = other.open_;
        is_dir_ = other.is_dir_;
        other.handle_ = -1;
        other.open_   = false;
        other.is_dir_ = false;
        other.path_[0] = '\0';
    }
    return *this;
}

bool FsFile::open(const char* path, uint8_t mode) {
    close();
    if (!path) return false;

    fs::path resolved = resolve_sd_path(path);
    std::error_code ec;

    if (fs::is_directory(resolved, ec)) {
        DirEntry d;
        d.path = resolved;
        handle_ = next_handle();
        dir_table()[handle_] = std::move(d);
        is_dir_ = true;
        open_   = true;
        auto name = resolved.filename().string();
        std::strncpy(name_, name.c_str(), sizeof(name_) - 1);
        name_[sizeof(name_) - 1] = '\0';
        auto path_string = resolved.string();
        std::strncpy(path_, path_string.c_str(), sizeof(path_) - 1);
        path_[sizeof(path_) - 1] = '\0';
        return true;
    }

    const bool want_create = (mode & O_CREAT) != 0;
    const bool want_excl   = (mode & O_EXCL)  != 0;
    const bool want_write  = (mode & O_WRITE) != 0;
    const bool want_append = (mode & O_APPEND)!= 0;
    const bool want_trunc  = (mode & O_TRUNC) != 0;
    const bool exists      = fs::exists(resolved, ec);

    if (want_excl && exists) return false;
    if (!exists && !want_create) return false;

    std::ios::openmode om = std::ios::binary;
    if (mode & O_READ) om |= std::ios::in;
    if (want_write)    om |= std::ios::out;
    if (want_append)   om |= std::ios::app;
    if (want_trunc)    om |= std::ios::trunc;

    if (!exists && want_create) {
        std::ofstream touch(resolved.string(), std::ios::binary);
        if (!touch) return false;
    }

    auto s = std::make_unique<std::fstream>(resolved.string(), om);
    if (!s->is_open()) return false;

    FileEntry f;
    f.path   = resolved;
    f.stream = std::move(s);
    handle_  = next_handle();
    file_table()[handle_] = std::move(f);
    is_dir_  = false;
    open_    = true;
    auto name = resolved.filename().string();
    std::strncpy(name_, name.c_str(), sizeof(name_) - 1);
    name_[sizeof(name_) - 1] = '\0';
    auto path_string = resolved.string();
    std::strncpy(path_, path_string.c_str(), sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
    return true;
}

bool FsFile::openNext(FsFile* dir, uint8_t mode) {
    if (!dir || !dir->open_ || !dir->is_dir_) return false;

    auto it = dir_table().find(dir->handle_);
    if (it == dir_table().end()) return false;
    DirEntry& d = it->second;

    if (!d.started) {
        std::error_code ec;
        d.it = fs::directory_iterator(d.path, ec);
        if (ec) return false;
        d.started = true;
    }

    if (d.it == fs::end(d.it)) return false;

    fs::path entry = d.it->path();
    ++d.it;

    close();
    std::error_code ec;
    if (fs::is_directory(entry, ec)) {
        DirEntry sub;
        sub.path = entry;
        handle_  = next_handle();
        dir_table()[handle_] = std::move(sub);
        is_dir_ = true;
        open_   = true;
        auto name = entry.filename().string();
        std::strncpy(name_, name.c_str(), sizeof(name_) - 1);
        name_[sizeof(name_) - 1] = '\0';
        auto path_string = entry.string();
        std::strncpy(path_, path_string.c_str(), sizeof(path_) - 1);
        path_[sizeof(path_) - 1] = '\0';
        return true;
    }

    std::ios::openmode om = std::ios::binary;
    if (mode & O_READ)  om |= std::ios::in;
    if (mode & O_WRITE) om |= std::ios::out;
    auto s = std::make_unique<std::fstream>(entry.string(), om);
    if (!s->is_open()) return false;

    FileEntry f;
    f.path   = entry;
    f.stream = std::move(s);
    handle_  = next_handle();
    file_table()[handle_] = std::move(f);
    is_dir_ = false;
    open_   = true;
    auto name = entry.filename().string();
    std::strncpy(name_, name.c_str(), sizeof(name_) - 1);
    name_[sizeof(name_) - 1] = '\0';
    auto path_string = entry.string();
    std::strncpy(path_, path_string.c_str(), sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
    return true;
}

bool FsFile::close() {
    if (open_) {
        if (is_dir_) dir_table().erase(handle_);
        else         file_table().erase(handle_);
    }
    handle_ = -1;
    open_   = false;
    is_dir_ = false;
    name_[0] = '\0';
    path_[0] = '\0';
    return true;
}

int FsFile::read(void* buf, size_t n) {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return -1;
    auto& s = *it->second.stream;
    s.read(static_cast<char*>(buf), static_cast<std::streamsize>(n));
    auto got = s.gcount();
    if (got == 0 && s.fail() && !s.eof()) return -1;
    if (s.eof()) s.clear();
    return static_cast<int>(got);
}

int FsFile::write(const void* buf, size_t n) {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return -1;
    auto& s = *it->second.stream;
    s.write(static_cast<const char*>(buf), static_cast<std::streamsize>(n));
    if (s.fail()) return -1;
    return static_cast<int>(n);
}

bool FsFile::seekSet(uint32_t pos) {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return false;
    auto& s = *it->second.stream;
    s.clear();
    s.seekg(pos);
    s.seekp(pos);
    return !s.fail();
}

uint32_t FsFile::curPosition() const {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return 0;
    auto p = it->second.stream->tellg();
    return p < 0 ? 0 : static_cast<uint32_t>(p);
}

uint32_t FsFile::fileSize() const {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return 0;
    std::error_code ec;
    auto sz = fs::file_size(it->second.path, ec);
    return ec ? 0 : static_cast<uint32_t>(sz);
}

bool FsFile::rewind() {
    if (is_dir_) {
        auto it = dir_table().find(handle_);
        if (it == dir_table().end()) return false;
        it->second.started = false;
        return true;
    }
    return seekSet(0);
}

size_t FsFile::getName(char* dst, size_t cap) const {
    if (!dst || cap == 0) return 0;
    size_t n = std::strlen(name_);
    if (n >= cap) n = cap - 1;
    std::memcpy(dst, name_, n);
    dst[n] = '\0';
    return n;
}

bool FsFile::sync() {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return false;
    it->second.stream->flush();
    return !it->second.stream->fail();
}

bool FsFile::truncate(uint32_t length) {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return false;
    std::error_code ec;
    fs::resize_file(it->second.path, length, ec);
    return !ec;
}

bool FsFile::remove() {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return false;
    fs::path p = it->second.path;
    close();
    std::error_code ec;
    return fs::remove(p, ec);
}

bool FsFile::preAllocate(uint32_t length) {
    auto it = file_table().find(handle_);
    if (it == file_table().end()) return false;
    std::error_code ec;
    fs::resize_file(it->second.path, length, ec);
    return !ec;
}

// ---------------- SdFat --------------------------------------------------

bool SdFat::exists(const char* path) {
    std::error_code ec;
    return fs::exists(resolve_sd_path(path), ec);
}

bool SdFat::mkdir(const char* path, bool recursive) {
    std::error_code ec;
    auto p = resolve_sd_path(path);
    if (recursive) fs::create_directories(p, ec);
    else           fs::create_directory(p, ec);
    if (ec && fs::exists(p)) return true;
    return !ec;
}

bool SdFat::rmdir(const char* path) {
    std::error_code ec;
    return fs::remove(resolve_sd_path(path), ec);
}

bool SdFat::remove(const char* path) {
    std::error_code ec;
    return fs::remove(resolve_sd_path(path), ec);
}

bool SdFat::rename(const char* oldName, const char* newName) {
    if (!oldName || !newName) return false;
    std::error_code ec;
    fs::rename(resolve_sd_path(oldName), resolve_sd_path(newName), ec);
    return !ec;
}

bool SdFat::chdir(const char* path) {
    if (!path) return false;
    fs::path resolved = resolve_sd_path(path);
    std::error_code ec;
    if (!fs::is_directory(resolved, ec)) return false;
    auto rel = fs::relative(resolved, sd_root(), ec);
    if (ec) return false;
    sd_cwd() = fs::path("/") / rel;
    return true;
}
