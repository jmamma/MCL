/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef GRIDENCODER_H__
#define GRIDENCODER_H__

#include "GUI.h"

class GridEncoder : public Encoder {
  /**
     \addtogroup gui_rangeencoder_class
     @{
   **/

public:
  /** Minimum value of the encoder. **/
  int16_t min;
  /** Maximum value of the encoder. **/
  int16_t max;

  uint8_t fxparam;
  uint8_t effect;
  uint8_t num;
  bool scroll_fastmode = false;
  /**
     Create a new range-limited encoder with max and min value, short
     name, initial value, and handling function. The initRangeEncoder
     will be called with the constructor arguments.
   **/

  GridEncoder(int16_t _max = 127, int16_t _min = 0, int16_t res = 1) {
    initGridEncoder(_max, _min, 0, res, (encoder_handle_t)NULL);
  }

  /**
     Initialize the encoder with the same argument as the constructor.

     The initRangeEncoder functions automatically determines which of
     min and max is the minimum value. As of now this can't be used to
     have an "inverted" encoder.

     The initial value is called without calling the handling function.
   **/
  void initGridEncoder(int16_t _max = 128, int16_t _min = 0,
                       int16_t init = 0, int16_t res = 1,
                       encoder_handle_t _handler = NULL) {
    rot_res = res;
    handler = _handler;
    if (_min > _max) {
      min = _max;
      max = _min;
    } else {
      min = _min;
      max = _max;
    }
    setValue(init);
  }

  /**
     Update the value of the encoder according to pressmode and
     fastmode, and limit the resulting value using limit_value().
   **/
  virtual int16_t update(encoder_t *enc);
  virtual void displayAt(int16_t i);

  /* @} */
};

#endif /* GRIDENCODER_H__ */
