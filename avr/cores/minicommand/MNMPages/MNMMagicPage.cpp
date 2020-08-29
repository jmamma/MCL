#include "MNM.h"
#include "MNMMagicPage.h"

void MagicMNMPage::show() {
  if (track != 255) {
    GUI.flash_strings_fill("MAGIC PAGE ", "");
    GUI.setLine(GUI.LINE2);
    GUI.flash_p_string_fill(MNM.getMachineName(MNM.kit.models[track]));
    GUI.setLine(GUI.LINE1);
    GUI.flash_put_value_at(11, track + 1);
  }
}

void MagicMNMPage::setup() {
  AutoEncoderPage<MNMEncoder>::setup();
  track = 255;
}

void MagicMNMPage::setup(uint8_t param1, uint8_t param2, uint8_t param3, uint8_t param4)  {
  params[0] = param1;
  params[1] = param2;
  params[2] = param3;
  params[3] = param4;
  setup();
}

void MagicMNMPage::setTrack(uint8_t _track)  {
  if (track == _track)
    return;
  track = _track;
  for (int i = 0; i < 4; i++) {
    realEncoders[i].initMNMEncoder(track, params[i], NULL);
    if (MNM.loadedKit) {
      realEncoders[i].setValue(MNM.kit.parameters[track][params[i]]);
    }
  }
  clearRecording();
}


void MagicMNMPage::setToCurrentTrack()  {
  uint8_t currentTrack = MNM.getCurrentTrack();
  if (currentTrack == 255) {
    GUI.flash_strings_fill("MNM TIMEOUT", "");
  } 
  else {
    show();
    setTrack(currentTrack);
  }
}

bool MagicMNMPage::handleEvent(gui_event_t *event)  {
  if (BUTTON_DOWN(Buttons.BUTTON4)) {
    for (int i = Buttons.ENCODER1; i <= Buttons.ENCODER4; i++) {
      if (EVENT_PRESSED(event, i)) {
	GUI.setLine(GUI.LINE1);
	GUI.flash_string_fill("CLEAR");
	GUI.setLine(GUI.LINE2);
	GUI.flash_put_value(0, i);
	clearRecording(i);
	return true;
      }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    setToCurrentTrack();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    startRecording();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    stopRecording();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4) || EVENT_RELEASED(event, Buttons.BUTTON4)) {
    return true;
  }

  return false;
}

void MagicSwitchPage::setup()  {

  magicPages[0].setup(MNM_MODEL_AMP_DEC, MNM_MODEL_FILT_WDTH, MNM_MODEL_FX_DSND, MNM_MODEL_FX_DFB);
  magicPages[0].setShortName("STD");
  magicPages[1].setup(MNM_MODEL_FX_DSND, MNM_MODEL_FX_DFB, MNM_MODEL_FX_DBAS, MNM_MODEL_FX_DWID);
  magicPages[1].setShortName("DEL");
  magicPages[2].setup(MNM_MODEL_FILT_BASE, MNM_MODEL_FILT_WDTH, MNM_MODEL_FILT_DEC, MNM_MODEL_FILT_WOFS);
  magicPages[2].setShortName("FLT");
  magicPages[3].setup(MNM_MODEL_AMP_DEC, MNM_MODEL_LFO1_DPTH, MNM_MODEL_LFO2_DPTH, MNM_MODEL_LFO3_DPTH);
  magicPages[3].setShortName("LFO");

  setPage(&magicPages[0]); 
}

void MagicSwitchPage::setPage(Page *page)  {
  if (currentPage != page) {
    currentPage = page;
    currentPage->redisplayPage();
    show();
  }
}

bool MagicSwitchPage::handleEvent(gui_event_t *event) {
  if (BUTTON_DOWN(Buttons.BUTTON4)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      GUI.flash_strings_fill("CLEAR ALL", "");
      for (int i = 0; i < 4; i++) {
	magicPages[i].clearRecording();
      }
      return true;
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    selectPage = true;
    redisplayPage();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    selectPage = false;
    if (currentPage != NULL) {
      redisplayPage();
    }
    return true;
  }
  if (selectPage) {
    for (int i = Buttons.ENCODER1; i <= Buttons.ENCODER4; i++) {
      if (pages[i] != NULL && EVENT_PRESSED(event, i)) {
	setPage(pages[i]);
	return true;
      }
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      setToCurrentTrack();
      return true;
    }
  }

  if (currentPage != NULL) {
    return currentPage->handleEvent(event);
  } 
  else {
    return false;
  }
}


void MagicSwitchPage::update()  {
  if (currentPage != NULL) {
    currentPage->update();
  }
}

void MagicSwitchPage::finalize()  {
  if (currentPage != NULL) {
    currentPage->finalize();
  }
}

void MagicSwitchPage::setToCurrentTrack() {
  uint8_t currentTrack = MNM.getCurrentTrack();
  if (currentTrack == 255) {
    GUI.flash_strings_fill("MNM TIMEOUT", "");
  } 
  else {
    GUI.flash_strings_fill("MAGIC PAGE ", "");
    GUI.setLine(GUI.LINE2);
    GUI.flash_p_string_fill(MNM.getMachineName(MNM.kit.models[currentTrack]));
    GUI.setLine(GUI.LINE1);
    GUI.flash_put_value_at(11, currentTrack + 1);

    for (int i = 0; i < 4; i++) {
      magicPages[i].setTrack(currentTrack);
    }
  }
}
  
void MagicSwitchPage::display() {
  if (currentPage != NULL && !selectPage) {
    currentPage->display();
  } 
  else {
    SwitchPage::display();
  }
}

void MagicSwitchPage::show(){
  if (currentPage != NULL) {
    currentPage->show();
  }
}

void MagicSwitchPage::hide() {
  if (currentPage != NULL) {
    currentPage->show();
  }
}
