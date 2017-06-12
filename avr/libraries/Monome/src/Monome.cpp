#include "Monome.h"

#define MONOME_GEN_ADDRESS(addr, data) ((((addr) & 0xF) << 4) | ((data) & 0xF))
#define MONOME_ADDRESS(data) (((data) >> 4) & 0xF)
#define MONOME_STATE(data) ((data) & 0xF)

#define MONOME_GEN_XY(x, y) ((((x) & 0xF) << 4) | ((y) & 0xF))
#define MONOME_X(data) ((data >> 4) & 0xF)
#define MONOME_Y(data) ((data) & 0xF)

#define MONOME_CMD_PRESS         0x00
#define MONOME_CMD_ADC_VAL       0x01
#define MONOME_CMD_LED           0x02
#define MONOME_CMD_LED_INTENSITY 0x03
#define MONOME_CMD_LED_TEST      0x04
#define MONOME_CMD_ADC_ENABLE    0x05
#define MONOME_CMD_SHUTDOWN      0x06
#define MONOME_CMD_LED_ROW       0x07
#define MONOME_CMD_LED_COLUMN    0x08


MonomeParentClass::MonomeParentClass() {
	activePage = NULL;
	for (uint8_t i = 0; i < 8; i++) {
		buf[i] = 0;
	}
	parse_state = MONOME_BYTE_1;
}

void MonomeParentClass::setLEDIntensity(uint8_t intensity) {
	sendMessage(MONOME_GEN_ADDRESS(MONOME_CMD_LED_INTENSITY, 0), intensity);
}

void MonomeParentClass::setLED(uint8_t x, uint8_t y, uint8_t status) {
	if (getBufLED(x, y) != status) {
		setBufLED(x, y, status);
		sendMessage(MONOME_GEN_ADDRESS(MONOME_CMD_LED, status), MONOME_GEN_XY(x, y));
	}
}

void MonomeParentClass::toggleLED(uint8_t x, uint8_t y) {
	if (getBufLED(x, y)) {
		clearLED(x, y);
	} else {
		setLED(x, y);
	}
}

void MonomeParentClass::setRow(uint8_t row, uint8_t leds) {
	for (uint8_t x = 0; x < 8; x++) {
		if (IS_BIT_SET(leds, x)) {
			setBufLED(x, row, 1);
		} else {
			setBufLED(x, row, 0);
		}
	}
	sendMessage(MONOME_GEN_ADDRESS(MONOME_CMD_LED_ROW, row), leds);
}

void MonomeParentClass::setColumn(uint8_t column, uint8_t leds) {
	for (uint8_t y = 0; y < 8; y++) {
		if (IS_BIT_SET(leds, y)) {
			setBufLED(column, y, 1);
		} else {
			setBufLED(column, y, 0);
		}
	}
	sendMessage(MONOME_GEN_ADDRESS(MONOME_CMD_LED_COLUMN, column), leds);
}

void MonomeParentClass::setBuffer() {
	for (uint8_t i = 0; i < 8; i++) {
		setRow(i, buf[i]);
	}
}

void MonomeParentClass::shutdown(uint8_t state) {
	sendMessage(MONOME_GEN_ADDRESS(MONOME_CMD_SHUTDOWN, 0), state);
}

void MonomeParentClass::handleByte(uint8_t byte) {
	static uint8_t lastByte = 0;
	switch (parse_state) {
	case MONOME_BYTE_1:
		parse_state = MONOME_BYTE_2;
		lastByte = byte;
		break;
		
	case MONOME_BYTE_2:
		{
			parse_state = MONOME_BYTE_1;
			monome_event_t event;
			switch (MONOME_ADDRESS(lastByte)) {
			case MONOME_CMD_PRESS:
				event.x = MONOME_X(byte);
				event.y = MONOME_Y(byte);
				event.state = MONOME_STATE(lastByte);
				eventRB.putp(&event);
				break;
			}
		}
		break;
	}
}

void MonomeParentClass::drawPage(MonomePage *page) {
	// XXX add support for clever masking and stuff
	for (uint8_t y = page->y; y < (page->y + page->height); y++) {
		setRow(y, page->buf[y]);
	}
	page->needsRefresh = false;
}

void MonomeParentClass::loop() {
  for (int i = 0; i < tasks.size; i++) {
    if (tasks.arr[i] != NULL) {
      tasks.arr[i]->checkTask();
    }
  }

  while (!eventRB.isEmpty()) {
    monome_event_t event;
    eventRB.getp(&event);
    for (int i = 0; i < eventHandlers.size; i++) {
      if (eventHandlers.arr[i] != NULL) {
				bool ret = eventHandlers.arr[i](&event);
				if (ret) {
					continue;
				}
      }
    }

		if (callbacks.callBool(&event)) {
			continue;
		}


    MonomePage *curPage = currentPage();
    if (curPage != NULL) {
			if ((event.x >= (curPage->x)) &&
					(event.x < (curPage->x + curPage->width)) &&
					(event.y >= (curPage->y)) &&
					(event.y < (curPage->y + curPage->height))) {
				if (curPage->handleEvent(&event)) {
					continue;
				}
			}
    }
		
  }

	MonomePage *curPage = currentPage();
	if (curPage != NULL) {
		curPage->loop();

		if (curPage->needsRefresh) {
			drawPage(curPage);
		}
	}
}

