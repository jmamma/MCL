/* Copyright (c) 2002, Steinar Haugen
   Copyright (c) 2009, Manuel Odendahl, http://ruinwesen.com/
   
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

/* avr/iom64cxx.h - defines for ATmega64 for use in c++ templates

   As of 2002-11-23:
   - This should be up to date with data sheet Rev. 2490C-AVR-09/02 */

#ifndef _AVR_IOM64CXX_H_
#define _AVR_IOM64CXX_H_ 1

#include "ioxxx_cpp_macros.h"

/* I/O registers */

/* Input Pins, Port F */
#define CXX_PINF      _SFR_IO8_CXX(0x00)

/* Input Pins, Port E */
#define CXX_PINE      _SFR_IO8_CXX(0x01)

/* Data Direction Register, Port E */
#define CXX_DDRE      _SFR_IO8_CXX(0x02)

/* Data Register, Port E */
#define CXX_PORTE     _SFR_IO8_CXX(0x03)

/* ADC Data Register */
#define CXX_ADCW      _SFR_IO16_CXX(0x04) /* for backwards compatibility */
#ifndef __ASSEMBLER__
#define CXX_ADC       _SFR_IO16_CXX(0x04)
#endif
#define CXX_ADCL      _SFR_IO8_CXX(0x04)
#define CXX_ADCH      _SFR_IO8_CXX(0x05)

/* ADC Control and Status Register A */
#define CXX_ADCSR     _SFR_IO8_CXX(0x06) /* for backwards compatibility */
#define CXX_ADCSRA    _SFR_IO8_CXX(0x06) 

/* ADC Multiplexer select */
#define CXX_ADMUX     _SFR_IO8_CXX(0x07)

/* Analog Comparator Control and Status Register */
#define CXX_ACSR      _SFR_IO8_CXX(0x08)

/* USART0 Baud Rate Register Low */
#define CXX_UBRR0L    _SFR_IO8_CXX(0x09)

/* USART0 Control and Status Register B */
#define CXX_UCSR0B    _SFR_IO8_CXX(0x0A)

/* USART0 Control and Status Register A */
#define CXX_UCSR0A    _SFR_IO8_CXX(0x0B)

/* USART0 I/O Data Register */
#define CXX_UDR0      _SFR_IO8_CXX(0x0C)

/* SPI Control Register */
#define CXX_SPCR      _SFR_IO8_CXX(0x0D)

/* SPI Status Register */
#define CXX_SPSR      _SFR_IO8_CXX(0x0E)

/* SPI I/O Data Register */
#define CXX_SPDR      _SFR_IO8_CXX(0x0F)

/* Input Pins, Port D */
#define CXX_PIND      _SFR_IO8_CXX(0x10)

/* Data Direction Register, Port D */
#define CXX_DDRD      _SFR_IO8_CXX(0x11)

/* Data Register, Port D */
#define CXX_PORTD     _SFR_IO8_CXX(0x12)

/* Input Pins, Port C */
#define CXX_PINC      _SFR_IO8_CXX(0x13)

/* Data Direction Register, Port C */
#define CXX_DDRC      _SFR_IO8_CXX(0x14)

/* Data Register, Port C */
#define CXX_PORTC     _SFR_IO8_CXX(0x15)

/* Input Pins, Port B */
#define CXX_PINB      _SFR_IO8_CXX(0x16)

/* Data Direction Register, Port B */
#define CXX_DDRB      _SFR_IO8_CXX(0x17)

/* Data Register, Port B */
#define CXX_PORTB     _SFR_IO8_CXX(0x18)

/* Input Pins, Port A */
#define CXX_PINA      _SFR_IO8_CXX(0x19)

/* Data Direction Register, Port A */
#define CXX_DDRA      _SFR_IO8_CXX(0x1A)

/* Data Register, Port A */
#define CXX_PORTA     _SFR_IO8_CXX(0x1B)

/* EEPROM Control Register */
#define CXX_EECR	_SFR_IO8_CXX(0x1C)

/* EEPROM Data Register */
#define CXX_EEDR	_SFR_IO8_CXX(0x1D)

/* EEPROM Address Register */
#define CXX_EEAR	_SFR_IO16_CXX(0x1E)
#define CXX_EEARL	_SFR_IO8_CXX(0x1E)
#define CXX_EEARH	_SFR_IO8_CXX(0x1F)

/* Special Function I/O Register */
#define CXX_SFIOR     _SFR_IO8_CXX(0x20)

/* Watchdog Timer Control Register */
#define CXX_WDTCR     _SFR_IO8_CXX(0x21)

/* On-chip Debug Register */
#define CXX_OCDR      _SFR_IO8_CXX(0x22)

/* Timer2 Output Compare Register */
#define CXX_OCR2      _SFR_IO8_CXX(0x23)

/* Timer/Counter 2 */
#define CXX_TCNT2     _SFR_IO8_CXX(0x24)

/* Timer/Counter 2 Control register */
#define CXX_TCCR2     _SFR_IO8_CXX(0x25)

/* T/C 1 Input Capture Register */
#define CXX_ICR1      _SFR_IO16_CXX(0x26)
#define CXX_ICR1L     _SFR_IO8_CXX(0x26)
#define CXX_ICR1H     _SFR_IO8_CXX(0x27)

/* Timer/Counter1 Output Compare Register B */
#define CXX_OCR1B     _SFR_IO16_CXX(0x28)
#define CXX_OCR1BL    _SFR_IO8_CXX(0x28)
#define CXX_OCR1BH    _SFR_IO8_CXX(0x29)

/* Timer/Counter1 Output Compare Register A */
#define CXX_OCR1A     _SFR_IO16_CXX(0x2A)
#define CXX_OCR1AL    _SFR_IO8_CXX(0x2A)
#define CXX_OCR1AH    _SFR_IO8_CXX(0x2B)

/* Timer/Counter 1 */
#define CXX_TCNT1     _SFR_IO16_CXX(0x2C)
#define CXX_TCNT1L    _SFR_IO8_CXX(0x2C)
#define CXX_TCNT1H    _SFR_IO8_CXX(0x2D)

/* Timer/Counter 1 Control and Status Register */
#define CXX_TCCR1B    _SFR_IO8_CXX(0x2E)

/* Timer/Counter 1 Control Register */
#define CXX_TCCR1A    _SFR_IO8_CXX(0x2F)

/* Timer/Counter 0 Asynchronous Control & Status Register */
#define CXX_ASSR      _SFR_IO8_CXX(0x30)

/* Output Compare Register 0 */
#define CXX_OCR0      _SFR_IO8_CXX(0x31)

/* Timer/Counter 0 */
#define CXX_TCNT0     _SFR_IO8_CXX(0x32)

/* Timer/Counter 0 Control Register */
#define CXX_TCCR0     _SFR_IO8_CXX(0x33)

/* MCU Status Register */
#define CXX_MCUSR     _SFR_IO8_CXX(0x34) /* for backwards compatibility */
#define CXX_MCUCSR    _SFR_IO8_CXX(0x34) 

/* MCU general Control Register */
#define CXX_MCUCR     _SFR_IO8_CXX(0x35)

/* Timer/Counter Interrupt Flag Register */
#define CXX_TIFR      _SFR_IO8_CXX(0x36)

/* Timer/Counter Interrupt MaSK register */
#define CXX_TIMSK     _SFR_IO8_CXX(0x37)

/* External Interrupt Flag Register */
#define CXX_EIFR      _SFR_IO8_CXX(0x38)

/* External Interrupt MaSK register */
#define CXX_EIMSK     _SFR_IO8_CXX(0x39)

/* External Interrupt Control Register B */
#define CXX_EICRB     _SFR_IO8_CXX(0x3A)

/* XDIV Divide control register */
#define CXX_XDIV      _SFR_IO8_CXX(0x3C)

/* 0x3D..0x3E SP */

/* 0x3F SREG */

/* Extended I/O registers */

/* Data Direction Register, Port F */
#define CXX_DDRF      _SFR_MEM8_CXX(0x61)

/* Data Register, Port F */
#define CXX_PORTF     _SFR_MEM8_CXX(0x62)

/* Input Pins, Port G */
#define CXX_PING      _SFR_MEM8_CXX(0x63)

/* Data Direction Register, Port G */
#define CXX_DDRG      _SFR_MEM8_CXX(0x64)

/* Data Register, Port G */
#define CXX_PORTG     _SFR_MEM8_CXX(0x65)

/* Store Program Memory Control and Status Register */
#define CXX_SPMCR     _SFR_MEM8_CXX(0x68) 
#define CXX_SPMCSR    _SFR_MEM8_CXX(0x68) /* for backwards compatibility with m128*/

/* External Interrupt Control Register A */
#define CXX_EICRA     _SFR_MEM8_CXX(0x6A)

/* External Memory Control Register B */
#define CXX_XMCRB     _SFR_MEM8_CXX(0x6C)

/* External Memory Control Register A */
#define CXX_XMCRA     _SFR_MEM8_CXX(0x6D)

/* Oscillator Calibration Register */
#define CXX_OSCCAL    _SFR_MEM8_CXX(0x6F)

/* 2-wire Serial Interface Bit Rate Register */
#define CXX_TWBR      _SFR_MEM8_CXX(0x70)

/* 2-wire Serial Interface Status Register */
#define CXX_TWSR      _SFR_MEM8_CXX(0x71)

/* 2-wire Serial Interface Address Register */
#define CXX_TWAR      _SFR_MEM8_CXX(0x72)

/* 2-wire Serial Interface Data Register */
#define CXX_TWDR      _SFR_MEM8_CXX(0x73)

/* 2-wire Serial Interface Control Register */
#define CXX_TWCR      _SFR_MEM8_CXX(0x74)

/* Time Counter 1 Output Compare Register C */
#define CXX_OCR1C     _SFR_MEM16_CXX(0x78)
#define CXX_OCR1CL    _SFR_MEM8_CXX(0x78)
#define CXX_OCR1CH    _SFR_MEM8_CXX(0x79)

/* Timer/Counter 1 Control Register C */
#define CXX_TCCR1C    _SFR_MEM8_CXX(0x7A)

/* Extended Timer Interrupt Flag Register */
#define CXX_ETIFR     _SFR_MEM8_CXX(0x7C)

/* Extended Timer Interrupt Mask Register */
#define CXX_ETIMSK    _SFR_MEM8_CXX(0x7D)

/* Timer/Counter 3 Input Capture Register */
#define CXX_ICR3      _SFR_MEM16_CXX(0x80)
#define CXX_ICR3L     _SFR_MEM8_CXX(0x80)
#define CXX_ICR3H     _SFR_MEM8_CXX(0x81)

/* Timer/Counter 3 Output Compare Register C */
#define CXX_OCR3C     _SFR_MEM16_CXX(0x82)
#define CXX_OCR3CL    _SFR_MEM8_CXX(0x82)
#define CXX_OCR3CH    _SFR_MEM8_CXX(0x83)

/* Timer/Counter 3 Output Compare Register B */
#define CXX_OCR3B     _SFR_MEM16_CXX(0x84)
#define CXX_OCR3BL    _SFR_MEM8_CXX(0x84)
#define CXX_OCR3BH    _SFR_MEM8_CXX(0x85)

/* Timer/Counter 3 Output Compare Register A */
#define CXX_OCR3A     _SFR_MEM16_CXX(0x86)
#define CXX_OCR3AL    _SFR_MEM8_CXX(0x86)
#define CXX_OCR3AH    _SFR_MEM8_CXX(0x87)

/* Timer/Counter 3 Counter Register */
#define CXX_TCNT3     _SFR_MEM16_CXX(0x88)
#define CXX_TCNT3L    _SFR_MEM8_CXX(0x88)
#define CXX_TCNT3H    _SFR_MEM8_CXX(0x89)

/* Timer/Counter 3 Control Register B */
#define CXX_TCCR3B    _SFR_MEM8_CXX(0x8A)

/* Timer/Counter 3 Control Register A */
#define CXX_TCCR3A    _SFR_MEM8_CXX(0x8B)

/* Timer/Counter 3 Control Register C */
#define CXX_TCCR3C    _SFR_MEM8_CXX(0x8C)

/* ADC Control and Status Register B */
#define CXX_ADCSRB    _SFR_MEM8_CXX(0x8E)

/* USART0 Baud Rate Register High */
#define CXX_UBRR0H    _SFR_MEM8_CXX(0x90)

/* USART0 Control and Status Register C */
#define CXX_UCSR0C    _SFR_MEM8_CXX(0x95)

/* USART1 Baud Rate Register High */
#define CXX_UBRR1H    _SFR_MEM8_CXX(0x98)

/* USART1 Baud Rate Register Low*/
#define CXX_UBRR1L    _SFR_MEM8_CXX(0x99)

/* USART1 Control and Status Register B */
#define CXX_UCSR1B    _SFR_MEM8_CXX(0x9A)

/* USART1 Control and Status Register A */
#define CXX_UCSR1A    _SFR_MEM8_CXX(0x9B)

/* USART1 I/O Data Register */
#define CXX_UDR1      _SFR_MEM8_CXX(0x9C)

/* USART1 Control and Status Register C */
#define CXX_UCSR1C    _SFR_MEM8_CXX(0x9D)

#endif /* _AVR_IOM64_CXX_H_ */
