#pragma once
#include "unpack.h"

struct resource_t {
	int len;
	uint8_t* block;
	int offset;
};

#define RM_POOLSIZE 32
#define RM_BUFSIZE 4096

class ResourceManager {
private:
	uint8_t m_buffer[RM_BUFSIZE];

public:
	ResourceManager();
};

extern ResourceManager R;
