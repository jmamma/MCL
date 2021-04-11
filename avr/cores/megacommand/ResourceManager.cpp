#include "ResourceManager.h"
#include <avr/pgmspace.h>

const resource_t __RESOURCES[] PROGMEM = {
};

ResourceManager::ResourceManager() {
  // m_seqId = 0;
  // for(int i = 0; i < RM_POOLSIZE; ++i) {
  // m_slotId[i] = 0xFF;
  // m_slotSeq[i] = 0x00;
  // m_slotPtr[i] = nullptr;
  //}
}

//uint8_t ResourceManager::FindResource(int RES_ID) {
  //++m_seqId;
  //uint8_t pid;
  //for (pid = 0; pid < RM_POOLSIZE; ++pid) {
    //if (RES_ID == m_slotId[pid])
      //break;
  //}
  //if (pid == RM_POOLSIZE) {
    //pid = 0;
    //uint8_t req = m_slotSeq[0];
    //for (uint8_t i = 1; i < RM_POOLSIZE; ++i) {
      //if (m_slotSeq[i] < req) {
        //req = m_slotSeq[i];
        //pid = i;
      //}
    //}
  //}
  //m_slotSeq[pid] = ++m_seqId;
  //return pid;
//}

void *ResourceManager::GetResourceImpl(int RES_ID) {
  resource_t resource;
  memcpy_P(&resource, __RESOURCES + RES_ID, sizeof(resource_t));
  return m_buffer;
}

ResourceManager R;
