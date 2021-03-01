#include "MCL_impl.h"

/// Definition of MCL global class instances with dependencies, to control the initialization order.

// -- Midi UART devices
MidiUartClass MidiUart((uint8_t *)BANK1_UART1_RX_BUFFER_START,
                       UART1_RX_BUFFER_LEN,
                       (uint8_t *)BANK1_UART1_TX_BUFFER_START,
                       UART1_TX_BUFFER_LEN);
MidiUartClass2 MidiUart2((uint8_t *)BANK1_UART2_RX_BUFFER_START,
                         UART2_RX_BUFFER_LEN,
                         (uint8_t *)BANK1_UART2_TX_BUFFER_START,
                         UART2_TX_BUFFER_LEN);

// -- Midi class objects
MidiClass Midi(&MidiUart, SYSEX1_DATA_LEN, (uint8_t*)BANK1_SYSEX1_DATA_START);
MidiClass Midi2(&MidiUart2, SYSEX2_DATA_LEN, (uint8_t*)BANK1_SYSEX2_DATA_START);

// -- Sysex listeners
MidiIDSysexListenerClass MidiIDSysexListener;
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
