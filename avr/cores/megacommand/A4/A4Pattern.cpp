#include "MCL_impl.h"
#ifdef HOST_MIDIDUINO
#include <stdio.h>
#endif

// #include "GUI.h"
bool A4Pattern::fromSysex(uint8_t *data, uint16_t len) {
	init();
	
	if ((len != (0xACA - 6)) && (len != (0x1521 - 6)))  {
#ifndef HOST_MIDIDUINO
    mcl_gui.draw_textbox("WRONG CHECKSUM", "");
#else
		printf("WRONG LENGTH: %x\n", len);
#endif
		return false;
	}
	
	
	if (!ElektronHelper::checkSysexChecksum(data, len)) {
		return false;
	}
	

	return true;
}




