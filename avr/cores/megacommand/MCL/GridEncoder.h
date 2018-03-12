/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef GRIDENCODER_H__
#define GRIDENCODER_H__
#include "MCL.h"

class GridEncoder : public Encoder {
  /**
     \addtogroup gui_rangeencoder_class
     @{
   **/

public:
  /** Minimum value of the encoder. **/
  int min;
  /** Maximum value of the encoder. **/
  int max;

  uint8_t fxparam;
  uint8_t effect;
  uint8_t num;

  /**
     Create a new range-limited encoder with max and min value, short
     name, initial value, and handling function. The initRangeEncoder
     will be called with the constructor arguments.
   **/

  GridEncoder(int _max = 127, int _min = 0, int res = 1) : Encoder() {
    initGridEncoder(_max, _min, (const char *)NULL, (int)0, res,
                    (encoder_handle_t)NULL);
  }

  /**
     Initialize the encoder with the same argument as the constructor.

     The initRangeEncoder functions automatically determines which of
     min and max is the minimum value. As of now this can't be used to
     have an "inverted" encoder.

     The initial value is called without calling the handling function.
   **/
  void initGridEncoder(int _max = 128, int _min = 0, const char *_name = NULL,
                       int init = 0, int res = 1,
                       encoder_handle_t _handler = NULL) {
    rot_res = res;
    setName(_name);
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
  virtual int update(encoder_t *enc);
  virtual void displayAt(int i);

  /* @} */
};

#endif /* GRIDENCODER_H__ */
