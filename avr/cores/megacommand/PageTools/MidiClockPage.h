#ifndef MIDICLOCKPAGE_H__
#define MIDICLOCKPAGE_H__

#include "WProgram.h"
#include "MidiClock.h"
#include "GUI.h"

class MidiClockPage : public EncoderPage {
public:
	EnumEncoder clockSourceEncoder;
	BoolEncoder transmitEncoder;
	BoolEncoder immediateEncoder;
	EnumEncoder mergerEncoder;
	static const char *clockSourceEnum[];
	
	void readClockSettings();
	void writeClockSettings();
	void readMergeSettings();
	void writeMergeSettings();
	
	virtual void setup();
	
	virtual void show();
	virtual void loop();
	virtual bool handleEvent(gui_event_t *event);
	virtual void display() {
		EncoderPage::display();
	}
};

void initClockPage();

extern MidiClockPage midiClockPage;

#endif /* MIDICLOCKPAGE_H__ */
