#ifndef MEMORY_H__
#define MEMORY_H__

#define SYSEX1_DATA_START 0x2200
#define SYSEX1_DATA_LEN 0x1830 //6KB

#define SYSEX2_DATA_START (SYSEX1_DATA_START + SYSEX2_DATA_LEN)
#define SYSEX2_DATA_LEN 0x4000 //16KB

extern inline uint8_t switch_ram_bank(uint8_t x) {
  uint8_t old_bank = (uint8_t) (PORTL >> PL6) & 0x01;

  if (x != old_bank) {
    PORTL ^= _BV(PL6);
    return old_bank;
  }
  return x;
}

#ifdef __cplusplus

class RamBankSelector {
  private:
  uint8_t m_oldbank;
  public:
  RamBankSelector(uint8_t bank) { m_oldbank = switch_ram_bank(bank); }
  ~RamBankSelector() { switch_ram_bank(m_oldbank); }
};

#define select_bank(x) RamBankSelector __bank_selector(x)

#endif// __cplusplus

extern inline void memcpy_bank1(volatile void *dst, volatile void *src, uint32_t len) {
  switch_ram_bank(1);
  memcpy(dst, src, len);
  switch_ram_bank(0);
}

extern inline void put_byte_bank1(volatile uint8_t *dst, uint8_t byte) {
  switch_ram_bank(1);
  *dst = byte;
  switch_ram_bank(0);
}

extern inline uint8_t get_byte_bank1(volatile uint8_t *dst) {
  switch_ram_bank(1);
  uint8_t c = *dst;
  switch_ram_bank(0);
  return c;
}


#endif /* MEMORY_H__ */
