#ifndef BANK1OBJECT_H__
#define BANK1OBJECT_H__
#include "WProgram.h"

/// <summary>
/// Store/retrieve portion of track object in mem bank1.
/// </summary>
template<int16_t sz_obj, uint8_t n_column_base, uint32_t p_addr_base> 
class Bank1Object {
  public:
  inline bool store_in_mem(uint8_t column, uint32_t region = p_addr_base) 
  {
    uint32_t pos = region + sz_obj * (uint32_t)(column - n_column_base);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(ptr, this, sz_obj);
    return true;
  }

  inline bool load_from_mem(uint8_t column, uint32_t region = p_addr_base)
  {
    uint32_t pos = region + sz_obj * (uint32_t)(column - n_column_base);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(this, ptr, sz_obj);
    return true;
  }
};

#endif // BANK1OBJECT_H__