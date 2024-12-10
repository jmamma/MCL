#include "MidiUart.h"
#include "MidiSysex.h"
#include "MidiIDSysex.h"
#include "Midi.h"
#include "memory.h"

// Buffer array definitions
uint8_t seq_tx1_buf[TX_SEQBUF_SIZE];
uint8_t seq_tx2_buf[TX_SEQBUF_SIZE];
uint8_t seq_tx3_buf[TX_SEQBUF_SIZE];
uint8_t seq_tx4_buf[TX_SEQBUF_SIZE];

uint8_t uart1_rx_buf[UART1_RX_BUFFER_LEN];
uint8_t uart1_tx_buf[UART1_TX_BUFFER_LEN];
uint8_t uart2_rx_buf[UART2_RX_BUFFER_LEN];
uint8_t uart2_tx_buf[UART2_TX_BUFFER_LEN];

uint8_t uart1_sysex_buf[SYSEX1_DATA_LEN];
uint8_t uart2_sysex_buf[SYSEX2_DATA_LEN];

// Sequencer ring buffers
RingBuffer seq_tx1_rb(seq_tx1_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx2_rb(seq_tx2_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx3_rb(seq_tx3_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx4_rb(seq_tx4_buf, TX_SEQBUF_SIZE);

// UART ring buffers
RingBuffer uart1_rx_rb(uart1_rx_buf, UART1_RX_BUFFER_LEN);
RingBuffer uart1_tx_rb(uart1_tx_buf, UART1_TX_BUFFER_LEN);
RingBuffer uart2_rx_rb(uart2_rx_buf, UART2_RX_BUFFER_LEN);
RingBuffer uart2_tx_rb(uart2_tx_buf, UART2_TX_BUFFER_LEN);

// Sysex ring buffers
RingBuffer uart1_sysex_rb(uart1_sysex_buf, SYSEX1_DATA_LEN);
RingBuffer uart2_sysex_rb(uart2_sysex_buf, SYSEX2_DATA_LEN);

// MIDI UART instances
MidiUartClass seq_tx1(uart0, nullptr, &seq_tx1_rb);
MidiUartClass seq_tx2(uart0, nullptr, &seq_tx2_rb);
MidiUartClass seq_tx3(uart1, nullptr, &seq_tx3_rb);
MidiUartClass seq_tx4(uart1, nullptr, &seq_tx4_rb);

MidiUartClass MidiUart(uart0, &uart1_rx_rb, &uart1_tx_rb);
MidiUartClass MidiUart2(uart1, &uart2_rx_rb, &uart2_tx_rb);

// Sysex instances
MidiSysexClass MidiSysex(&MidiUart, &uart1_sysex_rb);
MidiSysexClass MidiSysex2(&MidiUart2, &uart2_sysex_rb);

// MIDI class instances
MidiClass Midi(&MidiUart, &MidiSysex);
MidiClass Midi2(&MidiUart2, &MidiSysex2);

MidiIDSysexListenerClass MidiIDSysexListener;

void handleIncomingMidi() {
  Midi.processSysex();
  Midi2.processSysex();
  Midi.processMidi();
  Midi2.processMidi();
}
