#include "platform.h"
#include "global.h"
#include "MidiUart.h"
#include "DebugBuffer.h"
#ifdef DEBUGMODE
DebugBuffer debugBuffer(&MidiUartUSB);
#endif

health_status health_check() {

   // Read the checksum value that was embedded by the build script.

  uint32_t firmware_length = pgm_get_far_address(firmware_checksum);
  uint16_t stored_checksum = pgm_read_word_far(firmware_length);

  // Address of checksum in flash == length of firmware before checksum

  uint16_t calculated_checksum = 0;

  // Loop over the firmware, 2 bytes at a time, up to the start of the checksum.
  for (uint32_t i = 0; i < firmware_length; i += 2) {
    uint16_t word = 0;
    if (i + 1 < firmware_length) {
      word = pgm_read_word_far(i);
    } else {
      word = pgm_read_byte_far(i); // Handle final byte for odd-sized firmware
    }
    calculated_checksum += word;
  }

  DEBUG_PRINTLN("Health Check (Checksum-as-Length Method):");
  DEBUG_PRINT("  > Stored Checksum: ");
  DEBUG_PRINTLN(stored_checksum); // Prints the number and a newline

  DEBUG_PRINT("  > Calculated Sum:  ");
  DEBUG_PRINTLN(calculated_checksum); // Prints the number and a newline

  DEBUG_PRINT("  > Firmware Size:   ");
  DEBUG_PRINT(firmware_length); // Prints the number
  DEBUG_PRINTLN(" bytes");      // Prints " bytes" and a newline
                                //

  if (calculated_checksum != stored_checksum) {
    DEBUG_PRINTLN("  > ERROR: CHECKSUM MISMATCH!");
  } else {
    DEBUG_PRINTLN("  > SUCCESS: Checksum OK.");
  }

  return (calculated_checksum == stored_checksum) ? HEALTH_OK : BAD_CHECKSUM;
}

