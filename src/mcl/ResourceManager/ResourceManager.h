#pragma once
#include "unpack.h"
#include "R.h"

#if defined(__AVR__)
#define RM_BUFSIZE 6500
#else
#define RM_BUFSIZE 32768
#endif

class ResourceManager {
private:
	uint8_t m_buffer[RM_BUFSIZE];
	uint16_t m_bufsize;
	uint16_t m_persistent_size;
#if !defined(__AVR__)
	__T_machine_param_names *m_persistent_machine_param_names = nullptr;
	__T_machine_names_short *m_persistent_machine_names_short = nullptr;
#endif
#if defined(PLATFORM_TBD)
	__T_icons_knob *m_persistent_icons_knob = nullptr;
#endif
	byte* __use_resource(const void* pgm);

public:
	ResourceManager() = default;
	void Clear();
	void SetPersistent();
	void Save(uint8_t* buf, size_t* sz);
	void Restore(uint8_t* buf, size_t sz);
	// allocate memory in a stack-like manner
	byte* Allocate(size_t sz);
	// free memory in a stack-like manner
	void Free(size_t sz);
	void restore_menu_layout_deps();
	void restore_page_entry_deps();
	size_t Size();

#include "ResMan.h"
};

extern ResourceManager R;
