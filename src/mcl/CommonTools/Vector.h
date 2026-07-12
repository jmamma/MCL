/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

/** Represents an array of N elements of class T. This is slightly misnamed and should be called Set. **/
template <class T, int N>
class Vector {
	/**
	 * \addtogroup helpers_vector
	 * @{
	 **/
public:
  T arr[N] = {};
  static constexpr uint8_t size = N;

  Vector() = default;

	/** Add a new element t to the vector, in the first empty place. **/
  bool add(T t) {
    uint8_t empty = N;
    for (uint8_t i = 0; i < N; i++) {
      if (arr[i] == t) {
        return true;
      }
      if (arr[i] == (T)0 && empty == N) {
        empty = i;
      }
    }
    if (empty != N) {
      arr[empty] = t;
      return true;
    }
    return false;
  }

	/** Remove the element t from the vector. **/
  bool remove(T t) {
    for (uint8_t i = 0; i < N; i++) {
      if (arr[i] == t) {
				arr[i] = (T)0;
				return true;
      }
    }
    return false;
  }

	/** Returns the size of the vector. **/
  int length() {
    int res = 0;
    for (uint8_t i = 0; i < N; i++) {
      if (arr[i] != 0)
				res++;
    }
    return res;
  }

	/* @} */
};
