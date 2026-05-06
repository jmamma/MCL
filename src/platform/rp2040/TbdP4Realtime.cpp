#include "TbdP4Realtime.h"

#ifdef PLATFORM_TBD

#include "Arduino.h"
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>
#include <string.h>

namespace {

static constexpr uint32_t kSpiSpeed = 30000000;
static constexpr uint8_t kSpiSclk = 30;
static constexpr uint8_t kSpiMosi = 31;
static constexpr uint8_t kSpiMiso = 28;
static constexpr uint8_t kSpiCs = 29;
static constexpr uint8_t kP4ReadyPin = 22;

uint32_t now_ms() {
  return (uint32_t)(time_us_64() / 1000ULL);
}

} // namespace

TbdP4RealtimeTransport tbd_p4_realtime;

void TbdP4RealtimeTransport::init() {
  if (initialized_) return;

  memset(spi_trans_, 0, sizeof(spi_trans_));
  midi_tx_rd_ = midi_tx_wr_ = 0;
  midi_rx_rd_ = midi_rx_wr_ = 0;
  sending_trans_ = 0;
  receiving_trans_ = 0;
  spi_active_ = false;
  request_prepared_ = false;
  can_prepare_request_ = true;
  dma_reset_pending_ = false;
  next_request_sequence_ = 100;
  last_seen_response_ = 0;
  last_response_ms_ = 0;
  tx_frame_count_ = 0;
  tx_midi_bytes_ = 0;
  rx_midi_bytes_ = 0;
  fingerprint_error_count_ = 0;
  length_error_count_ = 0;
  crc_error_count_ = 0;
  sequence_error_count_ = 0;
  p4_not_ready_count_ = 0;
  dma_busy_count_ = 0;
  dma_timeout_count_ = 0;
  dma_unavailable_count_ = 0;

  gpio_init(kP4ReadyPin);
  gpio_set_dir(kP4ReadyPin, GPIO_IN);
  gpio_set_function(kSpiMiso, GPIO_FUNC_SPI);
  gpio_set_function(kSpiMosi, GPIO_FUNC_SPI);
  gpio_set_function(kSpiCs, GPIO_FUNC_SPI);
  gpio_set_function(kSpiSclk, GPIO_FUNC_SPI);
  spi_init(spi1, kSpiSpeed);
  spi_set_format(spi1, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

  dma_rx_channel_ = dma_claim_unused_channel(false);
  dma_tx_channel_ = dma_claim_unused_channel(false);
  if (!dma_ready()) {
    if (dma_rx_channel_ >= 0) {
      dma_channel_unclaim(dma_rx_channel_);
      dma_rx_channel_ = -1;
    }
    if (dma_tx_channel_ >= 0) {
      dma_channel_unclaim(dma_tx_channel_);
      dma_tx_channel_ = -1;
    }
    error_count_++;
    dma_unavailable_count_++;
  }

  initialized_ = true;
}

bool TbdP4RealtimeTransport::fifo_full(uint16_t wr, uint16_t rd) const {
  uint16_t next = wr + 1;
  if (next == kMidiFifoSize) next = 0;
  return next == rd;
}

bool TbdP4RealtimeTransport::can_enqueue_midi_byte() const {
  return !fifo_full(midi_tx_wr_, midi_tx_rd_);
}

bool TbdP4RealtimeTransport::enqueue_midi_byte_isr(uint8_t byte) {
  if (fifo_full(midi_tx_wr_, midi_tx_rd_)) {
    dropped_tx_bytes_++;
    return false;
  }
  midi_tx_fifo_[midi_tx_wr_] = byte;
  uint16_t next = midi_tx_wr_ + 1;
  if (next == kMidiFifoSize) next = 0;
  midi_tx_wr_ = next;
  return true;
}

bool TbdP4RealtimeTransport::pop_rx_midi_byte(uint8_t &byte) {
  LOCK();
  bool ok = pop_rx_midi_byte_isr(byte);
  CLEAR_LOCK();
  return ok;
}

bool TbdP4RealtimeTransport::pop_rx_midi_byte_isr(uint8_t &byte) {
  if (midi_rx_empty_isr()) return false;
  byte = midi_rx_fifo_[midi_rx_rd_];
  uint16_t next = midi_rx_rd_ + 1;
  if (next == kMidiFifoSize) next = 0;
  midi_rx_rd_ = next;
  return true;
}

bool TbdP4RealtimeTransport::pop_tx_midi_byte_locked(uint8_t &byte) {
  if (midi_tx_empty_isr()) return false;
  byte = midi_tx_fifo_[midi_tx_rd_];
  uint16_t next = midi_tx_rd_ + 1;
  if (next == kMidiFifoSize) next = 0;
  midi_tx_rd_ = next;
  return true;
}

bool TbdP4RealtimeTransport::enqueue_rx_midi_byte_locked(uint8_t byte) {
  if (fifo_full(midi_rx_wr_, midi_rx_rd_)) {
    dropped_rx_bytes_++;
    return false;
  }
  midi_rx_fifo_[midi_rx_wr_] = byte;
  uint16_t next = midi_rx_wr_ + 1;
  if (next == kMidiFifoSize) next = 0;
  midi_rx_wr_ = next;
  return true;
}

uint16_t TbdP4RealtimeTransport::calc_payload_crc(const uint8_t *data,
                                                  uint16_t length) const {
  uint16_t sum = 42;
  for (uint16_t i = 0; i < length; i++) {
    sum += data[i];
  }
  return sum;
}

uint8_t TbdP4RealtimeTransport::next_sequence(uint8_t current) const {
  return 100 + ((current + 1) % 100);
}

bool TbdP4RealtimeTransport::p4_ready() const {
  return gpio_get(kP4ReadyPin);
}

bool TbdP4RealtimeTransport::finish_transaction() {
  if (!spi_active_) return true;
  if (dma_reset_pending_) return false;
  if (!dma_ready()) {
    spi_active_ = false;
    can_prepare_request_ = true;
    request_prepared_ = false;
    return false;
  }

  if (dma_channel_is_busy(dma_rx_channel_) ||
      dma_channel_is_busy(dma_tx_channel_) ||
      (spi_get_hw(spi1)->sr & SPI_SSPSR_BSY_BITS)) {
    uint32_t now = now_ms();
    if (spi_deadline_ms_ != 0 && now > spi_deadline_ms_) {
      error_count_++;
      dma_timeout_count_++;
      dma_reset_pending_ = true;
      return false;
    }
    return false;
  }

  spi_get_hw(spi1)->dmacr = 0;
  dma_hw->intr = (1u << dma_rx_channel_) | (1u << dma_tx_channel_);
  spi_active_ = false;
  process_response();
  return true;
}

void TbdP4RealtimeTransport::abort_transaction_blocking() {
  if (!dma_ready()) return;

  spi_get_hw(spi1)->dmacr = 0;
  dma_channel_abort(dma_rx_channel_);
  dma_channel_abort(dma_tx_channel_);
  dma_hw->intr = (1u << dma_rx_channel_) | (1u << dma_tx_channel_);

  spi_active_ = false;
  dma_reset_pending_ = false;
  can_prepare_request_ = true;
  request_prepared_ = false;
}

void TbdP4RealtimeTransport::get_stats(TbdP4RealtimeStats &stats) const {
  stats.tx_frames = tx_frame_count_;
  stats.rx_frames = receive_count_;
  stats.tx_midi_bytes = tx_midi_bytes_;
  stats.rx_midi_bytes = rx_midi_bytes_;
  stats.dropped_tx_bytes = dropped_tx_bytes_;
  stats.dropped_rx_bytes = dropped_rx_bytes_;
  stats.error_count = error_count_;
  stats.fingerprint_errors = fingerprint_error_count_;
  stats.length_errors = length_error_count_;
  stats.crc_errors = crc_error_count_;
  stats.sequence_errors = sequence_error_count_;
  stats.p4_not_ready_count = p4_not_ready_count_;
  stats.dma_busy_count = dma_busy_count_;
  stats.dma_timeout_count = dma_timeout_count_;
  stats.dma_unavailable_count = dma_unavailable_count_;
  stats.last_spi_send_ms = last_spi_send_ms_;
  stats.last_response_ms = last_response_ms_;
  stats.last_response_sequence = last_seen_response_;
  stats.p4_alive = p4_alive_;
  stats.p4_ready_pin = p4_ready();
  stats.spi_active = spi_active_;
  stats.dma_ready = dma_ready();
  stats.dma_reset_pending = dma_reset_pending_;
}

void TbdP4RealtimeTransport::recover_blocking() {
  if (!initialized_) return;
  if (dma_reset_pending_) {
    abort_transaction_blocking();
  }
}

void TbdP4RealtimeTransport::prepare_request() {
  if (!can_prepare_request_ || request_prepared_) return;

  uint8_t *frame = spi_trans_[sending_trans_].out_buf;
  memset(frame, 0, TBD_P4_SPI_FRAME_SIZE);
  frame[0] = 0xCA;
  frame[1] = 0xFE;

  auto *header = reinterpret_cast<TbdP4SpiRequestHeader *>(frame + 2);
  auto *req = reinterpret_cast<TbdP4SpiRequest *>(frame + 2 + TBD_P4_SPI_HEADER_SIZE);

  uint16_t written = 0;
  LOCK();
  while (written < TBD_P4_SPI_MIDI_DATA_SIZE) {
    uint8_t byte = 0;
    if (!pop_tx_midi_byte_locked(byte)) break;
    req->synth_midi[written++] = byte;
  }
  CLEAR_LOCK();

  req->magic = 0xFEEDC0DE;
  req->synth_midi_length = written;
  req->sequencer_tempo = sequencer_tempo_;
  req->sequencer_active_track = sequencer_active_track_;
  req->magic2 = 0xDEADC0DE;

  header->magic = 0xCAFE;
  header->request_sequence_counter = next_request_sequence_;
  header->payload_length = sizeof(TbdP4SpiRequest);
  header->payload_crc =
      calc_payload_crc(reinterpret_cast<uint8_t *>(req), header->payload_length);

  tx_midi_bytes_ += written;
  can_prepare_request_ = false;
  request_prepared_ = true;
}

void TbdP4RealtimeTransport::start_transaction() {
  if (!request_prepared_) return;
  if (spi_active_) {
    dma_busy_count_++;
    return;
  }
  if (!dma_ready()) {
    error_count_++;
    dma_unavailable_count_++;
    return;
  }
  if (dma_reset_pending_) return;
  if (!p4_ready()) {
    p4_not_ready_count_++;
    return;
  }

  uint8_t *out = spi_trans_[sending_trans_].out_buf;
  uint8_t *in = spi_trans_[sending_trans_].in_buf;
  in[0] = 0;
  in[1] = 0;

  dma_channel_config tx_config = dma_channel_get_default_config(dma_tx_channel_);
  channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
  channel_config_set_read_increment(&tx_config, true);
  channel_config_set_write_increment(&tx_config, false);
  channel_config_set_dreq(&tx_config, spi_get_dreq(spi1, true));
  channel_config_set_chain_to(&tx_config, dma_tx_channel_);
  channel_config_set_irq_quiet(&tx_config, true);
  dma_channel_configure(dma_tx_channel_, &tx_config, &spi_get_hw(spi1)->dr,
                        out, TBD_P4_SPI_FRAME_SIZE, false);

  dma_channel_config rx_config = dma_channel_get_default_config(dma_rx_channel_);
  channel_config_set_transfer_data_size(&rx_config, DMA_SIZE_8);
  channel_config_set_read_increment(&rx_config, false);
  channel_config_set_write_increment(&rx_config, true);
  channel_config_set_dreq(&rx_config, spi_get_dreq(spi1, false));
  channel_config_set_chain_to(&rx_config, dma_rx_channel_);
  channel_config_set_irq_quiet(&rx_config, true);
  dma_channel_configure(dma_rx_channel_, &rx_config, in, &spi_get_hw(spi1)->dr,
                        TBD_P4_SPI_FRAME_SIZE, false);

  spi_get_hw(spi1)->dmacr = SPI_SSPDMACR_TXDMAE_BITS | SPI_SSPDMACR_RXDMAE_BITS;
  dma_start_channel_mask((1u << dma_rx_channel_) | (1u << dma_tx_channel_));

  receiving_trans_ = sending_trans_;
  sending_trans_ ^= 1;
  request_prepared_ = false;
  spi_active_ = true;
  tx_frame_count_++;
  last_spi_send_ms_ = now_ms();
  spi_deadline_ms_ = last_spi_send_ms_ + kSpiDeadlineMs;
  next_request_sequence_ = next_sequence(next_request_sequence_);
}

void TbdP4RealtimeTransport::process_response() {
  uint8_t *frame = spi_trans_[receiving_trans_].in_buf;
  if (frame[0] != 0xCA || frame[1] != 0xFE) {
    error_count_++;
    fingerprint_error_count_++;
    can_prepare_request_ = true;
    return;
  }

  auto *header = reinterpret_cast<TbdP4SpiResponseHeader *>(frame + 2);
  if (header->magic != 0xCAFE) {
    error_count_++;
    fingerprint_error_count_++;
    can_prepare_request_ = true;
    return;
  }
  if (header->payload_length != sizeof(TbdP4SpiResponse)) {
    error_count_++;
    length_error_count_++;
    can_prepare_request_ = true;
    return;
  }

  auto *response =
      reinterpret_cast<TbdP4SpiResponse *>(frame + 2 + TBD_P4_SPI_HEADER_SIZE);
  uint16_t crc =
      calc_payload_crc(reinterpret_cast<uint8_t *>(response), header->payload_length);
  if (crc != header->payload_crc) {
    error_count_++;
    crc_error_count_++;
    can_prepare_request_ = true;
    return;
  }
  if (response->magic != 0xFEEDC0DE || response->magic2 != 0xDEADC0DE) {
    error_count_++;
    fingerprint_error_count_++;
    can_prepare_request_ = true;
    return;
  }

  receive_count_++;
  uint8_t expected_sequence = next_sequence(last_seen_response_);
  if (header->response_sequence_counter != expected_sequence) {
    sequence_error_count_++;
  }
  last_seen_response_ = header->response_sequence_counter;
  last_response_ms_ = now_ms();
  led_color_ = response->led_color;
  webui_update_counter_ = response->webui_update_counter;
  network_status_ = response->network_status;
  input_peak_byte_ = response->input_peak_byte;
  output_peak_byte_ = response->output_peak_byte;
  memcpy(input_waveform_, response->input_waveform, sizeof(input_waveform_));
  memcpy(output_waveform_, response->output_waveform, sizeof(output_waveform_));

  uint32_t midi_len = response->usb_device_midi_length;
  if (midi_len > TBD_P4_SPI_USB_MIDI_DATA_SIZE) {
    midi_len = TBD_P4_SPI_USB_MIDI_DATA_SIZE;
    error_count_++;
    length_error_count_++;
  }

  LOCK();
  for (uint32_t i = 0; i < midi_len; i++) {
    enqueue_rx_midi_byte_locked(response->usb_device_midi[i]);
  }
  CLEAR_LOCK();
  rx_midi_bytes_ += midi_len;

  can_prepare_request_ = true;
}

void TbdP4RealtimeTransport::poll() {
  if (!initialized_) init();

  uint32_t now = now_ms();
  p4_alive_ = (last_response_ms_ != 0 && (now - last_response_ms_) <= 300);

  finish_transaction();
  prepare_request();

  if (now >= next_spi_send_ms_) {
    next_spi_send_ms_ = now + 1;
    start_transaction();
  }
}

#endif
