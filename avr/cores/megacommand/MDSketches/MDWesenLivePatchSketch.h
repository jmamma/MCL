/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDWesenLivePatchSketch_H__
#define MDWesenLivePatchSketch_H__
/Applications/MidiCtrl.app/Contents/Resources/Java/hardware/libraries/MDSketches/MDArpeggiatorSketch.h
#include <MD.h>
#include <AutoEncoderPage.h>
#include <BreakdownPage.h>

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
 * \addtogroup md_sketches_wesen_live_patch MachineDrum Wesen Live Patch Sketch
 *
 * @{
 **/

class MDWesenLivePatchSketch : 
public Sketch, public MDCallback {
public:
  MDFXEncoder flfEncoder, flwEncoder, fbEncoder, levEncoder;
  EncoderPage page;

  MDFXEncoder timEncoder, frqEncoder, modEncoder;  
  EncoderPage page2;

  MDEncoder pFlfEncoder, pFlwEncoder, pSrrEncoder, pVolEncoder;
  EncoderPage page4;
  
  BreakdownPage breakPage;
  AutoEncoderPage<MDEncoder> autoMDPage;
  SwitchPage switchPage;

  uint8_t ramP1Track;

  bool muted ;

  void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("MD  "), 5);
    m_strncpy_p(n2, PSTR("LIV "), 5);
  }

  void setupPages() {
    flfEncoder.initMDFXEncoder(MD_ECHO_FLTF, MD_FX_ECHO, "FLF", 0);
    flwEncoder.initMDFXEncoder(MD_ECHO_FLTW, MD_FX_ECHO, "FLW", 127);
    fbEncoder.initMDFXEncoder( MD_ECHO_FB,   MD_FX_ECHO, "FB",  32);
    levEncoder.initMDFXEncoder(MD_ECHO_LEV,  MD_FX_ECHO, "LEV", 100);
    page.setShortName("DL1");
    page.setEncoders(&flfEncoder, &flwEncoder, &fbEncoder, &levEncoder);

    timEncoder.initMDFXEncoder(MD_ECHO_TIME, MD_FX_ECHO, "TIM", 24);
    frqEncoder.initMDFXEncoder(MD_ECHO_MFRQ, MD_FX_ECHO, "FRQ", 0);
    modEncoder.initMDFXEncoder(MD_ECHO_MOD,  MD_FX_ECHO, "MOD", 0);
    page2.setEncoders(&timEncoder, &frqEncoder, &modEncoder, &fbEncoder);
    page2.setShortName("DL2");

		mdBreakdown.ramP1Track = ramP1Track = 15;
		
    pFlfEncoder.initMDEncoder(ramP1Track, MODEL_FLTF, "FLF", 0);
    pFlwEncoder.initMDEncoder(ramP1Track, MODEL_FLTW, "FLW", 127);
    pSrrEncoder.initMDEncoder(ramP1Track, MODEL_SRR, "SRR", 0);
    pVolEncoder.initMDEncoder(ramP1Track, MODEL_VOL, "VOL", 100);
    page4.setEncoders(&pFlfEncoder, &pFlwEncoder, &pSrrEncoder, &pVolEncoder);
    page4.setShortName("RAM");

    autoMDPage.setup();
    autoMDPage.setShortName("AUT");
    
    switchPage.initPages(&page, &page2, &page4, &autoMDPage);
    switchPage.parent = this;

    breakPage.setup();
  }

  virtual void setup() {
    muted = false;
    setupPages();
    
    MDTask.addOnKitChangeCallback(this, (md_callback_ptr_t)&MDWesenLivePatchSketch::onKitChanged);

    for (int i = 0; i < 4; i++) {
      ccHandler.addEncoder((CCEncoder *)page4.encoders[i]);
    }
    ccHandler.setup();
    //    ccHandler.setCallback(onLearnCallback);
  }

  virtual void show() {
		if (currentPage() == &breakPage)
			popPage(&breakPage);
		if (currentPage() == NULL)
			setPage(&page);
  }

	virtual void hide() {
		mdBreakdown.stopSupatrigga();
		if (currentPage() == &breakPage) {
			popPage(&breakPage);
		}
	}

  virtual void mute(bool pressed) {
    if (pressed) {
      muted = !muted;
      mdBreakdown.muted = muted;
      autoMDPage.muted = muted;
      if (muted) {
				GUI.flash_strings_fill("LIVE PATCH", "MUTED");
      } else {
				GUI.flash_strings_fill("LIVE PATCH", "UNMUTED");
      }
    }
  }

  virtual bool handleEvent(gui_event_t *event) {
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      pushPage(&switchPage);
    } else if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
      popPage(&switchPage);
    } else {
      if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
        pushPage(&breakPage);
      } 
      else if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
        popPage(&breakPage);
      } 
      else if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
        mdBreakdown.startSupatrigga();
      } 
      else if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
        mdBreakdown.stopSupatrigga();
      } else {
        return false;
      }
    }

    return true;
  }

  virtual void doExtra(bool pressed) {
    if (pressed) {
      mdBreakdown.startSupatrigga();
    } else {
      mdBreakdown.stopSupatrigga();
    }
  }

  virtual Page *getPage(uint8_t i) {
    if (i == 0) {
      return &page;
    } else if (i == 1) {
      return &breakPage;
    } else {
      return NULL;
    }
  }
  
  void onKitChanged() {
		//		MidiUart.printfString("PARSED KIT %b with name %s", MD.kit.origPosition, MD.kit.name);
    for (int i = 0; i < 16; i++) {
      if (MD.kit.models[i] == RAM_P1_MODEL) {
        ramP1Track = i;
        mdBreakdown.ramP1Track = i;
        break;
      }
    }
    for (int i = 0; i < 4; i++) {
      ((MDEncoder *)page4.encoders[i])->track = ramP1Track;
    }
    for (int i = 0; i < 4; i++) {
      ((MDFXEncoder *)page.encoders[i])->loadFromKit();
      ((MDFXEncoder *)page2.encoders[i])->loadFromKit();
      ((MDEncoder *)page4.encoders[i])->loadFromKit();
    }
  }  
};

/* @} @} @} */

#endif /* MDWesenLivePatchSketch_H__ */
