#include "Arduino.h"
#include "pico.h"
#include "core.h"
#include "MCL/MCL.h"
#include "platform.h"
#include "global.h"
#include "MidiUart.h"

void setup() {
    // Setup UART0 interrupt
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq_handler);
    // Setup UART1 interrupt
    irq_set_exclusive_handler(UART1_IRQ, uart1_irq_handler);
}

void loop() {

}
