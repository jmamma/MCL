#ifndef MNM_PATTERN_EUCLID_SKETCH_H__
#define MNM_PATTERN_EUCLID_SKETCH_H__

#include <MNM.h>
#include <MidiEuclidSketch.h>
#include <MNMPatternEuclid.h>

class MNMPatternEuclidSketch : public Sketch, MNMCallback {
 public:
	MNMPatternEuclid euclid;
	PitchEuclidConfigPage1 configPage1;
	PitchEuclidConfigPage2 configPage2;
	SwitchPage switchPage;
	MNMTrackFlashEncoder trackEncoder;

 MNMPatternEuclidSketch() :
	configPage1(&euclid), configPage2(&euclid), trackEncoder("TRK") {
		configPage2.encoders[0] = &trackEncoder;
	}

	void setup() {
		configPage1.setShortName("EUC");
		configPage2.setShortName("SCL");

		switchPage.initPages(&configPage1, &configPage2);
		switchPage.parent = this;

		MNMTask.addOnPatternChangeCallback(this, (mnm_callback_ptr_t)&MNMPatternEuclidSketch::onPatternChange);
		MNMSysexListener.addOnPatternMessageCallback(this,
																								 (mnm_callback_ptr_t)&MNMPatternEuclidSketch::onPatternMessage);
	}

	virtual void show() {
		if (currentPage() == NULL)
			setPage(&configPage1);
	}

	virtual void doExtra(bool pressed) {
		if (pressed) {
			euclid.randomizePitches();
			euclid.makeTrack(trackEncoder.getValue());
		}
	}

	void getName(char *n1, char *n2) {
		m_strncpy_p(n1, PSTR("EUC "), 5);
		m_strncpy_p(n2, PSTR("LID "), 5);
	}

	virtual bool handleEvent(gui_event_t *event) {
		if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
			pushPage(&switchPage);
			return true;
		} else if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
			popPage(&switchPage);
			return true;
		}
		if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
			MNM.requestPattern(MNM.currentPattern);
			return true;
		}
		if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
			euclid.randomizePitches();
			euclid.makeTrack(trackEncoder.getValue());
			return true;
		}

		return false;
	}

	void onPatternChange() {
		MNM.requestPattern(MNM.currentPattern);
	}

	void onPatternMessage() {
		if (euclid.pattern.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
			char name[5];
			MNM.getPatternName(euclid.pattern.origPosition, name);
			GUI.flash_strings_fill("PATTERN", name);
		} else {
			GUI.flash_strings_fill("SYSEX", "ERROR");
		}
	}
};

#endif /* MNM_PATTERN_EUCLID_SKETCH_H__ */
