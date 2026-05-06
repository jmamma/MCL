#pragma once

#include <stdint.h>

#ifdef PLATFORM_TBD

static constexpr uint16_t TBD_P4_SPI_FRAME_SIZE = 512;
static constexpr uint16_t TBD_P4_SPI_MIDI_DATA_SIZE = 256;
static constexpr uint16_t TBD_P4_SPI_USB_MIDI_DATA_SIZE = 256;
static constexpr uint16_t TBD_P4_SPI_HEADER_SIZE = 16;

struct __attribute__((packed)) TbdP4SpiRequestHeader {
  uint16_t magic;
  uint8_t request_sequence_counter;
  uint8_t reserved1[5];
  uint16_t payload_length;
  uint16_t payload_crc;
  uint32_t reserved2;
};

struct __attribute__((packed)) TbdP4SpiResponseHeader {
  uint16_t magic;
  uint8_t response_sequence_counter;
  uint8_t reserved1[5];
  uint16_t payload_length;
  uint16_t payload_crc;
  uint32_t reserved2;
};

struct __attribute__((packed)) TbdP4SpiRequest {
  uint32_t magic;
  uint32_t synth_midi_length;
  uint8_t synth_midi[TBD_P4_SPI_MIDI_DATA_SIZE];
  uint32_t sequencer_tempo;
  uint32_t sequencer_active_track;
  uint32_t magic2;
};

struct __attribute__((packed)) TbdP4SpiResponse {
  uint32_t magic;
  uint32_t usb_device_midi_length;
  uint8_t usb_device_midi[TBD_P4_SPI_USB_MIDI_DATA_SIZE];
  uint8_t input_waveform[64];
  uint8_t output_waveform[64];
  uint8_t link_data[64];
  uint32_t led_color;
  uint32_t webui_update_counter;
  uint32_t magic2;
  uint32_t screenshot_request_counter;
  uint8_t injected_button;
  uint8_t injected_button_event;
  uint8_t network_status;
  uint8_t reserved_input;
  uint8_t input_peak_byte;
  uint8_t output_peak_byte;
  uint8_t reserved_tail[2];
};

static_assert(sizeof(TbdP4SpiRequestHeader) == TBD_P4_SPI_HEADER_SIZE,
              "TBD P4 request header layout drifted");
static_assert(sizeof(TbdP4SpiResponseHeader) == TBD_P4_SPI_HEADER_SIZE,
              "TBD P4 response header layout drifted");
static_assert(sizeof(TbdP4SpiRequest) == 276,
              "TBD P4 request payload layout drifted");
static_assert(sizeof(TbdP4SpiResponse) == 480,
              "TBD P4 response payload layout drifted");
static_assert(2 + TBD_P4_SPI_HEADER_SIZE + sizeof(TbdP4SpiResponse) <=
                  TBD_P4_SPI_FRAME_SIZE,
              "TBD P4 response no longer fits in one SPI frame");

#endif
