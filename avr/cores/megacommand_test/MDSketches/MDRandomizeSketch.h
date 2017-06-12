/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_RANDOMIZE_SKETCH_H__
#define MD_RANDOMIZE_SKETCH_H__

#include <MDRandomizePage.h>

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
 * \addtogroup md_sketches_randomize MachineDrum Randomize Sketch
 *
 * @{
 **/


class MDRandomizeSketch : 
  public Sketch, public MDCallback {

public:
  MDRandomizerClass MDRandomizer;
  MDRandomizePage randomizePage;

  MDRandomizeSketch() : randomizePage(&MDRandomizer) {
  }
  
  void setup() {
    MDRandomizer.setup();
    MDTask.addOnKitChangeCallback(this, (md_callback_ptr_t)&MDRandomizeSketch::onKitChanged);
  }

  void onKitChanged() {
    MDRandomizer.onKitChanged();
  }


  virtual void show() {
    if (currentPage() == NULL)
      setPage(&randomizePage);
  }

  virtual void mute(bool pressed) {
  }

  virtual void doExtra(bool pressed) {
    randomizePage.randomize();
  }

  virtual Page *getPage(uint8_t i) {
    if (i == 0) {
      return &randomizePage;
    } else {
      return NULL;
    }
  }

  void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("MD  "), 5);
    m_strncpy_p(n2, PSTR("RND "), 5);
  }
};

/* @} @} @} */

#endif /* MD_RANDOMIZE_SKETCH_H__ */
