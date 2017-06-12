/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "MDPitchEuclid.h"

const scale_t *MDPitchEuclid::scales[MDPitchEuclid::NUM_SCALES] = {
  &ionianScale,
  &aeolianScale,
  &bluesScale,
  &majorPentatonicScale,
  &majorMaj7Arp,
  &majorMin7Arp,
  &minorMin7Arp
};

MDPitchEuclid::MDPitchEuclid() : track(3, 8, 0) {
	currentScale = scales[0];
	octaves = 0;
	muted = false;
	pitches_len = 0;
	pitches_idx = 0;
	setPitchLength(4);
	mdTrack = 0;
}

void MDPitchEuclid::setPitchLength(uint8_t len) {
	pitches_len = len;
	randomizePitches();
}

void MDPitchEuclid::randomizePitches() {
	for (uint8_t i = 0; i < pitches_len; i++) {
		pitches[i] = randomScalePitch(currentScale, octaves);
	}
}

void MDPitchEuclid::on16Callback(uint32_t counter) {
	static uint8_t lastPitch = 255;
	
	if (track.isHit(counter)) {
		uint8_t pitch = basePitch + pitches[pitches_idx];
		if (pitch <= 127) {
			if (!muted) {
				if (mdTrack <= 15) {
					// normal track
					MD.sendNoteOn(mdTrack, pitch, 100);
				} else if (mdTrack = 127) {
					// all tracks
					for (uint8_t i = 0; i < 16; i++) {
						MD.sendNoteOn(i, pitch, 100);
					}
				} else {
				}
				lastPitch = pitch;
			}
		}
		pitches_idx = (pitches_idx + 1) % pitches_len;
	}
}
