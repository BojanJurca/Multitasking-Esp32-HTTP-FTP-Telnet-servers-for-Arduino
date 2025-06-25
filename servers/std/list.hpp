/*
 *  list.hpp for Arduino
 * 
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 *
 *  In order to save as much memory as possible, list is implemented with minimal functionality. The elements are single - linked, so same
 *  functions, like pop_back for example can not be efficiently implemented.
 * 
 *  May 22, 2025, Bojan Jurca, with great help of Microsoft Copilot regarding templates 
 * 
 */


#ifndef __LIST_HPP__
    #define __LIST_HPP__


    // ----- TUNNING PARAMETERS -----

    // #define __THROW_LIST_EXCEPTIONS__  // uncomment this line if you want list to throw exceptions


    // error flags: there is only one type of error flags that can be set: err_bad_alloc - please note that all errors are negative (char) numbers
    #define err_ok           ((signed char) 0b00000000) //    0 - no error 
    #define err_bad_alloc    ((signed char) 0b10000001) // -127 - out of memory
    #define err_out_of_range ((signed char) 0b10000010) // -126 - invalid addressing of list element


    // type of memory used
    #define HEAP_MEM 2
    #define PSRAM_MEM 3
    #ifndef LIST_MEMORY_TYPE 
        #define LIST_MEMORY_TYPE HEAP_MEM // use heap by default
    #endif


    template <class listType> class list {

        // make __mergeSort__ function (algorithm.hpp) a friend so it can access internal structure
        template <typename forwardIterator> 
        friend void __mergeSort__(forwardIterator, forwardIterator);      

        private: 

            signed char __errorFlags__ = 0;

            struct node_t {
                listType element;
                node_t *next;
            };


        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }


           /*
            *  Constructor of list with no elements allows the following kinds of creation of listss: 
            *  
            *    list<int> A;
            *    list<int> C = { 100 };
            */
            
            list () {}


            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                   /*
                    *  Constructor of list from brace enclosed initializer list allows the following kinds of creation of list: 
                    *  
                    *     list<int> D = { 200, 300, 400 };
                    *     list<int> E ( { 500, 600 } );
                    */
            
                    list (const std::initializer_list<listType>& il) {
                        for (auto element: il)
                            if (push_back (element)) // error
                                break;
                    }
            #endif
            // #else
                // constructor accepting the array by reference, since AVR boards do not support initializer lists (thanks for this solution to Microsot Copilot)
                template <int N>
                list (const listType (&array) [N]) {
                    for (int i = 0; i < N; ++i)
                        if (push_back (array [i])) // error
                            break;
                }

                /*
                // list (const list& l) {} // not used, but generates compile-time error if not declared                    

                // helper function for easier initialization (AVR boards)
                template <int N>
                list<listType> initializer_list (const listType (&array) [N]) {
                    return list<listType> (array);
                } 
                */               
            // #endif
      
                  
           /*
            * List destructor - free the memory occupied by list elements
            */
            
            ~list () {
                clear ();
            }


           /*
            * Returns the number of elements in the list.
            */

            int size () { return __size__; }


           /*
            * Checks if list is empty.
            */

            bool empty () { return __size__ == 0; }


           /*
            * Clears all the elements from the list.
            */


            void clear () { 
                node_t *p = __front__;
                while (p) {
                    node_t *q = p->next;

                    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                        p->~node_t ();
                    #endif
                    free (p);

                    p = q;
                }
                __front__ = NULL;
                __size__ = 0;
                clearErrorFlags ();
            } 


           /*
            *  Copy-constructor of list allows the following kinds of creation of list: 
            *  
            *     list<int> F = E;
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            *  
            *  Calling program should check errorFlags () after constructor is beeing called for possible errors
            */
      
            list (list& other) {
                // copy other's elements
                for (auto element: other)
                    if (this->push_back (element)) // error
                        break;
            }


           /*
            *  Assignment operator of list allows the following kinds of assignements of list: 
            *  
            *     list<int> F;
            *     F = { 1, 2, 3 }; or F = {};
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            */
      
            list* operator = (const list& other) {
                this->clear (); // clear existing elements if needed

                // copy other's elements
                for (auto element: other)
                    if (this->push_back (element)) // error
                        break;
                return this;
            }
      
          
           /*
            *  Adds element to the end of a list, like:
            *  
            *    E.push_back (700);
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */
    
            signed char push_back (const listType& element) {
                // allocate new memory for element
                #if LIST_MEMORY_TYPE == PSRAM_MEM
                    node_t *newNode = (node_t *) ps_malloc (sizeof (node_t));
                #else // use heap
                    node_t *newNode = (node_t *) malloc (sizeof (node_t));
                #endif
                if (newNode == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    return err_bad_alloc;
                }

                memset (newNode, 0, sizeof (node_t));
                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    new (newNode) node_t; 
                #endif

                // add the new element to the end
                newNode->element = element;
                newNode->next = NULL;
                if (__front__ == NULL)
                    __front__ = newNode;
                if (__back__ != NULL)
                    __back__->next = newNode;
                __back__ = newNode;
                
                __size__ ++;
                return err_ok;
            }


           /*
            *  Adds element to the beginning of a list, like:
            *  
            *    E.push_front (600);
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */
              
            signed char push_front (const listType& element) {
                // allocate new memory for element
                #if LIST_MEMORY_TYPE == PSRAM_MEM
                    node_t *newNode = (node_t *) ps_malloc (sizeof (node_t));
                #else // use heap
                    node_t *newNode = (node_t *) malloc (sizeof (node_t));
                #endif
                if (newNode == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    return err_bad_alloc;
                }

                memset (newNode, 0, sizeof (node_t));
                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    new (newNode) node_t; 
                #endif

                // add the new element to the beginning
                newNode->element = element;
                newNode->next = __front__;
                __front__ = newNode;
                if (__back__ == NULL)
                    __back__ = newNode;
                
                __size__ ++;
                return err_ok;              
            }


           /*
            * Removes the first element from the list.
            */
        
            signed char pop_front () {
                if (__front__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    return err_out_of_range;
                }

                // remove the first element
                node_t *tmp = __front__;
                __front__ = __front__->next;
                __size__ --;
        
                // free the space occupied by the deleted element
                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    tmp->~node_t ();
                #endif
                free (tmp);
                return err_ok;
            }


           /*
            * Returns the reference to the first element of the list.
            */
        
            listType& front () {
                if (__front__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static listType ev = {};
                    return ev;
                }
                return __front__->element;
            }


           /*
            * Returns the reference to the last element of the list.
            */
        
            listType& back () {
                if (__back__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static listType ev = {};
                    return ev;
                }
                return __back__->element;
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
              
                // make __mergeSort__ function (algorithm.hpp) a friend so it can access internal structure
                template <typename forwardIterator> 
                friend void __mergeSort__(forwardIterator, forwardIterator);
                
                public:

                    // constructor
                    iterator (node_t *p) { 
                      if (p)
                        __p__ = p; 
                    }
                    
                    // * operator
                    listType& operator *() const { return __p__->element; }

                    // ++ (prefix) increment
                    iterator& operator ++ () { __p__ = __p__->next; return *this; }
                
                    // C++ will stop iterating when != operator returns false
                    friend bool operator != (const iterator& a, const iterator& b) { return a.__p__ != b.__p__; }
                    friend bool operator == (const iterator& a, const iterator& b) { return a.__p__ == b.__p__; }

                    // this will tell if iterator is valid (if there are not elements the iterator can not be valid)
                    operator bool () const { return __p__ != NULL; }

                private:

                    node_t * __p__ = NULL;

            };

            iterator begin () { return iterator (__front__); }              // first element
            iterator end () { return iterator (NULL); }                     // past the last element


           /*
            *  Define operator (pop_front - push_back) that will be used for (merge) sorting
            *     Since we are only switching the pointers no error can occur 
            */

            /*
            void operator >> (list& other) {

                if (this->__front__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    this->__errorFlags__ |= err_out_of_range;
                    other.__errorFlags__ |= err_out_of_range;
                    return;
                }

                // remove the first element from "this"
                node_t *tmp = this->__front__;
                this->__front__ = this->__front__->next;
                this->__size__ --;

                // add it as the last element to "other"
                tmp->next = NULL;
                if (other.__front__ == NULL)
                    other.__front__ = tmp;
                if (other.__back__ != NULL)
                    other.__back__->next = tmp;
                other.__back__ = tmp;
                
                other.__size__ ++;
            }
            */

            // print list to ostream
            friend ostream& operator << (ostream& os, list& l) {
                os << "FRONT→";
                for (auto e : l)
                    os << e << "→";
                os << "NULL";
                return os;
            }


      private:

            node_t *__front__ = NULL;         // points to the first list node, initially the list has no elements
            node_t *__back__ = NULL;          // points to the last list node, initially the list has no elements
            int __size__ = 0;                 // initially there are not elements in the list

    };


   /*
    * Arduino String list template specialization (a good source for template specialization: https://www.cprogramming.com/tutorial/template_specialization.html)
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

    template <> class list<String> {

        // make __mergeSort__ function (algorithm.hpp) a friend so it can access internal structure
        template <typename forwardIterator> 
        friend void __mergeSort__(forwardIterator, forwardIterator);      

        private: 

            signed char __errorFlags__ = 0;

            struct node_t {
                String element;
                node_t *next;
            };


        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }


           /*
            *  Constructor of list with no elements allows the following kinds of creation of listss: 
            *  
            *    list<String> A;
            *    list<String> C = { "hundret" };
            */
            
            list () {}


            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                   /*
                    *  Constructor of list from brace enclosed initializer list allows the following kinds of creation of list: 
                    *  
                    *     list<int> D = { "200", "300", "400" };
                    *     list<int> E ( { "500", "600" } );
                    */

                    list (std::initializer_list<String> il) {
                        for (String element: il) {
                            if (!element) {                             // ... check if parameter construction is valid
                                #ifdef __THROW_LIST_EXCEPTIONS__
                                    throw err_bad_alloc;
                                #endif
                                __errorFlags__ |= err_bad_alloc;       // report error if it is not
                                break;
                            }                          
                            if (push_back (element)) // error
                                break;
                        }
                    }
            #endif
            // #else
                // constructor accepting the array by reference, since AVR boards do not support initializer lists (thanks for this solution to Microsot Copilot)
                template <int N>
                list (const String (&array) [N]) {
                    for (int i = 0; i < N; ++i) {
                        if (!array [i]) {                             // ... check if parameter construction is valid
                            #ifdef __THROW_LIST_EXCEPTIONS__
                                throw err_bad_alloc;
                            #endif
                            __errorFlags__ |= err_bad_alloc;
                        }

                        if (push_back (array [i])) // error
                            break;
                    }
                }

                /*
                // list (const list& l) {} 

                // helper function for easier initialization (AVR boards)
                template <int N>
                list<String> initializer_list (const String (&array) [N]) {
                    return list<String> (array);
                } 
                */                        
            // #endif
      
                  
           /*
            * List destructor - free the memory occupied by list elements
            */
            
            ~list () {
                clear ();
            }


           /*
            * Returns the number of elements in the list.
            */

            int size () { return __size__; }


           /*
            * Checks if list is empty.
            */

            bool empty () { return __size__ == 0; }


           /*
            * Clears all the elements from the list.
            */

            void clear () { 
                node_t *p = __front__;
                while (p) {
                    node_t *q = p->next;

                    /*
                    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                        p->~node_t ();
                    #endif
                    free (p);
                    */
                    delete p; // internally calls destructor and free

                    p = q;
                }
                __front__ = NULL;

                __size__ = 0;
                clearErrorFlags ();
            } 


           /*
            *  Copy-constructor of list allows the following kinds of creation of list: 
            *  
            *     list<int> F = E;
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            *  
            *  Calling program should check errorFlags () after constructor is beeing called for possible errors
            */
      
            list (list& other) {
                // copy other's elements
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
            *  Assignment operator of list allows the following kinds of assignements of list: 
            *  
            *     list<String> F;
            *     F = { "1", "2", "3" }; or F = {};
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            */
      
            list* operator = (list other) {
                this->clear (); // clear existing elements if needed

                // copy other's elements
                for (String element: other) {
                    if (!element) {                             // ... check if parameter construction is valid
                        #ifdef __THROW_LIST_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;       // report error if it is not
                        return this;
                    }
                    if (this->push_back (element)) // error
                        break;
                }
                return this;
            }
      
          
           /*
            *  Adds element to the end of a list, like:
            *  
            *    E.push_back ("700");
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */
    
            signed char push_back (String element) {
                if (!element) {                             // ... check if parameter construction is valid
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;       // report error if it is not
                    return err_bad_alloc;
                }

                // allocate new memory for element
                #if LIST_MEMORY_TYPE == PSRAM_MEM
                    node_t *newNode = (node_t *) ps_malloc (sizeof (node_t));
                #else // use heap
                    node_t *newNode = (node_t *) malloc (sizeof (node_t));
                #endif
                if (newNode == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    return err_bad_alloc;
                }

                memset (newNode, 0, sizeof (node_t));
                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    new (newNode) node_t; 
                #endif

                // add the new element to the end
                __swapStrings__ (&newNode->element, &element);
                newNode->next = NULL;
                if (__front__ == NULL)
                    __front__ = newNode;
                if (__back__ != NULL)
                    __back__->next = newNode;
                __back__ = newNode;
                
                __size__ ++;
                return err_ok;
            }


           /*
            *  Adds element to the beginning of a list, like:
            *  
            *    E.push_front ("600");
            *    
            *  Returns OK or one of the error flags in case of error:
            *    - could not allocate enough memory for requested storage
            */
              
            signed char push_front (String element) {
                if (!element) {                             // ... check if parameter construction is valid
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;       // report error if it is not
                    return err_bad_alloc;
                }

                // allocate new memory for element
                #if LIST_MEMORY_TYPE == PSRAM_MEM
                    node_t *newNode = (node_t *) ps_malloc (sizeof (node_t));
                #else // use heap
                    node_t *newNode = (node_t *) malloc (sizeof (node_t));
                #endif
                if (newNode == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    return err_bad_alloc;
                }

                memset (newNode, 0, sizeof (node_t));
                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    new (newNode) node_t; 
                #endif

                // add the new element to the beginning
                __swapStrings__ (&newNode->element, &element);
                newNode->next = __front__;
                __front__ = newNode;
                if (__back__ == NULL)
                    __back__ = newNode;
                
                __size__ ++;
                return err_ok;              
            }


           /*
            * Removes the first element from the list.
            */
        
            signed char pop_front () {
                if (__front__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    return err_out_of_range;
                }

                // remove the first element
                node_t *tmp = __front__;
                __front__ = __front__->next;
                __size__ --;
        
                // free the space occupied by the deleted element
                /*
                #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                    tmp->~node_t ();
                #endif
                free (tmp);
                */
                delete tmp; // internally calls destructor and free

                return err_ok;
            }


           /*
            * Returns the reference to the first element of the list.
            */
        
            String& front () {
                if (__front__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static String ev = {};
                    return ev;
                }
                return __front__->element;
            }


           /*
            * Returns the reference to the last element of the list.
            */
        
            String& back () {
                if (__back__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    __errorFlags__ |= err_out_of_range;
                    static String ev = {};
                    return ev;
                }
                return __back__->element;
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

                // make __mergeSort__ function (algorithm.hpp) a friend so it can access internal structure
                template <typename forwardIterator> 
                friend void __mergeSort__(forwardIterator, forwardIterator);
                
                public:
                              
                    // constructor
                    iterator (node_t *p) { 
                      if (p)
                        __p__ = p; 
                    }
                    
                    // * operator
                    String& operator *() const { return __p__->element; }

                    // ++ (prefix) increment
                    iterator& operator ++ () { __p__ = __p__->next; return *this; }
                
                    // C++ will stop iterating when != operator returns false
                    friend bool operator != (const iterator& a, const iterator& b) { return a.__p__ != b.__p__; }
                    friend bool operator == (const iterator& a, const iterator& b) { return a.__p__ == b.__p__; }

                private:

                    node_t * __p__ = NULL;

            };

            iterator begin () { return iterator (__front__); }              // first element
            iterator end () { return iterator (NULL); }                     // past the last element


           /*
            *  Define operator (pop_front - push_back) that will be used for (merge) sorting
            *     Since we are only switching the pointers no error can occur 
            */

            /*
            void operator >> (list& other) {

                if (this->__front__ == NULL) {
                    #ifdef __THROW_LIST_EXCEPTIONS__
                        throw err_out_of_range;
                    #endif          
                    this->__errorFlags__ |= err_out_of_range;
                    other.__errorFlags__ |= err_out_of_range;
                    return;
                }

                // remove the first element from "this"
                node_t *tmp = this->__front__;
                this->__front__ = this->__front__->next;
                this->__size__ --;

                // add it as the last element to "other"
                tmp->next = NULL;
                if (other.__front__ == NULL)
                    other.__front__ = tmp;
                if (other.__back__ != NULL)
                    other.__back__->next = tmp;
                other.__back__ = tmp;
                
                other.__size__ ++;
            }
            */

            // print list to ostream
            friend ostream& operator << (ostream& os, list& l) {
                os << "FRONT→";
                for (auto e : l)
                    os << e << "→";
                os << "NULL";
                return os;
            }

      private:

            node_t *__front__ = NULL;         // points to the first list node, initially the list has no elements
            node_t *__back__ = NULL;          // points to the last list node, initially the list has no elements
            int __size__ = 0;                 // initially there are not elements in the list

            // swap strings by swapping their stack memory so constructors doesn't get called and nothing can go wrong like running out of memory meanwhile 
            void __swapStrings__ (String *a, String *b) {
                char tmp [sizeof (String)];
                memcpy (&tmp, a, sizeof (String));
                memcpy (a, b, sizeof (String));
                memcpy (b, tmp, sizeof (String));
            }

    };


    /*
    #ifdef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
        // Helper function to create aray from an initializer list
        template<typename T, size_t N>
        list<T> initializer_list (const T (&arr) [N]) {
            return list<T> (arr);
        }
    #endif
    */

#endif
