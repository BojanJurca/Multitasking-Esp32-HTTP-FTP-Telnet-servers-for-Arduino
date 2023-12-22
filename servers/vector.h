/*
 * vector.h for Arduino (ESP boards)
 * 
 * This file is part of Vectors-for-Arduino: https://github.com/BojanJurca/Vectors-for-Arduino
 * 
 * vector.h is based on Tom Stewart's work: https://github.com/tomstewart89/Vector with some differences:
 * 
 * - syntax is closer to STL C++ vectors
 * - error handling added
 * - internal storage structure is different and also the logic for handling capacity
 * - supports complex data types such as Strings and other kinds of objects
 * 
 * Vector internal storage is implemented as circular queue. It may have some free slots to let the vector breath a litle without having 
 * to resize it all the time. Normally a vector would require additional chunk of memory when it runs out of free slots. How much memory
 * would it require depends on increment parameter of a constuctor which is normally 1 but can also be some larger number.
 * 
 * Picture of vector intgernal storge:
 * 
 * vectorType __elements__ :   | | |5|4|3|2|1|0|5|6|7|8|9| | | | | | | |
 *                             |    |<--- __size__ ---->|              |
 *                             | __front__           __back__          |
 *                             |<-------------- __capacity__ --------->|  
 *
 * I tried to make the vector library as generic as possible but haven't succeeded completely. Please see the description of Arduino String template specialization below. 
 * It is possible that other specializations for other types of objects may be necessary as well.
 *
 * Vector functions are not thread-safe.
 * 
 *  Bojan Jurca, December 6, 2023
 *  
 */


#ifndef __VECTOR_H__
    #define __VECTOR_H__


    // #define __VECTOR_H_EXCEPTIONS__  // uncomment this line if you want vector to throw exceptions
    // #define __VECTOR_H_DEBUG__       // uncomment this line for debugging puroposes

    #ifdef __VECTOR_H_DEBUG__
        #define __vector_h_debug__(X) { Serial.print("__vector_h_debug__: ");Serial.println(X); }
    #else
        #define __vector_h_debug__(X) ;
    #endif


    template <class vectorType> class vector {

        public:

           /*
            * Error handling 
            * 
            * Some functions will return an error code or OK. 
            * 
            * They will also set lastErrorCode variable which will hold this value until it is reset (by assigning lastErrorCode = OK)
            * so it is not necessary to check success of each single operation in the code. 
            */
            
            enum errorCode {OK = 0, // not all error codes are needed here but they are shared among keyValuePairs and persistentKeyValuePairs as well
                            NOT_FOUND = -1,               // key is not found
                            BAD_ALLOC = -2,               // out of memory
                            OUT_OF_RANGE = -3,            // invalid index
                            NOT_UNIQUE = -4,              // the key is not unique
                            DATA_CHANGED = -5,            // unexpected data value found
                            FILE_IO_ERROR = -6,           // file operation error
                            NOT_WHILE_ITERATING = -7,     // operation can not be berformed while iterating
                            DATA_ALREADY_LOADED = -8      // can't load the data if it is already loaded 
            }; // note that all errors are negative numbers

            char *errorCodeText (int e) {
                switch (e) {
                    case OK:                  return (char *) "OK";
                    case NOT_FOUND:           return (char *) "NOT_FOUND";
                    case BAD_ALLOC:           return (char *) "BAD_ALLOC";
                    case OUT_OF_RANGE:        return (char *) "OUT_OF_RANGE";
                    case NOT_UNIQUE:          return (char *) "NOT_UNIQUE";
                    case DATA_CHANGED:        return (char *) "DATA_CHANGED";
                    case FILE_IO_ERROR:       return (char *) "FILE_IO_ERROR";
                    case NOT_WHILE_ITERATING: return (char *) "NOT_WHILE_ITERATING";
                    case DATA_ALREADY_LOADED: return (char *) "DATA_ALREADY_LOADED";
                }
                return NULL; // doesn't happen
            }
            
            errorCode lastErrorCode = OK;

            void clearLastErrorCode () { lastErrorCode = OK; }


           /*
            *  Constructor of vector with no elements allows the following kinds of creation of vectors: 
            *  
            *    vector<int> A;
            *    vector<int> B (10); // with increment of 10 elements when vector grows, to reduce how many times __elements__ will be resized (which is time consuming)
            *    vector<int> C = { 100 };
            */
            
            vector (int increment = 1) { __increment__ = increment < 1 ? 1 : increment; }
      
      
           /*
            *  Constructor of vector from brace enclosed initializer list allows the following kinds of creation of vectors: 
            *  
            *     vector<int> D = { 200, 300, 400 };
            *     vector<int> E ( { 500, 600} );
            */
      
            vector (std::initializer_list<vectorType> il) {
                if (reserve (il.size ()) != OK) {
                    __vector_h_debug__ ("constructor from brace enclosed initializer list - out of memory.");
                    return;
                }

                for (auto i: il)
                    push_back (i);
            }
      
            
           /*
            * Vector destructor - free the memory occupied by vector elements
            */
            
            ~vector () { if (__elements__ != NULL) delete [] __elements__; }


           /*
            * Returns the number of elements in vector.
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
            *  Returns OK if succeeds and errorCode in case of error:
            *    - requested capacity is less than current vector size
            *    - could not allocate enough memory for requested storage
            */
        
            errorCode reserve (int newCapacity) {
                if (newCapacity < __size__) {
                    __vector_h_debug__ ("reserve - can't change capacity without loosing some elements: BAD_ALLOC.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw BAD_ALLOC;
                    #endif                      
                    return lastErrorCode = BAD_ALLOC;
                }
                if (newCapacity > __size__) {
                    errorCode e = __changeCapacity__ (newCapacity);
                    if (e < OK) {
                        __vector_h_debug__ ("reserve - out of memory: BAD_ALLOC.");
                        return lastErrorCode = e;
                    }
                }
                __reservation__ = newCapacity;              
                return OK; // no change in capacity is needed
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
      
            vectorType &operator [] (int position) {
                if (position < 0 || position >= __size__) {
                    lastErrorCode = OUT_OF_RANGE;
                    __vector_h_debug__ ("operator []: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw OUT_OF_RANGE;
                    #endif                      
                }
                return __elements__ [(__front__ + position) % __capacity__];
            }

    
           /*
            *  Same as [] operator, so it is not really needed but added here because it is supported in standard C++ vectors
            */      

            vectorType &at (int position) {
                if (position < 0 || position >= __size__) {
                    lastErrorCode = OUT_OF_RANGE;
                    __vector_h_debug__ ("at: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw OUT_OF_RANGE;
                    #endif                      
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
            *  Calling program should check lastErrorCode member variable after constructor is beeing called for possible errors
            */
      
            vector (vector& other) {
                if (this->reserve (other.size ()) != OK) {
                    __vector_h_debug__ ("copy-constructor - out of memory.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw lastErrorCode;
                    #endif                      
                    return; // prevent resizing __elements__ for each element beeing pushed back
                }

                // copy other's elements - storage will not get resized meanwhile
                for (auto e: other)
                    this->push_back (e);
                    
                // alternativelly:
                //   for (int i = 0; i < other.size (); i++)
                //       this->push_back (other [i]);       
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
                if (this->reserve (other.size ()) != OK) {
                    __vector_h_debug__ ("operator = - out of memory.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw lastErrorCode;
                    #endif                      
                    return this; // prevent resizing __elements__ for each element beeing pushed back
                }
                // copy other's elements - storege will not get resized meanwhile
                for (auto e: other)
                    this->push_back (e);
                    
                // alternativelly:
                //   for (int i = 0; i < other.size (); i++)
                //       this->push_back (other [i]);               
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
            *  Returns OK if succeeds and errorCode in case of error:
            *    - could not allocate enough memory for requested storage
            */
    
            errorCode push_back (vectorType element) {
                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    errorCode e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e != OK) {
                        __vector_h_debug__ ("push_back: " + String (e));
                        #ifdef __VECTOR_H_EXCEPTIONS__
                            throw e;
                        #endif                                                                    
                        lastErrorCode = e;
                        return e;
                    }
                }          
        
                // add the new element at the end = (__front__ + __size__) % __capacity__, at this point we can be sure that there is enough __capacity__ of __elements__
                __elements__ [(__front__ + __size__) % __capacity__] = element;
                __size__ ++;
                return OK;
            }

           /*
            * push_front (unlike push_back) is not a standard C++ vector member function
            */
              
            errorCode push_front (vectorType element) {
                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    errorCode e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e != OK) {
                        __vector_h_debug__ ("push_front: " + String (e));
                        #ifdef __VECTOR_H_EXCEPTIONS__
                            throw e;
                        #endif                      
                        lastErrorCode = e;
                        return e;
                    }
                }
        
                // add the new element at the beginning, at this point we can be sure that there is enough __capacity__ of __elements__
                __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                __elements__ [__front__] = element;
                __size__ ++;
                return OK;
            }


           /*
            *  Deletes last element from the end of a vector, like:
            *  
            *    E.pop_back ();
            *    
            *  Returns OK if succeeds and errorCode in case of error:
            *    - element does't exist
            */
      
            errorCode pop_back () {
                if (__size__ == 0) {
                    lastErrorCode = OUT_OF_RANGE;
                    __vector_h_debug__ ("pop_back: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      if (__size__ == 0) throw OUT_OF_RANGE;
                    #endif          
                    return OUT_OF_RANGE;
                }
                
                // remove last element
                __size__ --;

                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) __changeCapacity__ (__size__); // doesn't matter if it does't succeed, the element is deleted anyway
                return OK;
            }


           /*
            * pop_front (unlike pop_back) is not a standard C++ vector member function
            */
        
            errorCode pop_front () {
                if (__size__ == 0) {
                    __vector_h_debug__ ("pop_front: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      if (__size__ == 0) throw OUT_OF_RANGE;
                    #endif                            
                    return OUT_OF_RANGE;
                }

                // remove first element
                __front__ = (__front__ + 1) % __capacity__; // __front__ + 1
                __size__ --;
        
                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) __changeCapacity__ (__size__);  // doesn't matter if it does't succeed, the elekent is deleted anyway
                return OK;
            }

      
           /*
            *  Returns a position (index) of the first occurence of the element in the vector if it exists, -1 otherwise. Example:
            *  
            *  Serial.println (D.find (400));
            *  Serial.println (D.find (500));
            */
      
            int find (vectorType element) {
                int e = __front__;
                for (int i = 0; i < __size__; i++) {
                  if (__elements__ [e] == element) return i;
                  e = (e + 1) % __capacity__;
                }
                
                return -1;
            }


           /*
            *  Erases the element occupying the position from the vector
            *  
            *  Returns OK if succeeds and errorCode in case of error:
            *    - element does't exist
            *
            *  It doesn't matter if internal failed to resize, if the element can be deleted the function still returns OK.
            */
      
            errorCode erase (int position) {
                // is position a valid index?
                if (position < 0 || position >= __size__) {
                    __vector_h_debug__ ("erase: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      throw OUT_OF_RANGE;
                    #endif                              
                    return OUT_OF_RANGE;
                }
      
                // try 2 faster options first
                if (position == __size__ - 1)                         return pop_back ();
                if (position == 0)                                    return pop_front (); 
      
                // do we have to free the space occupied by the element to be deleted? This is the slowest option
                if (__capacity__ > __size__ - 1 + __increment__ - 1)  
                    if (__changeCapacity__ (__size__ - 1, position, - 1) == OK)
                        return OK;
                    // else (if failed to change capacity) proceeed
      
                // we have to reposition the elements, weather from the __front__ or from the calculate back, whichever is faster
                if (position < __size__ - position) {
                    // move all elements form position to 1
                    int e1 = (__front__ + position) % __capacity__;
                    for (int i = position; i > 0; i --) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e1 - 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // delete the first element now
                    return pop_front (); // tere is no reason why this wouldn't succeed now, so OK
                } else {
                    // move elements from __size__ - 1 to position
                    int e1 = (__front__ + position) % __capacity__; 
                    for (int i = position; i < __size__ - 1; i ++) {
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
            *  Returns OK succeeds or errorCode:
            *    - could not allocate enough memory for requested storage
            */

            errorCode insert (int position, vectorType element) {
                // is position a valid index?
                if (position < 0 || position > __size__) { // allow size () so the insertion in an empty vector is possible
                    __vector_h_debug__ ("insert: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      throw OUT_OF_RANGE;
                    #endif                              
                    return OUT_OF_RANGE;
                }
      
                // try 2 faster options first
                if (position >= __size__)               return push_back (element);
                if (position == 0)                      return push_front (element);

                // do we have to resize the space occupied by existing the elements? This is the slowest option
                if (__capacity__ < __size__ + 1) {
                    errorCode e = __changeCapacity__ (__size__ + __increment__, -1, position);
                    if (e != 0 /* vector::OK */)        return e;
                    // else
                    __elements__ [position] = element;  return OK; 
                }
      
                // we have to reposition the elements, weather from the __front__ or from the calculated back, whichever is faster
      
                if (position < __size__ - position) {
                    // move elements form 0 to position 1 position down
                    __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                    __size__ ++;
                    int e1 = __front__;
                    for (int i = 0; i < position; i++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;
                    return OK;
                } else {
                    // move elements from __size__ - 1 to position 1 position up
                    int back = (__front__ + __size__) % __capacity__; // calculated back + 1
                    __size__ ++;
                    int e1 = back;
                    for (int i = __size__ - 1; i > position; i--) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e2 = e1 - 1
                        __elements__ [e1] = __elements__ [e2];
                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;        
                    return OK;
                }
            }


           /*
            *  Sorts vector elements in accending order using heap sort algorithm
            *
            *  For heap sort algorithm description please see: https://www.geeksforgeeks.org/heap-sort/ and https://builtin.com/data-science/heap-sort
            *  
            *  It works for almost of the data types as long as operator < is provided.
            */

            void sort () { 

                // build heap (rearrange array)
                for (int i = __size__ / 2 - 1; i >= 0; i --) {

                    // heapify i .. n
                    int j = i;
                    do {
                        int largest = j;        // initialize largest as root
                        int left = 2 * j + 1;   // left = 2 * j + 1
                        int right = 2 * j + 2;  // right = 2 * j + 2
                        
                        if (left < __size__ && at (largest) < at (left)) largest = left;     // if left child is larger than root
                        if (right < __size__ && at (largest) < at (right)) largest = right;  // if right child is larger than largest so far
                        
                        if (largest != j) {     // if largest is not root
                            // swap arr [j] and arr [largest]
                            vectorType tmp = at (j); at (j) = at (largest); at (largest) = tmp;
                            // heapify the affected subtree in the next iteration
                            j = largest;
                        } else {
                            break;
                        }
                    } while (true);

                }
                
                // one by one extract an element from heap
                for (int i = __size__ - 1; i > 0; i --) {
            
                    // move current root to end
                    vectorType tmp = at (0); at (0) = at (i); at (i) = tmp;

                    // heapify the reduced heap 0 .. i
                    int j = 0;
                    do {
                        int largest = j;        // initialize largest as root
                        int left = 2 * j + 1;   // left = 2*j + 1
                        int right = 2 * j + 2;  // right = 2*j + 2
                        
                        if (left < i && at (largest) < at (left)) largest = left;     // if left child is larger than root
                        if (right < i && at (largest) < at (right)) largest = right;  // if right child is larger than largest so far
                        
                        if (largest != j) {     // if largest is not root
                            // swap arr [j] and arr [largest]
                            vectorType tmp = at (j); at (j) = at (largest); at (largest) = tmp;
                            // heapify the affected subtree in the next iteration
                            j = largest;
                        } else {
                            break;
                        }
                    } while (true);            
        
                }        

            }


           /*
            *  Iterator is needed in order for standard C++ for each loop to work. 
            *  A good source for iterators is: https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
            *  
            *  Example:
            *  
            *    for (auto element: A) 
            *      Serial.println (element);
            */
      
            class Iterator {
              public:
                          
                // constructor
                Iterator (vector* vect) { __vector__ = vect; }
                
                // * operator
                vectorType& operator *() const { return __vector__->at (__position__); }
            
                // ++ (prefix) increment
                Iterator& operator ++ () { __position__ ++; return *this; }
      
                // C++ will stop iterating when != operator returns false, this is when __position__ counts to vector.size ()
                friend bool operator != (const Iterator& a, const Iterator& b) { return a.__position__ != a.__vector__->size (); }
        
            private:
      
                vector* __vector__;
                int __position__ = 0;
                
            };      
      
            Iterator begin () { return Iterator (this); }  
            Iterator end () { return Iterator (this); }


            #ifdef __VECTOR_H_DEBUG__
                // displays vector's internal structure
                void debug () {
                    Serial.printf ("\n\n"
                                  "__elements__  = %p\n"
                                  "__capacity__  = %i\n"
                                  "__increment__ = %i\n"
                                  "__size__      = %i\n"
                                  "__front__     = %i\n\n",
                                  __elements__, __capacity__, __increment__, __size__, __front__);
                    for (int i = 0; i < __capacity__; i++) {
                        bool used = __front__ + __size__ <= __capacity__ 
                                  ? i >= __front__ && i < __front__ + __size__
                                  : i >= __front__ || i < (__front__ + __size__) % __capacity__;
                        
                        Serial.printf ("   vector [%i]", i);  if (used) { Serial.print (" = "); Serial.print (__elements__ [i]); }
                                                              else        Serial.print (" --- not used ---");
                        if (i == __front__ && __size__ > 0)                 Serial.printf (" <- __front__");
                        if (i == (__front__ + __size__ - 1) % __capacity__) Serial.printf (" <- (calculated back)");
                        Serial.printf ("\n");
                    }
                    Serial.printf ("\n");
                }
              #endif


      private:

            vectorType *__elements__ = NULL;  // initially the vector has no elements, __elements__ buffer is empty
            int __capacity__ = 0;             // initial number of elements (or not occupied slots) in __elements__
            int __increment__ = 1;            // by default, increment elements buffer for one element when needed
            int __reservation__ = 0;          // no memory reservatio by default
            int __size__ = 0;                 // initially there are not elements in __elements__
            int __front__ = 0;                // points to the first element in __elements__, which do not exist yet at instance creation time

    
           /*
            *  Resizes __elements__ to new capacity with the option of deleting and adding an element meanwhile
            *  
            *  Returns true if succeeds and false in case of error:
            *    - could not allocate enough memory for requested storage
            */

            errorCode __changeCapacity__ (int newCapacity, int deleteElementAtPosition = -1, int leaveFreeSlotAtPosition = -1) {
                if (newCapacity < __reservation__) newCapacity = __reservation__;
                if (newCapacity == 0) {
                    // delete old buffer
                    if (__elements__ != NULL) delete [] __elements__;
                    
                    // update internal variables
                    __capacity__ = 0;
                    __elements__ = NULL;
                    __size__ = 0;
                    __front__ = 0;
                    return OK;
                } 
                // else
                
                #ifdef __VECTOR_H_EXCEPTIONS__
                    vectorType *newElements = new vectorType [newCapacity]; // allocate space for newCapacity elements
                #else
                    vectorType *newElements = new (std::nothrow) vectorType [newCapacity]; // allocate space for newCapacity elements
                #endif
                
                if (newElements == NULL) {
                    __vector_h_debug__ ("__changeCapacity__: couldn't resize internal storage: BAD_ALLOC.");
                    return BAD_ALLOC;
                }
                
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
                if (__elements__ != NULL) delete [] __elements__;
                
                // update internal variables
                __capacity__ = newCapacity;
                __elements__ = newElements;
                __front__ = 0;  // the first element is now aligned with 0
                return OK;
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


    template <> class vector <String> {

        public:
            
            /*
            * Error handling 
            * 
            * Some functions will return an error code or OK. 
            * 
            * They will also set lastErrorCode variable which will hold this value until it is reset (by assigning lastErrorCode = OK)
            * so it is not necessary to check success of each single operation in the code. 
            */
            
            enum errorCode {OK = 0, // not all error codes are needed here but they are shared among keyValuePairs and persistentKeyValuePairs as well 
                            NOT_FOUND = -1,               // key is not found
                            BAD_ALLOC = -2,               // out of memory
                            OUT_OF_RANGE = -3,            // invalid index
                            NOT_UNIQUE = -4,              // the key is not unique
                            DATA_CHANGED = -5,            // unexpected data value found
                            FILE_IO_ERROR = -6,           // file operation error
                            NOT_WHILE_ITERATING = -7,     // operation can not be berformed while iterating
                            DATA_ALREADY_LOADED = -8      // can't load the data if it is already loaded 
            }; // note that all errors are negative numbers

            char *errorCodeText (int e) {
                switch (e) {
                    case OK:                  return (char *) "OK";
                    case NOT_FOUND:           return (char *) "NOT_FOUND";
                    case BAD_ALLOC:           return (char *) "BAD_ALLOC";
                    case OUT_OF_RANGE:        return (char *) "OUT_OF_RANGE";
                    case NOT_UNIQUE:          return (char *) "NOT_UNIQUE";
                    case DATA_CHANGED:        return (char *) "DATA_CHANGED";
                    case FILE_IO_ERROR:       return (char *) "FILE_IO_ERROR";
                    case NOT_WHILE_ITERATING: return (char *) "NOT_WHILE_ITERATING";
                    case DATA_ALREADY_LOADED: return (char *) "DATA_ALREADY_LOADED";
                }
                return NULL; // doesn't happen
            }
            
            errorCode lastErrorCode = OK;

            void clearLastErrorCode () { lastErrorCode = OK; }


           /*
            *  Constructor of vector with no elements allows the following kinds of creation of vectors: 
            *  
            *    vector<String> A;
            *    vector<String> B ("10"); // with increment of 10 elements when vector grows, to reduce how many times __elements__ will be resized (which is time consuming)
            *    vector<String> C = { "100" };
            */
            
            vector (int increment = 1) { __increment__ = increment < 1 ? 1 : increment; }
      
      
           /*
            *  Constructor of vector from brace enclosed initializer list allows the following kinds of creation of vectors: 
            *  
            *     vector<String> D = { "200", "300", "400" };
            *     vector<String> E ( { "500", "600" } );
            */
      
            vector (std::initializer_list<String> il) {
                if (reserve (il.size ()) != OK) {
                    __vector_h_debug__ ("<String> constructor from brace enclosed initializer list - out of memory.");
                    return;
                }

                for (auto i: il) {
                    if (!i) { lastErrorCode = BAD_ALLOC; return; } // String template specialization
                    push_back (i);
                }                  
            }
      
            
           /*
            * Vector destructor - free the memory occupied by vector elements
            */
            
            ~vector () { if (__elements__ != NULL) delete [] __elements__; }


           /*
            * Returns the number of elements in vector.
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
            *  Returns OK if succeeds and errorCode in case of error:
            *    - requested capacity is less than current vector size
            *    - could not allocate enough memory for requested storage
            */
        
            errorCode reserve (int newCapacity) {
                if (newCapacity < __size__) {
                    __vector_h_debug__ ("<String>.reserve: can't change capacity without loosing some elements: BAD_ALLOC.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw BAD_ALLOC;
                    #endif                      
                    return lastErrorCode = BAD_ALLOC;
                }
                if (newCapacity > __size__) {
                    errorCode e = __changeCapacity__ (newCapacity);
                    if (e < OK) {
                        __vector_h_debug__ ("<String>.reserve: - out of memory: BAD_ALLOC.");
                        return lastErrorCode = e;
                    }
                }
                __reservation__ = newCapacity;              
                return OK; // no change in capacity is needed
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
                    lastErrorCode = OUT_OF_RANGE;
                    __vector_h_debug__ ("<String>.operator []: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw OUT_OF_RANGE;
                    #endif                      
                }
                return __elements__ [(__front__ + position) % __capacity__];
            }

    
           /*
            *  Same as [] operator, so it is not really needed but added here because it is supported in standard C++ vectors
            */      

            String &at (int position) {
                if (position < 0 || position >= __size__) {
                    lastErrorCode = OUT_OF_RANGE;
                    __vector_h_debug__ ("<String>.at: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw OUT_OF_RANGE;
                    #endif                      
                }          
                return __elements__ [(__front__ + position) % __capacity__];
            }


           /*
            *  Copy-constructor of vector allows the following kinds of creation of vectors: 
            *  
            *     vector<String> F = E;
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            *  
            *  Calling program should check lastErrorCode member variable after constructor is beeing called for possible errors
            */
      
            vector (vector& other) {
                if (this->reserve (other.size ()) != OK) {
                    __vector_h_debug__ ("<String> copy-constructor - out of memory.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw lastErrorCode;
                    #endif                      
                    return; // prevent resizing __elements__ for each element beeing pushed back
                }

                // copy other's elements - storage will not get resized meanwhile
                for (auto e: other) {
                    if (!e) { lastErrorCode = BAD_ALLOC; return; } // String template specialization
                    this->push_back (e);
                }                  
            }


           /*
            *  Assignment operator of vector allows the following kinds of assignements of vectors: 
            *  
            *     vector<String> F;
            *     F = { "1", "2", "3" }; or F = {};
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            */
      
            vector* operator = (vector other) {
                this->clear (); // clear existing elements if needed
                if (this->reserve (other.size ()) != OK) {
                    __vector_h_debug__ ("<String>.operator = - out of memory.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                        throw lastErrorCode;
                    #endif                      
                    return this; // prevent resizing __elements__ for each element beeing pushed back
                }
                // copy other's elements - storege will not get resized meanwhile
                for (auto e: other) {
                    if (!e) { lastErrorCode = BAD_ALLOC; return this; } // String template specialization
                    this->push_back (e);
                }
                return this;
            }

      
           /*
            * == operator allows comparison of vectors, like:
            * 
            *  Serial.println (F == E ? "vectors are equal" : "vectors are different");
            */
      
            bool operator == (vector& other) {
              if (other.lastErrorCode != other.OK) { lastErrorCode = BAD_ALLOC; return false; } // String template specialization
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
            *  Returns OK if succeeds and errorCode in case of error:
            *    - could not allocate enough memory for requested storage
            */
    
            errorCode push_back (String element) {
                if (!element) return lastErrorCode = BAD_ALLOC; // String template specialization
                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    errorCode e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e != OK) {
                        __vector_h_debug__ ("<String>.push_back: " + String (e));
                        #ifdef __VECTOR_H_EXCEPTIONS__
                            throw e;
                        #endif                                                                    
                        lastErrorCode = e;
                        return e;
                    }
                }          
        
                // add the new element at the end = (__front__ + __size__) % __capacity__, at this point we can be sure that there is enough __capacity__ of __elements__

                // String template specialization {
                    // __elements__ [(__front__ + __size__) % __capacity__] = element;
                    __swapStrings__ (&__elements__ [(__front__ + __size__) % __capacity__], &element);
                // } String template specialization

                //#endif
                __size__ ++;
                return OK;
            }

           /*
            * push_front (unlike push_back) is not a standard C++ vector member function
            */
              
            errorCode push_front (String element) {
                if (!element) return lastErrorCode = BAD_ALLOC; // String template specialization            
                // do we have to resize __elements__ first?
                if (__size__ == __capacity__) {
                    errorCode e = __changeCapacity__ (__capacity__ + __increment__);
                    if (e != OK) {
                        __vector_h_debug__ ("<String>.push_front: " + String (e));
                        #ifdef __VECTOR_H_EXCEPTIONS__
                            throw e;
                        #endif                      
                        lastErrorCode = e;
                        return e;
                    }
                }
        
                // add the new element at the beginning, at this point we can be sure that there is enough __capacity__ of __elements__
                __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                // String template specialization {
                    // __elements__ [__front__] = element;
                    __swapStrings__ (&__elements__ [__front__], &element);
                // } String template specialization

                __size__ ++;
                return OK;
            }


           /*
            *  Deletes last element from the end of a vector, like:
            *  
            *    E.pop_back ();
            *    
            *  Returns OK if succeeds and errorCode in case of error:
            *    - element does't exist
            */
      
            errorCode pop_back () {
                if (__size__ == 0) {
                    lastErrorCode = OUT_OF_RANGE;
                    __vector_h_debug__ ("<String>.pop_back: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      if (__size__ == 0) throw OUT_OF_RANGE;
                    #endif          
                    return OUT_OF_RANGE;
                }
                
                // remove last element
                __size__ --;

                __elements__ [(__front__ + __size__) % __capacity__] = ""; // String template specialization

                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) __changeCapacity__ (__size__); // doesn't matter if it does't succeed, the element is deleted anyway
                return OK;
            }


           /*
            * pop_front (unlike pop_back) is not a standard C++ vector member function
            */
        
            errorCode pop_front () {
                if (__size__ == 0) {
                    __vector_h_debug__ ("<String>.pop_front: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      if (__size__ == 0) throw OUT_OF_RANGE;
                    #endif
                    return OUT_OF_RANGE;
                }

                // remove first element
                __elements__ [__front__] = ""; // String template specialization

                __front__ = (__front__ + 1) % __capacity__; // __front__ + 1
                __size__ --;
        
                // do we have to free the space occupied by deleted element?
                if (__capacity__ > __size__ + __increment__ - 1) __changeCapacity__ (__size__);  // doesn't matter if it does't succeed, the elekent is deleted anyway
                return OK;
            }

      
           /*
            *  Returns a position (index) of the first occurence of the element in the vector if it exists, -1 otherwise. Example:
            *  
            *  Serial.println (D.find ("400"));
            *  Serial.println (D.find ("500"));
            */
      
            int find (String element) {
                if (!element) return BAD_ALLOC; // String template specialization            
                int e = __front__;
                for (int i = 0; i < __size__; i++) {
                  if (__elements__ [e] == element) return i;
                  e = (e + 1) % __capacity__;
                }
                
                return -1;
            }


           /*
            *  Erases the element occupying the position from the vector
            *  
            *  Returns OK if succeeds and errorCode in case of error:
            *    - element does't exist
            *
            *  It doesn't matter if internal failed to resize, if the element can be deleted the function still returns OK.
            */
      
            errorCode erase (int position) {
                // is position a valid index?
                if (position < 0 || position >= __size__) {
                    __vector_h_debug__ ("<String>.erase: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      throw OUT_OF_RANGE;
                    #endif
                    return OUT_OF_RANGE;
                }
      
                // try 2 faster options first
                if (position == __size__ - 1)                         return pop_back ();
                if (position == 0)                                    return pop_front (); 
      
                // do we have to free the space occupied by the element to be deleted? This is the slowest option
                if (__capacity__ > __size__ - 1 + __increment__ - 1)  
                    if (__changeCapacity__ (__size__ - 1, position, - 1) == OK)
                        return OK;
                    // else (if failed to change capacity) proceeed
      
                // we have to reposition the elements, weather from the __front__ or from the calculate back, whichever is faster

                if (position < __size__ - position) {

                    // move all elements form position to 1
                    int e1 = (__front__ + position) % __capacity__;
                    for (int i = position; i > 0; i --) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e1 - 1
                        
                        // String template specialization {
                            // __elements__ [e1] = __elements__ [e2];
                            __swapStrings__ (&__elements__ [e1], &__elements__ [e2]);
                        // } String template specialization

                        e1 = e2;
                    }
                    // delete the first element now
                    return pop_front (); // tere is no reason why this wouldn't succeed now, so OK
                } else {
                    // move elements from __size__ - 1 to position
                    int e1 = (__front__ + position) % __capacity__; 
                    for (int i = position; i < __size__ - 1; i ++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1
                        __elements__ [e1] = __elements__ [e2];

                        // String template specialization {
                            // __elements__ [e1] = __elements__ [e2];
                            __swapStrings__ (&__elements__ [e1], &__elements__ [e2]);
                        // } String template specialization

                        e1 = e2;
                    }

                    // delete the last element now
                    return pop_back (); // tere is no reason why this wouldn't succeed now, so OK
                }
            }

      
           /*
            *  Inserts a new element at the position into the vector
            *  
            *  Returns OK succeeds or errorCode:
            *    - could not allocate enough memory for requested storage
            */

            errorCode insert (int position, String element) {
                if (!element) return lastErrorCode = BAD_ALLOC; // String template specialization            
                // is position a valid index?
                if (position < 0 || position > __size__) { // allow size () so the insertion in an empty vector is possible
                    __vector_h_debug__ ("<String>.insert: OUT_OF_RANGE.");
                    #ifdef __VECTOR_H_EXCEPTIONS__
                      throw OUT_OF_RANGE;
                    #endif
                    return OUT_OF_RANGE;
                }
      
                // try 2 faster options first
                if (position >= __size__)               return push_back (element); 
                if (position == 0)                      return push_front (element); 
                // do we have to resize the space occupied by existing the elements? This is the slowest option
                if (__capacity__ < __size__ + 1) {
                    errorCode e = __changeCapacity__ (__size__ + __increment__, -1, position);
                    if (e != 0 /* vector::OK */)        return e;
                    // else
                    __elements__ [position] = element;  return OK; 
                }
      
                // we have to reposition the elements, weather from the __front__ or from the calculated back, whichever is faster
                if (position < __size__ - position) {
                    // move elements form 0 to position 1 position down
                    __front__ = (__front__ + __capacity__ - 1) % __capacity__; // __front__ - 1
                    __size__ ++;
                    int e1 = __front__;
                    for (int i = 0; i < position; i++) {
                        int e2 = (e1 + 1) % __capacity__; // e2 = e1 + 1

                        // String template specialization {
                            // __elements__ [e1] = __elements__ [e2];
                            __swapStrings__ (&__elements__ [e1], &__elements__ [e2]);
                        // } String template specialization

                        e1 = e2;
                    }
                    // insert the new element now
                    __elements__ [e1] = element;
                    return OK;
                } else {
                    // move elements from __size__ - 1 to position 1 position up
                    int back = (__front__ + __size__) % __capacity__; // calculated back + 1
                    __size__ ++;
                    int e1 = back;
                    for (int i = __size__ - 1; i > position; i--) {
                        int e2 = (e1 + __capacity__ - 1) % __capacity__; // e2 = e1 - 1

                        // String template specialization {
                            // __elements__ [e1] = __elements__ [e2];
                            __swapStrings__ (&__elements__ [e1], &__elements__ [e2]);
                        // } String template specialization

                        e1 = e2;
                    }
                    // insert the new element now

                    // String template specialization {
                        // __elements__ [e1] = element;        
                        __swapStrings__ (&__elements__ [e1], &element);
                    // } String template specialization

                    return OK;
                }
            }


           /*
            *  Sorts vector elements in accending order using heap sort algorithm
            *
            *  For heap sort algorithm description please see: https://www.geeksforgeeks.org/heap-sort/ and https://builtin.com/data-science/heap-sort
            *  
            *  It works for almost of the data types as long as operator < is provided.
            */

            void sort () { 

                // build heap (rearrange array)
                for (int i = __size__ / 2 - 1; i >= 0; i --) {

                    // heapify i .. n
                    int j = i;
                    do {
                        int largest = j;        // initialize largest as root
                        int left = 2 * j + 1;   // left = 2 * j + 1
                        int right = 2 * j + 2;  // right = 2 * j + 2
                        
                        if (left < __size__ && at (largest) < at (left)) largest = left;     // if left child is larger than root
                        if (right < __size__ && at (largest) < at (right)) largest = right;  // if right child is larger than largest so far
                        
                        if (largest != j) {     // if largest is not root
                            // swap arr [j] and arr [largest]
                            __swapStrings__ (&at (j), &at (largest));
                            // heapify the affected subtree in the next iteration
                            j = largest;
                        } else {
                            break;
                        }
                    } while (true);

                }
                
                // one by one extract an element from heap
                for (int i = __size__ - 1; i > 0; i --) {
            
                    // move current root to end
                    __swapStrings__ (&at (0), &at (i));

                    // heapify the reduced heap 0 .. i
                    int j = 0;
                    do {
                        int largest = j;        // initialize largest as root
                        int left = 2 * j + 1;   // left = 2*j + 1
                        int right = 2 * j + 2;  // right = 2*j + 2
                        
                        if (left < i && at (largest) < at (left)) largest = left;     // if left child is larger than root
                        if (right < i && at (largest) < at (right)) largest = right;  // if right child is larger than largest so far
                        
                        if (largest != j) {     // if largest is not root
                            // swap arr [j] and arr [largest]
                            __swapStrings__ (&at (j), &at (largest));
                            // heapify the affected subtree in the next iteration
                            j = largest;
                        } else {
                            break;
                        }
                    } while (true);            
        
                }        

            }

            
           /*
            *  Iterator is needed in order for standard C++ for each loop to work. 
            *  A good source for iterators is: https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
            *  
            *  Example:
            *  
            *    for (auto element: A) 
            *      Serial.println (element);
            */
      
            class Iterator {
              public:
                          
                // constructor
                Iterator (vector* vect) { __vector__ = vect; }
                
                // * operator
                String& operator *() const { return __vector__->at (__position__); }
            
                // ++ (prefix) increment
                Iterator& operator ++ () { __position__ ++; return *this; }
      
                // C++ will stop iterating when != operator returns false, this is when __position__ counts to vector.size ()
                friend bool operator != (const Iterator& a, const Iterator& b) { return a.__position__ != a.__vector__->size (); }
        
            private:
      
                vector* __vector__;
                int __position__ = 0;
                
            };      
      
            Iterator begin () { return Iterator (this); }  
            Iterator end () { return Iterator (this); }


            #ifdef __VECTOR_H_DEBUG__
                // displays vector's internal structure
                void debug () {
                    Serial.printf ("\n\n"
                                  "__elements__  = %p\n"
                                  "__capacity__  = %i\n"
                                  "__increment__ = %i\n"
                                  "__size__      = %i\n"
                                  "__front__     = %i\n\n",
                                  __elements__, __capacity__, __increment__, __size__, __front__);
                    for (int i = 0; i < __capacity__; i++) {
                        bool used = __front__ + __size__ <= __capacity__ 
                                  ? i >= __front__ && i < __front__ + __size__
                                  : i >= __front__ || i < (__front__ + __size__) % __capacity__;
                        
                        Serial.printf ("   vector [%i]", i);  if (used) { Serial.print (" = "); Serial.print (__elements__ [i]); }
                                                              else        Serial.print (" --- not used ---");
                        if (i == __front__ && __size__ > 0)                 Serial.printf (" <- __front__");
                        if (i == (__front__ + __size__ - 1) % __capacity__) Serial.printf (" <- (calculated back)");
                        Serial.printf ("\n");
                    }
                    Serial.printf ("\n");
                }
              #endif

      private:

            String *__elements__ = NULL;      // initially the vector has no elements, __elements__ buffer is empty
            int __capacity__ = 0;             // initial number of elements (or not occupied slots) in __elements__
            int __increment__ = 1;            // by default, increment elements buffer for one element when needed
            int __reservation__ = 0;          // no memory reservatio by default
            int __size__ = 0;                 // initially there are not elements in __elements__
            int __front__ = 0;                // points to the first element in __elements__, which do not exist yet at instance creation time

    
           /*
            *  Resizes __elements__ to new capacity with the option of deleting and adding an element meanwhile
            *  
            *  Returns true if succeeds and false in case of error:
            *    - could not allocate enough memory for requested storage
            */

            errorCode __changeCapacity__ (int newCapacity, int deleteElementAtPosition = -1, int leaveFreeSlotAtPosition = -1) {
                if (newCapacity < __reservation__) newCapacity = __reservation__;
                if (newCapacity == 0) {
                    // delete old buffer
                    if (__elements__ != NULL) delete [] __elements__;
                    
                    // update internal variables
                    __capacity__ = 0;
                    __elements__ = NULL;
                    __size__ = 0;
                    __front__ = 0;
                    return OK;
                } 
                // else
                
                #ifdef __VECTOR_H_EXCEPTIONS__
                    String *newElements = new String [newCapacity]; // allocate space for newCapacity elements
                #else
                    String *newElements = new (std::nothrow) String [newCapacity]; // allocate space for newCapacity elements
                #endif
                
                if (newElements == NULL) {
                    __vector_h_debug__ ("<String>.__changeCapacity__: couldn't resize internal storage: BAD_ALLOC.");
                    return BAD_ALLOC;
                }
                
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
                    
                    // String template specialization {
                        // newElements [i] = __elements__ [e];
                        // we don't need to care of an error occured while creating a newElement String, we'll replace it with a valid __elements__ string anyway
                        __swapStrings__ (&newElements [i], &__elements__ [e]);
                    // } String template specialization

                    e = (e + 1) % __capacity__;
                }
                
                // delete the old elements' buffer
                if (__elements__ != NULL) delete [] __elements__;
                
                // update internal variables
                __capacity__ = newCapacity;
                __elements__ = newElements;
                __front__ = 0;  // the first element is now aligned with 0
                return OK;
            }

            // swap strings by swapping their stack memory so constructors doesn't get called and nothing can go wrong like running out of memory meanwhile 
            void __swapStrings__ (String *a, String *b) {
                char tmp [sizeof (String)];
                memcpy (&tmp, a, sizeof (String));
                memcpy (a, b, sizeof (String));
                memcpy (b, tmp, sizeof (String));
            }

    };

#endif
