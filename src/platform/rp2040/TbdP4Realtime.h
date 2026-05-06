#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "TbdP4Protocol.h"
#include <stdint.h>

struct TbdP4RealtimeStats {
  uint32_t tx_frames;
  uint32_t rx_frames;
  uint32_t tx_midi_bytes;
  uint32_t rx_midi_bytes;
  uint32_t dropped_tx_bytes;
  uint32_t dropped_rx_bytes;
  uint32_t error_count;
  uint32_t fingerprint_errors;
  uint32_t length_errors;
  uint32_t crc_errors;
  uint32_t sequence_errors;
  uint32_t ws_sync_count;
  uint32_t missed_ws_sync_count;
  uint32_t p4_not_ready_count;
  uint32_t dma_busy_count;
  uint32_t dma_timeout_count;
  uint32_t dma_unavailable_count;
  uint32_t last_spi_send_ms;
  uint32_t last_response_ms;
  uint8_t last_response_sequence;
  bool p4_alive;
  bool p4_sync_seen;
  bool p4_ready_pin;
  bool spi_active;
  bool dma_ready;
  bool dma_reset_pending;
};

class TbdP4RealtimeTransport {
public:
  void init();
  void poll();
  void recover_blocking();

  bool can_enqueue_midi_byte() const;
  bool enqueue_midi_byte_isr(uint8_t byte);
  bool pop_rx_midi_byte(uint8_t &byte);
  bool pop_rx_midi_byte_isr(uint8_t &byte);
  void get_stats(TbdP4RealtimeStats &stats) const;

  void set_tempo_centi(uint32_t tempo_centi) { sequencer_tempo_ = tempo_centi; }
  void set_active_track(uint32_t active_track) {
    sequencer_active_track_ = active_track;
  }

  bool p4_alive() const { return p4_alive_; }
  uint32_t receive_count() const { return receive_count_; }
  uint32_t error_count() const { return error_count_; }
  uint32_t dropped_tx_bytes() const { return dropped_tx_bytes_; }
  uint32_t dropped_rx_bytes() const { return dropped_rx_bytes_; }
  uint32_t tx_frame_count() const { return tx_frame_count_; }
  uint32_t tx_midi_bytes() const { return tx_midi_bytes_; }
  uint32_t rx_midi_bytes() const { return rx_midi_bytes_; }

  uint32_t led_color() const { return led_color_; }
  uint32_t webui_update_counter() const { return webui_update_counter_; }
  uint8_t network_status() const { return network_status_; }
  uint8_t input_peak_byte() const { return input_peak_byte_; }
  uint8_t output_peak_byte() const { return output_peak_byte_; }
  const uint8_t *input_waveform() const { return input_waveform_; }
  const uint8_t *output_waveform() const { return output_waveform_; }

private:
  static constexpr uint16_t kMidiFifoSize = 2048;
  static constexpr uint16_t kSpiDeadlineMs = 3000;

  struct SpiTransaction {
    uint8_t out_buf[TBD_P4_SPI_FRAME_SIZE];
    uint8_t in_buf[TBD_P4_SPI_FRAME_SIZE];
  };

  bool fifo_full(uint16_t wr, uint16_t rd) const;
  bool midi_tx_empty_isr() const { return midi_tx_rd_ == midi_tx_wr_; }
  bool midi_rx_empty_isr() const { return midi_rx_rd_ == midi_rx_wr_; }
  bool pop_tx_midi_byte_locked(uint8_t &byte);
  bool enqueue_rx_midi_byte_locked(uint8_t byte);

  uint16_t calc_payload_crc(const uint8_t *data, uint16_t length) const;
  uint8_t next_sequence(uint8_t current) const;
  bool p4_ready() const;
  bool finish_transaction();
  void abort_transaction_blocking();
  void prepare_request();
  bool start_transaction();
  void process_response();
  bool dma_ready() const { return dma_rx_channel_ >= 0 && dma_tx_channel_ >= 0; }

  SpiTransaction spi_trans_[2];
  int dma_rx_channel_ = -1;
  int dma_tx_channel_ = -1;
  uint8_t sending_trans_ = 0;
  uint8_t receiving_trans_ = 0;
  bool initialized_ = false;
  bool spi_active_ = false;
  bool dma_reset_pending_ = false;
  bool request_prepared_ = false;
  bool can_prepare_request_ = true;
  uint32_t next_spi_send_ms_ = 0;
  uint32_t spi_deadline_ms_ = 0;
  uint32_t last_spi_send_ms_ = 0;
  uint32_t last_ws_sync_ms_ = 0;
  uint32_t last_response_ms_ = 0;
  uint8_t next_request_sequence_ = 100;
  uint8_t last_seen_response_ = 0;

  uint8_t midi_tx_fifo_[kMidiFifoSize];
  volatile uint16_t midi_tx_rd_ = 0;
  volatile uint16_t midi_tx_wr_ = 0;
  uint8_t midi_rx_fifo_[kMidiFifoSize];
  volatile uint16_t midi_rx_rd_ = 0;
  volatile uint16_t midi_rx_wr_ = 0;

  volatile uint32_t sequencer_tempo_ = 12000;
  volatile uint32_t sequencer_active_track_ = 0;

  bool p4_alive_ = false;
  uint32_t receive_count_ = 0;
  uint32_t tx_frame_count_ = 0;
  uint32_t tx_midi_bytes_ = 0;
  uint32_t rx_midi_bytes_ = 0;
  uint32_t error_count_ = 0;
  uint32_t fingerprint_error_count_ = 0;
  uint32_t length_error_count_ = 0;
  uint32_t crc_error_count_ = 0;
  uint32_t sequence_error_count_ = 0;
  uint32_t ws_sync_count_ = 0;
  uint32_t missed_ws_sync_count_ = 0;
  uint32_t p4_not_ready_count_ = 0;
  uint32_t dma_busy_count_ = 0;
  uint32_t dma_timeout_count_ = 0;
  uint32_t dma_unavailable_count_ = 0;
  uint32_t dropped_tx_bytes_ = 0;
  uint32_t dropped_rx_bytes_ = 0;
  uint32_t led_color_ = 0;
  uint32_t webui_update_counter_ = 0;
  uint8_t network_status_ = 0;
  uint8_t input_peak_byte_ = 0;
  uint8_t output_peak_byte_ = 0;
  uint8_t input_waveform_[64] = {};
  uint8_t output_waveform_[64] = {};
};

extern TbdP4RealtimeTransport tbd_p4_realtime;

#endif
