// SdFat.cpp — desktop shim implementation. Backed by std::filesystem.
#include "SdFat.h"

#include <cstring>
#include <system_error>

namespace fs = std::filesystem;

namespace {
fs::path& sd_root() {
    static fs::path root = fs::current_path() / "mcl_sd";
    return root;
}
fs::path& sd_cwd() {
    static fs::path cwd = "/";  // virtual cwd inside the SD root
    return cwd;
}
} // namespace

void mcl_desktop_set_sd_root(const char* abs_path) {
    if (!abs_path || !*abs_path) return;
    sd_root() = fs::path(abs_path);
    std::error_code ec;
    fs::create_directories(sd_root(), ec);
    sd_cwd() = "/";
}

fs::path mcl_desktop_resolve_sd_path(const char* path) {
    if (!path || !*path) {
        return sd_root() / sd_cwd().relative_path();
    }
    fs::path p(path);
    if (p.is_absolute() || (path[0] == '/')) {
        // Strip the leading '/' so it joins onto sd_root() rather than
        // overwriting it.
        std::string s = path[0] == '/' ? std::string(path + 1) : p.string();
        return sd_root() / s;
    }
    return sd_root() / sd_cwd().relative_path() / p;
}

// ---------------- FsFile -------------------------------------------------

FsFile::FsFile(FsFile&& other) noexcept {
    *this = std::move(other);
}
FsFile& FsFile::operator=(FsFile&& other) noexcept {
    if (this != &other) {
        close();
        path_                  = std::move(other.path_);
        stream_                = std::move(other.stream_);
        open_                  = other.open_;
        is_dir_                = other.is_dir_;
        dir_iter_initialised_  = other.dir_iter_initialised_;
        dir_it_                = std::move(other.dir_it_);
        other.open_ = false;
        other.is_dir_ = false;
        other.dir_iter_initialised_ = false;
    }
    return *this;
}

bool FsFile::open(const char* path, uint8_t mode) {
    close();
    if (!path) return false;

    fs::path resolved = mcl_desktop_resolve_sd_path(path);
    std::error_code ec;

    if (fs::is_directory(resolved, ec)) {
        path_   = resolved;
        is_dir_ = true;
        open_   = true;
        dir_iter_initialised_ = false;
        return true;
    }

    const bool want_create  = (mode & O_CREAT) != 0;
    const bool want_excl    = (mode & O_EXCL)  != 0;
    const bool want_write   = (mode & O_WRITE) != 0;
    const bool want_append  = (mode & O_APPEND)!= 0;
    const bool want_trunc   = (mode & O_TRUNC) != 0;
    const bool exists       = fs::exists(resolved, ec);

    if (want_excl && exists) return false;
    if (!exists && !want_create) return false;

    std::ios::openmode openmode = std::ios::binary;
    if (mode & O_READ)  openmode |= std::ios::in;
    if (want_write)     openmode |= std::ios::out;
    if (want_append)    openmode |= std::ios::app;
    if (want_trunc)     openmode |= std::ios::trunc;

    if (!exists && want_create) {
        // std::fstream won't create a file in in|out mode without trunc, so
        // touch it first.
        std::ofstream touch(resolved.string(), std::ios::binary);
        if (!touch) return false;
    }

    auto s = std::make_unique<std::fstream>(resolved.string(), openmode);
    if (!s->is_open()) return false;

    path_    = resolved;
    stream_  = std::move(s);
    is_dir_  = false;
    open_    = true;
    return true;
}

bool FsFile::openNext(FsFile* dir, uint8_t mode) {
    if (!dir || !dir->open_ || !dir->is_dir_) return false;

    if (!dir->dir_iter_initialised_) {
        std::error_code ec;
        dir->dir_it_ = fs::directory_iterator(dir->path_, ec);
        if (ec) return false;
        dir->dir_iter_initialised_ = true;
    }

    if (dir->dir_it_ == fs::end(dir->dir_it_)) {
        return false;
    }

    fs::path entry = dir->dir_it_->path();
    std::error_code ec;
    ++dir->dir_it_;
    if (ec) return false;

    close();
    if (fs::is_directory(entry, ec)) {
        path_   = entry;
        is_dir_ = true;
        open_   = true;
        dir_iter_initialised_ = false;
        return true;
    }

    std::ios::openmode openmode = std::ios::binary;
    if (mode & O_READ)  openmode |= std::ios::in;
    if (mode & O_WRITE) openmode |= std::ios::out;
    auto s = std::make_unique<std::fstream>(entry.string(), openmode);
    if (!s->is_open()) return false;

    path_   = entry;
    stream_ = std::move(s);
    is_dir_ = false;
    open_   = true;
    return true;
}

bool FsFile::close() {
    if (stream_) {
        stream_->close();
        stream_.reset();
    }
    open_   = false;
    is_dir_ = false;
    dir_iter_initialised_ = false;
    return true;
}

int FsFile::read(void* buf, size_t n) {
    if (!stream_ || is_dir_) return -1;
    stream_->read(static_cast<char*>(buf), static_cast<std::streamsize>(n));
    auto got = stream_->gcount();
    if (got == 0 && stream_->fail() && !stream_->eof()) return -1;
    if (stream_->eof()) stream_->clear();   // allow subsequent reads after EOF reset
    return static_cast<int>(got);
}

int FsFile::write(const void* buf, size_t n) {
    if (!stream_ || is_dir_) return -1;
    stream_->write(static_cast<const char*>(buf), static_cast<std::streamsize>(n));
    if (stream_->fail()) return -1;
    return static_cast<int>(n);
}

bool FsFile::seekSet(uint32_t pos) {
    if (!stream_ || is_dir_) return false;
    stream_->clear();
    stream_->seekg(pos);
    stream_->seekp(pos);
    return !stream_->fail();
}

uint32_t FsFile::curPosition() const {
    if (!stream_ || is_dir_) return 0;
    auto p = const_cast<std::fstream*>(stream_.get())->tellg();
    return p < 0 ? 0 : static_cast<uint32_t>(p);
}

uint32_t FsFile::fileSize() const {
    if (path_.empty()) return 0;
    std::error_code ec;
    auto sz = fs::file_size(path_, ec);
    return ec ? 0 : static_cast<uint32_t>(sz);
}

bool FsFile::rewind() {
    if (is_dir_) {
        dir_iter_initialised_ = false;
        return true;
    }
    return seekSet(0);
}

size_t FsFile::getName(char* dst, size_t cap) const {
    if (!dst || cap == 0 || path_.empty()) {
        if (dst && cap > 0) dst[0] = '\0';
        return 0;
    }
    auto name = path_.filename().string();
    size_t n = std::min(name.size(), cap - 1);
    std::memcpy(dst, name.c_str(), n);
    dst[n] = '\0';
    return n;
}

bool FsFile::sync() {
    if (!stream_ || is_dir_) return false;
    stream_->flush();
    return !stream_->fail();
}

bool FsFile::truncate(uint32_t length) {
    if (!stream_ || is_dir_ || path_.empty()) return false;
    std::error_code ec;
    fs::resize_file(path_, length, ec);
    return !ec;
}

bool FsFile::remove() {
    if (path_.empty()) return false;
    close();
    std::error_code ec;
    return fs::remove(path_, ec);
}

bool FsFile::preAllocate(uint32_t length) {
    // std::filesystem has resize_file; semantically extends or shrinks the
    // file. SdFat preAllocate reserves contiguous space, which we can't do on
    // a host filesystem — but extending the file produces equivalent behavior
    // from MCL's perspective.
    if (path_.empty()) return false;
    std::error_code ec;
    fs::resize_file(path_, length, ec);
    return !ec;
}

// ---------------- SdFat --------------------------------------------------

bool SdFat::exists(const char* path) {
    std::error_code ec;
    return fs::exists(mcl_desktop_resolve_sd_path(path), ec);
}

bool SdFat::mkdir(const char* path, bool recursive) {
    std::error_code ec;
    auto p = mcl_desktop_resolve_sd_path(path);
    if (recursive) {
        fs::create_directories(p, ec);
    } else {
        fs::create_directory(p, ec);
    }
    if (ec && fs::exists(p)) return true;
    return !ec;
}

bool SdFat::rmdir(const char* path) {
    std::error_code ec;
    return fs::remove(mcl_desktop_resolve_sd_path(path), ec);
}

bool SdFat::remove(const char* path) {
    std::error_code ec;
    return fs::remove(mcl_desktop_resolve_sd_path(path), ec);
}

bool SdFat::rename(const char* oldName, const char* newName) {
    if (!oldName || !newName) return false;
    std::error_code ec;
    fs::rename(mcl_desktop_resolve_sd_path(oldName),
               mcl_desktop_resolve_sd_path(newName), ec);
    return !ec;
}

bool SdFat::chdir(const char* path) {
    if (!path) return false;
    fs::path resolved = mcl_desktop_resolve_sd_path(path);
    std::error_code ec;
    if (!fs::is_directory(resolved, ec)) return false;
    // Express new cwd relative to sd_root.
    auto rel = fs::relative(resolved, sd_root(), ec);
    if (ec) return false;
    sd_cwd() = fs::path("/") / rel;
    return true;
}
