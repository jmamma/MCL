#include "Monome.h"

MonomePage::MonomePage(MonomeParentClass *_monome) {
	monome = _monome;
	for (uint8_t i = 0; i < sizeof(buf); i++) {
		buf[i] = 0;
		needsRefresh = true;
	}
	height = width = 8;
	x = y = 0;
}

void MonomePage::setLED(uint8_t x, uint8_t y, uint8_t status) {
	if (status) {
		SET_BIT(buf[y], x);
	} else {
		CLEAR_BIT(buf[y], x);
	}

	if (monome->currentPage() == this) {
		monome->setLED(x, y, status);
	} else {
		needsRefresh = true;
	}
}

void MonomePage::toggleLED(uint8_t x, uint8_t y) {
	if (getBufLED(x, y)) {
		clearLED(x, y);
	} else {
		setLED(x, y);
	}
}

void MonomePageSwitcher::setup() {
	monome->addCallback(this, (monome_callback_ptr_t)&MonomePageSwitcher::handleEvent);
}
	
bool MonomePageSwitcher::handleEvent(monome_event_t *event) {
	if (IS_BUTTON_PRESSED(event) && (event->y == 7)) {
		if (pages[event->x] != NULL) {
			setPage(event->x);
		}
		return true;
	}
	return false;
}

bool MonomePageSwitcher::setPage(uint8_t page) {
	if (pages[page] != NULL) {
		monome->setPage(pages[page]);
		for (uint8_t i = 0; i < 8; i++) {
			monome->setLED(i, 7, i == page ? 1 : 0);
		}
		return true;
	}
	return false;
}
