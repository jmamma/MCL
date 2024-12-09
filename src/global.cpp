#include "memory.h"
#include "MidiUart.h"

/// Definition of MCL global class instances with dependencies, to control the initialization order.

// -- Midi UART devices
// Define the buffer sizes as ring buffer lengths
// Sequencer MIDI buffers
MidiUartClass seq_tx1(uart1, TX_SEQBUF_SIZE);
MidiUartClass seq_tx2(uart1, TX_SEQBUF_SIZE);
MidiUartClass seq_tx3(uart2, TX_SEQBUF_SIZE);
MidiUartClass seq_tx4(uart2, TX_SEQBUF_SIZE);

// Main MIDI ports
MidiUartClass MidiUart(uart1, UART1_TX_BUFFER_LEN);
MidiUartClass MidiUart2(uart2, UART2_TX_BUFFER_LEN);
//MidiUartClass MidiUartUSB(uart0, UART0_TX_BUFFER_LEN);

