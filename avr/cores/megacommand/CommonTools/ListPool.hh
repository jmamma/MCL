/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef LISTPOOL_H__
#define LISTPOOL_H__

#include <inttypes.h>
#ifndef NULL
#define NULL 0
#endif

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \addtogroup helpers_list List classes
 *
 * @{
 *
 * \file
 * Callback classes.
 **/

/**
 * \addtogroup listelt List Element
 *
 * @{
 */

/** Simple class representing a doubly linked list elemnt of type C. **/
template <class C> class ListElt {
	/**
	 * \addtogroup listelt
	 * @{
	 */
public:
  C obj;
  ListElt *next;
  ListElt *prev;
	/* @} */
};

/** @} **/

/**
 * \addtogroup listpool List Pool
 *
 * @{
 **/

/**
 * Statically allocated doubly linked list with N elements.
 **/
template <class C, int N>
class ListPool {
	/**
	 * \addtogroup listpool
	 * @{
	 **/
public:

  ListElt<C> heap[N];
  ListElt<C> *freeList;

  ListPool() {
    ListElt<C> *prev = NULL;
    for (int i = 0; i < N; i++) {
      heap[i].prev = NULL;
      heap[i].next = NULL;

      if (prev != NULL) {
				prev->next = &heap[i];
				heap[i].prev = prev;
      }

      prev = &heap[i];
    }
    freeList = &heap[0];
  }

	/** Allocate a new element out of the pre-allocated buffer. Returns NULL if no elements are available. **/
  ListElt<C> *alloc() {
    if (freeList == NULL)
      return NULL;
    ListElt<C> *ret = freeList;
    ListElt<C> *next = ret->next;
    if (next != NULL) {
      next->prev = NULL;
    }
    ret->next = NULL;
    freeList = next;
    return ret;
  }

	/** Allocate a new element and store c inside it. Returns NULL if no elements are available.  **/
  ListElt<C> *alloc(C c) {
    ListElt<C> *ret = alloc();
    if (ret != NULL) {
      ret->obj = c;
    }
    return ret;
  }
  
	/** Allocate a new element and copy c into it. Returns NULL if no elements are available.  **/
  ListElt<C> *alloc(C *c) {
    ListElt<C> *ret = alloc();
    if (ret != NULL) {
      m_memcpy(&ret->obj, c, sizeof(C));
    }
    return ret;
  }

	/** Free the list element and put it back into the preallocated buffer. **/
  void free(ListElt<C> *elt) {
    if (freeList == NULL) {
      freeList = elt;
      elt->prev = elt->next = NULL;
    } else {
      elt->prev = NULL;
      elt->next = freeList;
      freeList = elt;
    }
  }
	/* @} */
};

/** @} **/


/**
 * \addtogroup list List 
 *
 * @{
 **/

/** Doubly-linked list with elements storing an object of type C. **/
template <class C> class List {
	/**
	 * \addtogroup list
	 * @{
	 **/
	
public:
  ListElt<C> *head;

  List() {
    head = NULL;
  }

	/** Returns the element at idx. **/
  ListElt<C> *getAt(uint8_t idx) {
    ListElt<C> *ptr;
    uint8_t i;
    
    for (i = 0, ptr = head; ptr != NULL && i < idx; i++, ptr = ptr->next) {
      ;
    }
    return ptr;
  }

	/** Returns the size of the list. **/
  uint8_t size() {
    uint8_t ret = 0;
    ListElt<C> *ptr;
    for (ptr = head; ptr != NULL; ptr = ptr->next) {
      ret++;
    }
    return ret;
  }
	
	/** Insert a new element at idx. **/
  ListElt<C> *insertAt(ListElt<C> *elt, uint8_t idx) {
    ListElt<C> *ptr = head;
    ListElt<C> *prev = NULL;
    for (uint8_t i = 0;
				 ptr != NULL != NULL && i < idx;
				 i++, prev = ptr, ptr = ptr->next) {
      ;
    }
    elt->prev = prev;
    elt->next = ptr;
    if (ptr != NULL) {
      ptr->prev = elt;
    }
    return elt;
  }

	/** Remove the element at idx. **/
  void removeAt(uint8_t idx) {
    ListElt<C> *ptr = getAt(idx);
    if (ptr != NULL) {
      remove(ptr);
    }
  }

	/** Returns the last element of the list. **/
  ListElt<C> *getLast() {
    if (head == NULL)
      return NULL;
    ListElt<C> *ptr = head;
    while (ptr->next != NULL) {
      ptr = ptr->next;
    }
    return ptr;
  }

	/** Removes the last element of the list and returns it. **/
  ListElt<C> *popLast() {
    ListElt<C> *ret = getLast();
    if (ret != NULL) {
      ret->prev->next = NULL;
      ret->prev = NULL;
    }
    return ret;
  }

	/** Push a new element at the end of the list. **/
  void pushLast(ListElt<C> *elt) {
    ListElt<C> *ptr = getLast();
    if (ptr != NULL) {
      ptr->next = elt;
      elt->prev = ptr;
      elt->next = NULL;
    } else {
      addFront(elt);
    }
  }

	/** Remove the element elt. **/
  void remove(ListElt<C> *elt) {
    ListElt<C> *prev = elt->prev;
    ListElt<C> *next = elt->next;
    if (prev != NULL) {
      prev->next = next;
    } else {
      // prev == NULL, first in List
      head = next;
    }
    if (next != NULL) {
      next->prev = prev;
    }
  }

	/** Removes and returns the first element of the list. **/
  ListElt<C> *pop() {
    ListElt<C> *ret = NULL;
    if (head != NULL) {
      ret = head;
      head = head->next;
    }

    return ret;
  }

	/** Push a new element in front of the list. **/
  void push(ListElt<C> *elt) {
    elt->next = head;
    elt->prev = NULL;
    if (head != NULL) {
      head->prev = elt;
    }
    head = elt;
  }

	/** Finds the first element storing c, starting at list element ptr. **/
  ListElt<C> *findFirstAfter(C c, ListElt<C> *ptr) {
    for (; ptr != NULL; ptr = ptr->next) {
      if (ptr->obj == c) {
				return ptr;
      }
    }
    return NULL;
  }

	/** Returns the first element storing c in the list. **/
  ListElt<C> *findFirst(C c) {
    return findFirstAfter(c, head);
  }

	/** Searches backwards for the first element storing c in the list, starting at element ptr. **/
  ListElt<C> *findFirstBefore(C c, ListElt<C> *ptr) {
    for (; ptr != NULL; ptr = ptr->prev) {
      if (ptr->obj == c) {
				return ptr;
      }
    }
    return NULL;
  }

	/** Swap the two list elements e1 and e2. **/
  void swap(ListElt<C> *e1, ListElt<C> *e2) {
    ListElt<C> *tmp;
    tmp = e1->prev;
    e1->prev = e2->prev;
    e2->prev = tmp;
    tmp = e1->next;
    e1->next = e2->next;
    e2->next = tmp;
  }

	/** Reverse the list. **/
  void reverse() {
    ListElt<C> *ptr = head;
    ListElt<C> *next = NULL;
    ListElt<C> *prev = NULL;

    for (; ptr != NULL;) {
      next = ptr->next;
      ptr->prev = next;
      ptr->next = prev;
      prev = ptr;
      ptr = next;
    }

    head = prev;
  }

	/* @} */
};

/** @} **/

/**
 * \addtogroup listwithpool List with auto-allocation pool
 *
 * @{
 **/

/** List class with automatic storage allocation of size N. **/
template <class C, int N> class ListWithPool : public List<C> {
	/**
	 * \addtogroup listwithpool
	 * @{
	 **/
	
public:
  ListPool<C, N> pool;

  void freeAll() {
    for (ListElt<C> *ptr = List<C>::pop(); ptr != NULL; ptr = List<C>::pop()) {
      pool.free(ptr);
    }
  }

  bool getLastValue(C &c) {
    ListElt<C> *elt = List<C>::getLast();
    if (elt != NULL) {
      c = elt->obj;
      return true;
    } else {
      return false;
    }
  }

  bool popLastValue(C &c) {
    ListElt<C> *elt = List<C>::popLast();
    if (elt != NULL) {
      c = elt->obj;
      pool.free(elt);
      return true;
    } else {
      return false;
    }
  }

  bool popValue(C &c) {
    ListElt<C> *elt = List<C>::pop();
    if (elt != NULL) {
      c = elt->obj;
      pool.free(elt);
      return true;
    } else {
      return false;
    }
  }

  bool pushValue(C *c) {
    ListElt<C> *elt = pool.alloc(c);
    if (elt == NULL) {
      return false;
    } else {
      List<C>::push(elt);
      return true;
    }
  }

  bool pushValue(C c) {
    ListElt<C> *elt = pool.alloc(c);
    if (elt == NULL) {
      return false;
    } else {
      List<C>::push(elt);
      return true;
    }
  }

  void pushLastValue(C *c) {
    ListElt<C> *elt = pool.alloc(c);
    if (elt == NULL) {
      return false;
    } else {
      List<C>::pushLast(elt);
      return true;
    }
  }

  void pushLastValue(C c) {
    ListElt<C> *elt = pool.alloc(c);
    if (elt == NULL) {
      return false;
    } else {
      List<C>::pushLast(elt);
      return true;
    }
  }
	/* @} */
};

/** @} **/

/**
 * \addtogroup callbacklist List to callback elements, with autoallocation
 *
 * @{
 **/

template <class C, int N> class CallbackList : public ListWithPool<C*, N> {
	/**
	 * \addtogroup callbacklist
	 * @{
	 **/
public:
  bool add(C *obj) {
    ListElt<C*> *elt = findFirst(obj);
    if (elt != NULL)
      return true;
    else 
      return pushValue(obj);
  }

  bool remove(C *obj) {
    ListElt<C*> *elt = findFirst(obj);
    if (elt != NULL) {
      List<C*>::remove(elt);
      return true;
    } else {
      return false;
    }
  }
	/* @} */
};
  


#endif /* LISTPOOL_H__ */
