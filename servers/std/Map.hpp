/*
 *  Map.hpp for Arduino
 * 
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 * 
 *  The data storage is internaly implemented as balanced binary search tree for good searching performance.
 *
 *  Map functions are not thread-safe.
 * 
 *  October 23, 2024, Bojan Jurca
 *  
 */


#ifndef __MAP_HPP__
    #define __MAP_HPP__

    // ----- TUNNING PARAMETERS -----

    #define __MAP_MAX_STACK_SIZE__ 32 // statically allocated stack needed for iterating through elements, 24 should be enough for the number of elemetns that fit into ESP32's memory

    // #define __USE_MAP_EXCEPTIONS__   // uncomment this line if you want Map to throw exceptions


    // error flags: there are only two types of error flags that can be set: OVERFLOW and OUT_OF_RANGE - please note that all errors are negative (char) numbers
    #define err_ok              ((signed char) 0b00000000)  //    0 - no error
    #define err_bad_alloc       ((signed char) 0b10000001)  // -127 - out of memory
    #define err_not_found       ((signed char) 0b10000100)  // -124 - key is not found
    #define err_not_unique      ((signed char) 0b10001000)  // -120 - key is not unique


    // type of memory used
    #define HEAP_MEM 2
    #define PSRAM_MEM 3
    #ifndef MAP_MEMORY_TYPE 
        #define MAP_MEMORY_TYPE HEAP_MEM // use heap by default
    #endif


    template <class keyType, class valueType> class Map {

        private: 

            signed char __errorFlags__ = 0;


        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }


            struct Pair {
                keyType first;          // node key
                valueType second;       // node value
            };


           /*
            *  Constructor of Map with no pairs allows the following kinds of creation od Map pairs: 
            *  
            *    Map<int, String> mpA;
            */
            
            Map () {}

    
          #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
               /*
                *  Constructor of Map from brace enclosed initializer list allows the following kinds of creation of Map pairs: 
                *  
                *     Map<int, String> mpB = { {1, "one"}, {2, "two"} };
                *     Map<int, String> mpC ( { {1, "one"}, {2, "two"} } );
                */
          
                Map (std::initializer_list<Pair> il) {
                    for (auto i: il) {

                        if (is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &i.first) {                           // ... check if parameter construction is valid
                                // log_e ("BAD_ALLOC");
                                __errorFlags__ = err_bad_alloc;         // report error if it is not
                                return;
                            }

                        if (is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &i.second) {                         // ... check if parameter construction is valid
                                // log_e ("BAD_ALLOC");
                                __errorFlags__ = err_bad_alloc;         // report error if it is not
                                return;
                            }

                        __balancedBinarySearchTreeNode__ *pInserted = NULL; 
                        signed char h = __insert__ (&__root__, i.first, i.second, &pInserted);
                        if  (h >= 0)  // OK, h contains the balanced binary search tree height 
                            __height__ = h;
                    }
                }
          #endif


           /*
            *  Map pairs destructor - free the memory occupied by pairs
            */
            
            ~Map () { __clear__ (&__root__); } // release memory occupied by balanced binary search tree


           /*
            *  Returns the number of pairs.
            */

            int size () { return __size__; }


           /*
            *  Returns the height of balanced binary search tree.
            */

            signed char height () { return __height__; }


           /*
            *  Checks if there are no pairs.
            */

            bool empty () { return __size__ == 0; }


           /*
            *  Clears all the elements from the balanced binary search tree.
            */

            void clear () { __clear__ (&__root__); } 


           /*
            *  Copy-constructor of Map pairs allows the following kinds of creation: 
            *  
            *     Map<int, String> mpD = mpC;
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            *  
            *  Calling program should check __errorFlags__ member variable after constructor is beeing called for possible errors
            */
      
            Map (Map& other) {
                // copy other's elements
                for (auto e: other) {
                    __balancedBinarySearchTreeNode__ *pInserted = NULL; 
                    int h = this->__insert__ (&__root__, e.first, e.second, &pInserted); if  (h >= 0) __height__ = h;
                }
                // copy the error flags as well
                __errorFlags__ |= other.__errorFlags__;
            }


            /*
            *  Assignment operator of Map pairs allows the following kinds of assignements: 
            *  
            *     Map<int, String> mpE;
            *     mpE = { {3, "tree"}, {4, "four"}, {5, "five"} }; or   mpE = { };
            *     
            *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
            */
      
            Map* operator = (Map other) {
                this->clear (); // clear existing pairs if needed

                // copy other's pairs
                for (auto e: other) {

                    if (is_same<keyType, String>::value)     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        if (!*(String *) &e.first) {                            // ... check if parameter construction is valid
                            // log_e ("BAD_ALLOC");
                            #ifdef __USE_MAP_EXCEPTIONS__
                                throw err_bad_alloc;
                            #endif
                            __errorFlags__ |= err_bad_alloc;          // report error if it is not
                            return this;
                        }

                    if (is_same<valueType, String>::value)   // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        if (!*(String *) &e.second) {                          // ... check if parameter construction is valid
                            // log_e ("BAD_ALLOC");
                            #ifdef __USE_MAP_EXCEPTIONS__
                                throw err_bad_alloc;
                            #endif
                            __errorFlags__ |= err_bad_alloc;          // report error if it is not
                            return this;
                        }

                    __balancedBinarySearchTreeNode__ *pInserted = NULL; 
                    int h = this->__insert__ (&__root__, e.first, e.second, &pInserted); if  (h >= 0) __height__ = h;
                }
                // copy the error flags as well
                __errorFlags__ |= other.__errorFlags__;

                return this;
            }


           /*
            *  [] operator enables Map elements to be conveniently addressed by their keys like:
            *
            *    value = mp [key];
            *       or
            *    mp [key] = value;
            *
            *  Operator searches for the node containing the key. It it can't be found it inserts a new pair into the Map.
            *  Error handling can be somewhat tricky. It may be a good idea to use __USE_MAP_EXCEPTIONS__ if using [] operator.
            */

            valueType &operator [] (keyType key) {
                static valueType dummyValue1 = {};
                static valueType dummyValue2 = {};

                if (is_same<keyType, String>::value)      // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {              // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;  // report error if it is not
                        dummyValue1 = dummyValue2;
                        return dummyValue1;               // operator must return a reference, so return the reference to dummy value (make a copy of the default value first)
                    }

                // find the right pair
                __balancedBinarySearchTreeNode__ *p = __root__;
                while (p != NULL) {
                    if (key < p->pair.first) 
                        p = p->leftSubtree;           // 1. case: continue searching in left subtree
                    else if (p->pair.first < key) 
                        p = p->rightSubtree;          // 2. case: continue searching in reight subtree
                    else {
                        return p->pair.second;        // 3. case: found, return the reference ot the value
                    }
                }
                // else                               // 4. case: not found, else insert a new pair
                __balancedBinarySearchTreeNode__ *pInserted = NULL; 
                signed char h = __insert__ (&__root__, key, dummyValue2, &pInserted);
                if  (h >= 0) {  // OK, h contains the balanced binary search tree height 
                    __height__ = h;
                    return pInserted->pair.second;
                }
                // else there was some other kind of error
                dummyValue1 = dummyValue2;
                return dummyValue1;                         // operator must return a reference, so return the reference to dummy value (make a copy of the default value first)
            }


           /*
            *  Erases the Map pair identified by key
            *  
            *  Returns OK if succeeds and error err_not_found or err_bad_alloc if String key parameter could not be constructed.
            */

            signed char erase (keyType key) { 

                if (is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                 // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;        // report error if it is not
                        return err_bad_alloc;                   // report error if it is not
                    }

                signed char h = __erase__ (&__root__, key); 
                if  (h >= 0) {  // OK, h contains the balanced binary search tree height 
                    __height__ = h;
                    __size__ --;
                    return err_ok;
                } else // h contains error flag
                    return h;
            }


           /*
            *  Inserts a new Map pair, returns OK or one of the errors.
            */

            signed char insert (Pair pair) { 

                if (is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &pair.first) {                        // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;        // report error if it is not
                        return err_bad_alloc;                   // report error if it is not
                    }

                if (is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &pair.second) {                      // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;        // report error if it is not
                        return err_bad_alloc;                   // report error if it is not
                    }

                __balancedBinarySearchTreeNode__ *pInserted = NULL; 
                int h = __insert__ (&__root__, pair.first, pair.second, &pInserted);                 
                if  (h >= 0) {  // OK, h contains the balanced binary search tree height 
                    __height__ = h;
                    return err_ok;
                } else
                    return h;
            }

            signed char insert (keyType key, valueType value) { 

                if (is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                             // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;        // report error if it is not
                        return err_bad_alloc;                   // report error if it is not
                    }

                if (is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &value) {               // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;        // report error if it is not
                        return err_bad_alloc;                   // report error if it is not
                    }

                __balancedBinarySearchTreeNode__ *pInserted = NULL;
                int h = __insert__ (&__root__, key, value, &pInserted);
                if  (h >= 0) {  // OK, h contains the balanced binary search tree height
                    __height__ = h;
                    return err_ok;
                } else
                    return h;
            }
    
        
            /*
            *   Iterator
            *   
            *   Example:
            *    for (auto pair: mp)
            *        Serial.println (String (pair.first) + "-" + String (pair.second));
            */
        
        private:
        
            struct __balancedBinarySearchTreeNode__;        // forward private declaration

        public:

            class iterator {

              public:

                // there are 2 cases when constructor gets called: begin (pointToFirstPair = true) and end (pointToFirstPair = false) 
                iterator (Map* mp, bool pointToFirstPair) {
                    __mp__ = mp;

                    if (!pointToFirstPair || __mp__->size () == 0) // when end () is beeing called, stack for balanced binary search tree iteration is not needed (yet)
                        return;
                    // else find the lowest pair in the balanced binary search tree (this would be the leftmost one) and fill the stack meanwhile

                    Map::__balancedBinarySearchTreeNode__* p = mp->__root__;
                    while (p) {
                        __stack__ [++ __stackPointer__] = p;                      
                        p = p->leftSubtree;
                    }
                    __lastVisitedPair__ = __stack__ [__stackPointer__]; // remember the last visited pair
                }

                // find the key, construct the stack meanwhile
                iterator (keyType key, Map* mp) {
                    __mp__ = mp;

                    if (__mp__->size () == 0)
                        return;
                    // else

                    Map::__balancedBinarySearchTreeNode__* p = mp->__root__;
                    while (p) {
                        __stack__ [++ __stackPointer__] = p;            

                        if (key < p->pair.first) 
                            p = p->leftSubtree;                                 // 1. case: continue searching in left subtree
                        else if (p->pair.first < key) 
                            p = p->rightSubtree;                                // 2. case: continue searching in reight subtree
                        else {
                            __lastVisitedPair__ = __stack__ [__stackPointer__]; // 3. case: found, return the reference ot the value,  remember the last visited pair
                            return;
                        }
                    }

                    __stackPointer__ = -1;                                      // 4. case: not found
                    __lastVisitedPair__ = NULL;
                }


                // * operator
                Pair& operator *() { return (__lastVisitedPair__->pair); }

                // -> operator
                Pair * operator -> () { return &(__lastVisitedPair__->pair); }

                // ++ (prefix) increment actually moves the state of the stack so that the last element points to the next balanced binary search tree node
                iterator& operator ++ () { 

                    // the current node is pointed to by stack pointer, move to the next node

                    // if the node has a right subtree find the leftmost element in the right subtree and fill the stack meanwhile
                    if (__stack__ [__stackPointer__]->rightSubtree != NULL) {
                        Map::__balancedBinarySearchTreeNode__* p = __stack__ [__stackPointer__]->rightSubtree;
                        if (p && p != __stack__ [__stackPointer__ + 1]) { // if the right subtree has not ben visited yet, proceed with the right subtree
                            while (p) {
                                __stack__ [++ __stackPointer__] = p;
                                p = p->leftSubtree;
                            }
                            __lastVisitedPair__ = __stack__ [__stackPointer__]; // remember the last visited pair
                            return *this;
                        }
                    }
                    // else proceed with climbing up the stack to the first pair that is greater than the current node
                    {
                        int8_t i = __stackPointer__;
                        if (-- __stackPointer__ >= 0)
                            while (__stackPointer__ >= 0 && __stack__ [__stackPointer__]->pair.first < __stack__ [i]->pair.first) 
                                __stackPointer__ --;

                        if (__stackPointer__ >= 0)
                          __lastVisitedPair__ = __stack__ [__stackPointer__]; // remember the last visited pair
                        else
                            __lastVisitedPair__ = NULL;

                        return *this;
                    }
                }  

                // -- (prefix) increment actually moves the state of the stack so that the last element points to the previous balanced binary search tree node
                iterator& operator -- () { 
                    if (__lastVisitedPair__ == NULL) { // we came here with -- end (), so the stack is not constructed yet, start with the last (rightmost) element
                        if (__mp__->size () == 0) return *this;
        
                        // find the highest pair in the balanced binary search tree (this would be the righttmost one) and fill the stack meanwhile
                        Map::__balancedBinarySearchTreeNode__* p = __mp__->__root__;
                        while (p) {
                            __stack__ [++ __stackPointer__] = p;                      
                            p = p->rightSubtree;
                        }
                        __lastVisitedPair__ = __stack__ [__stackPointer__]; // remember the last visited pair
                        return *this;
                    }

                    // the current node is pointed to by stack pointer, move to the previous node

                    // if the node has a left subtree find the rightmost element in the left subtree and fill the stack meanwhile
                    if (__stack__ [__stackPointer__]->leftSubtree != NULL) {
                        Map::__balancedBinarySearchTreeNode__* p = __stack__ [__stackPointer__]->leftSubtree;
                        if (p && p != __stack__ [__stackPointer__ + 1]) { // if the left subtree has not ben visited yet, proceed with the left subtree
                            while (p) {
                                __stack__ [++ __stackPointer__] = p;
                                p = p->rightSubtree;
                            }
                            __lastVisitedPair__ = __stack__ [__stackPointer__]; // remember the last visited pair
                            return *this; 
                        }
                    }
                    // else proceed with climbing up the stack to the first pair that is lesser than the current node
                    {
                        int8_t i = __stackPointer__;
                        if (-- __stackPointer__ >= 0)
                            while (__stackPointer__ >= 0 && __stack__ [__stackPointer__]->pair.first > __stack__ [i]->pair.first) 
                                __stackPointer__ --;

                        if (__stackPointer__ >= 0)
                          __lastVisitedPair__ = __stack__ [__stackPointer__]; // remember the last visited pair
                        else
                            __lastVisitedPair__ = NULL;

                        return *this;
                    }
                }

                // C++ will stop iterating when != operator returns false, this is when all nodes have been visited and stack pointer is negative
                friend bool operator != (const iterator& a, const iterator& b) { return a.__lastVisitedPair__ != b.__lastVisitedPair__; }
                friend bool operator == (const iterator& a, const iterator& b) { return a.__lastVisitedPair__ == b.__lastVisitedPair__; }

                // this will tell if iterator is valid (if there are not elements the iterator can not be valid)
                operator bool () const { return __mp__->size () > 0; }
            
            private:
            
                Map* __mp__ = NULL;

                // a stack is needed to iterate through binary balanced search tree nodes
                Map::__balancedBinarySearchTreeNode__ *__stack__ [__MAP_MAX_STACK_SIZE__] = {};
                int8_t __stackPointer__ = -1;
                Map::__balancedBinarySearchTreeNode__ *__lastVisitedPair__ = NULL;

            };      
  
            iterator begin () { return iterator (this, true); }
            iterator end ()   { return iterator (this, false); }


            /*
            *  Returns an iterator to the pair with the key, if key is found, end () if it is not. Example:
            *  
            *    auto it = mpB.find (1);
            *    if (it != mpB.end ()) 
            *        Serial.println (*it); 
            *    else 
            *        Serial.println ("not found");
            */
            
            iterator find (keyType key) {

                if (is_same<keyType, String>::value)      // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {              // ... check if parameter construction is valid
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;  // report error if it is not
                        return end ();
                    }

                return iterator (key, this);
            }


        private:
        
            // balanced binary search tree for keys
            
            struct __balancedBinarySearchTreeNode__ {
                Pair pair;
                __balancedBinarySearchTreeNode__ *leftSubtree;
                __balancedBinarySearchTreeNode__ *rightSubtree;
                int8_t leftSubtreeHeight;
                int8_t rightSubtreeHeight;
            };
    
            __balancedBinarySearchTreeNode__ *__root__ = NULL; 
            int __size__ = 0;
            int8_t __height__ = 0;

            // we need 2 values to handle err_not_found conditions with [] operator
            valueType __dummyValue1__ = {};            
            valueType __dummyValue2__ = {};          

            // Mega and Uno do no thave is_same implemented, so we have tio imelement it ourselves: https://stackoverflow.com/questions/15200516/compare-typedef-is-same-type
            template<typename T, typename U> struct is_same { static const bool value = false; };
            template<typename T> struct is_same<T, T> { static const bool value = true; };

            // internal functions
            
            signed char __insert__ (__balancedBinarySearchTreeNode__ **p, keyType& key, valueType& value, __balancedBinarySearchTreeNode__ **pInserted) { // returns the height of balanced binary search tree or error
                // 1. case: a leaf has been reached - add new node here
                if ((*p) == NULL) {
                    // log_i ("a leaf has been reached - add new node here");
                  
                    // different ways of allocation the memory for a new node
                    #if MAP_MEMORY_TYPE == PSRAM_MEM
                        __balancedBinarySearchTreeNode__ *n = (__balancedBinarySearchTreeNode__ *) ps_malloc (sizeof (__balancedBinarySearchTreeNode__));
                    #else
                        __balancedBinarySearchTreeNode__ *n = (__balancedBinarySearchTreeNode__ *) malloc (sizeof (__balancedBinarySearchTreeNode__));
                    #endif

                    if (n == NULL) {
                        // log_e ("BAD_ALLOC");
                        #ifdef __USE_MAP_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                    memset (n, 0, sizeof (__balancedBinarySearchTreeNode__)); // prevent caling String destructor at the following assignments
                    
                    *n = { {key, value}, NULL, NULL, 0, 0 };
                    *pInserted = n;

                        // in case of Strings - it is possible that key and value didn't get constructed, so just swap stack memory with parameters - this always succeeds
                        if (is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &n->pair.first)                       // ... check if parameter construction is valid
                                __swapStrings__ ((String *) &n->pair.first, (String *) &key); 
                        if (is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &n->pair.second)                     // ... check if parameter construction is valid
                                __swapStrings__ ((String *) &n->pair.second, (String *) &value);

                    *p = n;
                    __size__ ++;
                    return 1; // height of the (sub)tree so far
                }
                
                // 2. case: add a new node to the left subtree of the current node
                if (key < (*p)->pair.first) {
                    // log_i ("add a new node to the left subtree");

                    int h = __insert__ (&((*p)->leftSubtree), key, value, pInserted);
                    if (h < 0) return h; // < 0 means an error
                    (*p)->leftSubtreeHeight = h;
                    if ((*p)->leftSubtreeHeight - (*p)->rightSubtreeHeight > 1) {
                        /* the tree is unbalanced, left subtree is too high, perform right rotation
                                | = *p                 | = *p
                                Y                      X
                                / \                    / \
                              X   c       =>         a   Y
                              / \                        / \
                            a   b                      b   c
                        */
                        __balancedBinarySearchTreeNode__ *tmp = (*p)->leftSubtree;  // picture: tmp = X
                        (*p)->leftSubtree = tmp->rightSubtree;                      // picture: Y.leftSubtree = b
                        if (tmp->leftSubtree == NULL) { // the rightSubtree can not be NULL at this point otherwise tmp wouldn't be unbalanced
                            /* handle trivial case that happens at the leaves level and could preserve unbalance even after rotation
                                    | = tmp                | = tmp
                                    C                      C
                                    /                      /
                                  A           =>         B 
                                    \                    / 
                                    B                  A                           
                            */
                            tmp->rightSubtree->leftSubtree = tmp; 
                            tmp = tmp->rightSubtree;
                            tmp->leftSubtree->rightSubtree = NULL;
                            tmp->leftSubtree->rightSubtreeHeight = 0;
                            tmp->leftSubtreeHeight = 1;                
                        }
                        (*p)->leftSubtree = tmp->rightSubtree;
                        (*p)->leftSubtreeHeight = tmp->rightSubtreeHeight;          // correct the hight information of (picture) b branch
                        tmp->rightSubtree = (*p);                                   // picture: X.rightSubtree = Y
                        (*p) = tmp;                                                 // X becomes the new (subtree) root
                        // correct the height information of right subtree - we know that right subtree exists (picture: Y node)
                        (*p)->rightSubtreeHeight = max ((*p)->rightSubtree->leftSubtreeHeight, (*p)->rightSubtree->rightSubtreeHeight) + 1; // the height of (sub)tree after rotation
                    } 
                    return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1; // the new height of (sub)tree
                }             
    
                // 3. case: the node with the same values already exists 
                if (!((*p)->pair.first < key)) { // meaning at this point that key == (*p)->pair.first
                    // log_e ("NOT_UNIQUE");
                    #ifdef __USE_MAP_EXCEPTIONS__
                        throw err_not_unique;
                    #endif
                    __errorFlags__ |= err_not_unique;
                    return err_not_unique;
                }
        
                // 4. case: add a new node to the right subtree of the current node
                // log_i ("add a new node to the right subtree");

                int h = __insert__ (&((*p)->rightSubtree), key, value, pInserted);
                if (h < 0) return h; // < 0 means an error
                (*p)->rightSubtreeHeight = h;
                if ((*p)->rightSubtreeHeight - (*p)->leftSubtreeHeight > 1) {
                    /* the tree is unbalanced, right subtree is too high, perform left rotation
                            | = *p                 | = *p
                            X                      Y
                            / \                    / \
                          a   Y       =>         X   c
                              / \                / \
                            b   c              a   b
                    */
                    __balancedBinarySearchTreeNode__ *tmp = (*p)->rightSubtree; // picture: tmp = Y
                    (*p)->rightSubtree = tmp->leftSubtree;                      // picture: X.rightSubtree = b
                    if (tmp->rightSubtree == NULL) { // the leftSubtree can not be NULL at this point otherwise tmp wouldn't be unbalanced
                        /* handle trivial case that happens at the leaves level and could preserve unbalance even after rotation
                                | = tmp                | = tmp
                                A                      A
                                  \                      \
                                  C        =>            B 
                                  /                        \ 
                                B                          C                           
                        */
                        tmp->leftSubtree->rightSubtree = tmp; 
                        tmp = tmp->leftSubtree;
                        tmp->rightSubtree->leftSubtree = NULL;
                        tmp->rightSubtree->leftSubtreeHeight = 0;
                        tmp->rightSubtreeHeight = 1;
                    }
                    (*p)->rightSubtree = tmp->leftSubtree;
                    (*p)->rightSubtreeHeight = tmp->leftSubtreeHeight;  // correct the hight information of (picture) b branch
                    tmp->leftSubtree = (*p);                            // picture:Y.leftSubtree = X
                    (*p) = tmp;                                         // Y becomes the new (subtree) root
                    // correct the height information of left subtree - we know that left subtree exists (picture: X node)
                    (*p)->leftSubtreeHeight = max ((*p)->leftSubtree->leftSubtreeHeight, (*p)->leftSubtree->rightSubtreeHeight) + 1; // the height of (sub)tree after rotation
                } 
                return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1; // the new height of (sub)tree
            }
    
            signed char __erase__ (__balancedBinarySearchTreeNode__ **p, keyType& key) { // returns the height of balanced binary search tree or error
                // 1. case: a leaf has been reached - key was not found
                if ((*p) == NULL) {
                    // log_e ("NOT_FOUND");
                    #ifdef __USE_MAP_EXCEPTIONS__
                        throw err_not_found;
                    #endif                    
                    __errorFlags__ |= err_not_found;
                    return err_not_found; 
                }
                        
                // 2. case: delete the node from the left subtree
                if (key < (*p)->pair.first) {
                    int h = __erase__ (&((*p)->leftSubtree), key);
                    if (h < 0) return h; // < 0 means an error                    
                    (*p)->leftSubtreeHeight = h;
                    if ((*p)->rightSubtreeHeight - (*p)->leftSubtreeHeight > 1) {                        
                        /* the tree is unbalanced, right subtree is too high, perform left rotation
                                | = *p                 | = *p
                                X                      Y
                                / \                    / \
                              a   Y       =>         X   c
                                  / \                / \
                                b   c              a   b
                        */
                        __balancedBinarySearchTreeNode__ *tmp = (*p)->rightSubtree; // picture: tmp = Y
                        (*p)->rightSubtree = tmp->leftSubtree;                      // picture: X.rightSubtree = b
                        (*p)->rightSubtreeHeight = tmp->leftSubtreeHeight;          // correct the hight information of (picture) b branch
                        tmp->leftSubtree = (*p);                                    // picture:Y.leftSubtree = X
                        (*p) = tmp;                                                 // Y becomes the new (subtree) root
                        // correct height information of left subtree - we know that left subtree exists (picture: X node)
                        (*p)->leftSubtreeHeight = max ((*p)->leftSubtree->leftSubtreeHeight, (*p)->leftSubtree->rightSubtreeHeight) + 1;
                    }
                    return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1;
                }
    
                // 3. case: found
                if (!((*p)->pair.first < key)) { // meaning at this point that key == (*p)->pair.first
                    // 3.a. case: delete a node with no children
                    if ((*p)->leftSubtree == NULL && (*p)->rightSubtree == NULL) {
                        // remove the node
                        delete (*p);
                        (*p) = NULL;
                        // __size__ --; // we'll do it if erase () function instead
                        return err_ok;
                    }
                    // 3.b. case: delete a node with only left child 
                    if ((*p)->rightSubtree == NULL) {
                        // remove the node and replace it with its child
                        __balancedBinarySearchTreeNode__ *tmp = (*p);
                        (*p) = (*p)->leftSubtree;
                        delete tmp;
                        // __size__ --; // we'll do it if erase () 
                        return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1; // return the new hight of a subtree
                    }
                    // 3.c. case: delete a node with only right child 
                    if ((*p)->leftSubtree == NULL) {
                        // remove the node and replace it with its child
                        __balancedBinarySearchTreeNode__ *tmp = (*p);
                        (*p) = (*p)->rightSubtree;
                        delete tmp;
                        // __size__ --; // we'll do it if erase () 
                        return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1; // return the new hight of a subtree
                    }
                    // 3.d. case: deleting the node with both children
                        // replace the node with its inorder successor (inorder predecessor would also do) and then delete it
                        // find inorder successor = leftmost node from right subtree
                        __balancedBinarySearchTreeNode__ *q = (*p)->rightSubtree; while (q->leftSubtree) q = q->leftSubtree;
                        // remember inorder successor and then delete it from right subtree
                        __balancedBinarySearchTreeNode__ tmp = *q;
                        (*p)->rightSubtreeHeight = __erase__ (&((*p)->rightSubtree), q->pair.first);
                        (*p)->pair = tmp.pair;
                        if ((*p)->leftSubtreeHeight - (*p)->rightSubtreeHeight > 1) {
                            /* the tree is unbalanced, left subtree is too high, perform right rotation
                                    | = *p                 | = *p
                                    Y                      X
                                    / \                    / \
                                  X   c       =>         a   Y
                                  / \                        / \
                                a   b                      b   c
                            */
                            __balancedBinarySearchTreeNode__ *tmp = (*p)->leftSubtree;  // picture: tmp = X
                            (*p)->leftSubtree = tmp->rightSubtree;                      // picture: Y.leftSubtree = b
                            (*p)->leftSubtreeHeight = tmp->rightSubtreeHeight;          // correct the hight information of (picture) b branch
                            tmp->rightSubtree = (*p);                                   // picture: X.rightSubtree = Y
                            (*p) = tmp;                                                 // X becomes the new (subtree) root
                            // correct height information of right subtree - we know that right subtree exists (picture: Y node)
                            (*p)->rightSubtreeHeight = max ((*p)->rightSubtree->leftSubtreeHeight, (*p)->rightSubtree->rightSubtreeHeight) + 1;
                        }
                        // __size__ --; // we'll do it if erase () 
                        return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1;
                } // 3. case: found
                        
                // 4. case: delete the node from the right subtree of the current node
                    int h = __erase__ (&((*p)->rightSubtree), key);
                    if (h < 0) return h; // < 0 means an error                    
                    (*p)->rightSubtreeHeight = h;
                    if ((*p)->leftSubtreeHeight - (*p)->rightSubtreeHeight > 1) {
                        /* the tree is unbalanced, left subtree is too high, perform right rotation
                                | = *p                 | = *p
                                Y                      X
                                / \                    / \
                              X   c       =>         a   Y
                              / \                        / \
                            a   b                      b   c
                        */
                        __balancedBinarySearchTreeNode__ *tmp = (*p)->leftSubtree;  // picture: tmp = X
                        (*p)->leftSubtree = tmp->rightSubtree;                      // picture: Y.leftSubtree = b
                        (*p)->leftSubtreeHeight = tmp->rightSubtreeHeight;          // correct the hight information of (picture) b branch
                        tmp->rightSubtree = (*p);                                   // picture: X.rightSubtree = Y
                        (*p) = tmp;                                                 // X becomes the new (subtree) root
                        // correct height information of right subtree - we know that right subtree exists (picture: Y node)
                        (*p)->rightSubtreeHeight = max ((*p)->rightSubtree->leftSubtreeHeight, (*p)->rightSubtree->rightSubtreeHeight) + 1;
                    }
                
                return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1;
            }
    
            void __clear__ (__balancedBinarySearchTreeNode__ **p) {
                if ((*p) == NULL) return;        // stop recursion at NULL
                __clear__ (&(*p)->rightSubtree); // recursive delete right subtree  
                __clear__ (&(*p)->leftSubtree);  // recursive delete left subtree
                
                // different wasy of freeing the memory used by the node
                #ifdef __USE_IRAM__
                    free (p);
                #else
                    delete (*p);
                #endif

                (*p) = NULL;
                __size__ --;
                return;
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
