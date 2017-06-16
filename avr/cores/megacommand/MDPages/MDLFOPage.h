/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_LFO_PAGE_H__
#define MD_LFO_PAGE_H__

#include <MD.h>

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_pages MachineDrum Pages
 * 
 * @{
 **/

/**
 * \addtogroup md_lfo_page MachineDrum LFO Configuration Page
 *
 * @{
 **/

/**
 * This page is used to control a LFO on the MachineDrum. It can
 * control 4 different parameters of the LFO, automatically displaying
 * their names and values.
 **/
class MDLFOPage : public EncoderPage {
	/**
	 * \addtogroup md_lfo_page
	 *
	 * @{
	 **/
	
public:
  MDLFOEncoder lfoEncoders[4];

  void initLFOParams(uint8_t lfo1, uint8_t lfo2, uint8_t lfo3, uint8_t lfo4) {
    lfoEncoders[0].setParam(lfo1);
    lfoEncoders[1].setParam(lfo2);
    lfoEncoders[2].setParam(lfo3);
    lfoEncoders[3].setParam(lfo4);
    redisplay = true;
  }

  MDLFOPage() {  
    initLFOParams(MD_LFO_SHP1, MD_LFO_SHP2, MD_LFO_UPDTE, MD_LFO_SPEED);
    for (uint8_t i = 0; i < 4; i++) {
      encoders[i] = &lfoEncoders[i];
    }
  }

  void setTrack(uint8_t track) {
    for (uint8_t i = 0; i < 4; i++) {
      lfoEncoders[i].track = track;
      lfoEncoders[i].loadFromKit();
    }
  }

  void loadFromKit() {
    for (uint8_t i = 0; i < 4; i++) {
      lfoEncoders[i].loadFromKit();
    }
  }

	/* @} */
};

/**
 * This page is used to configure the parameters that the MDLFOPage controls.
 **/
class MDLFOConfigPage : public EncoderPage {
	/**
	 * \addtogroup md_lfo_page
	 *
	 * @{
	 **/
	
public:
  EnumEncoder paramEncoders[4];
  MDLFOPage *lfoPage;

  MDLFOConfigPage(MDLFOPage *_lfoPage) {
    lfoPage = _lfoPage;
    for (uint8_t i = 0; i < 4; i++) {
      paramEncoders[i].initEnumEncoder(MDLFONames, 8);
      encoders[i] = &paramEncoders[i];
    }
    paramEncoders[0].setName("P1");
    paramEncoders[1].setName("P2");
    paramEncoders[2].setName("P3");
    paramEncoders[3].setName("P4");
  }

  virtual void show() {
    for (uint8_t i = 0; i < 4; i++) {
      paramEncoders[i].setValue(lfoPage->lfoEncoders[i].param);
    }
  }

  virtual void loop() {
    for (uint8_t i = 0 ;i < 4; i++) {
      if (paramEncoders[i].hasChanged()) {
        lfoPage->lfoEncoders[i].setParam(paramEncoders[i].getValue());
      }
    }
  }

	/* @} */
};

/** This class is used to configure the destination track of an MDLFOPage object. **/
class MDLFOTrackSelectPage : public EncoderPage {
	/**
	 * \addtogroup md_lfo_page
	 *
	 * @{
	 **/
public:
  MDTrackFlashEncoder trackEncoder;
  MDLFOPage *lfoPage;

  MDLFOTrackSelectPage(MDLFOPage *_lfoPage) : trackEncoder("TRK") {
    encoders[0] = &trackEncoder;
    lfoPage = _lfoPage;
  }  

  virtual void loop() {
    if (trackEncoder.hasChanged()) {
      lfoPage->setTrack(trackEncoder.getValue());
    }
  }
	/* @} */
};

/* @} @} @} */

#endif /* MD_LFO_PAGE_H__ */
