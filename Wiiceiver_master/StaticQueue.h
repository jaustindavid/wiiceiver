// NB: I pasted & renamed TinyQueue.h + TinyQueue.cpp just to keep it in one sketch

/*
 * Austin David, austin@austindavid.com
 * 7 Sept 2013 http://austindavid.com/t/TinyStack
 *
 * all right reserved &c
 *
 * A simple "tiny" queue, appropriate for a device without Serial or pin 13
 * (e.g. ATtiny45/85 parts), and limited RAM
 *
 * For any ATMega-based project, see:
 *   http://playground.arduino.cc/Code/StackList
 *   http://playground.arduino.cc/Code/StackArray
 */

/*
 * (CC BY-NC-SA 4.0) 
 * http://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * WARNING WARNING WARNING: attaching motors to a skateboard is 
 * a terribly dangerous thing to do.  This software is totally
 * for amusement and/or educational purposes.  Don't obtain or
 * make a wiiceiver (see below for instructions and parts), 
 * don't attach it to a skateboard, and CERTAINLY don't use it
 * to zip around with just a tiny, ergonomic nunchuck instead
 * of a bulky R/C controller.
 *
 * This software is made freely available.  If you wish to 
 * sell it, don't.  If you wish to modify it, DO! (and please
 * let me know).  Much of the code is derived from others out
 * there, I've made attributuions where appropriate.
 *
 * http://austindavid.com/wiiceiver
 *  
 * latest software & schematic: 
 *    https://github.com/jaustindavid/wiiceiver
 *
 * Enjoy!  Be safe! 
 * 
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 12 May 2014
 *
 */
 
#ifndef TinyQueue_h
#define TinyQueue_h

#include "Arduino.h"

#ifndef TINYQUEUE_SIZE
// 50 items
#define TINYQUEUE_SIZE 50
#endif

template <typename T>
class StaticQueue {
    
  private:
    byte _head, _tail;
    T _sum;
    T _queue[TINYQUEUE_SIZE + 1]; 


    // helper function --- increment an index, wrap if needed
    byte increment(const byte index) {
      byte ret = index + 1;
      if (ret > TINYQUEUE_SIZE) {
    	return 0;
      } else {
    	return ret;
      }
    } // increment
    
    
    // opposite of increment
    byte decrement(const byte index) {
      byte ret = index - 1;
      if (ret == 255) { // aka ret < 0
        return TINYQUEUE_SIZE;
      } else {
        return ret;
      }
    } // byte decrement(index)
    

  public:


    // constructor
    StaticQueue(void) {
      reset();
    } // Queue()
    
    
    // reset the Queue to "empty"
    void reset(void) {
      _head = _tail = _sum = 0;
    } // Queue::reset()
    
    
    /* 
     * enqueue(value) to the front of the queue
     *
     * silently ejects the last element if the queue is full
     */
    void enqueue(const T value) {
      _queue[_tail] = value;
      _sum += value;
      _tail = increment(_tail);
      if (_tail == _head) {
        // silent discard, but track the sum
        _sum -= _queue[_head];
        _head = increment(_head);
      }
    } // enqueue(value)
    
    
    /*
     * take a value from the end of the queue
     * 
     * returns NULL if the queue is empty.
     */
    T dequeue(void) {
      T value;
      if (! isEmpty()) {
        value = _queue[_head];
        _head = increment(_head);
    	_sum -= value;
        return value;
      } else {
        return NULL;
      }
    } // T dequeue()
    
    
    // returns true if the queue isEmpty
    bool isEmpty(void) {
      return _head == _tail;
    } // boolean Queue::isEmpty()
    
    
    // returns a sum of elements
    // fast
    T sum(void) {
      return _sum;
    } // T sum()
    
    
    // adds the most-recent count elemets
    // compute-intense, but memory-light
    T sum(byte count) { 
      T tmpSum = 0;
      
      if (isEmpty()) {
        return tmpSum;
      }
      
      byte index = decrement(_tail);
      byte i = 0;
      
      while (i < count) {
        tmpSum += _queue[index];
        if (index == _head) {
          break;
        }
        i++;
        index = decrement(index);
      }
      
      return tmpSum;
    } // T sum(count)
    
    
    // dumps the queue contents to Serial (or whatever Print class)
    void dump(Print & printer) {
      printer.print("head = "); printer.print(_head);
      printer.print(", tail = "); printer.println(_tail);
      for (byte i = 0; i <= TINYQUEUE_SIZE; i++) {
        printer.print(i);
        printer.print(": ");
        printer.print(_queue[i]);
        if (i == _head) {
          printer.print(" <- head");
        } else if (i == _tail) {
          printer.print(" <- tail");
        }
        printer.println();
      }
    } // StaticQueue::dump()


};

#endif

