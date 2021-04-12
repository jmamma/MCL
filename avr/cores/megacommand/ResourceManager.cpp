#include "MCL_impl.h"
#include "ResourceManager.h"

ResourceManager::ResourceManager() { }

void ResourceManager::Clear() {
	m_bufsize = 0;
}

byte* ResourceManager::__use_resource(const void* pgm) {
	byte* pos = m_buffer + m_bufsize;
	uint16_t sz = unpack((byte*)pgm, pos);
	m_bufsize += sz;
	return pos;
}

ResourceManager R;
