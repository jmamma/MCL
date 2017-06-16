/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef VECTOR_H__
#define VECTOR_H__

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Vector (Set) class
 **/

/**
 * \addtogroup helpers_vector Vector (Set) class
 *
 * @{
 **/

/** Represents an array of N elements of class T. This is slightly misnamed and should be called Set. **/
template <class T, int N>
class Vector {
	/**
	 * \addtogroup helpers_vector
	 * @{
	 **/
	
public:
  T arr[N];
  int size;

  Vector() {
    size = N;
    for (int i = 0; i < N; i++) {
      arr[i] = (T)0;
    }
  }

	/** Add a new element t to the vector, in the first empty place. **/
  bool add(T t) {
    for (int i = 0; i < N; i++) {
      if (arr[i] == (T)0 || arr[i] == t) {
				arr[i] = t;
				return true;
      }
    }
    return false;
  }

	/** Remove the element t from the vector. **/
  bool remove(T t) {
    for (int i = 0; i < N; i++) {
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
    for (int i = 0; i < N; i++) {
      if (arr[i] != 0)
				res++;
    }
    return res;
  }

	/* @} */
};

/* @} @} */

#endif /* VECTOR_H__ */
