// linktest_main.cpp — smoke test for libmcl_desktop.a.
//
// Forces a real link of the static library so missing-symbol errors that
// `ar` would silently archive into unresolved refs surface here instead.
// Not part of the default build; built via `cmake --build BUILD_DIR --target
// mcl_desktop_linktest`.
#include "desktop_entry.h"

int main(int /*argc*/, char* /*argv*/[]) {
    mcl_desktop_setup();
    mcl_desktop_tick();
    return 0;
}
