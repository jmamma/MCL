#include "Profiler.h"
#include <avr/interrupt.h>

static volatile uint16_t profileIpBuf[256];
static volatile uint8_t profileIdx = 0;


static bool addedTask = false;
Task profilingTask(300, sendProfilingData);

void sendProfilingData() {
  disableProfiling();

  uint16_t ipBuf[256];
  m_memcpy(ipBuf, (void *)profileIpBuf, sizeof(ipBuf));
  m_memclr((void *)profileIpBuf, sizeof(profileIpBuf));
  profileIdx = 0;
  
  MidiUart.putc(0xF0);
  MidiUart.putc(0x01);
  for (int i = 0; i < 256; i++) {
    MidiUart.putc((ipBuf[i] >> 14) & 0x7F);
    MidiUart.putc((ipBuf[i] >> 7) & 0x7F);
    MidiUart.putc((ipBuf[i] >> 0) & 0x7F);
  }
  MidiUart.putc(0xF7);
  enableProfiling();
}

void enableProfiling() {
  SET_BIT(TIMSK, TOIE0);
  if (!addedTask) {
    GUI.addTask(&profilingTask);
    addedTask = true;
  }
}

void disableProfiling() {
  CLEAR_BIT(TIMSK, TOIE0);
}

static uint8_t profileCnt0 = 255;
ISR(TIMER0_OVF_vect) {
  uint16_t fp = (uint16_t)__builtin_frame_address(0);
  uint8_t a, b;
  fp += 12;
  a = (*((uint8_t *)fp));
  b = *((uint8_t *)(fp + 1));
  //  a <<= 1;
  //  b <<= 1;
  uint16_t address = ((a << 8) | b) << 1;
  //  uint16_t address = (((a << 1) & 0xFF) << 8 | ((b << 1) & 0xFF));
  // uint16_t address = (a << 8) | b;
  profileIpBuf[(profileIdx++)] = address;

  // avoid aliasing
  profileCnt0--;
  if (profileCnt0 < 128) {
    profileCnt0 = 255;
  }
  TCNT0 = profileCnt0;
}
