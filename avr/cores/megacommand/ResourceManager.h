#pragma once

#include "uzlib.h"

struct resource_t {
	int len;
	uint8_t* block;
	int offset;
};

//#define RM_POOLSIZE 32
#define RM_BUFSIZE 4096

class ResourceManager {
private:
	void* GetResourceImpl(int RES_ID);
	//uint8_t FindResource(int RES_ID);
	//uint16_t m_seqId;
	//uint8_t m_slotId[RM_POOLSIZE];
	//uint8_t m_slotSeq[RM_POOLSIZE];
	//void* m_slotPtr[RM_POOLSIZE];
	//int m_slotSize[RM_POOLSIZE];
	uint8_t m_buffer[RM_BUFSIZE];

public:
	ResourceManager();

	template<typename T>
	T* GetResource(int RES_ID) {
		return (T*)GetResourceImpl(RES_ID);
	}
};

extern ResourceManager R;
