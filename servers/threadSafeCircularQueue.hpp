/*
    theadSafeCircularQueue.hpp
 
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
 
    May 22, 2022, Bojan Jurca

*/


#include "std/queue.hpp"


#ifndef __THREAD_SAFE_CIRCULAR_QUEUE_HPP__
    #define __THREAD_SAFE_CIRCULAR_QUEUE_HPP__


    template<class queueType, size_t maxSize> class threadSafeCircularQueue : public queue<queueType> {

        public:

            threadSafeCircularQueue () { queue<queueType>::reserve (maxSize); }

            void Lock () { xSemaphoreTakeRecursive (__semaphore__, portMAX_DELAY); } 

            void Unlock () { xSemaphoreGiveRecursive (__semaphore__); }

            void push_back (queueType element) {
                Lock ();
                if (queue<queueType>::size () >= maxSize) {
                    popped_front (queue<queueType>::front ());
                    queue<queueType>::pop_front ();
                }
                pushed_back (element);
                queue<queueType>::push_back (element);
                Unlock ();
            }

            void pop_front () {
                Lock ();
                queue<queueType>::pop_front ();
                Unlock ();
            }

            virtual void pushed_back (queueType& element) {}
            virtual void popped_front (queueType& element) {}

            class iterator : public queue<queueType>::iterator {
              
                public:

                    iterator (threadSafeCircularQueue* cq, size_t position) : queue<queueType>::iterator (cq, position) { __cq__ = cq; }

                    ~iterator () { __cq__->Unlock (); }

                private:

                    threadSafeCircularQueue *__cq__;

            };

            iterator begin () { 
                Lock (); 
                return iterator (this, 0); 
            } 
            
            iterator end () { 
                Lock (); 
                return iterator (this, queue<queueType>::size ()); 
            } 


        private:

            SemaphoreHandle_t __semaphore__ = xSemaphoreCreateRecursiveMutex (); 

    };


#endif
