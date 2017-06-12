#ifndef MONOME_H__
#define MONOME_H__

#include "WProgram.h"
#include <inttypes.h>
#include "Callback.hh"
#include "Task.hh"

class MonomePage;

typedef struct {
	uint8_t x, y;
	uint8_t state;
} monome_event_t;

#define IS_BUTTON_PRESSED(evt)  ((evt)->state == 1)
#define IS_BUTTON_RELEASED(evt) ((evt)->state == 0)

class MonomeCallback {
};

typedef bool(MonomeCallback::*monome_callback_ptr_t)(monome_event_t *evt);
typedef bool (*monome_event_handler_t)(monome_event_t *event);

#include "MonomePages.hh"

class MonomeParentClass : public MonomePageContainer {
protected:
  Vector<monome_event_handler_t, 4> eventHandlers;
  Vector<Task *, 8> tasks;
	volatile CRingBuffer<monome_event_t, 8> eventRB;
	
public:
	MonomePage *activePage;

	MonomeParentClass();
	~MonomeParentClass() {
	}

	void setup() {
		setLEDIntensity(15);
	}

	void drawPage(MonomePage *page);
	void loop();

	/* commands */
	void setLED(uint8_t x, uint8_t y, uint8_t status = 1);
	void toggleLED(uint8_t x, uint8_t y);
	void clearLED(uint8_t x, uint8_t y) {
		setLED(x, y, 0);
	}
	void setRow(uint8_t row, uint8_t leds);
	void setColumn(uint8_t column, uint8_t leds);
	void setLEDIntensity(uint8_t intensity);
	void shutdown(uint8_t state = 0);

	/* host interface */
	virtual void sendBuf(uint8_t *data, uint8_t len) {
	}
	virtual void sendMessage(uint8_t byte1, uint8_t byte2) {
	}
	
	/* handle input */
	void handleByte(uint8_t byte);

	/* callbacks */
	BoolCallbackVector1<MonomeCallback, 8, monome_event_t *> callbacks;
	void addCallback(MonomeCallback *obj, monome_callback_ptr_t func) {
		callbacks.add(obj, func);
	}
	void removeCallback(MonomeCallback *obj, monome_callback_ptr_t func = NULL) {
		if (func == NULL) {
			callbacks.remove(obj);
		} else {
			callbacks.remove(obj, func);
		}
	}

  void addEventHandler(monome_event_handler_t handler) {
    eventHandlers.add(handler);
  }
  void removeEventHandler(monome_event_handler_t handler) {
    eventHandlers.remove(handler);
  }

	/* tasks */
  void addTask(Task *task) {
    tasks.add(task);
  }
  void removeTask(Task *task) {
    tasks.remove(task);
  }

	void setBuffer();

protected:
	/* buffer leds */
	enum {
		MONOME_BYTE_1 = 0,
		MONOME_BYTE_2
	} parse_state;
	uint8_t buf[8]; // blitting buffer
	void setBufLED(uint8_t x, uint8_t y, uint8_t status) {
		if (status) {
			SET_BIT(buf[y], x);
		} else {
			CLEAR_BIT(buf[y], x);
		}
	}
	uint8_t getBufLED(uint8_t x, uint8_t y) {
		if (IS_BIT_SET(buf[y], x)) {
			return 1;
		} else {
			return 0;
		}
	}
};


#endif /* MONOME_H__ */
