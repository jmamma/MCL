/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLENCODER_H__
#define MCLENCODER_H__

#include "Encoders.hh"

class MCLEncoder : public Encoder {
    /**
       \addtogroup gui_rangeencoder_class
       @{
     **/

  public:
    /** Minimum value of the encoder. **/
    int min;
    /** Maximum value of the encoder. **/
    int max;

    /**
       Create a new range-limited encoder with max and min value, short
       name, initial value, and handling function. The initRangeEncoder
       will be called with the constructor arguments.
     **/
    MCLEncoder(int _max = 127, int _min = 0, int res = 1) : Encoder() {
      initMCLEncoder(_max, _min, (int) 0, res, (encoder_handle_t) NULL);
    }


    /**
       Initialize the encoder with the same argument as the constructor.

       The initRangeEncoder functions automatically determines which of
       min and max is the minimum value. As of now this can't be used to
       have an "inverted" encoder.

       The initial value is called without calling the handling function.
     **/
    void initMCLEncoder(int _max = 128, int _min = 0, int init = 0, int res = 1, encoder_handle_t _handler = NULL) {
      rot_res = res;
      //		setName(_name);
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
    //  virtual void displayAt(int i);

    /* @} */
};

#endif /* MCLENCODER_H__ */
