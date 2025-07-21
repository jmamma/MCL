#include "Arduino.h"
#include "MidiUart.h"
#include "MidiSysex.h"
#include "MidiIDSysex.h"
#include "Midi.h"
#include "MidiSetup.h"
#include "memory.h"
#include "oled.h"
#include "GUI.h"
#include "MD.h"
#include "MNM.h"
#include "A4.h"
#include "Elektron.h"
#include "MidiIDSysex.h"

// Buffer array definitions

uint8_t (*seq_tx1_buf)[TX_SEQBUF_SIZE] = (uint8_t(*)[TX_SEQBUF_SIZE])BANK1_UARTSEQ_TX1_BUFFER_START;
uint8_t (*seq_tx2_buf)[TX_SEQBUF_SIZE] = (uint8_t(*)[TX_SEQBUF_SIZE])BANK1_UARTSEQ_TX2_BUFFER_START;
uint8_t (*seq_tx3_buf)[TX_SEQBUF_SIZE] = (uint8_t(*)[TX_SEQBUF_SIZE])BANK1_UARTSEQ_TX3_BUFFER_START;
uint8_t (*seq_tx4_buf)[TX_SEQBUF_SIZE] = (uint8_t(*)[TX_SEQBUF_SIZE])BANK1_UARTSEQ_TX4_BUFFER_START;

// UART1 buffer pointers
uint8_t (*uart1_rx_buf)[UART1_RX_BUFFER_LEN] = (uint8_t(*)[UART1_RX_BUFFER_LEN])BANK1_UART1_RX_BUFFER_START;
uint8_t (*uart1_tx_buf)[UART1_TX_BUFFER_LEN] = (uint8_t(*)[UART1_TX_BUFFER_LEN])BANK1_UART1_TX_BUFFER_START;
uint8_t (*uart1_sysex_buf)[SYSEX1_DATA_LEN] = (uint8_t(*)[SYSEX1_DATA_LEN])BANK1_SYSEX1_DATA_START;

// UART2 buffer pointers
uint8_t (*uart2_rx_buf)[UART2_RX_BUFFER_LEN] = (uint8_t(*)[UART2_RX_BUFFER_LEN])BANK1_UART2_RX_BUFFER_START;
uint8_t (*uart2_tx_buf)[UART2_TX_BUFFER_LEN] = (uint8_t(*)[UART2_TX_BUFFER_LEN])BANK1_UART2_TX_BUFFER_START;
uint8_t (*uart2_sysex_buf)[SYSEX2_DATA_LEN] = (uint8_t(*)[SYSEX2_DATA_LEN])BANK1_SYSEX2_DATA_START;

// UART USB buffer pointers
uint8_t (*uartusb_rx_buf)[UART0_RX_BUFFER_LEN] = (uint8_t(*)[UART0_RX_BUFFER_LEN])BANK1_UART0_RX_BUFFER_START;
uint8_t (*uartusb_tx_buf)[UART0_TX_BUFFER_LEN] = (uint8_t(*)[UART0_TX_BUFFER_LEN])BANK1_UART0_TX_BUFFER_START;
uint8_t (*uartusb_sysex_buf)[SYSEX3_DATA_LEN] = (uint8_t(*)[SYSEX3_DATA_LEN])BANK1_SYSEX3_DATA_START;

// Sequencer ring buffers
RingBuffer seq_tx1_rb(*seq_tx1_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx2_rb(*seq_tx2_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx3_rb(*seq_tx3_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx4_rb(*seq_tx4_buf, TX_SEQBUF_SIZE);

// UART ring buffers
RingBuffer uart1_rx_rb(*uart1_rx_buf, UART1_RX_BUFFER_LEN);
RingBuffer uart1_tx_rb(*uart1_tx_buf, UART1_TX_BUFFER_LEN);
RingBuffer uart1_sysex_rb(*uart1_sysex_buf, SYSEX1_DATA_LEN);

RingBuffer uart2_rx_rb(*uart2_rx_buf, UART2_RX_BUFFER_LEN);
RingBuffer uart2_tx_rb(*uart2_tx_buf, UART2_TX_BUFFER_LEN);
RingBuffer uart2_sysex_rb(*uart2_sysex_buf, SYSEX2_DATA_LEN);

RingBuffer uartusb_rx_rb(*uartusb_rx_buf, UART0_RX_BUFFER_LEN);
RingBuffer uartusb_tx_rb(*uartusb_tx_buf, UART0_TX_BUFFER_LEN);
RingBuffer uartusb_sysex_rb(*uartusb_sysex_buf, SYSEX3_DATA_LEN);

// MIDI UART instances
MidiUartClass seq_tx1(nullptr, nullptr, &seq_tx1_rb);
MidiUartClass seq_tx2(nullptr, nullptr, &seq_tx2_rb);
MidiUartClass seq_tx3(nullptr, nullptr, &seq_tx3_rb);
MidiUartClass seq_tx4(nullptr, nullptr, &seq_tx4_rb);

MidiUartClass MidiUart(&UDR1, &uart1_rx_rb, &uart1_tx_rb);
MidiUartClass MidiUart2(&UDR2, &uart2_rx_rb, &uart2_tx_rb);
MidiUartClass MidiUartUSB(&UDR0, &uartusb_rx_rb, &uartusb_tx_rb);

// Sysex instances
MidiSysexClass MidiSysex(&MidiUart, &uart1_sysex_rb);
MidiSysexClass MidiSysex2(&MidiUart2, &uart2_sysex_rb);
MidiSysexClass MidiSysexUSB(&MidiUartUSB, &uartusb_sysex_rb);

// MIDI class instances
MidiClass Midi(&MidiUart, &MidiSysex);
MidiClass Midi2(&MidiUart2, &MidiSysex2);
MidiClass MidiUSB(&MidiUartUSB, &MidiSysexUSB);

MidiIDSysexListenerClass MidiIDSysexListener;

// Global Variables

volatile uint8_t MidiUartParent::handle_midi_lock = 0;
volatile uint16_t g_clock_fast = 0;
volatile uint16_t g_clock_ms = 0;
volatile uint16_t g_clock_ticks = 0;
volatile uint16_t g_clock_minutes = 0;

volatile uint16_t g_clock_fps = 0;
volatile uint16_t g_fps = 0;

volatile uint8_t *rand_ptr = nullptr;

// GUI object
GuiClass GUI;

// -- Sysex listeners
MDSysexListenerClass MDSysexListener;
MNMSysexListenerClass MNMSysexListener;
A4SysexListenerClass A4SysexListener;

// -- Device drivers
MDClass MD;
MNMClass MNM;
A4Class Analog4;
GenericMidiDevice generic_midi_device;
NullMidiDevice null_midi_device;

// -- Device manager
MidiActivePeering midi_active_peering;
MidiSetup midi_setup;
//Oled Display

Adafruit_SSD1305 oled_display(OLED_DC, OLED_RESET, OLED_CS);

SdFat_ SD __attribute__((section(".sdcard")));

void handleIncomingMidi() {
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;

  Midi.processSysex();
  Midi2.processSysex();
  MidiUSB.processSysex();
  Midi.processMidi();
  Midi2.processMidi();
  MidiUSB.processMidi();

  MidiUartParent::handle_midi_lock = 0;
}

