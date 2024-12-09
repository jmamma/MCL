#include "MidiUart.h"
#include "memory.h"

// Sequencer ring buffers
RingBuffer<TX_SEQBUF_SIZE> seq_tx1_rb;
RingBuffer<TX_SEQBUF_SIZE> seq_tx2_rb;
RingBuffer<TX_SEQBUF_SIZE> seq_tx3_rb;
RingBuffer<TX_SEQBUF_SIZE> seq_tx4_rb;

MidiUartClass seq_tx1(uart0, nullptr, &seq_tx1_rb);
MidiUartClass seq_tx2(uart0, nullptr, &seq_tx2_rb);
MidiUartClass seq_tx3(uart1, nullptr, &seq_tx3_rb);
MidiUartClass seq_tx4(uart1, nullptr, &seq_tx4_rb);

// UART ring buffers
RingBuffer<UART1_RX_BUFFER_LEN> uart1_rx_rb;
RingBuffer<UART1_TX_BUFFER_LEN> uart1_tx_rb;
RingBuffer<UART2_RX_BUFFER_LEN> uart2_rx_rb;
RingBuffer<UART2_TX_BUFFER_LEN> uart2_tx_rb;

MidiUartClass MidiUart(uart0, &uart1_rx_rb, &uart1_tx_rb);
MidiUartClass MidiUart2(uart1, &uart2_rx_rb, &uart2_tx_rb);

// Sysex ring buffers
RingBuffer<SYSEX1_DATA_LEN> uart1_sysex_rb;
RingBuffer<SYSEX2_DATA_LEN> uart2_sysex_rb;

MidiSysexClass MidiSysex(MidiUart, &uart1_sysex_rb);
MidiSysexClass MidiSysex2(MidiUart2, &uart2_sysex_rb);

MidiClass Midi(&MidiUart, &MidiSysex);
MidiClass Midi2(&MidiUart2, &MidiSysex2);

MidiIDSysexListenerClass MidiIDSysexListener;
