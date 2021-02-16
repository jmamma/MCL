#pragma once

// MegaCommand core doesn't have the full new definition
// We just implement the support for placement-new
template<typename T> void* operator new (size_t, T* p) noexcept { return p;}

