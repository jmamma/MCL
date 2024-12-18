#include "MidiUart.h"
#include "MidiSysex.h"
#include "MidiIDSysex.h"
#include "Midi.h"
#include "memory.h"
#include "oled.h"
#include "GUI.h"

// Buffer array definitions
uint8_t seq_tx1_buf[TX_SEQBUF_SIZE];
uint8_t seq_tx2_buf[TX_SEQBUF_SIZE];
uint8_t seq_tx3_buf[TX_SEQBUF_SIZE];
uint8_t seq_tx4_buf[TX_SEQBUF_SIZE];

uint8_t uart1_rx_buf[UART1_RX_BUFFER_LEN];
uint8_t uart1_tx_buf[UART1_TX_BUFFER_LEN];
uint8_t uart1_sysex_buf[SYSEX1_DATA_LEN];

uint8_t uart2_rx_buf[UART2_RX_BUFFER_LEN];
uint8_t uart2_tx_buf[UART2_TX_BUFFER_LEN];
uint8_t uart2_sysex_buf[SYSEX2_DATA_LEN];

uint8_t uartusb_rx_buf[UARTUSB_RX_BUFFER_LEN];
uint8_t uartusb_tx_buf[UARTUSB_TX_BUFFER_LEN];
uint8_t uartusb_sysex_buf[SYSEXUSB_DATA_LEN];

// Sequencer ring buffers
RingBuffer seq_tx1_rb(seq_tx1_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx2_rb(seq_tx2_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx3_rb(seq_tx3_buf, TX_SEQBUF_SIZE);
RingBuffer seq_tx4_rb(seq_tx4_buf, TX_SEQBUF_SIZE);

// UART ring buffers
RingBuffer uart1_rx_rb(uart1_rx_buf, UART1_RX_BUFFER_LEN);
RingBuffer uart1_tx_rb(uart1_tx_buf, UART1_TX_BUFFER_LEN);
RingBuffer uart1_sysex_rb(uart1_sysex_buf, SYSEX1_DATA_LEN);

RingBuffer uart2_rx_rb(uart2_rx_buf, UART2_RX_BUFFER_LEN);
RingBuffer uart2_tx_rb(uart2_tx_buf, UART2_TX_BUFFER_LEN);
RingBuffer uart2_sysex_rb(uart2_sysex_buf, SYSEX2_DATA_LEN);

RingBuffer uartusb_rx_rb(uartusb_rx_buf, UARTUSB_RX_BUFFER_LEN);
RingBuffer uartusb_tx_rb(uartusb_tx_buf, UARTUSB_TX_BUFFER_LEN);
RingBuffer uartusb_sysex_rb(uartusb_sysex_buf, SYSEXUSB_DATA_LEN);

// MIDI UART instances
MidiUartClass seq_tx1(uart0, nullptr, &seq_tx1_rb);
MidiUartClass seq_tx2(uart0, nullptr, &seq_tx2_rb);
MidiUartClass seq_tx3(uart1, nullptr, &seq_tx3_rb);
MidiUartClass seq_tx4(uart1, nullptr, &seq_tx4_rb);

MidiUartClass MidiUart(uart0, &uart1_rx_rb, &uart1_tx_rb);
MidiUartClass MidiUart2(uart1, &uart2_rx_rb, &uart2_tx_rb);
MidiUartClass MidiUartUSB(nullptr, &uartusb_rx_rb, &uartusb_tx_rb);

// Sysex instances
MidiSysexClass MidiSysex(&MidiUart, &uart1_sysex_rb);
MidiSysexClass MidiSysex2(&MidiUart2, &uart2_sysex_rb);
MidiSysexClass MidiSysexUSB(&MidiUartUSB, &uartusb_sysex_rb);

// MIDI class instances
MidiClass Midi(&MidiUart, &MidiSysex);
MidiClass Midi2(&MidiUart2, &MidiSysex2);

MidiIDSysexListenerClass MidiIDSysexListener;

// Global Variables

volatile uint8_t MidiUartParent::handle_midi_lock = 0;
volatile uint16_t g_clock_fast = 0;
volatile uint16_t g_clock_ms = 0;
volatile uint16_t g_clock_minutes = 0;

volatile uint16_t g_clock_fps = 0;
volatile uint16_t g_fps = 0;


// GUI object

GuiClass GUI;

//Oled Display

Oled oled_display(OLED_WIDTH, OLED_HEIGHT, &SPI1, OLED_DC, OLED_RST, OLED_CS, OLED_SPEED);

void init_oled() {
  DEBUG_FUNC();
  SPI1.setTX(OLED_MOSI);
  SPI1.setSCK(OLED_SCLK);

  // Configure control pins
  pinMode(OLED_CS, OUTPUT);
  pinMode(OLED_RST, OUTPUT);
  pinMode(OLED_DC, OUTPUT);

  // Reset the display
//  digitalWrite(OLED_RST, LOW);
//  delay(10);
//  digitalWrite(OLED_RST, HIGH);
//  delay(100);

  // Initialize display
  if (!oled_display.begin()) {
    DEBUG_PRINTLN("OLED initialization failed");
    while (1);
  }

  oled_display.display(); // show splashscreen
  delay(1000);
  oled_display.clearDisplay();
  oled_display.invertDisplay(0);
  oled_display.setRotation(2);
  oled_display.setTextSize(1);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setCursor(0, 0);
  oled_display.setTextWrap(false);
  oled_display.display();

}

void handleIncomingMidi() {
  Midi.processSysex();
  Midi2.processSysex();
  Midi.processMidi();
  Midi2.processMidi();
}
