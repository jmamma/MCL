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

void ResourceManager::Save(uint8_t *buf, size_t *sz) {
	memcpy(buf, m_buffer, m_bufsize);
	*sz = m_bufsize;
}

void ResourceManager::Restore(uint8_t *buf, size_t sz) {
	memcpy(m_buffer, buf, sz);
	m_bufsize = sz;
}

ResourceManager R;
