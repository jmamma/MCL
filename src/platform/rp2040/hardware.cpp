#include "Adafruit_TinyUSB.h"
#include <arduino/msc/Adafruit_USBD_MSC.h>
#include "MCLSD.h"
#include "Project.h"
#include "hardware.h"

Adafruit_USBD_MSC usb_msc;
// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize) {
  bool rc;

  rc = SD.card()->readSectors(lba, (uint8_t*) buffer, bufsize/512);

  return rc ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  bool rc;

  digitalWrite(LED_BUILTIN, HIGH);

  rc = SD.card()->writeSectors(lba, buffer, bufsize/512);

  return rc ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb (void) {
  SD.card()->syncDevice();

  digitalWrite(LED_BUILTIN, LOW);
}
void change_usb_mode(uint8_t mode) {
  /*
  uint8_t change_mode_msg[] = {0xF0, 0x7D, 0x4D, 0x43, 0x4C, 0x01, mode, 0xF7};
  MidiUartUSB.m_putc(change_mode_msg, sizeof(change_mode_msg));
  delay(200);
  if (mode == USB_SERIAL) {
     MidiUartUSB.mode = UART_SERIAL; MidiUartUSB.set_speed(SERIAL_SPEED);
  }
  */

  if (mode == USB_STORAGE) {
    usb_msc.setID("MegaCommand", "SD Card", "1.0");
    usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    usb_msc.setUnitReady(false);
    usb_msc.begin();
    if (TinyUSBDevice.mounted()) {
      TinyUSBDevice.detach();
      delay(10);
      TinyUSBDevice.attach();
    }
    DEBUG_PRINTLN("USB Mass Storage Initializing SD");
    proj.close_project();
    uint32_t block_count = SD.card()->sectorCount();
    usb_msc.setCapacity(block_count, 512);
    usb_msc.setUnitReady(true);
  }
}


