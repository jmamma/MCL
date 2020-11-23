/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDI_COMMON_H__
#define MIDI_COMMON_H__

#include <inttypes.h>
#include "MidiNotes.h"
#ifndef NULL
#define NULL 0
#endif

/**
 * \addtogroup Midi
 *
 * @{
 **/

class MidiCallback {
 public:

#ifdef HOST_MIDIDUINO
  virtual ~MidiCallback() { }
#endif
};

typedef enum {
  midi_wait_status = 0,
  midi_wait_byte_2,
  midi_wait_byte_1,
  midi_wait_sysex,
  midi_ignore_message
} midi_state_t;

/**## MIDI definitions and MIDI parsing macros
 **/

/**
 ** Parse midi message macros
 **/
#define MIDI_IS_STATUS_BYTE(b)       ((b) & 0x80)
#define MIDI_IS_VOICE_STATUS_BYTE(b) ((b) <= 0xEF)
#define MIDI_VOICE_TYPE_NIBBLE(b)    ((b) & 0xF0)
#define MIDI_VOICE_CHANNEL(b)        ((b) & 0x0F)

#define MIDI_IS_SYSCOMMON_STATUS_BYTE(b) (((b) >= 0xF0) & ((b) < 0xF8))
#define MIDI_IS_REALTIME_STATUS_BYTE(b)  ((b) >= 0xF8)

/**
 ** Voice category messages
 **/
#define MIDI_NOTE_OFF         0x80 /* 2 bytes data */
#define MIDI_NOTE_ON          0x90 /* 2 bytes data */
#define MIDI_AFTER_TOUCH      0xA0 /* 2 bytes data */
#define MIDI_CONTROL_CHANGE   0xB0 /* 2 bytes data */
#define MIDI_PROGRAM_CHANGE   0xC0 /* 1 byte data */
#define MIDI_CHANNEL_PRESSURE 0xD0 /* 1 byte data */
#define MIDI_PITCH_WHEEL      0xE0 /* 2 bytes data */

/**
 ** System common category messages
 **/
#define MIDI_SYSEX_START       0xF0
#define MIDI_SYSEX_END         0xF7
#define MIDI_MTC_QUARTER_FRAME 0xF1 /* 1 byte data */
#define MIDI_SONG_POSITION_PTR 0xF2 /* 2 bytes data */
#define MIDI_SONG_SELECT       0xF3 /* 1 byte data */
#define MIDI_TUNE_REQUEST      0xF6 /* no data */

/**
 ** Realtime category messages, can be sent anytime
 **/
#define MIDI_CLOCK             0xF8 /* no data */
#define MIDI_TICK              0xF9 /* no data */
#define MIDI_START             0xFA /* no data */
#define MIDI_CONTINUE          0xFB /* no data */
#define MIDI_STOP              0xFC /* no data */
#define MIDI_ACTIVE_SENSE      0xFE /* no data */
#define MIDI_RESET             0xFF /* no data */

/* sysex wesen types */
#define MIDIDUINO_SYSEX_VENDOR_1 0x00
#define MIDIDUINO_SYSEX_VENDOR_2 0x13
#define MIDIDUINO_SYSEX_VENDOR_3 BOARD_ID

#define CMD_BOOT_DATA_BLOCK    0x01
#define CMD_DATA_BLOCK_ACK     0x02
#define CMD_FIRMWARE_CHECKSUM  0x03
#define CMD_MAIN_PROGRAM       0x04
#define CMD_START_BOOTLOADER   0x05

#define CMD_DATA_BLOCK_NAK     0x10
#define CMD_SEND_PAGE          0x11
#define CMD_SWITCH_PATCH       0x12
#define CMD_GET_PATCH          0x13
#define CMD_SAVE_PAGE          0x14
#define CMD_SET_FLASH          0x15

#define CMD_STRING             0x30

// typedef void (*midi_callback_t)(uint8_t *msg);

/* @} */

#endif /* MIDI_COMMON_H__ */

