/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef DATA_ENCODER_H__
#define DATA_ENCODER_H__

#include "WProgram.h"
#include "Midi.h"
#include "MidiUartParent.h"

#ifdef HOST_MIDIDUINO
#include "DataEncoderUnchecking.h"
//#include "DataEncoderChecking.h"
#else
#include "DataEncoderUnchecking.h"
#endif

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \addtogroup helpers_data_encoder Data encoding classes
 *
 * @{
 *
 **/

/**
 * \addtogroup uartdataencoder Uart Data Encoder
 * DataEncoder that directly outputs encoded data on the uart.
 *
 * @{
 */

class UartDataEncoder : public DataEncoder {
	/**
	 * \addtogroup uartdataencoder Uart Data Encoder
	 * @{
	 **/
		 
public:
	MidiUartParent *uart;
	UartDataEncoder(MidiUartParent *_uart) {
		uart = _uart;
	}

	DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb) {
//        delayMicroseconds(50);
        uart->m_putc(inb);
		DATA_ENCODER_TRUE();
	}

	/* @} */
};

/* @} @} @} */

#endif /* DATA_ENCODER_H__ */
