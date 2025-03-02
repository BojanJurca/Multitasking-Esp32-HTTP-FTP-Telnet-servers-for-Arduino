/*
 *  vector.hpp for Arduino
 * 
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 * 
 *  vector.hpp is based on Tom Stewart's work: https://github.com/tomstewart89/Vector with the following differences:
 * 
 *  - syntax is closer to STL C++ vectors
 *  - error handling added
 *  - internal storage structure is different and also the logic for handling capacity
 *  - Iterator is implemented
 * 
 *  Vector internal storage is implemented as circular queue. It may have some free slots to let the vector breath a litle without having 
 *  to resize it all the time. Normally a vector would require additional chunk of memory when it runs out of free slots. How much memory
 *  would it require depends on increment parameter of a constuctor which is normally 1 but can also be some larger number.
 * 
 *  Picture of vector intgernal storge:
 * 
 *  vectorType __elements__ :   | | |5|4|3|2|1|0|5|6|7|8|9| | | | | | | |
 *                              |    |<--- __size__ ---->|              |
 *                              | __front__           __back__          |
 *                              |<-------------- __capacity__ --------->|  
 *
 *  October 10, 2024, Bojan Jurca
 * 
 */


#ifndef __VECTOR_HPP__
    #define __VECTOR_HPP__


    // ----- TUNNING PARAMETERS -----

    // #define __THROW_VECTOR_QUEUE_EXCEPTIONS__  // uncomment this line if you want vector to throw exceptions


    // error flags: there are only two types of error flags that can be set: err_bad_alloc and err_out_of_range - please note that all errors are negative (char) numbers
    #define err_ok           ((signed char) 0b00000000) //    0 - no error 
    #define err_bad_alloc    ((signed char) 0b10000001) // -127 - out of memory
    #define err_out_of_range ((signed char) 0b10000010) // -126 - invalid index      


    // type of memory used
    #define HEAP_MEM 2
    #define PSRAM_MEM 3
    #ifndef VECTOR_QUEUE_MEMORY_TYPE 
        #define VECTOR_QUEUE_MEMORY_TYPE HEAP_MEM // use heap by default
    #endif


    template <class vectorType> class vector {

        private: 

            signed char __errorFlags__ = 0;


        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }


           /*
            *  Constructor of vector with no elements allows the following kinds of creation of vectors: 
            *  
            *    vector<int> A;
            *    vector<int> C = { 100 };
            */
            
            vector () {}

            // vector (int increment = 1) { __increment__ = increment < 1 ? 1 : increment; }


            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                   /*
                    *  Constructor of vector from brace enclosed initializer list allows the following kinds of creation of vectors: 
                    *  
                    *     vector<int> D = { 200, 300, 400 };
                    *     vector<int> E ( { 500, 600 } );
                    */
            
                    vector (std::initializer_list<vectorType> il) {
                        if (reserve (il.size ())) { // != OK
                            return;
                        }

                        for (auto element: il)
                            if (push_back (element)) // error
                                break;
                    }
            #endif


           /*
            * Vector destructor - free the memory occupied by vector elements
            */
            
            ~vector () { 
                if (__elements__ != NULL) {
                    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                        for (int i = 0; i < __capacity__; i++)
                            __elements__ [i].~vectorType ();
                    #endif
                    free (__elements__);
                }
            }


           /*
            * Returns the number of elements in the vector.
            */

            int size () { return __size__; }


           /*
            * Returns current storage capacity = the number of elements that can fit into the vector without needing to resize the storage.
            * 
            */

            int capacity () { return __capacity__; }


           /*
            *  Changes storage capacity.
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - requested capacity is less than current vector size
            *    - could not allocate enough memory for requested storage
            */
        
            signed char reserve (int newCapacity) {
                if (newCapacity < __size__) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;   
                    return err_bad_alloc;
                }
                if (newCapacity > __size__) {
                    signed char e = __changeCapacity__ (newCapacity);
                    if (e) { // error
                        return e;
                    }
                }
                __reservation__ = newCapacity;              
                return err_ok; // no change in capacity is needed
            }


           /*
            * Checks if vector is empty.
            */

            bool empty () { return __size__ == 0; }


           /*
            * Clears all the elements from the vector.
            */

            void clear () { 
                __reservation__ = 0; // also clear the reservation if it was made
                if (__elements__ != NULL) 
                    __changeCapacity__ (0); 
            } // there is no reason why __changeCapacity__ would fail here


           /*
            *  [] operator enables elements of vector to be addressed by their positions (indexes) like:
            *  
            *    for (int i = 0; i < E.size (); i++)
            *      Serial.printf ("E [%i] = %i\n", i, E [i]);    
            *    
            *    or
            *    
            *     E [0] = E [1];
            *     
            *  If the index is not a valid index, the result is unpredictable
            */
      
            vectorType &operator [] (int position) {
                if (position < 0 || position >= __size__) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif                      
                    __errorFlags__ |= err_out_of_range;                    
                }
                return __elements__ [(__front__ + position) % __capacity__];
            }


           /*
            *  Same as [] operator, so it is not really needed but added here because it is supported in STL C++ vectors
            */      

            vectorType &at (int position) {
                if (position < 0 || position >= __size__) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif                      
                    __errorFlags__ |= err_out_of_range;                    
                }
                return __elements__ [(__front__ + position) % __capacity__];
            }


           /*
            *  Copy-constructor of vector allows the following kinds of creation of vectors: 
            *  
            *     vector<int> F = E;
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            *  
            *  Calling program should check errorFlags () after constructor is beeing called for possible errors
            */
      
            vector (vector& other) {
                signed char e = this->reserve (other.size ());
                if (e) { // != OK
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw e;
                    #endif                      
                    return; // prevent resizing __elements__ for each element beeing pushed back
                }

                // copy other's elements - storage will not get resized meanwhile
                for (auto element: other)
                    if (this->push_back (element)) // error
                        break;
            }


           /*
            *  Assignment operator of vector allows the following kinds of assignements of vectors: 
            *  
            *     vector<int> F;
            *     F = { 1, 2, 3 }; or F = {};
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            */
      
            vector* operator = (vector other) {
                this->clear (); // clear existing elements if needed
                signed char e = this->reserve (other.size ());
                if (e) { // != OK
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw e;
                    #endif                      
                    return this; // prevent resizing __elements__ for each element beeing pushed back
                }
                // copy other's elements - storege will not get resized meanwhile
                for (auto element: other)
                    if (this->push_back (element)) // error
                        break;
                return this;
            }

      
           /*
            * == operator allows comparison of vectors, like:
            * 
            *  Serial.println (F == E ? "vectors are equal" : "vectors are different");
            */
      
            bool operator == (vector& other) {
                if (this->__size__ != other.size ()) return false;
                int e = this->__front__;
                for (int i = 0; i < this->__size__; i++) {
                    if (this->__elements__ [e] != other [i])
                        return false;
                    e = (e + 1) % this->__capacity__;
                }
                return true;
            }
      
          
           /*
            *  Adds element to the end of a vector, like:
            *  
            *    E.push_back (700);
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */
    
            signed char push_back (vectorType element) {
                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    signed char e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e) { // != OK
                        #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                            throw e;
                        #endif                                                                    
                        return e;
                    }
                }          
        
                // add the new element at the end = (__front__ + __size__) % __capacity__, at this point we can be sure that there is enough __capacity__ of __elements__
                __elements__ [(__front__ + __size__) % __capacity__] = element;
                __size__ ++;
                return err_ok;
            }


           /*
            * push_front (unlike push_back) is not a STL C++ vector member function
            */
              
            signed char push_front (vectorType element) {
                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    signed char e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e) { // != OK
                        #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                            throw e;
                        #endif                      
                        return e;
                    }
                }
        
                // add the new element at the beginning, at this point we can be sure that there is enough __capacity__ of __elements__
                __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                __elements__ [__front__] = element;
                __size__ ++;
                return err_ok;
            }


           /*
            *  Deletes last element from the end of a vector, like:
            *  
            *    E.pop_back ();
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - element does't exist
            */
      
            signed char pop_back () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;                    
                    return err_out_of_range;
                }
                
                // remove last element
                __size__ --;

                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) 
                    __changeCapacity__ (__size__); // doesn't matter if it does't succeed, the element is deleted anyway
                return err_ok;
            }


           /*
            * pop_front (unlike pop_back) is not a STL C++ vector member function
            */
        
            signed char pop_front () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;                                      
                    return err_out_of_range;
                }

                // remove first element
                __front__ = (__front__ + 1) % __capacity__; // __front__ + 1
                __size__ --;
        
                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) 
                    __changeCapacity__ (__size__);  // doesn't matter if it does't succeed, the elekent is deleted anyway
                return err_ok;
            }


           /*
            * Returns the reference to the first element of the vector.
            */
        
            vectorType& front () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static vectorType ev = {};
                    return ev;
                }
                return __elements__ [__front__];
            }


           /*
            * Returns the reference to the last element of the vector.
            */
        
            vectorType& back () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static vectorType ev = {};
                    return ev;
                }
                return __elements__ [(__front__ + __size__ - 1) % __capacity__];
            }

      
           /*
            *  Iterator is needed in order for STL C++ for each loop to work. 
            *  A good source for iterators is: https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
            *  
            *  Example:
            *  
            *    for (auto element: A) 
            *      Serial.println (element);
            */
      
            class iterator {
                public:

                    // constructor
                    iterator (vector* vect, int pos) { 
                        __vector__ = vect; 
                        __position__ = pos;
                    }
                    
                    // * operator
                    vectorType& operator *() const { return __vector__->at (__position__); }

                    // ++ (prefix) increment
                    iterator& operator ++ () { ++ __position__; return *this; }

                    // -- (prefix) decrement
                    iterator& operator -- () { -- __position__; return *this; }

                    // - operator (needed by sort function to calculate size from iterators)
                    int operator - (const iterator& other) { return this->__position__ - other.__position__; }

                    // + operator (needed by sort function for random access)
                    iterator operator + (const int& position) { return iterator (__vector__, this->__position__ + position); }
                    iterator operator - (const int& position) { return iterator (__vector__, this->__position__ - position); }
                
                    // C++ will stop iterating when != operator returns false, this is when __position__ counts to vector.size ()
                    friend bool operator != (const iterator& a, const iterator& b) { return a.__position__ != b.__position__; }
                    friend bool operator == (const iterator& a, const iterator& b) { return a.__position__ == b.__position__; }

                    // this will tell if iterator is valid (if there are not elements the iterator can not be valid)
                    operator bool () const { return __vector__->size () > 0; }

                private:

                    vector* __vector__;
                    int __position__;

            };

            iterator begin () { return iterator (this, 0); }            // first element
            iterator end () { return iterator (this, this->size ()); }  // past the last element


           /*
            *  Erases the element occupying the position from the vector
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - element does't exist
            *
            *  It doesn't matter if internal failed to resize, if the element can be deleted the function still returns OK.
            */

            signed char erase (iterator position) {
                // check if position is valid
                iterator e = end ();
                if (!(position != e)) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif                           
                    __errorFlags__ |= err_out_of_range;                       
                    return err_out_of_range;
                }                 

                // try 2 faster options first
                if (!(position != begin ()))               
                    return pop_front (); 
                if (!(position != -- e))                     
                    return pop_back ();
                
                // calculate logical index of element to be deleted
                int pos = position - begin ();

                // do we have to free the space occupied by the element to be deleted? This is the slowest option
                if (__capacity__ > __size__ - 1 + __increment__ - 1)  
                    if (__changeCapacity__ (__size__ - 1, pos, - 1) == err_ok)
                        return err_ok;
                    // else (if failed to change capacity) proceeed

                // we have to reposition the elements, weather from the __front__ or from the calculated back, whichever is faster
                if (pos < __size__ - pos) {
                    // move all elements form position to 1
                    int e1 = (__front__ + pos) % __capacity__;
                    for (int i = pos; i > 0; i --) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e1 - 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // delete the first element now
                    return pop_front (); // tere is no reason why this wouldn't succeed now, so OK
                } else {
                    // move elements from __size__ - 1 to position
                    int e1 = (__front__ + pos) % __capacity__; 
                    for (int i = pos; i < __size__ - 1; i ++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // delete the last element now
                    return pop_back (); // tere is no reason why this wouldn't succeed now, so OK
                }
            }


           /*
            *  Inserts a new element at the position into the vector
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */

            signed char insert (iterator position, vectorType element) {
                if (position == end ())
                    return push_back (element);

                if (!(position != begin ()))               
                    return push_front (element); 
                
                // calculate logical index of element to be deleted
                int pos = position - begin ();

                // do we have to resize the space occupied by existing the elements? This is the slowest option
                if (__capacity__ < __size__ + 1) {
                    signed char e = __changeCapacity__ (__size__ + __increment__, -1, pos);
                    if (e)                              
                        return e;
                    // else
                    __elements__ [pos] = element;  
                    return err_ok; 
                }
      
                // we have to reposition the elements, weather from the __front__ or from the calculated back, whichever is faster
                if (pos < __size__ - pos) {
                    // move elements form 0 to position 1 position down
                    __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                    __size__ ++;
                    int e1 = __front__;
                    for (int i = 0; i < pos; i++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;
                    return err_ok;
                } else {
                    // move elements from __size__ - 1 to position 1 position up
                    int back = (__front__ + __size__) % __capacity__; // calculated back + 1
                    __size__ ++;
                    int e1 = back;
                    for (int i = __size__ - 1; i > pos; i--) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e2 = e1 - 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;        
                    return err_ok;
                }
            }


      private:

            vectorType *__elements__ = NULL;  // initially the vector has no elements, __elements__ buffer is empty
            int __capacity__ = 0;             // initial number of elements (or not occupied slots) in __elements__
            int __increment__ = 5;            // by default, increment elements buffer for 5 element when needed
            int __reservation__ = 0;          // no memory reservatio by default
            int __size__ = 0;                 // initially there are not elements in __elements__
            int __front__ = 0;                // points to the first element in __elements__, which do not exist yet at instance creation time

    
           /*
            *  Resizes __elements__ to new capacity with the option of deleting and adding an element meanwhile
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */

            signed char __changeCapacity__ (int newCapacity, int deleteElementAtPosition = -1, int leaveFreeSlotAtPosition = -1) {
                if (newCapacity < __reservation__) newCapacity = __reservation__;
                if (newCapacity == 0) {
                    // delete old buffer
                    if (__elements__ != NULL) {
                        #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                            for (int i = 0; i < __capacity__; i++)
                                __elements__ [i].~vectorType ();
                        #endif
                        free (__elements__);
                    }
                    
                    // update internal variables
                    __capacity__ = 0;
                    __elements__ = NULL;
                    __size__ = 0;
                    __front__ = 0;
                    return err_ok;
                } 
                // else

                // allocate new memory for the vector
                #if VECTOR_QUEUE_MEMORY_TYPE == PSRAM_MEM
                    vectorType *newElements = (vectorType *) ps_malloc (sizeof (vectorType) * newCapacity);
                #else // use heap
                    vectorType *newElements = (vectorType *) malloc (sizeof (vectorType) * newCapacity);
                #endif
                if (newElements == NULL) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    return err_bad_alloc;
                }

                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    new (newElements) vectorType [newCapacity]; 
                #else                 
                    // we can skip this step but won't be able to use objects as vector elements then
                    memset (newElements, 0, sizeof (vectorType) * newCapacity);
                #endif

                // copy existing elements to the new buffer
                if (deleteElementAtPosition >= 0) __size__ --;      // one element will be deleted
                if (leaveFreeSlotAtPosition >= 0) __size__ ++;      // a slot for 1 element will be added
                if (__size__ > newCapacity) __size__ = newCapacity; // shouldn't really happen
                
                int e = __front__;
                for (int i = 0; i < __size__; i++) {
                    // is i-th element supposed to be deleted? Don't copy it then ...
                    if (i == deleteElementAtPosition) e = (e + 1) % __capacity__; // e ++
                    
                    // do we have to leave a free slot for a new element at i-th place? Continue with the next index ...
                    if (i == leaveFreeSlotAtPosition) continue;
                    
                    newElements [i] = __elements__ [e];
                    e = (e + 1) % __capacity__;
                }

                // delete the old elements' buffer
                if (__elements__ != NULL) {
                    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                        for (int i = 0; i < __capacity__; i++)
                            __elements__ [i].~vectorType ();
                    #endif
                    free (__elements__);
                }
                
                // update internal variables
                __capacity__ = newCapacity;
                __elements__ = newElements;
                __front__ = 0;  // the first element is now aligned with 0
                return err_ok;
            }

    };
    

   /*
    * Arduino String vector template specialization (a good source for template specialization: https://www.cprogramming.com/tutorial/template_specialization.html)
    *
    * 1. String creation error checking
    *
    * Opposite to simple data types a String creation may fail (id controller runs out of memory for example). Success or failure of the creation can be checked with
    * String bool operator:
    *
    *  String s = "abc";
    *  if (s) // success ...
    *
    * 2. Movement of a String
    * 
    * Consider moving the String from one variable to the other in a form like this:
    *
    *  String a;
    *  String b = "abc";
    *  a = b;
    *
    * After the String is moved from memory occupied by variable b to memory occupied by variable a, String b may be destroyed. The constructor od a gets called during this process
    * and then also the destructor of b. This takes necessary time and memory space. Arduino Strings reside in both, stack and heap memory. If we just copy stack memory from one 
    * variable to the other, the pointer to the heap memory gets copied as well. There is no need to also move the heap memory too. But we must avoid calling the String destructor 
    * twice, when both Strings pointing to the same heap space will get destroyed.
    */

    template <> class vector<String> {

        private: 

            signed char __errorFlags__ = 0;


        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }


           /*
            *  Constructor of vector with no elements allows the following kinds of creation of vectors: 
            *  
            *    vector<int> A;
            *    vector<int> C = { "100" };
            */
            
            vector () {}

            // vector (int increment = 1) { __increment__ = increment < 1 ? 1 : increment; }


            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                /*
                    *  Constructor of vector from brace enclosed initializer list allows the following kinds of creation of vectors: 
                    *  
                    *     vector<String> D = { "200", "300", "400" };
                    *     vector<String> E ( { "500", "600" } );
                    */
            
                    vector (std::initializer_list<String> il) {
                        if (reserve (il.size ())) { // != OK
                            return;
                        }

                        for (String element: il) {
                            if (!element) { // String constructor failed
                                #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                                    throw err_bad_alloc;
                                #endif
                                __errorFlags__ |= err_bad_alloc;   
                                break;
                            }
                            if (push_back (element)) // error
                                break;
                        }
                    }
            #endif


           /*
            * Vector destructor - free the memory occupied by vector elements
            */
            
            ~vector () {
                if (__elements__ != NULL) {
                    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                        for (int i = 0; i < __capacity__; i++)
                            __elements__ [i].~String ();
                    #endif
                    free (__elements__);
                }
            }


           /*
            * Returns the number of elements in the vector.
            */

            int size () { return __size__; }


           /*
            * Returns current storage capacity = the number of elements that can fit into the vector without needing to resize the storage.
            * 
            */

            int capacity () { return __capacity__; }


           /*
            *  Changes storage capacity.
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - requested capacity is less than current vector size
            *    - could not allocate enough memory for requested storage
            */
        
            signed char reserve (int newCapacity) {
                if (newCapacity < __size__) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;   
                    return err_bad_alloc;
                }
                if (newCapacity > __size__) {
                    signed char e = __changeCapacity__ (newCapacity);
                    if (e) { // != OK
                        return e;
                    }
                }
                __reservation__ = newCapacity;              
                return err_ok; // no change in capacity is needed
            }


           /*
            * Checks if vector is empty.
            */

            bool empty () { return __size__ == 0; }


           /*
            * Clears all the elements from the vector.
            */

            void clear () { 
                __reservation__ = 0; // also clear the reservation if it was made
                if (__elements__ != NULL) __changeCapacity__ (0); 
            } // there is no reason why __changeCapacity__ would fail here


           /*
            *  [] operator enables elements of vector to be addressed by their positions (indexes) like:
            *  
            *    for (int i = 0; i < E.size (); i++)
            *      Serial.printf ("E [%i] = %i\n", i, E [i]);    
            *    
            *    or
            *    
            *     E [0] = E [1];
            *     
            *  If the index is not a valid index, the result is unpredictable
            */
      
            String &operator [] (int position) {
                if (position < 0 || position >= __size__) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif 
                    __errorFlags__ |= err_out_of_range;                                         
                }
                return __elements__ [(__front__ + position) % __capacity__];
            }

    
           /*
            *  Same as [] operator, so it is not really needed but added here because it is supported in STL C++ vectors
            */      

            String &at (int position) {
                if (position < 0 || position >= __size__) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif    
                    __errorFlags__ |= err_out_of_range;                                      
                }
                return __elements__ [(__front__ + position) % __capacity__];
            }


           /*
            *  Copy-constructor of vector allows the following kinds of creation of vectors: 
            *  
            *     vector<int> F = E;
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be destroyed.
            *  
            *  Calling program should check errorFlags () after constructor is beeing called for possible errors
            */
      
            vector (vector& other) {
                signed char e = this->reserve (other.size ());
                if (e) { // != OK
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw e;
                    #endif                      
                    return; // prevent resizing __elements__ for each element beeing pushed back
                }

                // copy other's elements - storage will not get resized meanwhile
                for (String element: other) {
                    if (!element) {                             // ... check if parameter construction is valid
                        #ifdef __THROW_LIST_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;       // report error if it is not
                        break;
                    }
                    if (this->push_back (element)) // error
                        break;
                }
            }


           /*
            *  Assignment operator of vector allows the following kinds of assignements of vectors: 
            *  
            *     vector<String> F;
            *     F = { "1", "2", "3" }; or F = {};
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be destroyed.
            */
      
            vector* operator = (vector other) {
                this->clear (); // clear existing elements if needed
                signed char e = this->reserve (other.size ());
                if (e) { // != OK
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw e;
                    #endif                      
                    return this; // prevent resizing __elements__ for each element beeing pushed back
                }
                // copy other's elements - storege will not get resized meanwhile
                for (String element: other) {
                    if (!element) {                             // ... check if parameter construction is valid
                        #ifdef __THROW_LIST_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;       // report error if it is not
                        break;
                    }
                    if (this->push_back (element))
                        break;
                }
                return this;
            }

      
           /*
            * == operator allows comparison of vectors, like:
            * 
            *  Serial.println (F == E ? "vectors are equal" : "vectors are different");
            */
      
            bool operator == (vector& other) {
                if (this->__size__ != other.size ()) return false;
                int e = this->__front__;
                for (int i = 0; i < this->__size__; i++) {
                    if (this->__elements__ [e] != other [i])
                        return false;
                    e = (e + 1) % this->__capacity__;
                }
                return true;
            }
      
          
           /*
            *  Adds element to the end of a vector, like:
            *  
            *    E.push_back ("700");
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */
    
            signed char push_back (String element) {
                if (!element) {                             // ... check if parameter construction is valid
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;       // report error if it is not
                    return err_bad_alloc;
                }

                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    signed char e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e) { // != OK
                        #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                            throw e;
                        #endif                                                                    
                        return e;
                    }
                }          
        
                // add the new element at the end = (__front__ + __size__) % __capacity__, at this point we can be sure that there is enough __capacity__ of __elements__
                __swapStrings__ (&__elements__ [(__front__ + __size__) % __capacity__], &element);
                __size__ ++;
                return err_ok;
            }

           /*
            * push_front (unlike push_back) is not a STL C++ vector member function
            */
              
            signed char push_front (String element) {
                if (!element) {                             // ... check if parameter construction is valid
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;       // report error if it is not
                    return err_bad_alloc;
                }

                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    signed char e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e) { // != OK
                        #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                            throw e;
                        #endif                      
                        return e;
                    }
                }
        
                // add the new element at the beginning, at this point we can be sure that there is enough __capacity__ of __elements__
                __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                __swapStrings__ (&__elements__ [__front__], &element);
                __size__ ++;
                return err_ok;
            }


           /*
            *  Deletes last element from the end of a vector, like:
            *  
            *    E.pop_back ();
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - element does't exist
            */
      
            signed char pop_back () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        if (__size__ == 0) throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;                    
                    return err_out_of_range;
                }
                
                // remove last element
                __size__ --;

                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) 
                    __changeCapacity__ (__size__); // doesn't matter if it does't succeed, the element is deleted anyway
                return err_ok;
            }


           /*
            * pop_front (unlike pop_back) is not a STL C++ vector member function
            */
        
            signed char pop_front () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        if (__size__ == 0) throw err_out_of_range;
                    #endif        
                    __errorFlags__ |= err_out_of_range;                                        
                    return err_out_of_range;
                }

                // remove first element
                __front__ = (__front__ + 1) % __capacity__; // __front__ + 1
                __size__ --;
        
                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) 
                    __changeCapacity__ (__size__);  // doesn't matter if it does't succeed, the elekent is deleted anyway
                return err_ok;
            }


           /*
            * Returns the reference to the first element of the vector.
            */
        
            String& front () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        if (__size__ == 0) throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static String ev = {};
                    return ev;
                }
                return __elements__ [__front__];
            }


           /*
            * Returns the reference to the last element of the vector.
            */

            String& back () {
                if (__size__ == 0) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        if (__size__ == 0) throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static String ev = {};
                    return ev;
                }
                return __elements__ [(__front__ + __size__ - 1) % __capacity__];
            }

      
           /*
            *  Iterator is needed in order for STL C++ for each loop to work. 
            *  A good source for iterators is: https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
            *  
            *  Example:
            *  
            *    for (auto element: A) 
            *      Serial.println (element);
            */
      
            class iterator {
                public:
                            
                    // constructor
                    iterator (vector* vect, int pos) { 
                        __vector__ = vect; 
                        __position__ = pos;
                    }
                    
                    // * operator
                    String& operator *() const { return __vector__->at (__position__); }
                
                    // ++ (prefix) increment
                    iterator& operator ++ () { __position__ ++; return *this; }

                    // -- (prefix) decrement
                    iterator& operator -- () { -- __position__; return *this; }

                    // - operator (needed by sort function)
                    int operator - (const iterator& other) { return this->__position__ - other.__position__; }

                    // + operator (needed by sort function for random access)
                    iterator operator + (const int& position) { return iterator (__vector__, this->__position__ + position); }
                    iterator operator - (const int& position) { return iterator (__vector__, this->__position__ - position); }
                
                    // C++ will stop iterating when != operator returns false, this is when __position__ counts to vector.size ()
                    friend bool operator != (const iterator& a, const iterator& b) { return a.__position__ != b.__position__; }
                    friend bool operator == (const iterator& a, const iterator& b) { return a.__position__ == b.__position__; }

                    // this will tell if iterator is valid (if the vector doesn't have elements the iterator can not be valid)
                    operator bool () const { return __vector__->size () > 0; }
            
                private:
          
                    vector* __vector__;
                    int __position__;
                    
            };      
      
            iterator begin () { return iterator (this, 0); }  
            iterator end () { return iterator (this, this->size ()); }


           /*
            *  Erases the element occupying the position from the vector
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - element does't exist
            *
            *  It doesn't matter if internal failed to resize, if the element can be deleted the function still returns OK.
            */
 
            signed char erase (iterator position) {
                // check if position is valid
                iterator e = end ();
                if (!(position != e)) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif                           
                    __errorFlags__ |= err_out_of_range;                       
                    return err_out_of_range;
                }                 

                // try 2 faster options first
                if (!(position != begin ()))               
                    return pop_front (); 
                if (!(position != -- e))                     
                    return pop_back ();
                
                // calculate logical index of element to be deleted
                int pos = position - begin ();

                // do we have to free the space occupied by the element to be deleted? This is the slowest option
                if (__capacity__ > __size__ - 1 + __increment__ - 1)  
                    if (__changeCapacity__ (__size__ - 1, pos, - 1) == err_ok)
                        return err_ok;
                    // else (if failed to change capacity) proceeed

                // we have to reposition the elements, weather from the __front__ or from the calculated back, whichever is faster
                if (position < __size__ - pos) {
                    // move all elements form position to 1
                    int e1 = (__front__ + pos) % __capacity__;
                    for (int i = pos; i > 0; i --) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e1 - 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // delete the first element now
                    return pop_front (); // tere is no reason why this wouldn't succeed now, so OK
                } else {
                    // move elements from __size__ - 1 to position
                    int e1 = (__front__ + pos) % __capacity__; 
                    for (int i = pos; i < __size__ - 1; i ++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // delete the last element now
                    return pop_back (); // tere is no reason why this wouldn't succeed now, so OK
                }
            }


           /*
            *  Inserts a new element at the position into the vector
            *  
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */

            signed char insert (iterator position, String element) {
                if (!element) { // String constuctor failed
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;   
                    return err_bad_alloc;
                }

                if (position == end ())
                    return push_back (element);

                if (!(position != begin ()))               
                    return push_front (element); 
                
                // calculate logical index of element to be deleted
                int pos = position - begin ();

                // do we have to resize the space occupied by existing the elements? This is the slowest option
                if (__capacity__ < __size__ + 1) {
                    signed char e = __changeCapacity__ (__size__ + __increment__, -1, pos);
                    if (e)                              
                        return e;
                    // else
                    __elements__ [pos] = element;  
                    return err_ok; 
                }
      
                // we have to reposition the elements, weather from the __front__ or from the calculated back, whichever is faster
                if (pos < __size__ - pos) {
                    // move elements form 0 to position 1 position down
                    __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                    __size__ ++;
                    int e1 = __front__;
                    for (int i = 0; i < pos; i++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;
                    return err_ok;
                } else {
                    // move elements from __size__ - 1 to position 1 position up
                    int back = (__front__ + __size__) % __capacity__; // calculated back + 1
                    __size__ ++;
                    int e1 = back;
                    for (int i = __size__ - 1; i > pos; i--) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e2 = e1 - 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;        
                    return err_ok;
                }
            }


      private:

            String *__elements__ = NULL;      // initially the vector has no elements, __elements__ buffer is empty
            int __capacity__ = 0;             // initial number of elements (or not occupied slots) in __elements__
            int __increment__ = 5;            // by default, increment elements buffer for 5 element when needed
            int __reservation__ = 0;          // no memory reservatio by default
            int __size__ = 0;                 // initially there are not elements in __elements__
            int __front__ = 0;                // points to the first element in __elements__, which do not exist yet at instance creation time

    
           /*
            *  Resizes __elements__ to new capacity with the option of deleting and adding an element meanwhile
            *  
            *  Returns true if succeeds and false in case of error:
            *    - could not allocate enough memory for requested storage
            */

            signed char __changeCapacity__ (int newCapacity, int deleteElementAtPosition = -1, int leaveFreeSlotAtPosition = -1) {
                if (newCapacity < __reservation__) newCapacity = __reservation__;
                if (newCapacity == 0) {
                    // delete old buffer
                    if (__elements__ != NULL) {
                        #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                            for (int i = 0; i < __capacity__; i++)
                                __elements__ [i].~String ();
                        #endif
                        free (__elements__);
                    }

                    // update internal variables
                    __capacity__ = 0;
                    __elements__ = NULL;
                    __size__ = 0;
                    __front__ = 0;
                    return err_ok;
                } 

                // allocate new memory for the vector
                #if VECTOR_QUEUE_MEMORY_TYPE == PSRAM_MEM
                    String *newElements = (String *) ps_malloc (sizeof (String) * newCapacity);
                #else // use heap
                    String *newElements = (String *) malloc (sizeof (String) * newCapacity);
                #endif
                if (newElements == NULL) {
                    #ifdef __THROW_VECTOR_QUEUE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    return err_bad_alloc;
                }

                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    new (newElements) String [newCapacity];
                #else
                    // if this is not supported by older boards, we can use the following instead:
                    memset (newElements, 0, sizeof (String) * newCapacity); // prevent caling String destructors at the following assignments
                    for (int i = 0; i < newCapacity; i++) newElements [i] = String (); // assign empty String
                #endif

                // copy existing elements to the new buffer
                if (deleteElementAtPosition >= 0) __size__ --;      // one element will be deleted
                if (leaveFreeSlotAtPosition >= 0) __size__ ++;      // a slot for 1 element will be added
                if (__size__ > newCapacity) __size__ = newCapacity; // shouldn't really happen
                
                int e = __front__;
                for (int i = 0; i < __size__; i++) {
                    // is i-th element supposed to be deleted? Don't copy it then ...
                    if (i == deleteElementAtPosition) e = (e + 1) % __capacity__; // e ++
                    
                    // do we have to leave a free slot for a new element at i-th place? Continue with the next index ...
                    if (i == leaveFreeSlotAtPosition) continue;
                    
                    // we don't need to care of an error occured while creating a newElement String, we'll replace it with a valid __elements__ string anyway
                    __swapStrings__ (&newElements [i], &__elements__ [e]);

                    e = (e + 1) % __capacity__;
                }
                
                // delete the old elements' buffer   
                if (__elements__ != NULL) {
                    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                        for (int i = 0; i < __capacity__; i++)
                            __elements__ [i].~String ();
                    #endif
                    free (__elements__);
                }

                // update internal variables
                __capacity__ = newCapacity;
                __elements__ = newElements;
                __front__ = 0;  // the first element is now aligned with 0
                return err_ok;
            }


            // swap strings by swapping their stack memory so constructors doesn't get called and nothing can go wrong like running out of memory meanwhile 
            void __swapStrings__ (String *a, String *b) {
                char tmp [sizeof (String)];
                memcpy (tmp, a, sizeof (String));
                memcpy (a, b, sizeof (String));
                memcpy (b, tmp, sizeof (String));
            }

    };

#endif
