/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef STACK_H__
#define STACK_H__

#include <WProgram.h>
#include <inttypes.h>
#include "helpers.h"

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Stack class
 **/

/**
 * \addtogroup helpers_stack Stack class
 *
 * @{
 **/

/** Stack with N elements of class T. **/
template <class T, int N>
class Stack {
	/**
	 * \addtogroup helpers_stack
	 * @{
	 **/
	
 public:
  volatile uint8_t wr, start;
  T buf[N];
  
  Stack();
	/** Push a new element pointed to by t. **/
  bool push(T *t);
	/** Push a new element t. **/
  bool push(T t);
	/** Remove an element, and copy it to t. **/
  bool pop(T *t = NULL);
	/** Copy the top element to t. **/
  bool peek(T *t);
	/** Copy the element at idx into t. **/
  bool peekAt(T *t, uint8_t idx);
	/** Returns true if the stack is empty. **/
  bool isEmpty();
	/** Returns true if the stack is full. **/
  bool isFull();
	/** Empty the stack. **/
  void reset();
	/** Returns the number of elements in the stack. **/
  uint8_t size();

	/* @} */
};

#define STACK_INC(x) (uint8_t)(((x) + 1) % N)
#define STACK_DEC(x) (uint8_t)(((x) == 0) ? (N - 1) : (x) - 1)
#define STACK_START() (uint8_t)((wr  + start) % N)
#define STACK_LAST() (uint8_t)((STACK_DEC(wr)  + start) % N)
#define STACK_AT(x) (uint8_t)((STACK_DEC(wr) + start - x) % N)

template <class T, int N>
Stack<T,N>::Stack() {
  wr = 0;
  start = 0;
}

template <class T, int N>
void Stack<T,N>::reset() {
  //  uint8_t tmp = SREG;
  //  cli();
  wr = start = 0;
  //  SREG = tmp;
}

template <class T, int N>
bool Stack<T,N>::push(T *t) {
  //  uint8_t tmp = SREG;
  //  cli();
  
  if (isFull()) {
    start = STACK_INC(start);
  }
  m_memcpy(&buf[STACK_START()], t, sizeof(T));
  wr = STACK_INC(wr);

  //  SREG = tmp;

  return true;
}

template <class T, int N>
bool Stack<T,N>::push(T t) {
  return push(&t);
}

template <class T, int N>
bool Stack<T,N>::pop(T *t) {
  //  uint8_t tmp = SREG;
  //  cli();
  
  bool ret = peek(t);
  if (ret) {
    wr = STACK_DEC(wr);
  }

  // SREG = tmp;
  
  return ret;
}

template <class T, int N>
bool Stack<T,N>::peek(T *t) {
  //  uint8_t tmp = SREG;
  //  cli();
  
  if (isEmpty()) {
    //    SREG = tmp;
    
    return false;
  }
  if (t != NULL) {
    m_memcpy(t, &buf[STACK_LAST()], sizeof(T));
  }

  //  SREG = tmp;

  return true;
}

template <class T, int N>
  bool Stack<T,N>::peekAt(T *t, uint8_t idx) {
  if (size() <= idx) {
    return false;
  }
  if (t != NULL) {
    m_memcpy(t, &buf[STACK_AT(idx)], sizeof(T));
  }

  return true;
}

template <class T, int N>
bool Stack<T,N>::isEmpty() {
  //  uint8_t tmp = SREG;
  //  cli();

  bool ret = (wr == start);

  //  SREG = tmp;

  return ret;
}

template <class T, int N>
uint8_t Stack<T, N>::size() {
  //  uint8_t tmp = SREG;
  //  cli();

  uint8_t ret = 0;
  if (start > wr) {
    ret = N - start + wr;
  } else {
    ret = wr - start;
  }

  //  SREG = tmp;

  return ret;
}

template <class T, int N>
bool Stack<T,N>::isFull() {
  //  uint8_t tmp = SREG;
  //  cli();
  
  bool ret = (size() == (N - 1));

  // SREG = tmp;
  
  return ret;
}

/* @} @} */

#endif /* STACK_H__ */
