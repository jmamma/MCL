#include "TbdP4Realtime.h"

#ifdef PLATFORM_TBD

#include "Arduino.h"
#include "DaDa_SPI.h"
#include "../../mcl/Midi/midi-common.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/sync.h>
#include <new>
#include <pico/time.h>
#include <string.h>

namespace {

static constexpr uint32_t kSpiSpeed = 30000000;
static constexpr uint8_t kSpiSclk = 30;
static constexpr uint8_t kSpiMosi = 31;
static constexpr uint8_t kSpiMiso = 28;
static constexpr uint8_t kSpiCs = 29;
static constexpr uint8_t kWsPin = 27;
static constexpr uint8_t kWsDivider = 32;
static constexpr uint8_t kP4ReadyPin = 22;

volatile uint32_t ws_block_count = 0;
volatile uint8_t ws_edge_count = 0;

alignas(DaDa_SPI) uint8_t realtime_spi_storage[sizeof(DaDa_SPI)];
DaDa_SPI *realtime_spi = nullptr;

uint32_t now_ms() {
  return (uint32_t)(time_us_64() / 1000ULL);
}

uint8_t midi_message_length(uint8_t status) {
  if (status >= 0xF8) return 1;
  if (status < 0xF0) {
    switch (status & 0xF0) {
    case MIDI_PROGRAM_CHANGE:
    case MIDI_CHANNEL_PRESSURE:
      return 2;
    case MIDI_NOTE_OFF:
    case MIDI_NOTE_ON:
    case MIDI_AFTER_TOUCH:
    case MIDI_CONTROL_CHANGE:
    case MIDI_PITCH_WHEEL:
      return 3;
    default:
      return 0;
    }
  }

  switch (status) {
  case MIDI_MTC_QUARTER_FRAME:
  case MIDI_SONG_SELECT:
    return 2;
  case MIDI_SONG_POSITION_PTR:
    return 3;
  case MIDI_TUNE_REQUEST:
    return 1;
  default:
    return 0;
  }
}

void __not_in_flash_func(tbd_ws_sync_cb)(uint gpio, uint32_t events) {
  if (gpio != kWsPin || !(events & GPIO_IRQ_EDGE_FALL)) return;

  uint8_t edge_count = ws_edge_count + 1;
  if (edge_count >= kWsDivider) {
    ws_edge_count = 0;
    ws_block_count++;
  } else {
    ws_edge_count = edge_count;
  }
}

void reset_ws_sync() {
  uint32_t saved_irq = save_and_disable_interrupts();
  ws_block_count = 0;
  ws_edge_count = 0;
  restore_interrupts(saved_irq);
}

uint32_t take_ws_blocks() {
  uint32_t saved_irq = save_and_disable_interrupts();
  uint32_t count = ws_block_count;
  ws_block_count = 0;
  restore_interrupts(saved_irq);
  return count;
}

DaDa_SPI &realtime_spi_instance() {
  if (realtime_spi == nullptr) {
    realtime_spi = new (realtime_spi_storage)
        DaDa_SPI(spi1, kSpiCs, kSpiMosi, kSpiMiso, kSpiSclk, kP4ReadyPin,
                 kSpiSpeed);
  }
  return *realtime_spi;
}

} // namespace

TbdP4RealtimeTransport tbd_p4_realtime;

void TbdP4RealtimeTransport::init() {
  if (initialized_) return;

  memset(spi_trans_, 0, sizeof(spi_trans_));
  memset(midi_tx_messages_, 0, sizeof(midi_tx_messages_));
  midi_tx_msg_rd_ = midi_tx_msg_wr_ = 0;
  midi_rx_rd_ = midi_rx_wr_ = 0;
  memset(tx_build_data_, 0, sizeof(tx_build_data_));
  tx_build_length_ = 0;
  tx_build_needed_ = 0;
  tx_running_status_ = 0;
  tx_in_sysex_ = false;
  sending_trans_ = 0;
  receiving_trans_ = 0;
  spi_active_ = false;
  request_prepared_ = false;
  can_prepare_request_ = true;
  dma_reset_pending_ = false;
  spi_ready_ = false;
  next_spi_send_ms_ = 0;
  next_request_sequence_ = 100;
  last_seen_response_ = 0;
  last_response_ms_ = 0;
  last_ws_sync_ms_ = 0;
  receive_count_ = 0;
  error_count_ = 0;
  tx_frame_count_ = 0;
  queued_tx_midi_bytes_ = 0;
  tx_midi_bytes_ = 0;
  rx_midi_bytes_ = 0;
  tx_note_on_messages_ = 0;
  tx_note_off_messages_ = 0;
  tx_cc_messages_ = 0;
  tx_realtime_messages_ = 0;
  last_tx_status_ = 0;
  last_tx_data1_ = 0;
  last_tx_data2_ = 0;
  fingerprint_error_count_ = 0;
  length_error_count_ = 0;
  crc_error_count_ = 0;
  sequence_error_count_ = 0;
  ws_sync_count_ = 0;
  missed_ws_sync_count_ = 0;
  p4_not_ready_count_ = 0;
  dma_busy_count_ = 0;
  dma_timeout_count_ = 0;
  dma_unavailable_count_ = 0;
  poll_count_ = 0;
  prepare_count_ = 0;
  start_attempt_count_ = 0;
  start_no_request_count_ = 0;
  dropped_tx_bytes_ = 0;
  dropped_rx_bytes_ = 0;
  led_color_ = 0;
  webui_update_counter_ = 0;
  network_status_ = 0;
  input_peak_byte_ = 0;
  output_peak_byte_ = 0;
  extended_response_seen_ = false;
  memset(input_waveform_, 0, sizeof(input_waveform_));
  memset(output_waveform_, 0, sizeof(output_waveform_));

  reset_ws_sync();
  gpio_init(kWsPin);
  gpio_set_dir(kWsPin, GPIO_IN);
  gpio_pull_down(kWsPin);
  gpio_set_irq_enabled_with_callback(kWsPin, GPIO_IRQ_EDGE_FALL, true,
                                     &tbd_ws_sync_cb);

  realtime_spi_instance();
  spi_ready_ = true;

  initialized_ = true;
}

bool TbdP4RealtimeTransport::midi_rx_fifo_full(uint16_t wr, uint16_t rd) const {
  uint16_t next = wr + 1;
  if (next == kMidiByteFifoSize) next = 0;
  return next == rd;
}

bool TbdP4RealtimeTransport::midi_tx_message_fifo_full(uint16_t wr,
                                                       uint16_t rd) const {
  uint16_t next = wr + 1;
  if (next == kMidiTxMessageFifoSize) next = 0;
  return next == rd;
}

bool TbdP4RealtimeTransport::can_enqueue_midi_byte() const {
  return !midi_tx_message_fifo_full(midi_tx_msg_wr_, midi_tx_msg_rd_);
}

bool TbdP4RealtimeTransport::enqueue_midi_byte_isr(uint8_t byte) {
  if (byte >= 0xF8) {
    return enqueue_tx_midi_message_locked(&byte, 1);
  }

  if (tx_in_sysex_) {
    if (byte == MIDI_SYSEX_END) {
      tx_in_sysex_ = false;
    }
    return true;
  }

  if (byte & 0x80) {
    tx_build_length_ = 0;
    tx_build_needed_ = 0;

    if (byte == MIDI_SYSEX_START) {
      tx_in_sysex_ = true;
      tx_running_status_ = 0;
      return true;
    }

    const uint8_t length = midi_message_length(byte);
    if (length == 0) {
      if (byte >= 0xF0) {
        tx_running_status_ = 0;
      }
      return true;
    }

    tx_build_data_[0] = byte;
    tx_build_length_ = 1;
    tx_build_needed_ = length;
    if (byte < 0xF0) {
      tx_running_status_ = byte;
    } else {
      tx_running_status_ = 0;
    }

    if (tx_build_length_ >= tx_build_needed_) {
      const bool ok =
          enqueue_tx_midi_message_locked(tx_build_data_, tx_build_length_);
      tx_build_length_ = 0;
      tx_build_needed_ = 0;
      return ok;
    }
    return true;
  }

  if (tx_build_length_ == 0) {
    if (tx_running_status_ == 0) {
      dropped_tx_bytes_++;
      return true;
    }

    tx_build_data_[0] = tx_running_status_;
    tx_build_data_[1] = byte;
    tx_build_length_ = 2;
    tx_build_needed_ = midi_message_length(tx_running_status_);
  } else if (tx_build_length_ < kMidiTxMessageMaxBytes) {
    tx_build_data_[tx_build_length_++] = byte;
  } else {
    tx_build_length_ = 0;
    tx_build_needed_ = 0;
    dropped_tx_bytes_++;
    return true;
  }

  if (tx_build_needed_ != 0 && tx_build_length_ >= tx_build_needed_) {
    const bool ok =
        enqueue_tx_midi_message_locked(tx_build_data_, tx_build_length_);
    tx_build_length_ = 0;
    tx_build_needed_ = 0;
    return ok;
  }

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
  if (next == kMidiByteFifoSize) next = 0;
  midi_rx_rd_ = next;
  return true;
}

bool TbdP4RealtimeTransport::enqueue_tx_midi_message_locked(const uint8_t *data,
                                                            uint8_t length) {
  if (data == nullptr || length == 0 || length > kMidiTxMessageMaxBytes) {
    return true;
  }
  if (midi_tx_message_fifo_full(midi_tx_msg_wr_, midi_tx_msg_rd_)) {
    dropped_tx_bytes_ += length;
    return false;
  }

  MidiTxMessage &msg = midi_tx_messages_[midi_tx_msg_wr_];
  memset(msg.data, 0, sizeof(msg.data));
  memcpy(msg.data, data, length);
  msg.length = length;

  uint16_t next = midi_tx_msg_wr_ + 1;
  if (next == kMidiTxMessageFifoSize) next = 0;
  midi_tx_msg_wr_ = next;
  queued_tx_midi_bytes_ += length;
  return true;
}

bool TbdP4RealtimeTransport::peek_tx_midi_message_locked(
    MidiTxMessage &msg) const {
  if (midi_tx_empty_isr()) return false;
  msg = midi_tx_messages_[midi_tx_msg_rd_];
  return true;
}

bool TbdP4RealtimeTransport::pop_tx_midi_message_locked(MidiTxMessage &msg) {
  if (!peek_tx_midi_message_locked(msg)) return false;
  uint16_t next = midi_tx_msg_rd_ + 1;
  if (next == kMidiTxMessageFifoSize) next = 0;
  midi_tx_msg_rd_ = next;
  return true;
}

bool TbdP4RealtimeTransport::enqueue_rx_midi_byte_locked(uint8_t byte) {
  if (midi_rx_fifo_full(midi_rx_wr_, midi_rx_rd_)) {
    dropped_rx_bytes_++;
    return false;
  }
  midi_rx_fifo_[midi_rx_wr_] = byte;
  uint16_t next = midi_rx_wr_ + 1;
  if (next == kMidiByteFifoSize) next = 0;
  midi_rx_wr_ = next;
  return true;
}

void TbdP4RealtimeTransport::observe_tx_midi_message(
    const MidiTxMessage &msg) {
  if (msg.length == 0) return;

  const uint8_t status = msg.data[0];
  if (status >= 0xF8) {
    tx_realtime_messages_++;
    last_tx_status_ = status;
    last_tx_data1_ = 0;
    last_tx_data2_ = 0;
    return;
  }

  const uint8_t status_class = status & 0xF0;
  if (status_class == MIDI_NOTE_ON) {
    if (msg.length > 2 && msg.data[2] == 0) {
      tx_note_off_messages_++;
    } else {
      tx_note_on_messages_++;
    }
  } else if (status_class == MIDI_NOTE_OFF) {
    tx_note_off_messages_++;
  } else if (status_class == MIDI_CONTROL_CHANGE) {
    tx_cc_messages_++;
  }

  last_tx_status_ = status;
  last_tx_data1_ = msg.length > 1 ? msg.data[1] : 0;
  last_tx_data2_ = msg.length > 2 ? msg.data[2] : 0;
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
  return realtime_spi_instance().GetP4Ready();
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

  DaDa_SPI &spi = realtime_spi_instance();
  if (spi.IsBusy()) {
    uint32_t now = now_ms();
    if (spi_deadline_ms_ != 0 && now > spi_deadline_ms_) {
      error_count_++;
      dma_timeout_count_++;
      spi_active_ = false;
      can_prepare_request_ = true;
      request_prepared_ = false;
      last_seen_response_ = 0;
      return false;
    }
    return false;
  }

  spi_active_ = false;
  process_response();
  return true;
}

void TbdP4RealtimeTransport::abort_transaction_blocking() {
  spi_active_ = false;
  dma_reset_pending_ = false;
  can_prepare_request_ = true;
  request_prepared_ = false;
}

void TbdP4RealtimeTransport::get_stats(TbdP4RealtimeStats &stats) const {
  stats.tx_frames = tx_frame_count_;
  stats.rx_frames = receive_count_;
  stats.queued_tx_midi_bytes = queued_tx_midi_bytes_;
  stats.tx_midi_bytes = tx_midi_bytes_;
  stats.rx_midi_bytes = rx_midi_bytes_;
  stats.tx_note_on_messages = tx_note_on_messages_;
  stats.tx_note_off_messages = tx_note_off_messages_;
  stats.tx_cc_messages = tx_cc_messages_;
  stats.tx_realtime_messages = tx_realtime_messages_;
  stats.last_tx_status = last_tx_status_;
  stats.last_tx_data1 = last_tx_data1_;
  stats.last_tx_data2 = last_tx_data2_;
  stats.dropped_tx_bytes = dropped_tx_bytes_;
  stats.dropped_rx_bytes = dropped_rx_bytes_;
  stats.error_count = error_count_;
  stats.fingerprint_errors = fingerprint_error_count_;
  stats.length_errors = length_error_count_;
  stats.crc_errors = crc_error_count_;
  stats.sequence_errors = sequence_error_count_;
  stats.ws_sync_count = ws_sync_count_;
  stats.missed_ws_sync_count = missed_ws_sync_count_;
  stats.p4_not_ready_count = p4_not_ready_count_;
  stats.dma_busy_count = dma_busy_count_;
  stats.dma_timeout_count = dma_timeout_count_;
  stats.dma_unavailable_count = dma_unavailable_count_;
  stats.poll_count = poll_count_;
  stats.prepare_count = prepare_count_;
  stats.start_attempt_count = start_attempt_count_;
  stats.start_no_request_count = start_no_request_count_;
  stats.last_spi_send_ms = last_spi_send_ms_;
  stats.last_response_ms = last_response_ms_;
  stats.last_response_sequence = last_seen_response_;
  stats.input_peak_byte = input_peak_byte_;
  stats.output_peak_byte = output_peak_byte_;
  stats.extended_response_seen = extended_response_seen_;
  stats.p4_alive = p4_alive_;
  stats.p4_sync_seen = last_ws_sync_ms_ != 0 &&
                       (now_ms() - last_ws_sync_ms_) <= 300;
  stats.p4_ready_pin = p4_ready();
  stats.spi_active = spi_active_;
  stats.dma_ready = dma_ready();
  stats.dma_reset_pending = dma_reset_pending_;
  stats.request_prepared = request_prepared_;
  stats.can_prepare_request = can_prepare_request_;
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
  MidiTxMessage msg;
  while (peek_tx_midi_message_locked(msg)) {
    if (written + msg.length >= TBD_P4_SPI_MIDI_DATA_SIZE) {
      break;
    }
    (void)pop_tx_midi_message_locked(msg);
    memcpy(req->synth_midi + written, msg.data, msg.length);
    written += msg.length;
    observe_tx_midi_message(msg);
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
  prepare_count_++;
}

bool TbdP4RealtimeTransport::start_transaction() {
  start_attempt_count_++;
  if (spi_active_) {
    dma_busy_count_++;
    return false;
  }
  if (!request_prepared_) {
    start_no_request_count_++;
    return false;
  }
  if (!dma_ready()) {
    error_count_++;
    dma_unavailable_count_++;
    return false;
  }
  if (dma_reset_pending_) return false;
  if (!p4_ready()) {
    p4_not_ready_count_++;
    return false;
  }
  DaDa_SPI &spi = realtime_spi_instance();
  if (spi.IsBusy()) {
    dma_busy_count_++;
    return false;
  }

  uint8_t *out = spi_trans_[sending_trans_].out_buf;
  uint8_t *in = spi_trans_[sending_trans_].in_buf;
  in[0] = 0;
  in[1] = 0;
  spi.StartDMA(out, in, TBD_P4_SPI_FRAME_SIZE);

  receiving_trans_ = sending_trans_;
  sending_trans_ ^= 1;
  request_prepared_ = false;
  spi_active_ = true;
  tx_frame_count_++;
  last_spi_send_ms_ = now_ms();
  spi_deadline_ms_ = last_spi_send_ms_ + kSpiDeadlineMs;
  next_request_sequence_ = next_sequence(next_request_sequence_);
  return true;
}

bool TbdP4RealtimeTransport::enter_poll() {
  uint32_t saved_irq = save_and_disable_interrupts();
  if (poll_active_) {
    restore_interrupts(saved_irq);
    return false;
  }
  poll_active_ = true;
  restore_interrupts(saved_irq);
  return true;
}

void TbdP4RealtimeTransport::leave_poll() {
  uint32_t saved_irq = save_and_disable_interrupts();
  poll_active_ = false;
  restore_interrupts(saved_irq);
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
  bool legacy_response =
      header->payload_length == TBD_P4_SPI_RESPONSE_LEGACY_SIZE;
  bool extended_response = header->payload_length == sizeof(TbdP4SpiResponse);
  if (!legacy_response && !extended_response) {
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
  receive_count_++;
  uint8_t expected_sequence = next_sequence(last_seen_response_);
  if (header->response_sequence_counter != expected_sequence) {
    sequence_error_count_++;
  }
  last_seen_response_ = header->response_sequence_counter;
  last_response_ms_ = now_ms();
  led_color_ = response->led_color;
  webui_update_counter_ = response->webui_update_counter;
  network_status_ = extended_response ? response->network_status : 0;
  input_peak_byte_ = extended_response ? response->input_peak_byte : 0;
  output_peak_byte_ = extended_response ? response->output_peak_byte : 0;
  extended_response_seen_ = extended_response;
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
  if (!enter_poll()) return;
  poll_count_++;

  uint32_t now = now_ms();
  uint32_t ws_blocks = take_ws_blocks();
  if (ws_blocks > 0) {
    ws_sync_count_ += ws_blocks;
    if (ws_blocks > 1) {
      missed_ws_sync_count_ += ws_blocks - 1;
    }
    last_ws_sync_ms_ = now;
  }

  p4_alive_ = (last_ws_sync_ms_ != 0 && (now - last_ws_sync_ms_) <= 300);

  finish_transaction();
  prepare_request();

  if (now < next_spi_send_ms_) {
    leave_poll();
    return;
  }
  if (start_transaction()) {
    next_spi_send_ms_ = now + 1;
  }
  leave_poll();
}

#endif
