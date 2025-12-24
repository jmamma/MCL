#pragma once

// ResourceManager.h stub for desktop builds
// We don't need resource management on desktop

#include <stdint.h>
#include <stddef.h>

#define RM_BUFSIZE 6500

class ResourceManager {
public:
    uint8_t* icons_device = nullptr;
    uint8_t* icons_logo = nullptr;

    ResourceManager() {}
    void Clear() {}
    void Save(uint8_t* buf, size_t* sz) { (void)buf; (void)sz; }
    void Restore(uint8_t* buf, size_t sz) { (void)buf; (void)sz; }
    uint8_t* Allocate(size_t sz) { (void)sz; return nullptr; }
    void Free(size_t sz) { (void)sz; }
    void restore_menu_layout_deps() {}
    void restore_page_entry_deps() {}
    size_t Size() { return 0; }
};

inline ResourceManager R;
