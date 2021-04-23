#pragma once
#include "unpack.h"
#include "resources/R.h"

#define RM_BUFSIZE 6500

class ResourceManager {
private:
	uint8_t m_buffer[RM_BUFSIZE];
	uint16_t m_bufsize = 0;
	byte* __use_resource(const void* pgm);

public:
	ResourceManager();
	void Clear();
	void Save(uint8_t* buf, size_t* sz);
	void Restore(uint8_t* buf, size_t sz);
	void restore_menu_layout_deps();
	void restore_page_entry_deps();
	size_t Size();

#include "resources/ResMan.h"
};

extern ResourceManager R;
