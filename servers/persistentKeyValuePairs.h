/*
 * persistentKeyValuePairs.h for Arduino (ESP boards with flash disk)
 * 
 * This file is part of Key-value-database-for-Arduino: https://github.com/BojanJurca/Key-value-database-for-Arduino
 *
 * Key-value-database-for-Arduino may be used as a simple database with the following functions (see examples in BasicUsage.ino).
 * The functions are thread-safe:
 *
 *    - Insert (key, value)                                   - inserts a new key-value pair
 *
 *    - FindBlockOffset (key)                                 - searches (memory) keyValuePairs for key
 *    - FindValue (key, optional block offset)                - searches (memory) keyValuePairs for blockOffset connected to key and then it reads the value from (disk) data file (it works slightly faster if block offset is already known, such as during iterations)
 *
 *    - Update (key, new value, optional block offset)        - updates the value associated by the key (it works slightly faster if block offset is already known, such as during iterations)
 *    - Update (key, callback function, optional blockoffset) - if the calculation is made with existing value then this is prefered method, since calculation is performed while database is being loceks
 *
 *    - Upsert (key, new value)                               - update the value if the key already exists, else insert a new one
 *    - Update (key, callback function, default value)        - if the calculation is made with existing value then this is prefered method, since calculation is performed while database is being loceks
 *
 *    - Delete (key)                                          - deletes key-value pair identified by the key
 *    - Truncate                                              - deletes all key-value pairs
 *
 *    - Iterate                                               - iterate (list) through all the keys and their blockOffsets with an Iterator
 *
 *    - Lock                                                  - locks (takes the semaphore) to (temporary) prevent other taska accessing persistentKeyValuePairs
 *    - Unlock                                                - frees the lock
 *
 * Data storage structure used for persistentKeyValuePairs:
 *
 * (disk) persistentKeyValuePairs consists of:
 *    - data file
 *    - (memory) keyValuePairs that keep keys and pointers (offsets) to data in the data file
 *    - (memory) vector that keeps pointers (offsets) to free blocks in the data file
 *    - semaphore to synchronize (possible) multi-tasking accesses to persistentKeyValuePairs
 *
 *    (disk) data file structure:
 *       - data file consists consecutive of blocks
 *       - Each block starts with int16_t number which denotes the size of the block (in bytes). If the number is positive the block is considered to be used
 *         with useful data, if the number is negative the block is considered to be deleted (free). Positive int16_t numbers can vary from 0 to 32768, so
 *         32768 is the maximum size of a single data block.
 *       - after the block size number, a key and its value are stored in the block (only if the block is beeing used).
 *
 *    (memory) keyValuePairs structure:
 *       - the key is the same key as used for persistentKeyValuePairs
 *       - the value is an offset to data file block containing the data, persistentKeyValuePairs' value will be fetched from there. Data file offset is
 *         stored in uint32_t so maximum data file offest can theoretically be 4294967296, but ESP32 files can't be that large.
 *
 *    (memory) vector structure:
 *       - a free block list vector contains structures with:
 *            - data file offset (uint16_t) of a free block
 *            - size of a free block (int16_t)
 * 
 * Bojan Jurca, November 26, 2023
 *  
 */


#ifndef __PERSISTENT_KEY_VALUE_PAIRS_H__
    #define __PERSISTENT_KEY_VALUE_PAIRS_H__


    // TUNNING PARAMETERS

    #define __PERSISTENT_KEY_VALUE_PAIRS_PCT_FREE__ 0.2 // how much space is left free in data block to let data "breed" a little - only makes sense for String values 

    
    #include "keyValuePairs.h"
    #include "vector.h"

    // #define __PERSISTENT_KEY_VALUE_PAIR_H_EXCEPTIONS__   // uncomment this line if you want keyValuePairs to throw exceptions
    // #define __PERSISTENT_KEY_VALUE_PAIRS_H_DEBUG__       // uncomment this line for debugging puroposes

    #ifdef __PERSISTENT_KEY_VALUE_PAIRS_H_DEBUG__
        #define __persistent_key_value_pairs_h_debug__(X) { Serial.print("__persistent_key_value_pairs_h_debug__: ");Serial.println(X); }
    #else
        #define __persistent_key_value_pairs_h_debug__(X) ;
    #endif


    static SemaphoreHandle_t __persistentKeyValuePairsSemaphore__ = xSemaphoreCreateMutex (); 

    template <class keyType, class valueType> class persistentKeyValuePairs : private keyValuePairs<keyType, uint32_t> {
  
        public:

            enum errorCode {OK = 0, // not all error codes are needed here but they are shared among keyValuePairs and vectors as well 
                            NOT_FOUND = -1,               // key is not found
                            BAD_ALLOC = -2,               // out of memory
                            OUT_OF_RANGE = -3,            // invalid index
                            NOT_UNIQUE = -4,              // the key is not unique
                            DATA_CHANGED = -5,            // unexpected data value found
                            FILE_IO_ERROR = -6,           // file operation error
                            NOT_WHILE_ITERATING = -7,     // operation can not be performed while iterating
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
            *  Constructor of persistentKeyValuePairs that does not load the data. Subsequential call to loadData function is needed:
            *  
            *     persistentKeyValuePairs<int, String> pkvpA;
            *
            *     void setup () {
            *         Serial.begin (115200);
            *
            *         fileSystem.mountFAT (true);    
            *
            *         kvp.loadData ("/persistentKeyValuePairs/A.kvp");
            *
            *          ...
            */
            
            persistentKeyValuePairs () {}

           /*
            *  Constructor of persistentKeyValuePairs that also loads the data from data file: 
            *  
            *     persistentKeyValuePairs<int, String> pkvpA ("/persistentKeyValuePairs/A.kvp");
            *     if (pkvpA.lastErrorCode != pkvpA.OK) 
            *         Serial.printf ("pkvpA constructor failed: %s, all the data may not be indexed\n", pkvpA.errorCodeText (pkvpA.lastErrorCode));
            *
            */
            
            persistentKeyValuePairs (const char *dataFileName) { 
                loadData (dataFileName);
            }

            ~persistentKeyValuePairs () { 
                if (__dataFile__) {
                    __dataFile__.close ();
                    __persistent_key_value_pairs_h_debug__ ("DATA FILE CLOSED: " + String (__dataFileName__));
                }
            } 


           /*
            *  Loads the data from data file.
            *  
            */

            errorCode loadData (const char *dataFileName) {
              __persistent_key_value_pairs_h_debug__ ("LOADDATA (" + String (dataFileName) + ")");
                Lock ();
                if (__dataFile__) {
                    __persistent_key_value_pairs_h_debug__ ("load: data already loaded.");
                    Unlock (); 
                    return DATA_ALREADY_LOADED;
                }

                // load new data

                strcpy (__dataFileName__, dataFileName);

                if (!fileSystem.isFile (dataFileName)) {
                    __persistent_key_value_pairs_h_debug__ ("loadData: creating empty data file.");
                    __dataFile__ = fileSystem.open (dataFileName, "w", true);
                    if (__dataFile__) {
                          __dataFile__.close (); 
                    } else {
                        __persistent_key_value_pairs_h_debug__ ("loadData: failed creating the empty data file.");
                        Unlock (); 
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                }

                __dataFile__ = fileSystem.open (dataFileName, "r+", false);
                __persistent_key_value_pairs_h_debug__ ("loadData: opened data file, size at load = " + String (__dataFile__.size ()));
                if (!__dataFile__) {
                    lastErrorCode = FILE_IO_ERROR;
                    __persistent_key_value_pairs_h_debug__ ("loadData: failed opening the data file.");
                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                }

                __dataFileSize__ = __dataFile__.size ();
                uint64_t blockOffset = 0;

                while (blockOffset < __dataFileSize__ &&  blockOffset <= 0xFFFFFFFF) { // max uint32_t
                    int16_t blockSize;
                    keyType key;
                    valueType value;

                    if (!__readBlock__ (blockSize, key, value, (uint32_t) blockOffset, true)) {
                        __persistent_key_value_pairs_h_debug__ ("loadData: __readBlock__ failed.");
                        __dataFile__.close ();
                        Unlock (); 
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }                         
                    }
                    if (blockSize > 0) { // block containining the data -> insert into keyValuePairs
                        __persistent_key_value_pairs_h_debug__ ("constructor: blockOffset: " + String (blockOffset));
                        __persistent_key_value_pairs_h_debug__ ("constructor: used blockSize: " + String (blockSize));
                        if (keyValuePairs<keyType, uint32_t>::insert (key, (uint32_t) blockOffset) != keyValuePairs<keyType, uint32_t>::OK) {
                            __persistent_key_value_pairs_h_debug__ ("load: insert failed, error " + String (keyValuePairs<keyType, uint32_t>::lastErrorCode));
                            __dataFile__.close ();
                            errorCode e = lastErrorCode = (errorCode) keyValuePairs<keyType, uint32_t>::lastErrorCode;
                            Unlock (); 
                            return e;
                        }
                    } else { // free block -> insert into __freeBlockList__
                        __persistent_key_value_pairs_h_debug__ ("loadData: blockOffset: " + String (blockOffset));
                        __persistent_key_value_pairs_h_debug__ ("loadData: free blockSize: " + String (-blockSize));
                        blockSize = (int16_t) -blockSize;
                        if (__freeBlocksList__.push_back ( {(uint32_t) blockOffset, blockSize} ) != __freeBlocksList__.OK) {
                            __persistent_key_value_pairs_h_debug__ ("loadData: push_back failed.");
                            __dataFile__.close ();
                            errorCode e = lastErrorCode = (errorCode) __freeBlocksList__.lastErrorCode;
                            Unlock (); 
                            return e;
                        }
                    } 

                    blockOffset += blockSize;
                }

                Unlock (); 
                return OK;
            }


           /*
            *  Returns true if the data has already been successfully loaded from data file.
            *  
            */

            bool dataLoaded () {
                return __dataFile__;
            }


           /*
            * Returns the number of key-value pairs.
            */

            int size () { return keyValuePairs<keyType, uint32_t>::size (); }


           /*
            *  Inserts a new key-value pair, returns OK or one of the error codes.
            */

            errorCode Insert (keyType key, valueType value) {
                __persistent_key_value_pairs_h_debug__ ("INSERT (... , ...)");
                if (!__dataFile__) { 
                    __persistent_key_value_pairs_h_debug__ ("Insert: __dataFile__ not opened.");
                    lastErrorCode = FILE_IO_ERROR; 
                    return FILE_IO_ERROR; 
                }

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Insert: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }                                                      // report error if it is not
                    }
                if (std::is_same<valueType, String>::value)                                                                   // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &value) {                                                                                 // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Insert: value constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }                                                      // report error if it is not
                    }

                Lock (); 
                if (__inIteration__) {
                    __persistent_key_value_pairs_h_debug__ ("Insert: can not insert while iterating.");
                    Unlock (); 
                    { lastErrorCode = NOT_WHILE_ITERATING; return NOT_WHILE_ITERATING; }
                }

                // 1. get ready for writting into __dataFile__
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 1 - calculate block size");
                size_t dataSize = sizeof (int16_t); // block size information
                size_t blockSize = dataSize;
                if (std::is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &key)->length () + 1); // add 1 for closing 0
                    blockSize += (((String *) &key)->length () + 1) + (((String *) &key)->length () + 1) * __PERSISTENT_KEY_VALUE_PAIRS_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size key
                    dataSize += sizeof (keyType);
                    blockSize += sizeof (keyType);
                }                
                if (std::is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &value)->length () + 1); // add 1 for closing 0
                    blockSize += (((String *) &value)->length () + 1) + (((String *) &value)->length () + 1) * __PERSISTENT_KEY_VALUE_PAIRS_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size value
                    dataSize += sizeof (valueType);
                    blockSize += sizeof (valueType);
                }
                if (blockSize > 32768) {
                    __persistent_key_value_pairs_h_debug__ ("Insert: dataSize too large.");
                    Unlock (); 
                    { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }      
                }
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 1 - block size = " + String (blockSize) + " dataSize = " + String (dataSize));

                // 2. search __freeBlocksList__ for most suitable free block, if it exists
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 2 - search for a free block if it exists");
                int freeBlockIndex = -1;
                uint32_t minWaste = 0xFFFFFFFF;
                for (int i = 0; i < __freeBlocksList__.size (); i ++) {
                    if (__freeBlocksList__ [i].blockSize >= dataSize && __freeBlocksList__ [i].blockSize - dataSize < minWaste) {
                        freeBlockIndex = i;
                        minWaste = __freeBlocksList__ [i].blockSize - dataSize;
                    }
                }
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 2 - free block = " + String (freeBlockIndex));

                // 3. reposition __dataFile__ pointer
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 3 - reposition data file pointer");
                uint32_t blockOffset;                
                if (freeBlockIndex == -1) { // append data to the end of __dataFile__
                    __persistent_key_value_pairs_h_debug__ ("Insert: appending data to the end of data file, blockOffst: " + String (blockOffset));
                    blockOffset = __dataFileSize__;
                } else { // writte data to free block in __dataFile__
                    __persistent_key_value_pairs_h_debug__ ("Insert: writing to free block " + String (freeBlockIndex) + ", blockOffst: " + String (blockOffset) + ", blockSize: " + String (__freeBlocksList__ [freeBlockIndex].blockSize));
                    blockOffset = __freeBlocksList__ [freeBlockIndex].blockOffset;
                    blockSize = __freeBlocksList__ [freeBlockIndex].blockSize;
                }
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    __persistent_key_value_pairs_h_debug__ ("Insert: seek failed (3).");
                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; } 
                }

                // 4. update (memory) keyValuePairs structure 
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 4 - update key-value pairs");
                if (keyValuePairs<keyType, uint32_t>::insert (key, blockOffset) != keyValuePairs<keyType, uint32_t>::OK) {
                    __persistent_key_value_pairs_h_debug__ ("Insert: insert failed (4).");
                    errorCode e = lastErrorCode = (errorCode) keyValuePairs<keyType, uint32_t>::lastErrorCode;
                    Unlock (); 
                    return e;
                }

                // 5. construct the block to be written
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 5 - construct the block");
                byte *block = (byte *) malloc (blockSize);
                if (!block) {
                    __persistent_key_value_pairs_h_debug__ ("Insert: malloc failed (5).");
                    Unlock (); 
                    { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                }

                int16_t i = 0;
                int16_t bs = (int16_t) blockSize;
                memcpy (block + i, &bs, sizeof (bs)); i += sizeof (bs);
                if (std::is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    size_t l = ((String *) &key)->length () + 1; // add 1 for closing 0
                    memcpy (block + i, ((String *) &key)->c_str (), l); i += l;
                } else { // fixed size key
                    memcpy (block + i, &key, sizeof (key)); i += sizeof (key);
                }       
                if (std::is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    size_t l = ((String *) &value)->length () + 1; // add 1 for closing 0
                    memcpy (block + i, ((String *) &value)->c_str (), l); i += l;
                } else { // fixed size value
                    memcpy (block + i, &value, sizeof (value)); i += sizeof (value);
                }

                // 6. write block to __dataFile__
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 6 - write the block to data file");
                if (__dataFile__.write (block, blockSize) != blockSize) {
                    __persistent_key_value_pairs_h_debug__ ("Insert: write failed (6).");
                    free (block);

                    // 7. (try to) roll-back
                    __persistent_key_value_pairs_h_debug__ ("INSERT: step 7 - roll-back");
                    if (__dataFile__.seek (blockOffset, SeekSet)) {
                        blockSize = (int16_t) -blockSize;
                        if (__dataFile__.write ((byte *) &blockSize, sizeof (blockSize)) != sizeof (blockSize)) { // can't roll-back
                            __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        }
                    } else { // can't roll-back
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                    }

                    keyValuePairs<keyType, uint32_t>::erase (key);

                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                }
                __dataFile__.flush ();
                free (block);

                // 8. roll-out
                __persistent_key_value_pairs_h_debug__ ("INSERT: step 8 - roll-out");
                if (freeBlockIndex == -1) { // data appended to the end of __dataFile__
                    __dataFileSize__ += blockSize;
                } else { // data written to free block in __dataFile__
                    __freeBlocksList__.erase (freeBlockIndex); // doesn't fail
                }

                Unlock (); 
                return OK;
            }


           /*
            *  Retrieve blockOffset from (memory) keyValuePairs, so it is fast.
            */

            errorCode FindBlockOffset (keyType key, uint32_t& blockOffset) {
                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("FindBlockOffset: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock ();
                keyValuePairs<keyType, uint32_t>::clearLastErrorCode ();
                uint32_t *p = keyValuePairs<keyType, uint32_t>::find (key);
                if (p) { // if found
                    blockOffset = *p;
                    Unlock ();  
                    return OK;
                } else { // not found or error
                    if (keyValuePairs<keyType, uint32_t>::lastErrorCode == keyValuePairs<keyType, uint32_t>::OK) {
                        __persistent_key_value_pairs_h_debug__ ("FindBlockOffset: NOT_FOUND.");
                        Unlock ();  
                        return NOT_FOUND;
                    } else {
                        errorCode e = lastErrorCode = (errorCode) keyValuePairs<keyType, uint32_t>::lastErrorCode;
                        __persistent_key_value_pairs_h_debug__ ("FindBlockOffset error " + String (lastErrorCode));
                        Unlock ();  
                        return e;
                    }
                }
            }


           /*
            *  Read the value from (disk) __dataFile__, so it is slow. 
            */

            errorCode FindValue (keyType key, valueType *value, uint32_t blockOffset = 0xFFFFFFFF) { 
                if (!__dataFile__) { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; } 

                keyType storedKey = {};

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("FindValue: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock (); 

                if (blockOffset == 0xFFFFFFFF) { // if block offset was not specified find it from keyValuePairs
                    keyValuePairs<keyType, uint32_t>::clearLastErrorCode ();
                    uint32_t *pBlockOffset = keyValuePairs<keyType, uint32_t>::find (key);
                    if (!pBlockOffset) { // if not found or error
                        if (keyValuePairs<keyType, uint32_t>::lastErrorCode == keyValuePairs<keyType, uint32_t>::OK) {
                            __persistent_key_value_pairs_h_debug__ ("FindValue: NOT_FOUND.");
                            Unlock ();  
                            return NOT_FOUND;
                        } else {
                            errorCode e = lastErrorCode = (errorCode) keyValuePairs<keyType, uint32_t>::lastErrorCode;
                            __persistent_key_value_pairs_h_debug__ ("FindValue: error " + String (lastErrorCode));
                            Unlock ();  
                            return e;
                        }
                    }
                    blockOffset = *pBlockOffset;
                }

                int16_t blockSize;
                if (__readBlock__ (blockSize, storedKey, *value, blockOffset)) {
                    if (blockSize > 0 && storedKey == key) {
                        Unlock ();  
                        return OK; // success  
                    } else {
                        __persistent_key_value_pairs_h_debug__ ("FindValue: DATA_CHANGED.");
                        Unlock ();  
                        { lastErrorCode = DATA_CHANGED; return DATA_CHANGED; } // shouldn't happen, but check anyway ...
                    }
                } else {
                    __persistent_key_value_pairs_h_debug__ ("FindValue: __readBlock__ error");
                    errorCode e = lastErrorCode;
                    Unlock ();  
                    return e; 
                } 
            }


           /*
            *  Updates the value associated with the key
            */

            errorCode Update (keyType key, valueType newValue, uint32_t *pBlockOffset = NULL) {
                __persistent_key_value_pairs_h_debug__ ("UPDATE ( ... , ... )");
                if (!__dataFile__) { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }
                if (std::is_same<valueType, String>::value)                                                                   // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &newValue) {                                                                              // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: value constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock (); 

                // 1. get blockOffset
                __persistent_key_value_pairs_h_debug__ ("UPDATE (step 1): get block offset");
                if (!pBlockOffset) { // fing block offset if not provided by the calling program
                    keyValuePairs<keyType, uint32_t>::clearLastErrorCode ();
                    pBlockOffset = keyValuePairs<keyType, uint32_t>::find (key);
                    if (!pBlockOffset) { // if not found
                        if (keyValuePairs<keyType, uint32_t>::lastErrorCode == keyValuePairs<keyType, uint32_t>::OK) {
                            __persistent_key_value_pairs_h_debug__ ("Update: NOT_FOUND.");
                            Unlock ();  
                            return NOT_FOUND;
                        } else {
                            errorCode e = lastErrorCode = (errorCode) keyValuePairs<keyType, uint32_t>::lastErrorCode;
                            Unlock ();  
                            return e;
                        }
                    }
                }
                __persistent_key_value_pairs_h_debug__ ("UPDATE (step 1): get block offset: " + String (*pBlockOffset));

                // 2. read the block size and stored key
                __persistent_key_value_pairs_h_debug__ ("UPDATE (step 2): read block size");
                int16_t blockSize;
                size_t newBlockSize;
                keyType storedKey;
                valueType storedValue;
                if (!__readBlock__ (blockSize, storedKey, storedValue, *pBlockOffset, true)) {
                    __persistent_key_value_pairs_h_debug__ ("Update: __readBlock__ failed.");
                    errorCode e = lastErrorCode;
                    Unlock ();  
                    return e;
                }
                if (blockSize <= 0 || storedKey != key) {
                    __persistent_key_value_pairs_h_debug__ ("Update: DATA_CHANGED.");
                    Unlock ();  
                    { lastErrorCode = DATA_CHANGED; return DATA_CHANGED; } // shouldn't happen, but check anyway ...
                }
                __persistent_key_value_pairs_h_debug__ ("UPDATE (step 2): read block size: " + String (blockSize));

                // 3. calculate new block and data size
                __persistent_key_value_pairs_h_debug__ ("UPDATE (step 3): calculte block size for changed data");
                size_t dataSize = sizeof (int16_t); // block size information
                newBlockSize = dataSize;
                if (std::is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &key)->length () + 1); // add 1 for closing 0
                    newBlockSize += (((String *) &key)->length () + 1) + (((String *) &key)->length () + 1) * __PERSISTENT_KEY_VALUE_PAIRS_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size key
                    dataSize += sizeof (keyType);
                    newBlockSize += sizeof (keyType);
                }                
                if (std::is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &newValue)->length () + 1); // add 1 for closing 0
                    newBlockSize += (((String *) &newValue)->length () + 1) + (((String *) &newValue)->length () + 1) * __PERSISTENT_KEY_VALUE_PAIRS_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size value
                    dataSize += sizeof (valueType);
                    newBlockSize += sizeof (valueType);
                }
                if (newBlockSize > 32768) {
                    __persistent_key_value_pairs_h_debug__ ("Update: dataSize too large.");
                    Unlock (); 
                    { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }      
                }
                __persistent_key_value_pairs_h_debug__ ("UPDATE (step 3): calculte block size for changed data: " + String (newBlockSize));

                // 4. decide where to write the new value: existing block or a new one
                if (dataSize <= blockSize) { // there is enough space for new data in the existing block - easier case
                    __persistent_key_value_pairs_h_debug__ ("Update: writing changed data to the same block, dataSize = " + String (dataSize));
                    uint32_t dataFileOffset = *pBlockOffset + sizeof (int16_t); // skip block size information
                    if (std::is_same<keyType, String>::value) { // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        dataFileOffset += (((String *) &key)->length () + 1); // add 1 for closing 0
                    } else { // fixed size key
                        dataFileOffset += sizeof (keyType);
                    }                

                    // 5. write new value to __dataFile__
                    __persistent_key_value_pairs_h_debug__ ("UPDATE (step 5): overwrite existing block");
                    if (!__dataFile__.seek (dataFileOffset, SeekSet)) {
                        __persistent_key_value_pairs_h_debug__ ("Update: seek failed (5).");
                        Unlock ();  
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                    int bytesToWrite;
                    int bytesWritten;
                    if (std::is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        bytesToWrite = (((String *) &newValue)->length () + 1);
                        bytesWritten = __dataFile__.write ((byte *) ((String *) &newValue)->c_str () , bytesToWrite);
                        __dataFile__.flush ();
                    } else {
                        bytesToWrite = sizeof (newValue);
                        bytesWritten = __dataFile__.write ((byte *) &newValue , bytesToWrite);
                        __dataFile__.flush ();
                    }
                    if (bytesWritten != bytesToWrite) { // file IO error, it is highly unlikely that rolling-back to the old value would succeed
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        Unlock ();  
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                    // success
                    __persistent_key_value_pairs_h_debug__ ("UPDATE (step 5): overwrite existing block: OK");


                } else { // existing block is not big eneugh, we'll need a new block - more difficult case
                    __persistent_key_value_pairs_h_debug__ ("Update: writing changed data to new block");

                    // 6. search __freeBlocksList__ for most suitable free block, if it exists
                    __persistent_key_value_pairs_h_debug__ ("UPDATE (step 6): sarching for best free block if it exists");
                    int freeBlockIndex = -1;
                    uint32_t minWaste = 0xFFFFFFFF;
                    for (int i = 0; i < __freeBlocksList__.size (); i ++) {
                        if (__freeBlocksList__ [i].blockSize >= dataSize && __freeBlocksList__ [i].blockSize - dataSize < minWaste) {
                            freeBlockIndex = i;
                            minWaste = __freeBlocksList__ [i].blockSize - dataSize;
                        }
                    }

                    // 7. reposition __dataFile__ pointer
                    uint32_t newBlockOffset;                
                    if (freeBlockIndex == -1) { // append data to the end of __dataFile__
                        __persistent_key_value_pairs_h_debug__ ("Update: writing to new block, dataSize: " + String (dataSize) + ", blockSize: " + String (newBlockSize));
                        newBlockOffset = __dataFileSize__;
                    } else { // writte data to free block in __dataFile__
                        __persistent_key_value_pairs_h_debug__ ("Update: writing to free block: " + String (dataSize) + ", " + String (__freeBlocksList__ [freeBlockIndex].blockSize));
                        newBlockOffset = __freeBlocksList__ [freeBlockIndex].blockOffset;
                        newBlockSize = __freeBlocksList__ [freeBlockIndex].blockSize;
                    }
                    if (!__dataFile__.seek (newBlockOffset, SeekSet)) {
                        __persistent_key_value_pairs_h_debug__ ("Update: seek failed (7).");
                        Unlock (); 
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                    __persistent_key_value_pairs_h_debug__ ("UPDATE (step 7): data file offset: " + String (newBlockOffset));

                    // 8. construct the block to be written
                    byte *block = (byte *) malloc (newBlockSize);
                    if (!block) {
                        __persistent_key_value_pairs_h_debug__ ("Update: malloc failed (8).");
                        Unlock (); 
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                    int16_t i = 0;
                    int16_t bs = (int16_t) newBlockSize;
                    memcpy (block + i, &bs, sizeof (bs)); i += sizeof (bs);
                    if (std::is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        size_t l = ((String *) &key)->length () + 1; // add 1 for closing 0
                        memcpy (block + i, ((String *) &key)->c_str (), l); i += l;
                    } else { // fixed size key
                        memcpy (block + i, &key, sizeof (key)); i += sizeof (key);
                    }       
                    if (std::is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        size_t l = ((String *) &newValue)->length () + 1; // add 1 for closing 0
                        memcpy (block + i, ((String *) &newValue)->c_str (), l); i += l;
                    } else { // fixed size value
                        memcpy (block + i, &newValue, sizeof (newValue)); i += sizeof (newValue);
                    }

                    // 9. write new block to __dataFile__
                    if (__dataFile__.write (block, dataSize) != dataSize) {
                        __persistent_key_value_pairs_h_debug__ ("Update: write failed (9).");
                        free (block);

                        // 10. (try to) roll-back
                        if (__dataFile__.seek (newBlockOffset, SeekSet)) {
                            newBlockSize = (int16_t) -newBlockSize;
                            if (__dataFile__.write ((byte *) &newBlockSize, sizeof (newBlockSize)) != sizeof (newBlockSize)) { // can't roll-back
                                __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                            }
                        } else { // can't roll-back
                            __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        }
                        Unlock (); 
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                    free (block);
                    __dataFile__.flush ();

                    // 11. roll-out
                    if (freeBlockIndex == -1) { // data appended to the end of __dataFile__
                        __dataFileSize__ += newBlockSize;
                    } else { // data written to free block in __dataFile__
                        __freeBlocksList__.erase (freeBlockIndex); // doesn't fail
                    }
                    // mark old block as free
                    if (!__dataFile__.seek (*pBlockOffset, SeekSet)) {
                        __persistent_key_value_pairs_h_debug__ ("Update: seek failed (11).");
                        __dataFile__.close (); // data file is corrupt (it contains two entries with the same key) and it is not likely we can roll it back
                        Unlock (); 
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                    blockSize = (int16_t) -blockSize;
                    if (__dataFile__.write ((byte *) &blockSize, sizeof (blockSize)) != sizeof (blockSize)) {
                        __persistent_key_value_pairs_h_debug__ ("Update: write failed (12).");
                        __dataFile__.close (); // data file is corrupt (it contains two entries with the same key) and it si not likely we can roll it back
                        Unlock (); 
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }
                    __dataFile__.flush ();
                    // update __freeBlocklist__
                    __persistent_key_value_pairs_h_debug__ ("UPDATE (step 11): add block to free list ( offset , size ) = ( " + String (*pBlockOffset) + " , " + String (-blockSize) + " )");
                    if (__freeBlocksList__.push_back ( {*pBlockOffset, (int16_t) -blockSize} ) != __freeBlocksList__.OK) {
                        __persistent_key_value_pairs_h_debug__ ("Update: push_back failed (12).");
                        // it is not really important to return with an error here, persistentKeyValuePairs can continue working with this error 
                         
                        // return keyValuePairs<keyType, uint32_t>::lastErrorCode = keyValuePairs<keyType, uint32_t>::BAD_ALLOC;; 
                    }
                    // update keyValuePairs information
                    *pBlockOffset = newBlockOffset; // there is no reason this would fail
                }

                Unlock ();  
                return OK;
            }


           /*
            *  Updates the value associated with the key throught callback function (usefull for counting, etc, when all the calculation should be done while locking is in place)
            */

            errorCode Update (keyType key, void (*updateCallback) (valueType &value), uint32_t *pBlockOffset = NULL) {
                __persistent_key_value_pairs_h_debug__ ("UPDATE ( " + String (key) + " , callbackFunction () , " + (pBlockOffset ? String (*pBlockOffset) : "NULL") + " )");
                if (!__dataFile__) { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock (); 

                valueType value;
                errorCode e = FindValue (key, &value); 
                if (e != OK) {
                    __persistent_key_value_pairs_h_debug__ ("Update: error.");
                    Unlock ();  
                    return e;
                }

                updateCallback (value);

                e = Update (key, value, pBlockOffset); 
                if (e == OK) {
                    __persistent_key_value_pairs_h_debug__ ("Update: error");
                    Unlock ();  
                    return e;
                }                

                Unlock ();
                return OK;
            }


           /*
            *  Updates or inserts key-value pair
            */

            errorCode Upsert (keyType key, valueType newValue) {
                __persistent_key_value_pairs_h_debug__ ("UPSERT ( ... , ... )");
                if (!__dataFile__) { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }
                if (std::is_same<valueType, String>::value)                                                                   // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &newValue) {                                                                              // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: value constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock (); 
                errorCode e = Update (key, newValue);
                if (e == NOT_FOUND) e = Insert (key, newValue);
                Unlock ();

                return e; 
            }

           /*
            *  Updates or inserts the value associated with the key throught callback function (usefull for counting, etc, when all the calculation should be done while locking is in place)
            */

            errorCode Upsert (keyType key, void (*updateCallback) (valueType &value), valueType defaultValue) {
                __persistent_key_value_pairs_h_debug__ ("UPSERT ( " + String (key) + " , callbackFunction () )");
                if (!__dataFile__) { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }
                if (std::is_same<valueType, String>::value)                                                                   // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &defaultValue) {                                                                          // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Update: value constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock (); 
                errorCode e = Update (key, updateCallback);
                if (e == NOT_FOUND) e = Insert (key, defaultValue);
                Unlock ();

                return e;                 
            }


           /*
            *  Deletes key-value pair, returns OK or one of the error codes.
            */

            errorCode Delete (keyType key) {
                __persistent_key_value_pairs_h_debug__ ("DELETE ( " + String (key) + " )");              
                if (!__dataFile__) { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }

                if (std::is_same<keyType, String>::value)                                                                     // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        __persistent_key_value_pairs_h_debug__ ("Delete: key constructor failed.");
                        { lastErrorCode = BAD_ALLOC; return BAD_ALLOC; }
                    }

                Lock (); 
                if (__inIteration__) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: can not delete while iterating.");
                    Unlock (); 
                    { lastErrorCode = NOT_WHILE_ITERATING; return NOT_WHILE_ITERATING; }
                }

                // 1. get blockOffset
                __persistent_key_value_pairs_h_debug__ ("DELETE (step 1): get block offset");
                uint32_t blockOffset;
                if (FindBlockOffset (key, blockOffset) != OK) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: FindBlockOffset failed (1).");
                    errorCode e = lastErrorCode;
                    Unlock (); 
                    return e;
                }
                __persistent_key_value_pairs_h_debug__ ("DELETE (step 1): get block offset: " + String (blockOffset));

                // 2. read the block size
                __persistent_key_value_pairs_h_debug__ ("DEELTE (step 2): read block size");
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: seek failed (2).");
                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                }
                int16_t blockSize;
                if (__dataFile__.read ((uint8_t *) &blockSize, sizeof (int16_t)) != sizeof (blockSize)) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: read failed (2).");
                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                }
                if (blockSize < 0) { 
                    __persistent_key_value_pairs_h_debug__ ("Delete: the block is already free (2).");
                    Unlock (); 
                    { lastErrorCode = DATA_CHANGED; return DATA_CHANGED; } // shouldn't happen, but check anyway ...
                }
                __persistent_key_value_pairs_h_debug__ ("DELETE (step 2): read block size: " + String (blockSize));

                // 3. erase the key from keyValuePairs
                __persistent_key_value_pairs_h_debug__ ("DELETE (step 3): erase key-value pair");
                if (keyValuePairs<keyType, uint32_t>::erase (key) != keyValuePairs<keyType, uint32_t>::OK) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: erase failed (3).");
                    errorCode e = lastErrorCode = (errorCode) keyValuePairs<keyType, uint32_t>::lastErrorCode;
                    Unlock (); 
                    return e;
                }                

                // 4. write back negative block size designatin a free block
                __persistent_key_value_pairs_h_debug__ ("DELETE (step 4): mark block as free (write negative block size)");
                blockSize = (int16_t) -blockSize;
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: seek failed (4).");
                    // 5. (try to) roll-back
                    if (keyValuePairs<keyType, uint32_t>::insert (key, (uint32_t) blockOffset) != keyValuePairs<keyType, uint32_t>::OK) {
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost the file, this would cause all disk related operations from now on to fail
                    }
                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                }
                if (__dataFile__.write ((byte *) &blockSize, sizeof (blockSize)) != sizeof (blockSize)) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: write failed (4).");
                     // 5. (try to) roll-back
                    if (keyValuePairs<keyType, uint32_t>::insert (key, (uint32_t) blockOffset) != keyValuePairs<keyType, uint32_t>::OK) {
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                    }
                    Unlock (); 
                    { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                }
                __dataFile__.flush ();

                // 5. add the block to __freeBlockList__
                __persistent_key_value_pairs_h_debug__ ("DELETE (step 5): add block to free list ( offset , size ) = ( " + String (blockOffset) + " , " + String (-blockSize) + " )");
                blockSize = (int16_t) -blockSize;
                if (__freeBlocksList__.push_back ( {(uint32_t) blockOffset, blockSize} ) != __freeBlocksList__.OK) {
                    __persistent_key_value_pairs_h_debug__ ("Delete: push_back failed (5).");
                    // it is not really important to return with an error here, persistentKeyValuePairs can continue working with this error 
                     
                    // return keyValuePairs<keyType, uint32_t>::lastErrorCode = keyValuePairs<keyType, uint32_t>::BAD_ALLOC;; 
                }

                Unlock ();  
                return OK;
            }


           /*
            *  Truncates key-value pairs, returns OK or one of the error codes.
            */

            errorCode Truncate () {
                Lock (); 
                    if (__inIteration__) {
                        __persistent_key_value_pairs_h_debug__ ("Truncate: can not truncate while iterating.");
                        Unlock (); 
                        { lastErrorCode = NOT_WHILE_ITERATING; return NOT_WHILE_ITERATING; }
                    }

                    if (__dataFile__) __dataFile__.close (); 

                    __dataFile__ = fileSystem.open (__dataFileName__, "w", true);
                    if (__dataFile__) {
                        __dataFile__.close (); 
                    } else {
                        __persistent_key_value_pairs_h_debug__ ("Truncate: failed creating the data file.");
                        Unlock ();  
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }

                    __dataFile__ = fileSystem.open (__dataFileName__, "r+", false);
                    if (!__dataFile__) {
                        __persistent_key_value_pairs_h_debug__ ("constructor: failed opening the data file.");
                        Unlock ();  
                        { lastErrorCode = FILE_IO_ERROR; return FILE_IO_ERROR; }
                    }

                    __dataFileSize__ = 0; 
                    keyValuePairs<keyType, uint32_t>::clear ();
                    __freeBlocksList__.clear ();

                Unlock ();  
                return OK;
            }


           /*
            *  The following iterator overloading is needed so that the calling program can iterate with key-blockOffset pair instead of key-value (value holding the blockOffset) pair.
            *  
            *  Unfortunately iterator's * oprator returns a pointer reather than the reference so the calling program should use "->key" instead of ".key" when
            *  refering to key (and blockOffset):
            *
            *      for (auto p: pkvpA) {
            *          // keys are always kept in memory and are obtained fast
            *          Serial.print (p->key); Serial.print (", "); Serial.print (p->blockOffset); Serial.print (" -> "); 
            *          
            *          // values are read from disk, obtaining a value may be much slower
            *          String value;
            *          persistentKeyValuePairs<int, String>::errorCode e = pkvpA.FindValue (p->key, &value, p->blockOffset);
            *          if (e == pkvpA.OK) 
            *              Serial.println (value);
            *          else
            *              Serial.printf ("Error: %s\n", pkvpA.errorCodeText (e));
            *      }
            */

            struct keyBlockOffsetPair {
                keyType key;          // node key
                uint32_t blockOffset; // __dataFile__ offset of block containing a key-value pair
            };        

            class Iterator : public keyValuePairs<keyType, uint32_t>::Iterator {
                public:
            
                    Iterator (persistentKeyValuePairs* pkvp, int8_t stackSize) : keyValuePairs<keyType, uint32_t>::Iterator (pkvp, stackSize) {
                        __pkvp__ = pkvp;
                        if (__pkvp__) {
                            __pkvp__->Lock (); 
                            __pkvp__->__inIteration__ ++;
                        }
                    }

                    ~Iterator () {
                        if (__pkvp__) {
                            __pkvp__->__inIteration__ --;
                            __pkvp__->Unlock (); 
                        }
                    }

                    // keyBlockOffsetPair * operator * () const { return (keyBlockOffsetPair *) &keyValuePairs<keyType, uint32_t>::Iterator::operator *(); }
                    keyBlockOffsetPair& operator * () const { return (keyBlockOffsetPair&) keyValuePairs<keyType, uint32_t>::Iterator::operator *(); }

                private:
          
                    persistentKeyValuePairs* __pkvp__;

            };      

            Iterator begin () { return Iterator (this, this->height ()); } 
            Iterator end ()   { return Iterator (NULL, 0); } 


           /*
            * Locking mechanism
            */

            void Lock () { xSemaphoreTakeRecursive (__semaphore__, portMAX_DELAY); } 

            void Unlock () { xSemaphoreGiveRecursive (__semaphore__); }


            #ifdef __PERSISTENT_KEY_VALUE_PAIRS_H_DEBUG__

                void debug () {
                    Serial.println ("-----debugKeyValuePairs-----");
                        // keyValuePairs<keyType, uint32_t>::debug ();
                    Serial.println ("=====debugKeyValuePairs=====");

                    Serial.println ("-----iterateKeyValuePairs-----");
                        Serial.printf ("no of kvp: %i   no of pkvp: %i\n", keyValuePairs<keyType, uint32_t>::size (), size ());
                        for (auto e = keyValuePairs<keyType, uint32_t>::begin (); e != keyValuePairs<keyType, uint32_t>::end (); ++ e) {
                            // Serial.print (e.key); Serial.print (" - "); Serial.println (e.value);
                            Serial.println (" - ");
                        }
                    Serial.println ("=====iterateKeyValuePairs=====");

                    Serial.println ("-----debugDataFileStructure-----");
                        uint64_t blockOffset = 0;

                        while (blockOffset < __dataFileSize__ &&  blockOffset <= 0xFFFFFFFF) { // max uint32_t
                            int16_t blockSize;
                            keyType key;
                            valueType value;

                            Serial.println ("debug [blockOffset = " + String (blockOffset));
                            if (!__readBlock__ (blockSize, key, value, (uint32_t) blockOffset, true)) {
                                Serial.println ("       __readBlock__ failed\n]");
                                return;
                            }
                            if (blockSize > 0) { 
                                Serial.println ("       block is used, blockSize: " + String (blockSize));
                                Serial.println ("       key: " + String (key));
                                Serial.println ("      ]");
                            } else { 
                                Serial.println ("       block is free, blockSize: " + String (blockSize));
                                Serial.println ("      ]");
                                blockSize = (int16_t) -blockSize;
                            } 

                            blockOffset += blockSize;
                        }
                    Serial.println ("=====debugDataFileStructure=====");

                    Serial.println ("-----debugFreeBlockList-----");
                        for (auto block: __freeBlocksList__) 
                            Serial.print (String (block.blockOffset) + " [" + String (block.blockSize) + "]   ");
                    Serial.println ("\n=====debugFreeBlockList=====");

                }

            #endif

        private:

            char __dataFileName__ [FILE_PATH_MAX_LENGTH + 1];
            File __dataFile__;
            uint64_t __dataFileSize__;

            struct freeBlockType {
                uint32_t blockOffset;
                int16_t blockSize;
            };
            vector<freeBlockType> __freeBlocksList__;

            SemaphoreHandle_t __semaphore__ = xSemaphoreCreateRecursiveMutex (); 
            int __inIteration__ = 0;

            
           /*
            *  Reads the value from __dataFile__.
            *  
            *  Returns success, in case of error it also sets lastErrorCode.
            *
            *  This function does not handle the __semaphore__.
            */

            bool __readBlock__ (int16_t& blockSize, keyType& key, valueType& value, uint32_t blockOffset, bool skipReadingValue = false) {
                // reposition file pointer to the beginning of a block
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    lastErrorCode = FILE_IO_ERROR;
                    return false;
                }

                // read block size
                if (__dataFile__.read ((uint8_t *) &blockSize, sizeof (int16_t)) != sizeof (blockSize)) {
                    lastErrorCode = FILE_IO_ERROR;
                    return false;
                }
                // if block is free the reading is already done
                if (blockSize < 0) { 
                    return true;
                }

                // read key
                if (std::is_same<keyType, String>::value) { // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    // read the file until 0 is read
                    while (__dataFile__.available ()) { 
                            char c = (char) __dataFile__.read (); 
                            if (!c) break;
                            if (!((String *) &key)->concat (c)) {
                                lastErrorCode = BAD_ALLOC;
                                return false;
                            }
                    }
                } else {
                    // fixed size key
                    if (__dataFile__.read ((uint8_t *) &key, sizeof (key)) != sizeof (key)) {
                        lastErrorCode = FILE_IO_ERROR;
                        return false;
                    }                                
                }

                // read value
                if (!skipReadingValue) {
                    if (std::is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        // read the file until 0 is read
                        while (__dataFile__.available ()) { 
                                char c = (char) __dataFile__.read (); 
                                if (!c) break;
                                if (!((String *) &value)->concat (c)) {
                                    lastErrorCode = BAD_ALLOC;
                                    return false;     
                                }
                        }
                    } else {
                        // fixed size value
                        if (__dataFile__.read ((uint8_t *) &value, sizeof (value)) != sizeof (value)) {
                            lastErrorCode = FILE_IO_ERROR;
                            return false;      
                        }                                
                    }
                }

                return true;
            }

    };

#endif
