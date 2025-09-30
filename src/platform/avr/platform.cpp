#include "platform.h"
#include "global.h"
#include "MidiUart.h"
#include "DebugBuffer.h"
#ifdef DEBUGMODE
DebugBuffer debugBuffer(&MidiUartUSB);
#endif

uint16_t calculate_checksum_for_range(const uint32_t start_addr, const uint32_t end_addr) {
  uint16_t checksum = 0;
  for (uint32_t i = start_addr; i < end_addr; i += 2) {
    uint16_t word = 0;
    // Check if there are at least 2 bytes left to read
    if (i + 1 < end_addr) {
      word = pgm_read_word_far(i);
    } else {
      // Handle the final byte if the section has an odd size
      word = pgm_read_byte_far(i);
    }
    checksum += word;
  }
  return checksum;
}


health_status health_check() {
  uint16_t stored_checksum = pgm_read_word(&firmware_checksum);

  // --- 2. Get the memory addresses from the linker symbols ---
  const uint32_t text_start_addr = (uint32_t)&__text_start;
  const uint32_t text_end_addr   = (uint32_t)&__text_end;
  const uint32_t data_start_addr = (uint32_t)&__data_load_start;
  const uint32_t data_end_addr   = (uint32_t)&__data_load_end;

  // --- 3. Calculate the checksum for each section individually ---
  uint16_t text_checksum = calculate_checksum_for_range(text_start_addr, text_end_addr);
  uint16_t data_checksum = calculate_checksum_for_range(data_start_addr, data_end_addr);

  // --- 4. Combine the checksums to get the final result ---
  // This addition mimics the build script's process of concatenating the
  // section data and then calculating a single sum.
  uint16_t calculated_checksum = text_checksum + data_checksum;


  // --- 5. Print debug information and compare ---
  DEBUG_PRINTLN("Health Check (Multi-Section Method):");
  DEBUG_PRINT("  > Stored Checksum:   0x"); DEBUG_PRINTLN(stored_checksum);
  DEBUG_PRINT("  > Calculated Sum:    0x"); DEBUG_PRINTLN(calculated_checksum);
  DEBUG_PRINTLN("--- Section Details ---");
  DEBUG_PRINT("  > .text section:  0x"); DEBUG_PRINT(text_start_addr);
  DEBUG_PRINT(" - 0x"); DEBUG_PRINTLN(text_end_addr);
  DEBUG_PRINT("  > .data section:  0x"); DEBUG_PRINT(data_start_addr);
  DEBUG_PRINT(" - 0x"); DEBUG_PRINTLN(data_end_addr);
  DEBUG_PRINTLN("-----------------------");


  if (calculated_checksum != stored_checksum) {
    DEBUG_PRINTLN("  > ERROR: CHECKSUM MISMATCH!");
    return BAD_CHECKSUM;
  } else {
    DEBUG_PRINTLN("  > SUCCESS: Checksum OK.");
    return HEALTH_OK;
  }
}


