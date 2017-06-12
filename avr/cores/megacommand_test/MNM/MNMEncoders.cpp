#include "MNMEncoders.h"

#ifndef HOST_MIDIDUINO
MNMEncoder::MNMEncoder(uint8_t _track, uint8_t _param, char *_name, uint8_t init) :
  CCEncoder(0, 0, _name, init) {
  initMNMEncoder(_track, _param);
  handler = CCEncoderHandle;
}

uint8_t MNMEncoder::getCC() {
  uint8_t cc = 0;
  if (param == 100) {
    cc = 0x3; // MUT
  } else if (param == 101) {
    cc = 0x7; // LEV;
  } else if (param < 0x10) {
    cc = param + 0x30;
  } else if (param < 0x28) {
    cc = param + 0x38;
  } else {
    cc = param + 0x40;
  }
  return cc;
}

uint8_t MNMEncoder::getChannel() {
  return MNM.global.baseChannel + track;
}

void MNMEncoder::initCCEncoder(uint8_t _channel, uint8_t _cc) {
  if (MNM.parseCC(_channel, _cc, &track, &param)) {
    initMNMEncoder(track, param);
  }
}

void MNMEncoder::loadFromKit() {
  if (param == 101) {
    setValue(MNM.kit.levels[track]);
  } else if (param < 72) {
    setValue(MNM.kit.parameters[track][param]);
  }
}

void MNMEncoder::initMNMEncoder(uint8_t _track, uint8_t _param,
				char *_name, uint8_t init) {
  track = _track;
  param = _param;
  if (_name == NULL) {
    if (MNM.loadedKit) {
      PGM_P name= NULL;
      name = MNM.getModelParamName(MNM.kit.models[track], param);
      if (name != NULL) {
	char myName[4];
	m_strncpy_p(myName, name, 4);
	setName(myName);
	GUI.redisplay();
      } else {
	setName("XXX");
	GUI.redisplay();
      }
    }
  } else {
    setName(_name);
    GUI.redisplay();
  }
      
  setValue(init);
}

static const uint8_t flashOffset[4] = {
  4, 8, 0, 0
};

void MNMTrackFlashEncoder::displayAt(int i) {
  uint8_t track = getValue();
  GUI.setLine(GUI.LINE2);
  GUI.put_value(i, track + 1);
  redisplay = false;
  GUI.flash_put_value(i, track + 1);
  GUI.flash_p_string_at_fill(flashOffset[i], MNM.getMachineName(MNM.kit.models[track]));
}

#endif /* HOST_MIDIDUINO */

