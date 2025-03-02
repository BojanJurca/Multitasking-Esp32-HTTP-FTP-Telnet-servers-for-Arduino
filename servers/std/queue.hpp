/*
 *  queue.hpp for Arduino (ESP boards)
 * 
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 * 
 *  Implementing queue is easy, since everithing is already implemented in vectors. All we have to do is inherit form there.
 *
 *  October 10, 2024, Bojan Jurca
 * 
 */


#ifndef __QUEUE_HPP__
    #define __QUEUE_HPP__


    // ----- TUNNING PARAMETERS -----

    // #define __THROW_VECTOR_QUEUE_EXCEPTIONS__  // uncomment this line if you want queue to throw exceptions


    #include "vector.hpp"

    template <class queueType> class queue : public vector<queueType> {
        public:
            // using vector<queueType>::vector; // inherit all the constuctors as well

            queue () : vector<queueType> () {}

            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                queue (std::initializer_list<queueType> il) : vector<queueType> (il) {}
            #endif

            queue (queue& other) : vector<queueType> (other) {} // copy-constructor

            queue* operator = (queue other) {
                vector<queueType>::operator = (other);
                return this; 
            }

            inline signed char push (queueType element) __attribute__((always_inline)) {
                return vector<queueType>::push_back (element);
            }

            inline signed char pop () __attribute__((always_inline)) {
                return vector<queueType>::pop_front ();
            }

    };

#endif
