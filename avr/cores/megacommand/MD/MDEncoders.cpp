/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "MDEncoders.h"
#include "GUI.h"
#include "MD.h"
#include "MDParams.hh"

#ifdef MIDIDUINO_USE_GUI

void MDEncoderHandle(Encoder *enc) {
  MDEncoder *mdEnc = (MDEncoder *)enc;
  MD.setTrackParam(mdEnc->track, mdEnc->param, mdEnc->getValue());
}

MDEncoder::MDEncoder(uint8_t _track, uint8_t _param, char *_name, uint8_t init) :
  CCEncoder(0, 0, _name, init) {
  initMDEncoder(_track, _param);
  handler = CCEncoderHandle;
  //  handler = MDEncoderHandle;
}

uint8_t MDEncoder::getCC() {
  uint8_t b = track & 3;
  uint8_t cc = param;
  if (param == 32) {
    cc = b + 12;
  } else if (param == 33) {
    cc = b + 8;
  } else if (b < 2) {
    cc += 16 + b * 24;
  } else {
    cc += 24 + b * 24;
  }
  return cc;
}

uint8_t MDEncoder::getChannel() {
  uint8_t channel = track >> 2;
  return MD.global.baseChannel + channel;
}

void MDEncoder::initCCEncoder(uint8_t _channel, uint8_t _cc) {
  MD.parseCC(_channel, _cc, &track, &param);
  if (MD.loadedKit && (track != 255)) {
    PGM_P name = NULL;
    name = model_param_name(MD.kit.models[track], param);
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
}

void MDEncoder::loadFromKit() {
  setValue(MD.kit.params[track][param]);
}

void MDFXEncoderHandle(Encoder *enc) {
  MDFXEncoder *mdEnc = (MDFXEncoder *)enc;
  MD.sendFXParam(mdEnc->param, mdEnc->getValue(), mdEnc->effect);
}

MDFXEncoder::MDFXEncoder(uint8_t _param, uint8_t _effect, char *_name, uint8_t init) :
  RangeEncoder(127, 0, _name, init) {
  initMDFXEncoder(_effect, _param);
  handler = MDFXEncoderHandle;
}

void MDFXEncoder::loadFromKit() {
  switch (effect) {
  case MD_FX_ECHO:
    setValue(MD.kit.delay[param]);
    break;

  case MD_FX_REV:
    setValue(MD.kit.reverb[param]);
    break;

  case MD_FX_EQ:
    setValue(MD.kit.eq[param]);
    break;

  case MD_FX_DYN:
    setValue(MD.kit.dynamics[param]);
    break;
  }
}

void MDLFOEncoderHandle(Encoder *enc) {
  MDLFOEncoder *mdEnc = (MDLFOEncoder *)enc;
  MD.setLFOParam(mdEnc->track, mdEnc->param, mdEnc->getValue());
}

MDLFOEncoder::MDLFOEncoder(uint8_t _param, uint8_t _track, char *_name, uint8_t init) :
  RangeEncoder(127, 0, _name, init) {
  initMDLFOEncoder(_param, _track, _name, init);
  handler = MDLFOEncoderHandle;
}

void MDLFOEncoder::setLFOParamName() {
  setName(MDLFONames[param]);
}

void MDLFOEncoder::setParam(uint8_t _param) {
  param = _param;
  setLFOParamName();

  switch (param) {
  case MD_LFO_TRACK:
    max = 15;
    break;
    
  case MD_LFO_PARAM:
    max = 23;
    break;

  case MD_LFO_SHP1:
    max = 5;
    break;
    
  case MD_LFO_SHP2:
    max = 5;
    break;
    
  case MD_LFO_UPDTE:
    max = 2;
    break;
    
  case MD_LFO_SPEED:
    max = 127;
    break;
    
  case MD_LFO_DEPTH:
    max = 127;
    break;
    
  case MD_LFO_SHMIX:
    max = 127;
    break;
  }

  loadFromKit();
  
}

void MDLFOEncoder::loadFromKit() {
  if (MD.loadedKit) {
    switch (param) {
    case MD_LFO_TRACK:
      setValue(MD.kit.lfos[track].destinationTrack);
      break;

    case MD_LFO_PARAM:
      setValue(MD.kit.lfos[track].destinationParam);
      break;

    case MD_LFO_SHP1:
      setValue(MD.kit.lfos[track].shape1);
      break;

    case MD_LFO_SHP2:
      setValue(MD.kit.lfos[track].shape2);
      break;

    case MD_LFO_UPDTE:
      setValue(MD.kit.lfos[track].type);
      break;

    case MD_LFO_SPEED:
      setValue(MD.kit.lfos[track].speed);
      break;

    case MD_LFO_DEPTH:
      setValue(MD.kit.lfos[track].depth);
      break;

    case MD_LFO_SHMIX:
      setValue(MD.kit.lfos[track].mix);
      break;
    }
  }
}

static const char *lfoUpdateStrings[] = { "FRE", "TRG", "HLD" };
static const char *lfoShapeStrings[] = { "TRI", "SAW", "SQR", "RMP", "EXP", "RND" };

static void MDTrackDisplayAt(Encoder *enc, int i, uint8_t track);
static void MDParamDisplayAt(Encoder *enc, int i, uint8_t track, uint8_t param);

void MDLFOEncoder::displayAt(int i) {
  switch (param) {
  case MD_LFO_TRACK:
    MDTrackDisplayAt(this, i, getValue());
    break;

  case MD_LFO_PARAM: {
    GUI.setLine(GUI.LINE2);
    PGM_P name = NULL;
    name = model_param_name(0xFF, getValue());
    GUI.put_p_string(i, name);
    redisplay = false;
  }
    break;
    
  case MD_LFO_SHP1:
  case MD_LFO_SHP2:
    GUI.put_string_at(i * 4, lfoShapeStrings[getValue()]);
    redisplay = false;
    break;

  case MD_LFO_UPDTE:
    GUI.put_string_at(i * 4, lfoUpdateStrings[getValue()]);
    redisplay = false;
    break;

  default:
    Encoder::displayAt(i);
    break;
  }
}

void MDKitSelectEncoderHandle(Encoder *enc) {
  MD.loadKit(enc->getValue());
}

MDKitSelectEncoder::MDKitSelectEncoder(const char *_name, uint8_t init) :
  RangeEncoder(0, 63, _name, init) {
  handler = MDKitSelectEncoderHandle;
}

void MDKitSelectEncoder::displayAt(int i) {
  GUI.setLine(GUI.LINE2);
  uint8_t kit = getValue();
  GUI.put_value(i, kit + 1);
  redisplay = false;
}

void MDPatternSelectEncoderHandle(Encoder *enc) {
  MD.loadPattern(enc->getValue());
}

MDPatternSelectEncoder::MDPatternSelectEncoder(const char *_name, uint8_t init) :
  RangeEncoder(0, 127, _name, init) {
  handler = MDPatternSelectEncoderHandle;
}

void MDPatternSelectEncoder::displayAt(int i) {
  GUI.setLine(GUI.LINE2);
  char name[5];
  uint8_t pattern = getValue();
  MD.getPatternName(pattern, name);
  GUI.put_string_at(i * 4, name);
}

static const uint8_t flashOffset[4] = {
  4, 8, 0, 0
};

static void MDTrackDisplayAt(Encoder *enc, int i, uint8_t track) {
  GUI.setLine(GUI.LINE2);
  GUI.put_value(i, track + 1);
  enc->redisplay = false;
  if (MD.loadedKit && enc->hasChanged()) {
    GUI.flash_p_string_at_fill(flashOffset[i], MD.getMachineName(MD.kit.models[track]));
    GUI.flash_put_value(i, track + 1);
  }
}

void MDTrackFlashEncoder::displayAt(int i) {
  MDTrackDisplayAt(this, i, getValue());
}

void MDMelodicTrackFlashEncoder::displayAt(int i) {
  uint8_t track = getValue();
  GUI.setLine(GUI.LINE2);
  GUI.put_value(i, track + 1);
  redisplay = false;
  if (hasChanged()) {
    if (MD.loadedKit) {
      if (MD.isMelodicTrack(track)) {
	GUI.flash_p_string_at_fill(flashOffset[i], MD.getMachineName(MD.kit.models[track]));
      } else {
	GUI.flash_p_string_at_fill(flashOffset[i], PSTR("XXX"));
      }
    }
    GUI.flash_put_value(i, track + 1);
  }
}

void MDAssignMachineEncoderHandle(Encoder *enc) {
  uint8_t model = pgm_read_byte(&machine_names[enc->getValue()].id);
  MD.assignMachine(((MDAssignMachineEncoder*)enc)->track, model);
}

MDAssignMachineEncoder::MDAssignMachineEncoder(uint8_t _track, const char *_name, uint8_t init) :
  RangeEncoder(countof(machine_names), 0, _name, init) {
  track = _track;
  handler = MDAssignMachineEncoderHandle;
}

void MDAssignMachineEncoder::displayAt(int i) {
  uint8_t model = pgm_read_byte(&machine_names[getValue()].id);
  GUI.setLine(GUI.LINE2);
  GUI.put_value(i, model);
  redisplay = false;
  if (hasChanged()) {
    GUI.flash_p_string_at_fill(flashOffset[i], MD.getMachineName(model));
    GUI.flash_put_value(i, model);
  }
}

void MDAssignMachineEncoder::loadFromMD() {
  if (MD.loadedKit) {
    for (uint8_t i = 0; i < countof(machine_names); i++) {
      if (pgm_read_byte(&machine_names[i].id) == MD.kit.models[track])
				setValue(i);
    }
  }
}

void MDTrigGroupEncoderHandle(Encoder *enc) {
  uint8_t track = enc->getValue();
  MD.setTrigGroup(((MDTrigGroupEncoder*)enc)->track, (track > 15 ? 127 : track));
}

MDTrigGroupEncoder::MDTrigGroupEncoder(uint8_t _track, const char *_name, uint8_t init) :
  RangeEncoder(0, 16, _name, init) {
  track = _track;
  handler = MDTrigGroupEncoderHandle;
}

void MDTrigGroupEncoder::displayAt(int i) {
  GUI.setLine(GUI.LINE2);
  uint8_t track = getValue();
  redisplay = false;
  if (track == 16) {
    GUI.put_p_string(i, PSTR("---"));

    if (hasChanged()) {
      GUI.flash_p_string_at_fill(i << 2, PSTR("---"));
    }
  } else {
    GUI.put_value(i, track + 1);
    if (MD.loadedKit && hasChanged()) {
      if (track <= 15) {
	GUI.flash_p_string_at_fill(flashOffset[i], MD.getMachineName(MD.kit.models[track]));
	GUI.flash_put_value(i, track + 1);
      }
    }
  }
}

void MDTrigGroupEncoder::loadFromMD() {
  if (MD.loadedKit) {
    uint8_t group = MD.kit.trigGroups[track];
    if (group == 0xFF) {
      group = 16;
    }
    setValue(group);
  }
}

void MDMuteGroupEncoderHandle(Encoder *enc) {
  uint8_t track = enc->getValue();
  MD.setMuteGroup(((MDMuteGroupEncoder*)enc)->track, (track > 15 ? 127 : track));
}

MDMuteGroupEncoder::MDMuteGroupEncoder(uint8_t _track, const char *_name, uint8_t init) :
  RangeEncoder(0, 16, _name, init) {
  track = _track;
  handler = MDMuteGroupEncoderHandle;
}

void MDMuteGroupEncoder::displayAt(int i) {
  GUI.setLine(GUI.LINE2);
  uint8_t track = getValue();
  redisplay = false;
  if (track == 16) {
    GUI.put_p_string(i, PSTR("---"));
    if (hasChanged()) {
      GUI.flash_p_string_at_fill(i << 2, PSTR("---"));
    }
  } else {
    GUI.put_value(i, track + 1);
    if (MD.loadedKit && hasChanged()) {
      if (track <= 15) {
	GUI.flash_p_string_at_fill(flashOffset[i], MD.getMachineName(MD.kit.models[track]));
	GUI.flash_put_value(i, track + 1);
      }
    }
  }
}

void MDMuteGroupEncoder::loadFromMD() {
  if (MD.loadedKit) {
    uint8_t group = MD.kit.muteGroups[track];
    if (group == 0xFF) {
      group = 16;
    }
    setValue(group);
  }
}

static void MDParamDisplayAt(Encoder *enc, int i, uint8_t track, uint8_t param) {
  GUI.setLine(GUI.LINE2);
  PGM_P name = NULL;
  if (MD.loadedKit) {
    name = model_param_name(MD.kit.models[track], param);
  } else {
    name = model_param_name(0xFF, param);
  }
  GUI.put_p_string(i, name);
  enc->redisplay = false;
}

void MDParamSelectEncoder::displayAt(int i) {
  MDParamDisplayAt(this, i, track, getValue());
}

#endif


