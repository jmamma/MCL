#pragma once
#include "unpack.h"
#include "resources/R.h"

#define RM_POOLSIZE 32
#define RM_BUFSIZE 4096

class ResourceManager {
private:
	uint8_t m_buffer[RM_BUFSIZE];
	uint16_t m_bufsize = 0;
	byte* __use_resource(void* pgm);

public:
	ResourceManager();
	void Clear();

#include "resources/ResMan.h"
};

extern ResourceManager R;
