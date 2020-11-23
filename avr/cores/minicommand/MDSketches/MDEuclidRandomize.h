/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDEUCLIDRANDOMIZE_H__
#define MDEUCLIDRANDOMIZE_H__

#include <MD.h>
#include <MDRandomizer.h>
#include <Sequencer.h>

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_sketches MachineDrum Sketches
 * 
 * @{
 **/

/**
 * \addtogroup md_sketches_euclid_randomizer MachineDrum Euclid Randomizer
 *
 * @{
 **/

class MDEuclidRandomizer {
 public:
	MDRandomizerClass *randomizer;
	EuclidDrumTrack track;

	uint8_t params[32][24];
	uint8_t params_len;
	uint8_t params_idx;

	uint8_t mdTrack;
	uint8_t amount;
	uint8_t selectMask;

	bool muted;
	
 MDEuclidRandomizer(MDRandomizerClass *_randomizer) : track(3, 8, 0) {
		muted = false;

		params_len = 0;
		params_idx = 0;
		amount = 0;
		selectMask = 0;

		setTrack(0);
		setParamsLength(4);
	}

	void setTrack(uint8_t _mdTrack) {
		mdTrack = _mdTrack;
		randomizer->setTrack(mdTrack);
	}

	void setParamsLength(uint8_t len) {
		params_len = len;
		randomizeParams();
	}

	void randomizeParams() {
		for (uint8_t i = 0; i < params_len; i++) {
			/* randomize for all here because the selection according to mask
			 * is actually done in the timer callback.
			 */
			randomizer->randomize(amount, MDRandomizerClass::ALL_MASK, params[i]);
		}
	}

	void on16Callback(uint32_t counter) {
		if (track.isHit(counter)) {
			for (uint8_t i = 0; i < 24; i++) {
				uint32_t trackMask = MDRandomizerClass::paramSelectMask[selectMask];
				if (IS_BIT_SET32(trackMask, i)) {
					MD.setTrackParam(mdTrack, i, params[params_idx][i]);
				}
			}
			params_idx = (params_idx + 1) % params_len;
		}
	}
};

class MDEuclidRandomizerConfigPage1 : 
public EncoderPage {
public:
  RangeEncoder paramLengthEncoder;
  RangeEncoder pulseEncoder;
  RangeEncoder lengthEncoder;
  RangeEncoder offsetEncoder;
  MDEuclidRandomizer *euclid;

  MDEuclidRandomizerConfigPage1(MDEuclidRandomizer *_euclid) :
  euclid(_euclid), 
  paramLengthEncoder(1, 32, "PAR", 4),
  pulseEncoder(1, 32, "PLS", 3),
  lengthEncoder(2, 32, "LEN", 8),
  offsetEncoder(0, 32, "OFF", 0) {
    encoders[0] = &paramLengthEncoder;
    encoders[1] = &pulseEncoder;
    encoders[2] = &lengthEncoder;
    encoders[3] = &offsetEncoder;
  }

  void loop() {
    if (pulseEncoder.hasChanged() || lengthEncoder.hasChanged() || offsetEncoder.hasChanged()) {
      euclid->track.setEuclid(pulseEncoder.getValue(), lengthEncoder.getValue(),
      offsetEncoder.getValue());
    }
    if (paramLengthEncoder.hasChanged()) {
      euclid->setParamsLength(paramLengthEncoder.getValue());
    }
  }
};


class MDEuclidRandomizePage : public EncoderPage {
 public:
	MDEuclidRandomizer *randomEuclid;
	MDTrackFlashEncoder trackEncoder;
	RangeEncoder amtEncoder;
	EnumEncoder selectEncoder;
	
 MDEuclidRandomizePage(MDEuclidRandomizer *_randomEuclid) :
	randomEuclid(_randomEuclid),
		trackEncoder("TRK"),
		amtEncoder(0, 128, "AMT"),
		selectEncoder(MDRandomizerClass::selectNames, countof(MDRandomizerClass::selectNames), "SEL") {
		encoders[0] = &trackEncoder;
		encoders[1] = &amtEncoder;
		encoders[2] = &selectEncoder;

		randomEuclid->setTrack(trackEncoder.getValue());
	}

	virtual void loop() {
		if (trackEncoder.hasChanged()) {
			randomEuclid->setTrack(trackEncoder.getValue());
		}
		if (amtEncoder.hasChanged()) {
			randomEuclid->amount = amtEncoder.getValue();
		}
		if (selectEncoder.hasChanged()) {
			randomEuclid->selectMask = selectEncoder.getValue();
		}
	}

	bool handleEvent(gui_event_t *event) {
		return false;
	}
};

class MDEuclidRandomizeSketch :
public Sketch, public MDCallback, public ClockCallback {
 public:
	MDEuclidRandomizePage randomizePage;
	MDEuclidRandomizerConfigPage1 configPage;
	MDRandomizerClass randomizer;
	MDEuclidRandomizer randomEuclid;

 MDEuclidRandomizeSketch() : randomEuclid(&randomizer), configPage(&randomEuclid), randomizePage(&randomEuclid) {
	}

	void setup() {
		randomizer.setup();
		MDTask.addOnKitChangeCallback(this, (md_callback_ptr_t)&MDEuclidRandomizeSketch::onKitChanged);
		MidiClock.addOn16Callback(this, (midi_clock_callback_ptr_t)&MDEuclidRandomizeSketch::on16Callback);
	}

	void onKitChanged() {
		randomizer.onKitChanged();
	}

	virtual void show() {
		if (currentPage() == NULL) {
			setPage(&randomizePage);
		}
	}

	virtual void mute(bool pressed) {
		if (pressed) {
			randomEuclid.muted != randomEuclid.muted;
			if (randomEuclid.muted) {
				GUI.flash_strings_fill("EUCLID", "MUTED");
			} else {
				GUI.flash_strings_fill("EUCLID", "UNMUTED");
			}
		}
	}

	virtual void doExtra(bool pressed) {
		if (pressed) {
			randomEuclid.randomizeParams();
		}
	}

	virtual Page *getPage(uint8_t i) {
		if (i == 0) {
			return &randomizePage;
		} else if (i == 1) {
			return &configPage;
		} else {
			return NULL;
		}
	}

	bool handleEvent(gui_event_t *event) {
		if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
			GUI.setPage(&randomizePage);
			return true;
		} else if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
			GUI.setPage(&configPage);
			return true;
		} else if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
			randomEuclid.randomizeParams();
			return true;
		}

		return false;
	}

	void on16Callback(uint32_t counter) {
		randomEuclid.on16Callback(counter);
	}

	void getName(char *n1, char *n2) {
		m_strncpy_p(n1, PSTR("EUC"), 5);
		m_strncpy_p(n2, PSTR("RND"), 5);
	}
};

/* @} @} @} */

	
#endif /* MDEUCLIDRANDOMIZE_H__ */
