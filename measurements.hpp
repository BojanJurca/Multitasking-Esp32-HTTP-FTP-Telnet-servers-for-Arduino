/*
  
    measurements.hpp 
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    Measurements.hpp include circular queue for storing measurements data set.
  
    April 27, 2026, Bojan Jurca

*/


#include <threadSafeCircularqueue.hpp>


#ifndef __MEASUREMENTS__
    #define __MEASUREMENTS__
    
    struct measurement_t {
        unsigned char scale;
        int16_t       value;
    };


     // define measurements circular queue
    template<size_t maxSize> class measurements : public threadSafeCircularQueue<measurement_t, maxSize> {

        public:

            void increase_valueCounter () {
                threadSafeCircularQueue<measurement_t, maxSize>::Lock ();
                __valueCounter__ ++;
                threadSafeCircularQueue<measurement_t, maxSize>::Unlock ();
            }

            void reset_valueCounter () {
                threadSafeCircularQueue<measurement_t, maxSize>::Lock ();
                __valueCounter__ = 0;
                threadSafeCircularQueue<measurement_t, maxSize>::Unlock ();
            }

            void push_back_and_reset_valueCounter (unsigned char scale) {
                threadSafeCircularQueue<measurement_t, maxSize>::Lock ();
                threadSafeCircularQueue<measurement_t, maxSize>::push_back ( { scale, __valueCounter__ } );
                __valueCounter__ = 0;
                threadSafeCircularQueue<measurement_t, maxSize>::Unlock ();
            }

            String toJson () {
                bool first = true;
                String s1 = "{\"average\":" + (threadSafeCircularQueue<measurement_t, maxSize>::size () == 0 ? "null" : String (average ())) + 
                             ",\"scale\":[";
                String s2 = "],\"value\":[";

                for (auto e = threadSafeCircularQueue<measurement_t, maxSize>::begin (); e != threadSafeCircularQueue<measurement_t, maxSize>::end (); ++ e) { // scan measurements with iterator where the locking mechanism is already implemented
                    if (!first) {
                        s1 += ",";
                        s2 += ",";
                    }
                    first = false;
                    s1 += String ((*e).scale);
                    s2 += String ((*e).value);
                }

                s2 += "]}\r\n";
                return s1 + s2;
            }

            virtual void pushed_back (measurement_t& element) { __sum__ += element.value; }
            virtual void popped_front (measurement_t& element) { __sum__ -= element.value; }

            inline long sum () __attribute__((always_inline)) { 
                return __sum__;
            }

            float average () { 
                threadSafeCircularQueue<measurement_t, maxSize>::Lock ();
                    float a = (float) __sum__ / (float) threadSafeCircularQueue<measurement_t, maxSize>::size ();
                threadSafeCircularQueue<measurement_t, maxSize>::Unlock ();
                return a;
            }  

        private:

            int16_t __valueCounter__ = 0; // in case measurements will be entered by counting

            long __sum__ =  {};
    
    };

#endif    
