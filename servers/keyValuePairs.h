/*
 * keyValuePairs.h for Arduino (ESP boards)
 * 
 * This file is part of Key-value-pairs-for-Arduino: https://github.com/BojanJurca/Key-value-pairs-for-Arduino
 * 
 * The data storage is internaly implemented as balanced binary search tree for good searching performance.
 *
 * Key-valu-pairs functions are not thread-safe.
 * 
 * Bojan Jurca, November 26, 2023
 *  
 */


#ifndef __KEY_VALUE_PAIRS_H__
    #define __KEY_VALUE_PAIRS_H__

    // TUNNING PARAMETERS

    #define __KEY_VALUE_PAIRS_MAX_STACK_SIZE__ 24 // statically allocated stack needed for iterating through elements, 24 should be enough for the number of elemetns that fit into ESP32's memory


    // #define __KEY_VALUE_PAIR_H_EXCEPTIONS__   // uncomment this line if you want keyValuePairs to throw exceptions
    // #define __KEY_VALUE_PAIR_H_DEBUG__        // uncomment this line for debugging puroposes

    #ifdef __KEY_VALUE_PAIR_H_DEBUG__
        #define __key_value_pair_h_debug__(X) { Serial.print("__key_value_pair_h_debug__: ");Serial.println(X); }
    #else
        #define __key_value_pair_h_debug__(X) ;
    #endif


    template <class keyType, class valueType> class keyValuePairs {
  
      public:

           /*
           * Error handling 
           * 
           * Some functions will return an error code or OK. 
           * 
           * They will also set lastErrorCode variable which will hold this value until it is reset (by assigning lastErrorCode = OK)
           * so it is not necessary to check success of each single operation in the code. 
           */

          enum errorCode {OK = 0, // not all error codes are needed here but they are shared among persistentKeyValuePairs and vectors as well 
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


          struct keyValuePair {
              keyType key;          // node key
              valueType value;      // node value
          };


          /*
           *  Constructor of keyValuePairs with no pairs allows the following kinds of creation od key-value pairs: 
           *  
           *    keyValuePairs<int, String> kvpA;
           */
           
          keyValuePairs () {}
    
  
          /*
           *  Constructor of keyValuePairs from brace enclosed initializer list allows the following kinds of creation of key-value pairs: 
           *  
           *     keyValuePairs<int, String> kvpB = { {1, "one"}, {2, "two"} };
           *     keyValuePairs<int, String> kvpC ( { {1, "one"}, {2, "two"} } );
           */
    
          keyValuePairs (std::initializer_list<keyValuePair> il) {
              for (auto i: il) {

                  if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                      if (!i.key) {                           // ... check if parameter construction is valid
                          lastErrorCode = BAD_ALLOC;          // report error if it is not
                          return;
                      }
                  if (std::is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                      if (!i.value) {                         // ... check if parameter construction is valid
                          lastErrorCode = BAD_ALLOC;          // report error if it is not
                          return;
                      }

                int h = __insert__ (&__root__, i.key, i.value); if  (h >= 0) __height__ = h;
              }  
          }


          /*
           * Key-value pairs destructor - free the memory occupied by pairs
           */
          
          ~keyValuePairs () { __clear__ (&__root__); } // release memory occupied by balanced binary search tree

          
          /*
           * Returns the number of pairs.
           */

          int size () { return __size__; }


          /*
           * Returns the height of balanced binary search tree.
           */

          int8_t height () { return __height__; }


          /*
           * Checks if there are no pairs.
           */

          bool empty () { return __size__ == 0; }


          /*
           * Clears all the elements from the balanced binary search tree.
           */

          void clear () { __clear__ (&__root__); } 


          /*
           *  Copy-constructor of key-value pairs allows the following kinds of creation: 
           *  
           *     keyValuePairs<int, String> kvpD = kvpC;
           *     
           *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
           *  
           *  Calling program should check lastErrorCode member variable after constructor is beeing called for possible errors
           */
    
          keyValuePairs (keyValuePairs& other) {
              // copy other's elements
              for (auto e: other) {
                  int h = this->__insert__ (&__root__, e.key, e.value); if  (h >= 0) __height__ = h;
              }
              // copy the error code as well
              if (!lastErrorCode) lastErrorCode = other.lastErrorCode;
          }


          /*
           *  Assignment operator of key-value pairs allows the following kinds of assignements: 
           *  
           *     keyValuePairs<int, String> kvpE;
           *     kvpE = { {3, "tree"}, {4, "four"}, {5, "five"} }; or   kvpE = { };
           *     
           *  Without properly handling it, = operator would probably just copy one instance over another which would result in crash when instances will be distroyed.
           */
    
          keyValuePairs* operator = (keyValuePairs other) {
              this->clear (); // clear existing pairs if needed

              // copy other's pairs
              for (auto e: other) {

                  if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                      if (!e.key) {                           // ... check if parameter construction is valid
                          lastErrorCode = BAD_ALLOC;          // report error if it is not
                          return this;
                      }
                  if (std::is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                      if (!e.value) {                         // ... check if parameter construction is valid
                          lastErrorCode = BAD_ALLOC;          // report error if it is not
                          return this;
                      }

                  int h = this->__insert__ (&__root__, e.key, e.value); if  (h >= 0) __height__ = h;
              }
              // copy the error code as well
              if (!lastErrorCode) lastErrorCode = other.lastErrorCode;

              return this;
          }


          /*
           *  Returns a pointer to the value associated with the key, if key is found, NULL if it is not. Example:
           *  
           *    String *value = kvpB.find (1);
           *    if (value) 
           *        Serial.println (*value); 
           *    else 
           *        Serial.println ("not found");
           */
          
          valueType *find (keyType key) {

              if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                  if (!(String *) &key) {                 // ... check if parameter construction is valid
                      lastErrorCode = BAD_ALLOC;          // report error if it is not
                      return NULL;
                  }

              return __find__ (__root__, key);
          }


          /*
           *  Erases the key-value pair identified by key
           *  
           *  Returns OK if succeeds and errorCode NOT_FOUND or BAD_ALLOC if String key parameter could not be constructed.
           */

          errorCode erase (keyType key) { 

              if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                  if (!(String *) &key) {                 // ... check if parameter construction is valid
                      return BAD_ALLOC;                   // report error if it is not
                  }

              int h = __erase__ (&__root__, key); 
              if (h < 0) {
                  __key_value_pair_h_debug__ ("erase: " + String (h));
                  return (errorCode) h;
              } else {
                  __height__ = h;
                  return OK;
              }
          }


          /*
           *  Inserts a new key-value pair, returns OK or one of the error codes.
           */

          errorCode insert (keyValuePair pair) { 

              if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                  if (!pair.key)                          // ... check if parameter construction is valid
                      return lastErrorCode = BAD_ALLOC;   // report error if it is not
              if (std::is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                  if (!pair.value)                        // ... check if parameter construction is valid
                      return lastErrorCode = BAD_ALLOC;   // report error if it is not

              int h = __insert__ (&__root__, pair.key, pair.value); 
              if (h < 0) {
                  __key_value_pair_h_debug__ ("insert: " + String (h));
                  return lastErrorCode = (errorCode) h;
              } else {
                  __height__ = h;
                  return OK;
              }
          }

          errorCode insert (keyType key, valueType value) { 

              if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                  if (!key)                               // ... check if parameter construction is valid
                      return lastErrorCode = BAD_ALLOC;   // report error if it is not
              if (std::is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                  if (!(String *) &value)                 // ... check if parameter construction is valid
                      return lastErrorCode = BAD_ALLOC;   // report error if it is not

              int h = __insert__ (&__root__, key, value); 
              if (h < 0) {
                  __key_value_pair_h_debug__ ("insert: " + String (h));
                  lastErrorCode = (errorCode) h;
                  return (errorCode) h;
              } else {
                  __height__ = h;
                  return OK;
              }
          }
  
      
          // iterator
      
      private:
      
          struct __balancedBinarySearchTreeNode__; // forward private declaration

      public:
                
          class Iterator {
            public:
            
              // constructor
              Iterator (keyValuePairs* kvp, int8_t stackSize) {
                  __kvp__ = kvp;
  
                  if (!kvp || !stackSize) return; // when end () is beeing called stack for balanced binary search tree iteration is not needed
  
                  // create a stack for balanced binary search tree iteration
                  
                  /// #ifdef __KEY_VALUE_PAIR_H_EXCEPTIONS__
                  ///     __stack__ = new keyValuePairs::__balancedBinarySearchTreeNode__ *[stackSize](); // initialize stack with NULL pointers
                  /// #else
                  ///     __stack__ = new (std::nothrow) keyValuePairs::__balancedBinarySearchTreeNode__ *[stackSize](); // initialize stack with NULL pointers
                  /// #endif
                  /// if (__stack__ == NULL) {
                  ///     __key_value_pair_h_debug__ ("Iterator: not enough memory to create stack.");
                  ///     __kvp__->lastErrorCode = BAD_ALLOC;
                  ///     return;
                  /// }
                  // memset ((byte *) __stack__, 0, sizeof (__stack__)); // clear the stack

                  if (stackSize >= __KEY_VALUE_PAIRS_MAX_STACK_SIZE__) throw (BAD_ALLOC);
  
                  // find the lowest pair in the balanced binary search tree (this would be the leftmost one) and fill the stack meanwhile
                  keyValuePairs::__balancedBinarySearchTreeNode__* p = kvp->__root__;

                  while (p) {
                      __stack__ [++ __stackPointer__] = p;                      
                      p = p->leftSubtree;
                  }
                  __key_value_pair_h_debug__ ("Iterator: starting at: " + String ( __stack__ [__stackPointer__]->pair.key ));
              }


              // free the memory occupied by the stack
              ~Iterator () { 
                  /// if (__stack__) delete [] __stack__; 
              }

              
              // * operator
              keyValuePair & operator * () const { return __stack__ [__stackPointer__]->pair; }
          
              // ++ (prefix) increment actually moves the state of the stack so that the last element points to the next balanced binary search tree node
              Iterator& operator ++ () { 

                  // the current node is pointed to by stack pointer, move to the next node 
                  
                  // if the node has a right subtree find the leftmost element in the right subtree and fill the stack meanwhile
                  if (__stack__ [__stackPointer__]->rightSubtree != NULL) {
                      __key_value_pair_h_debug__ ("Iterator: going to the right subtree");
                      keyValuePairs::__balancedBinarySearchTreeNode__* p = __stack__ [__stackPointer__]->rightSubtree;
                      if (p && p != __stack__ [__stackPointer__ + 1]) { // if the right subtree has not ben visited yet, proceed with the right subtree
                            while (p) {
                                __stack__ [++ __stackPointer__] = p;
                                p = p->leftSubtree;
                            }
                         return *this; 
                      }
                  }
                  // else proceed with climbing up the stack to the first pair that is greater than the current node
                  {
                      __key_value_pair_h_debug__ ("Iterator: climb up the stack to the first greater key");
                      int8_t i = __stackPointer__;
                      -- __stackPointer__;
                      while (__stackPointer__ >= 0 && __stack__ [__stackPointer__]->pair.key < __stack__ [i]->pair.key) __stackPointer__ --;
                      return *this;
                  }
              }  

              // C++ will stop iterating when != operator returns false, this is when all nodes have been visited and stack pointer is negative
              friend bool operator != (const Iterator& a, const Iterator& b) { return a.__stackPointer__ >= 0; };     
          
          private:
          
              keyValuePairs* __kvp__;

              // a stack is needed to iterate through tree nodes
              /// keyValuePairs::__balancedBinarySearchTreeNode__ **__stack__ = NULL;
              keyValuePairs::__balancedBinarySearchTreeNode__ *__stack__ [__KEY_VALUE_PAIRS_MAX_STACK_SIZE__] = {};
              int8_t __stackPointer__ = -1;

          };      
 
          Iterator begin () { return Iterator (this, __height__); } // C++ only iterates with begin instance ...
          Iterator end ()   { return Iterator (NULL, 0); } // ... so end instance is not really needed and doesn't need its own stack at all


          #ifdef __KEY_VALUE_PAIR_H_DEBUG__
              // displays vector's internal structure
              void debug () {
                debug (__root__, 0, '-');
              }

              void debug (__balancedBinarySearchTreeNode__ *p, int treeLevel, char branch) {
                  if (!p) return; // stop at nullptr
                  
                  debug (p->rightSubtree, treeLevel + 1, '/'); // display right subtree first 
                  
                  // display current node
                  for (int i = 0; i < treeLevel; i ++) Serial.print ("                "); Serial.print (String (branch) + " ");
                  Serial.println ("[" + String (p->pair.key) + "]=" + String (p->pair.value) + "(" + String (p->leftSubtreeHeight) + "," + String (p->rightSubtreeHeight) +")");
                  
                  debug (p->leftSubtree, treeLevel + 1, '\\'); // display left subtree
              }              
          #endif

      private:
      
          // balanced binary search tree for keys
          
          struct __balancedBinarySearchTreeNode__ {
              keyValuePair pair;
              __balancedBinarySearchTreeNode__ *leftSubtree;
              __balancedBinarySearchTreeNode__ *rightSubtree;
              int8_t leftSubtreeHeight;
              int8_t rightSubtreeHeight;
          };
  
          __balancedBinarySearchTreeNode__ *__root__ = NULL; 
          int __size__ = 0;
          int8_t __height__ = 0;
          
          // internal functions
          
          int __insert__ (__balancedBinarySearchTreeNode__ **p, keyType& key, valueType& value) { // returns the height of balanced binary search tree or error
              // 1. case: a leaf has been reached - add new node here
              if ((*p) == NULL) {
                  // Serial.println ("keyValuePairs.__insert__: 1. case: a leaf has been reached - add new node here. Key = " + String (key));
                
                  #ifdef __KEY_VALUE_PAIR_H_EXCEPTIONS__
                      __balancedBinarySearchTreeNode__ *n = new __balancedBinarySearchTreeNode__;
                  #else
                      __balancedBinarySearchTreeNode__ *n = new (std::nothrow) __balancedBinarySearchTreeNode__;    
                  #endif

                  if (n == NULL) return lastErrorCode = BAD_ALLOC;
                  
                  *n = { {key, value}, NULL, NULL, 0, 0 };

                      // in case of Strings - it is possible that key and value didn't get constructed, so just swap stack memory with parameters - this always succeeds
                      if (std::is_same<keyType, String>::value)   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                          if (!n->pair.key)                       // ... check if parameter construction is valid
                              __swapStrings__ ((String *) &n->pair.key, (String *) &key); 
                      if (std::is_same<valueType, String>::value) // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                          if (!n->pair.value)                     // ... check if parameter construction is valid
                              __swapStrings__ ((String *) &n->pair.value, (String *) &value);

                  *p = n;
                  __size__ ++;
                  return 1; // height of the (sub)tree so far
              }
              
              // 2. case: add a new node to the left subtree of the current node
              if (key < (*p)->pair.key) {
                  // Serial.println ("keyValuePairs.__insert__: 2. case: add a new node to the left subtree of the current node. Key = " + String (key) + ", node = " + String ((*p)->pair.key));
                  int h = __insert__ (&((*p)->leftSubtree), key, value);
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
              if (!((*p)->pair.key < key)) { // meaning at this point that key == (*p)->pair.key
                // Serial.println ("keyValuePairs.__insert__: 3. case: the node with the same values already exists. Key = " + String (key) + ", node = " + String ((*p)->pair.key));
                return lastErrorCode = NOT_UNIQUE;
              }
      
              // 4. case: add a new node to the right subtree of the current node
              // Serial.println ("keyValuePairs.__insert__: 4. case: add a new node to the right subtree of the current node. Key = " + String (key) + ", node = " + String ((*p)->pair.key));
              int h = __insert__ (&((*p)->rightSubtree), key, value);
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
  
          valueType *__find__ (__balancedBinarySearchTreeNode__ *p, keyType& key) {
              if (p == NULL) return NULL;                                     // 1. case: not found
              if (key < p->pair.key) return __find__ (p->leftSubtree, key);   // 2. case: continue searching in left subtree
              if (p->pair.key < key) return __find__ (p->rightSubtree, key);  // 3. case: continue searching in reight subtree
              return &(p->pair.value);                                        // 4. case: found
          }
  
          int __erase__ (__balancedBinarySearchTreeNode__ **p, keyType& key) { // returns the height of balanced binary search tree or error
              __key_value_pair_h_debug__ ("__erase__ ( ... , " + String (key) + " )");

              // 1. case: a leaf has been reached - key was not found
              if ((*p) == NULL) {
                  __key_value_pair_h_debug__ ("__erase__: 1. case: a leaf has been reached - key was not found.");
                  return NOT_FOUND; 
              }
                      
              // 2. case: delete the node from the left subtree
              if (key < (*p)->pair.key) {
                  __key_value_pair_h_debug__ ("__erase__: 2. case: delete the node from the left subtree.");
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
              if (!((*p)->pair.key < key)) { // meaning at this point that key == (*p)->pair.key
                  // 3.a. case: delete a node with no children
                  __key_value_pair_h_debug__ ("__erase__: 3.a. case: delete a node with no children.");
                  if ((*p)->leftSubtree == NULL && (*p)->rightSubtree == NULL) {
                      // remove the node
                      delete (*p);
                      (*p) = NULL;
                      __size__ --;
                      return OK;
                  }
                  // 3.b. case: delete a node with only left child 
                  __key_value_pair_h_debug__ ("__erase__: 3.b. case: delete a node with only left child.");
                  if ((*p)->rightSubtree == NULL) {
                      // remove the node and replace it with its child
                      __balancedBinarySearchTreeNode__ *tmp = (*p);
                      (*p) = (*p)->leftSubtree;
                      delete tmp;
                      __size__ --;
                      return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1; // return the new hight of a subtree
                  }
                  // 3.c. case: delete a node with only right child 
                  __key_value_pair_h_debug__ ("__erase__: 3.c. case: delete a node with only right child.");
                  if ((*p)->leftSubtree == NULL) {
                      // remove the node and replace it with its child
                      __balancedBinarySearchTreeNode__ *tmp = (*p);
                      (*p) = (*p)->rightSubtree;
                      delete tmp;
                      __size__ --;
                      return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1; // return the new hight of a subtree
                  }
                  // 3.d. case: deleting the node with both children
                      __key_value_pair_h_debug__ ("__erase__: 3.d. case: delete a node with both children.");
                      // replace the node with its inorder successor (inorder predecessor would also do) and then delete it
                      // find inorder successor = leftmost node from right subtree
                      __balancedBinarySearchTreeNode__ *q = (*p)->rightSubtree; while (q->leftSubtree) q = q->leftSubtree;
                      // remember inorder successor and than delete it from right subtree
                      __balancedBinarySearchTreeNode__ tmp = *q;
                      (*p)->rightSubtreeHeight = __erase__ (&((*p)->rightSubtree), q->pair.key);
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
                      __size__ --;
                      return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1;
              } // 3. case: found
                      
              // 4. case: delete the node from the right subtree of the current node
                  __key_value_pair_h_debug__ ("__erase__: 4. case: delete the node from the right subtree.");
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
              
              __key_value_pair_h_debug__ ("__erase__: unexpected end of function.");
              return max ((*p)->leftSubtreeHeight, (*p)->rightSubtreeHeight) + 1;
          }
  
          void __clear__ (__balancedBinarySearchTreeNode__ **p) {
              if ((*p) == NULL) return;        // stop recursion at NULL
              __clear__ (&(*p)->rightSubtree); // recursive delete right subtree  
              __clear__ (&(*p)->leftSubtree);  // recursive delete left subtree
              
              // delete the current node
              delete (*p);
              (*p) = NULL;
              __size__ --;
              return;
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
