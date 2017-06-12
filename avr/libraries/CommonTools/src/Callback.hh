/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef CALLBACK_H__
#define CALLBACK_H__

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \addtogroup helpers_callback Callback classes
 *
 * @{
 *
 * \file
 * Callback classes.
 **/

/**
 * \addtogroup callbackvector Callback vector
 *
 * @{
 */


#include <inttypes.h>
#include "helpers.h"

/**
 * Templated class that holds an array of N pointers to callback
 * objects of class C, with a callback function of type M.
 **/
template <class C, int N, typename M = void(C::*)()> class CallbackVector {
	/**
	 * \addtogroup callbackvector
	 * @{
	 **/
public:
  struct {
    C* obj;
    M ptr;
  } callbacks[N];

  uint8_t size;

  CallbackVector() {
    CallbackVector<C,N,M>::size = 0;
  }

	/** Add obj with callback method ptr to the callback vector, returns false if no room left. **/
  bool add(C *obj, M ptr) {
    if (size >= N) {
      return false;
    } else {
      for (uint8_t i = 0; i < size; i++) {
				if (callbacks[i].obj == obj &&
						callbacks[i].ptr == ptr)
					return true;
      }
      callbacks[size].obj = obj;
      callbacks[size].ptr = ptr;
      size++;
      return true;
    }
  }

	/** Remove obj with callback method ptr from the callback vector. **/
  void remove(C *obj, M ptr) {
    USE_LOCK();
    SET_LOCK();
    for (uint8_t i = 0; i < size; i++) {
      if (callbacks[i].obj == obj &&
					callbacks[i].ptr == ptr) {
				m_memcpy(callbacks + i, callbacks + i + 1, sizeof(callbacks[0]) * (size - i - 1));
				size--;
				break;
      }
    }
    CLEAR_LOCK();
  }

	/** Remove all instances of callback object obj from the vector. **/
  void remove(C *obj) {
    USE_LOCK();
    SET_LOCK();
  again:
    for (uint8_t i = 0; i < size; i++) {
      if (callbacks[i].obj == obj) {
				m_memcpy(callbacks + i, callbacks + i + 1, sizeof(callbacks[0]) * (size - i - 1));
				size--;
				goto again;
      }
    }
    CLEAR_LOCK();
  }

	/** Call all the stored callback objects. **/
  void call() {
    for (uint8_t i = 0; i < size; i++) {
      ((callbacks[i].obj)->*(callbacks[i].ptr))();
    }
  }

	/* @} */
};

/** Templated callback vector class with callback method taking one argument of type Arg1. **/
template <class C, int N = 4, typename Arg1 = void, typename M = void(C::*)(Arg1)> class CallbackVector1 :
  public CallbackVector<C, N, M> {
public:

	/** Call the callback method with argument a1. **/
  void call(Arg1 a1) {
    for (uint8_t i = 0; i < CallbackVector<C,N,M>::size; i++) {
      ((CallbackVector<C,N,M>::callbacks[i].obj)->*(CallbackVector<C,N,M>::callbacks[i].ptr))(a1);
    }
  }
};

/** Templated callback vector class with callback method taking two arguments of type Arg1 and Arg2. **/
template <class C, int N = 4, typename Arg1 = void, typename Arg2 = void, typename M = void(C::*)(Arg1, Arg2)>
class CallbackVector2 :
  public CallbackVector<C, N, M> {
public:

	/** Call the callback method with argument a1 and 2. **/
  void call(Arg1 a1, Arg2 a2) {
    for (uint8_t i = 0; i < CallbackVector<C,N,M>::size; i++) {
      ((CallbackVector<C,N,M>::callbacks[i].obj)->*(CallbackVector<C,N,M>::callbacks[i].ptr))(a1, a2);
    }
  }
};

/**
 * Templated callback vector class with callback method taking one
 * argument of type Arg1 and returning a boolean.
 *
 * This one is used mainly for GUI event callbacks, where the boolean
 * value indicates whether the event was handled.
 **/
template <class C, int N = 4, typename Arg1 = void, typename M = bool(C::*)(Arg1)>
class BoolCallbackVector1 :
  public CallbackVector<C, N, M> {
public:

	/** Call the callback methods until one callback returns true. **/
  bool callBool(Arg1 a1) {
    for (uint8_t i = 0; i < CallbackVector<C,N,M>::size; i++) {
      if (((CallbackVector<C,N,M>::callbacks[i].obj)->*(CallbackVector<C,N,M>::callbacks[i].ptr))(a1)) {
				return true;
			}
    }
		return false;
  }
};

/* @} @} @} */

#endif /* CALLBACK_H__ */
