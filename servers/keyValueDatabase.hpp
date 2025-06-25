/*
    keyValueDatabase.hpp for Arduino (ESP boards with flash disk)
    
    This file is part of Key-value-database-for-Arduino: https://github.com/BojanJurca/Key-value-database-for-Arduino

    Key-value-database-for-Arduino may be used as a simple database with the following functions (see examples in BasicUsage.ino).
    The functions are thread-safe:

        - Insert (key, value)                                   - inserts a new key-value pair

        - FindBlockOffset (key)                                 - searches (memory) Map for key
        - FindValue (key, optional block offset)                - searches (memory) Map for blockOffset connected to key and then it reads the value from (disk) data file (it works slightly faster if block offset is already known, such as during iterations)

        - Update (key, new value, optional block offset)        - updates the value associated by the key (it works slightly faster if block offset is already known, such as during iterations)
        - Update (key, callback function, optional blockoffset) - if the calculation is made with existing value then this is prefered method, since calculation is performed while database is being loceks

        - Upsert (key, new value)                               - update the value if the key already exists, else insert a new one
        - Upsert (key, callback function, default value)        - if the calculation is made with existing value then this is prefered method, since calculation is performed while database is being loceks

        - Delete (key)                                          - deletes key-value pair identified by the key
        - Truncate                                              - deletes all key-value pairs

        - Iterate                                               - iterate (list) through all the keys and their blockOffsets with an Iterator

        - Lock                                                  - locks (takes the semaphore) to (temporary) prevent other taska accessing keyValueDatabase
        - Unlock                                                - frees the lock

    Data storage structure used for keyValueDatabase:

    (disk) keyValueDatabase consists of:
        - data file
        - (memory) Map that keep keys and pointers (offsets) to data in the data file
        - (memory) vector that keeps pointers (offsets) to free blocks in the data file
        - semaphore to synchronize (possible) multi-tasking accesses to keyValueDatabase

    (disk) data file structure:
        - data file consists consecutive of blocks
        - Each block starts with int16_t number which denotes the size of the block (in bytes). If the number is positive the block is considered to be used
          with useful data, if the number is negative the block is considered to be deleted (free). Positive int16_t numbers can vary from 0 to 32768, so
          32768 is the maximum size of a single data block.
        - after the block size number, a key and its value are stored in the block (only if the block is beeing used).

    (memory) Map structure:
        - the key is the same key as used for keyValueDatabase
        - the value is an offset to data file block containing the data, keyValueDatabase' value will be fetched from there. Data file offset is
          stored in uint32_t so maximum data file offest can theoretically be 4294967296, but ESP32 files can't be that large.

    (memory) vector structure:
        - a free block list vector contains structures with:
        - data file offset (uint16_t) of a free block
        - size of a free block (int16_t)
 
    May 22, 2025, Bojan Jurca
  
*/


#ifndef __KEY_VALUE_DATABASE_HPP__
    #define __KEY_VALUE_DATABASE_HPP__


    // ----- TUNNING PARAMETERS -----

    #define __KEY_VALUE_DATABASE_PCT_FREE__ 0.2 // how much space is left free in data block to let data "breed" a little - only makes sense for String values 

    // #define __USE_KEY_VALUE_DATABASE_EXCEPTIONS__   // uncomment this line if you want Map to throw exceptions



    // ----- CODE -----
    
    #include "std/Map.hpp"
    #include "std/vector.hpp"

    // error flags - only tose not defined in Map.hpp, please, note that all error flgs are negative (char) numbers
    #define err_data_changed    ((signed char) 0b10010000) // -112 - unexpected data value found
    #define err_file_io         ((signed char) 0b10100000) //  -96 - file operation error
    #define err_cant_do_it_now  ((signed char) 0b11000000) //  -64 - for example changing the data while iterating or loading the data if already loaded
    // the other error codes #defined in Map.hpp and vector.hpp
    // #define err_ok              ((signed char) 0b00000000)  //    0 - no error
    // #define err_not_unique      ((signed char) 0b10001000)  // -120 - key is not unique
    // #define err_not_found       ((signed char) 0b10000100)  // -124 - key is not found
    // #define err_out_of_range    ((signed char) 0b10000010)  // -126 - invalid index      
    // #define err_bad_alloc       ((signed char) 0b10000001)  // -127 - out of memory


    #ifdef SEMAPHORE_H // RTOS is running beneath Arduino sketch, multitasking (and semaphores) is supported
        static SemaphoreHandle_t __keyValueDatabaseSemaphore__ = xSemaphoreCreateMutex (); 
    #endif

    template <class keyType, class valueType> class keyValueDatabase : private Map<keyType, uint32_t> {
        
        friend class Proxy;
  
        private:

            signed char __errorFlags__ = 0;


        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }

            ~keyValueDatabase () { 
                Close ();
            } 


           /*
            *  Loads the data from data file.
            *  
            */
            
            signed char Open (const char *dataFileName) {

                Lock ();
                if (__dataFile__) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_cant_do_it_now;
                    #endif
                    __errorFlags__ |= err_cant_do_it_now;
                    Unlock (); 
                    return err_cant_do_it_now;
                }

                // load new data
                strcpy (__dataFileName__, dataFileName);

                __dataFile__ = fileSystem.open (dataFileName, "r+"); // , false);
                if (!__dataFile__) {
                    __dataFile__ = fileSystem.open (dataFileName, "w"); // , true);
                    if (__dataFile__) {
                          __dataFile__.close (); 
                          __dataFile__ = fileSystem.open (dataFileName, "r+"); // , false);
                    } else {
                        Unlock (); 
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock ();
                        return err_file_io;
                    }
                }

                if (__dataFile__.isDirectory ()) {
                    __dataFile__.close ();
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }

                __dataFileSize__ = __dataFile__.size ();         
                uint64_t blockOffset = 0;

                while (blockOffset < __dataFileSize__ &&  blockOffset <= 0xFFFFFFFF) { // max uint32_t
                    int16_t blockSize;
                    keyType key;
                    valueType value;

                    signed char e = __readBlock__ (blockSize, key, value, (uint32_t) blockOffset, true);
                    if (e) { // != OK
                        __dataFile__.close ();
                        Unlock (); 
                        return e;
                    }
                    if (blockSize > 0) { // block containining the data -> insert into Map
                        signed char e = Map<keyType, uint32_t>::insert (key, (uint32_t) blockOffset);
                        if (e) { // != OK
                            __dataFile__.close ();
                            __errorFlags__ |= Map<keyType, uint32_t>::errorFlags ();
                            Unlock (); 
                            return e;
                        }
                    } else { // free block -> insert into __freeBlockList__
                        blockSize = (int16_t) -blockSize;
                        signed char e = __freeBlocksList__.push_back ( {(uint32_t) blockOffset, blockSize} );
                        if (e) { // != OK
                            __dataFile__.close ();
                            __errorFlags__ |= __freeBlocksList__.errorFlags ();
                            Unlock (); 
                            return e;
                        }
                    } 

                    blockOffset += blockSize;
                }

                Unlock (); 

                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[key-value database] " << dataFileName << " opened" << ", free heap left: " << esp_get_free_heap_size ();
                #endif

                return err_ok;
            }

            void Close () {
                if (__dataFile__) {
                    __dataFile__.close ();
                }
            }


           /*
            *  Returns true if the data has already been successfully loaded from data file.
            *  
            */

            bool dataLoaded () {
                return __dataFile__;
            }


           /*
            * Returns the lenght of a data file.
            */

            unsigned long dataFileSize () { return __dataFileSize__; } 


           /*
            * Returns the number of key-value pairs.
            */

            int size () { return Map<keyType, uint32_t>::size (); }


           /*
            *  Inserts a new key-value pair, returns OK or one of the error codes.
            */

            signed char Insert (keyType key, valueType value) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                if (is_same<valueType, String>::value)                                                                        // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &value) {                                                                                 // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock (); 
                if (__inIteration__) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_cant_do_it_now;
                    #endif
                    __errorFlags__ |= err_cant_do_it_now;
                    Unlock (); 
                    // DEBUG: Serial.print ("   Insert ("); Serial.print (key); Serial.print (", "); Serial.print (value); Serial.println (" can't Insert while iterating");
                    return err_cant_do_it_now;
                }

                // 1. get ready for writting into __dataFile__
                size_t dataSize = sizeof (int16_t); // block size information
                size_t blockSize = dataSize;
                if (is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &key)->length () + 1); // add 1 for closing 0
                    blockSize += (((String *) &key)->length () + 1) + (((String *) &key)->length () + 1) * __KEY_VALUE_DATABASE_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size key
                    dataSize += sizeof (keyType);
                    blockSize += sizeof (keyType);
                }                
                if (is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &value)->length () + 1); // add 1 for closing 0
                    blockSize += (((String *) &value)->length () + 1) + (((String *) &value)->length () + 1) * __KEY_VALUE_DATABASE_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size value
                    dataSize += sizeof (valueType);
                    blockSize += sizeof (valueType);
                }
                if (blockSize > 32768) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    Unlock (); 
                    return err_bad_alloc;
                }

                // 2. search __freeBlocksList__ for most suitable free block, if it exists
                int freeBlockIndex = -1;
                uint32_t minWaste = 0xFFFFFFFF;
                for (int i = 0; i < __freeBlocksList__.size (); i ++) {
                    if (__freeBlocksList__ [i].blockSize >= dataSize && __freeBlocksList__ [i].blockSize - dataSize < minWaste) {
                        freeBlockIndex = i;
                        minWaste = __freeBlocksList__ [i].blockSize - dataSize;
                    }
                }

                // 3. reposition __dataFile__ pointer
                uint32_t blockOffset;                
                if (freeBlockIndex == -1) { // append data to the end of __dataFile__
                    blockOffset = __dataFileSize__;
                } else { // writte data to free block in __dataFile__
                    blockOffset = __freeBlocksList__ [freeBlockIndex].blockOffset;
                    blockSize = __freeBlocksList__ [freeBlockIndex].blockSize;
                }
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }

                // 4. update (memory) Map structure 
                signed char e = Map<keyType, uint32_t>::insert (key, blockOffset);
                if (e) { // != OK
                    __errorFlags__ |= e;
                    Unlock (); 
                    return e;
                }

                // 5. construct the block to be written
                byte *block = (byte *) malloc (blockSize);
                if (!block) {

                    // 7. (try to) roll-back
                    signed char e = Map<keyType, uint32_t>::erase (key);
                    if (e) { // != OK
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        __errorFlags__ |= e;
                        Unlock (); 
                        return e;
                    }
                    // roll-back succeded
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    Unlock (); 
                    return err_bad_alloc;
                }

                int16_t i = 0;
                int16_t bs = (int16_t) blockSize;
                memcpy (block + i, &bs, sizeof (bs)); i += sizeof (bs);
                if (is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    size_t l = ((String *) &key)->length () + 1; // add 1 for closing 0
                    memcpy (block + i, ((String *) &key)->c_str (), l); i += l;
                } else { // fixed size key
                    memcpy (block + i, &key, sizeof (key)); i += sizeof (key);
                }       
                if (is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    size_t l = ((String *) &value)->length () + 1; // add 1 for closing 0
                    memcpy (block + i, ((String *) &value)->c_str (), l); i += l;
                } else { // fixed size value
                    memcpy (block + i, &value, sizeof (value)); i += sizeof (value);
                }

                // 6. write block to __dataFile__
                if (__dataFile__.write (block, blockSize) != blockSize) {
                    free (block);

                    // 9. (try to) roll-back
                    if (__dataFile__.seek (blockOffset, SeekSet)) {
                        blockSize = (int16_t) -blockSize;
                        if (__dataFile__.write ((byte *) &blockSize, sizeof (blockSize)) != sizeof (blockSize)) { // can't roll-back
                            __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        }
                    } else { // can't roll-back
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                    }
                    __dataFile__.flush ();

                    signed char e = Map<keyType, uint32_t>::erase (key);
                    if (e) { // != OK
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        __errorFlags__ |= Map<keyType, uint32_t>::errorFlags ();
                        Unlock (); 
                        return e;
                    }
                    // roll-back succeded
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }

                // write succeeded
                __dataFile__.flush ();
                free (block);

                // 8. roll-out
                if (freeBlockIndex == -1) { // data appended to the end of __dataFile__
                    __dataFileSize__ += blockSize;       
                } else { // data written to free block in __dataFile__
                    __freeBlocksList__.erase (__freeBlocksList__.begin () + freeBlockIndex); // doesn't fail
                }
                
                Unlock (); 
                return err_ok;
            }


           /*
            *  Retrieve blockOffset from (memory) Map, so it is fast.
            */

            signed char FindBlockOffset (keyType key, uint32_t& blockOffset) {
                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock ();
                Map<keyType, uint32_t>::clearErrorFlags ();
                auto p = Map<keyType, uint32_t>::find (key);
                if (p != Map<keyType, uint32_t>::end ()) { // if found
                    blockOffset = p->second;
                    Unlock ();  
                    return err_ok;
                } else { // not found or error
                    signed char e = Map<keyType, uint32_t>::errorFlags ();
                    if (e) { // error
                        __errorFlags__ |= e;
                        Unlock ();  
                        return e;
                    } else {
                        // __errorFlags__ |= err_not_found; // do not flag this error, just return err_not_found
                        Unlock ();  
                        return err_not_found;                      
                    }
                }
            }


           /*
            *  Read the value from (disk) __dataFile__, so it is slow. 
            */

            signed char FindValue (keyType key, valueType *value, uint32_t blockOffset = 0xFFFFFFFF) { 
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                keyType storedKey = {};

                Lock (); 

                if (blockOffset == 0xFFFFFFFF) { // if block offset was not specified find it from Map
                    Map<keyType, uint32_t>::clearErrorFlags ();
                    auto p = Map<keyType, uint32_t>::find (key); 
                    if (p == Map<keyType, uint32_t>::end ()) { // if not found or error
                        signed char e = Map<keyType, uint32_t>::errorFlags ();
                        if (e) { // error
                            __errorFlags__ |= e;
                            Unlock ();  
                            return e;
                        } else {
                            // __errorFlags__ |= err_not_found; // do not flag tis error, just return err_not_found
                            Unlock ();
                            return err_not_found;
                        }
                    }
                    blockOffset = p->second;
                }

                int16_t blockSize;
                if (!__readBlock__ (blockSize, storedKey, *value, blockOffset)) {
                    if (blockSize > 0 && storedKey == key) {
                        Unlock ();  
                        return err_ok; // success  
                    } else {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_data_changed;
                        #endif
                        __errorFlags__ |= err_data_changed;
                        Unlock ();  
                        return err_data_changed; // shouldn't happen, but check anyway ...
                    }
                } else {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock ();  
                    return err_file_io; 
                } 
            }


           /*
            *  Updates the value associated with the key
            */

            signed char Update (keyType key, valueType newValue, uint32_t *pBlockOffset = NULL) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                if (is_same<valueType, String>::value)                                                                        // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &newValue) {                                                                              // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock (); 

                // 1. get blockOffset
                if (!pBlockOffset) { // find block offset if not provided by the calling program

                    Map<keyType, uint32_t>::clearErrorFlags ();
                    auto p = Map<keyType, uint32_t>::find (key);
                    if (p == Map<keyType, uint32_t>::end ()) { // if not found
                        signed char e = Map<keyType, uint32_t>::errorFlags ();
                        if (e) { // error
                            __errorFlags__ |= e;
                            Unlock ();  
                            return e;
                        } else {
                            __errorFlags__ |= err_not_found;
                            Unlock ();
                            return err_not_found;
                        }
                    }
                    pBlockOffset = &(p->second);
                } else {
                    ; // step 1: block offset already profided by the calling program
                }

                // 2. read the block size and stored key
                int16_t blockSize;
                size_t newBlockSize;
                keyType storedKey;
                valueType storedValue;

                signed char e  = __readBlock__ (blockSize, storedKey, storedValue, *pBlockOffset, true);
                if (e) { // != OK
                    Unlock ();  
                    return err_file_io;
                } 
                if (blockSize <= 0 || storedKey != key) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_data_changed;
                    #endif
                    __errorFlags__ |= err_data_changed;
                    Unlock ();  
                    return err_data_changed; // shouldn't happen, but check anyway ...
                }
                // 3. calculate new block and data size
                size_t dataSize = sizeof (int16_t); // block size information
                newBlockSize = dataSize;
                if (is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &key)->length () + 1); // add 1 for closing 0
                    newBlockSize += (((String *) &key)->length () + 1) + (((String *) &key)->length () + 1) * __KEY_VALUE_DATABASE_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size key
                    dataSize += sizeof (keyType);
                    newBlockSize += sizeof (keyType);
                }                
                if (is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    dataSize += (((String *) &newValue)->length () + 1); // add 1 for closing 0
                    newBlockSize += (((String *) &newValue)->length () + 1) + (((String *) &newValue)->length () + 1) * __KEY_VALUE_DATABASE_PCT_FREE__ + 0.5; // add PCT_FREE for Strings
                } else { // fixed size value
                    dataSize += sizeof (valueType);
                    newBlockSize += sizeof (valueType);
                }
                if (newBlockSize > 32768) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_bad_alloc;
                    #endif
                    __errorFlags__ |= err_bad_alloc;
                    Unlock ();
                    return err_bad_alloc;
                }

                // 4. decide where to write the new value: existing block or a new one
                if (dataSize <= blockSize) { // there is enough space for new data in the existing block - easier case
                    uint32_t dataFileOffset = *pBlockOffset + sizeof (int16_t); // skip block size information
                    if (is_same<keyType, String>::value) { // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        dataFileOffset += (((String *) &key)->length () + 1); // add 1 for closing 0
                    } else { // fixed size key
                        dataFileOffset += sizeof (keyType);
                    }                

                    // 5. write new value to __dataFile__
                    if (!__dataFile__.seek (dataFileOffset, SeekSet)) {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock ();  
                        return err_file_io;
                    }
                    int bytesToWrite;
                    int bytesWritten;
                    if (is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        bytesToWrite = (((String *) &newValue)->length () + 1);
                        bytesWritten = __dataFile__.write ((byte *) ((String *) &newValue)->c_str () , bytesToWrite);
                    } else {
                        bytesToWrite = sizeof (newValue);
                        bytesWritten = __dataFile__.write ((byte *) &newValue , bytesToWrite);
                    }
                    if (bytesWritten != bytesToWrite) { // file IO error, it is highly unlikely that rolling-back to the old value would succeed
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock ();  
                        return err_file_io;
                    }

                    // success
                    __dataFile__.flush ();
                    Unlock ();  
                    return err_ok;

                } else { // existing block is not big eneugh, we'll need a new block - more difficult case

                    // 6. search __freeBlocksList__ for most suitable free block, if it exists
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
                        newBlockOffset = __dataFileSize__;
                    } else { // writte data to free block in __dataFile__
                        newBlockOffset = __freeBlocksList__ [freeBlockIndex].blockOffset;
                        newBlockSize = __freeBlocksList__ [freeBlockIndex].blockSize;
                    }
                    if (!__dataFile__.seek (newBlockOffset, SeekSet)) {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock (); 
                        return err_file_io;
                    }

                    // 8. construct the block to be written
                    byte *block = (byte *) malloc (newBlockSize);
                    if (!block) {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        Unlock (); 
                        return err_bad_alloc;
                    }

                    int16_t i = 0;
                    int16_t bs = (int16_t) newBlockSize;
                    memcpy (block + i, &bs, sizeof (bs)); i += sizeof (bs);
                    if (is_same<keyType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        size_t l = ((String *) &key)->length () + 1; // add 1 for closing 0
                        memcpy (block + i, ((String *) &key)->c_str (), l); i += l;
                    } else { // fixed size key
                        memcpy (block + i, &key, sizeof (key)); i += sizeof (key);
                    }       
                    if (is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        size_t l = ((String *) &newValue)->length () + 1; // add 1 for closing 0
                        memcpy (block + i, ((String *) &newValue)->c_str (), l); i += l;
                    } else { // fixed size value
                        memcpy (block + i, &newValue, sizeof (newValue)); i += sizeof (newValue);
                    }

                    // 9. write new block to __dataFile__
                    if (__dataFile__.write (block, dataSize) != dataSize) {
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
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock (); 
                        return err_file_io;
                    }
                    free (block);
                    __dataFile__.flush ();

                    // 11. roll-out
                    if (freeBlockIndex == -1) { // data appended to the end of __dataFile__
                        __dataFileSize__ += newBlockSize;
                    } else { // data written to free block in __dataFile__
                        __freeBlocksList__.erase (__freeBlocksList__.begin () + freeBlockIndex); // doesn't fail
                    }
                    // mark old block as free
                    if (!__dataFile__.seek (*pBlockOffset, SeekSet)) {
                        __dataFile__.close (); // data file is corrupt (it contains two entries with the same key) and it is not likely we can roll it back
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock (); 
                        return err_file_io;
                    }
                    blockSize = (int16_t) -blockSize;
                    if (__dataFile__.write ((byte *) &blockSize, sizeof (blockSize)) != sizeof (blockSize)) {
                        __dataFile__.close (); // data file is corrupt (it contains two entries with the same key) and it si not likely we can roll it back
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock (); 
                        return err_file_io;
                    }
                    __dataFile__.flush ();
                    // update __freeBlocklist__
                    if (__freeBlocksList__.push_back ( {*pBlockOffset, (int16_t) -blockSize} )) { // != OK
                        ;
                    }
                    // update Map information
                    *pBlockOffset = newBlockOffset; // there is no reason this would fail
                    Unlock ();  
                    return err_ok;
                }

                // Unlock ();  
                // return err_ok;
            }


           /*
            *  Updates the value associated with the key throught callback function (usefull for counting, etc, when all the calculation should be done while locking is in place)
            */

            signed char Update (keyType key, void (*updateCallback) (valueType &value), uint32_t *pBlockOffset = NULL) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock (); 

                valueType value;
                signed char e = FindValue (key, &value); 
                if (e) {
                    __errorFlags__ |= e;
                    Unlock ();  
                    return e;
                }

                updateCallback (value);

                e = Update (key, value, pBlockOffset); 
                if (e) {
                    __errorFlags__ |= e;
                    Unlock ();  
                    return e;
                }                

                Unlock ();
                return err_ok;
            }


           /*
            *  Updates or inserts key-value pair
            */

            signed char Upsert (keyType key, valueType newValue) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                if (is_same<valueType, String>::value)                                                                        // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &newValue) {                                                                              // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock ();
                signed char e;
                e = Insert (key, newValue);
                if (e == err_not_unique) {
                    e = Update (key, newValue, NULL);
                }
                if (e) { // != OK
                    __errorFlags__ |= e;
                } else {
                    ;
                } 
                Unlock ();
                return e; 
            }

           /*
            *  Updates or inserts the value associated with the key throught callback function (usefull for counting, etc, when all the calculation should be done while locking is in place)
            */

            signed char Upsert (keyType key, void (*updateCallback) (valueType &value), valueType defaultValue) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                   // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                if (is_same<valueType, String>::value)                                                                        // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &defaultValue) {                                                                         // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock (); 
                signed char e;
                e = Insert (key, defaultValue);
                if (e) // != OK
                    e = Update (key, updateCallback);
                if (e) { // != OK
                    __errorFlags__ |= e;
                } else {
                    ;
                }
                Unlock ();
                return e; 
            }

           /*
            *  Updates or inserts the value associated with the key throught callback function (usefull for counting, etc, when all the calculation should be done while locking is in place)
            */

            signed char Upsert (keyType key, void (*upsertCallback) (valueType &value), uint32_t *pBlockOffset = NULL) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                  // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock (); 

                valueType value = {};
                signed char e = FindValue (key, &value); 
                switch (e) {
                    case err_ok:        // found
                                        upsertCallback (value);
                                        e = Update (key, value, pBlockOffset);
                                        break;
                    case err_not_found: // not found
                                        upsertCallback (value);
                                        e = Insert (key, value);
                                        break;
                    default:            // errror
                                        __errorFlags__ |= e;
                                        Unlock ();  
                                        return e;
                }
                
                if (e) {
                    __errorFlags__ |= e;
                    Unlock ();  
                    return e;
                }                

                Unlock ();
                return err_ok;
            }


           /*
            *  Deletes key-value pair, returns OK or one of the error codes.
            */

            signed char Delete (keyType key) {
                if (!__dataFile__) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io; 
                }

                if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    if (!*(String *) &key) {                                                                                  // ... check if parameter construction is valid
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_bad_alloc;
                        #endif
                        __errorFlags__ |= err_bad_alloc;
                        return err_bad_alloc;
                    }

                Lock (); 

                if (__inIteration__) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_cant_do_it_now;
                    #endif
                    __errorFlags__ |= err_cant_do_it_now;
                    Unlock (); 
                    return err_cant_do_it_now;
                }

                // 1. get blockOffset
                uint32_t blockOffset;
                signed char e = FindBlockOffset (key, blockOffset);
                if (e) { // != OK
                    Unlock (); 
                    return e;
                }

                // 2. read the block size
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }
                int16_t blockSize;
                if (__dataFile__.read ((uint8_t *) &blockSize, sizeof (int16_t)) != sizeof (blockSize)) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }
                if (blockSize < 0) { 
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_data_changed;
                    #endif
                    __errorFlags__ |= err_data_changed;
                    Unlock (); 
                    return err_data_changed; // shouldn't happen, but check anyway ...
                }

                // 3. erase the key from Map
                e = Map<keyType, uint32_t>::erase (key);
                if (e) { // != OK
                    __errorFlags__ |= e;
                    Unlock (); 
                    return e;
                }

                // 4. write back negative block size designating a free block
                blockSize = (int16_t) -blockSize;
                if (!__dataFile__.seek (blockOffset, SeekSet)) {

                    // 5. (try to) roll-back
                    if (Map<keyType, uint32_t>::insert (key, (uint32_t) blockOffset)) { // != OK
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost the file, this would cause all disk related operations from now on to fail
                    }
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }
                if (__dataFile__.write ((byte *) &blockSize, sizeof (blockSize)) != sizeof (blockSize)) {
                     // 5. (try to) roll-back
                    if (Map<keyType, uint32_t>::insert (key, (uint32_t) blockOffset)) { // != OK
                        __dataFile__.close (); // memory key value pairs and disk data file are synchronized any more - it is better to clost he file, this would cause all disk related operations from now on to fail
                    }
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    Unlock (); 
                    return err_file_io;
                }
                __dataFile__.flush ();

                // 5. roll-out
                // add the block to __freeBlockList__
                blockSize = (int16_t) -blockSize;
                if (__freeBlocksList__.push_back ( {(uint32_t) blockOffset, blockSize} )) { // != OK
                    // it is not really important to return with an error here, keyValueDatabase can continue working with this error
                }

                Unlock ();  
                return err_ok;
            }


           /*
            *  [] operator enables key-valu database elements to be conveniently addressed by their keys like:
            *
            *    value = kvp [key];
            *       or
            *    kvp [key] = value;
            */

            // Proxy class declaration
            class Proxy {

                private:
                    keyValueDatabase *__parent__;
                    keyType __key__;

                public:
                    Proxy (keyValueDatabase *parent, keyType *key) {
                        __parent__ = parent;
                        __key__ = *key;

                        if (is_same<keyType, String>::value)                                                                          // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &__key__) {                                                                              // ... check if parameter construction is valid
                                #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                                    throw err_bad_alloc;
                                #endif
                                __parent__->__errorFlags__ |= err_bad_alloc;
                            }

                        __parent__->Lock ();
                    }

                    ~Proxy () {
                        __parent__->Unlock ();
                    }

                    // conversion operator to support reading
                    operator valueType () const {                
                        valueType value = {};
                        signed char e = __parent__->FindValue (__key__, &value);
                        if (e == err_not_found) // flag err_not_found
                            __parent__->__errorFlags__ |= err_not_found;
                        // DEBUG: Serial.print ("   find ["); Serial.print (__key__); Serial.print ("] = "); Serial.println (value);

                        return value;
                    }

                    // assignment operator to support writing
                    Proxy& operator = (valueType value) {

                        if (is_same<valueType, String>::value)                                                                        // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &value) {                                                                                // ... check if parameter construction is valid
                                #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                                    throw err_bad_alloc;
                                #endif
                                __parent__->__errorFlags__ |= err_bad_alloc;
                                return *this;
                            }

                        __parent__->Upsert (__key__, value);
                        return *this;
                    }

                    // prefix ++ operator
                    Proxy& operator ++ () {
                        valueType value = {};
                        __parent__->FindValue (__key__, &value); // if error or not found we start with 0
                        ++ value;

                        return operator = (value);
                    }

                    // postfix ++ operator
                    valueType operator ++ (int n) {
                        static int result = *this;
                        operator ++ ();
                        return (valueType) result;
                    }

                    // prefix -- operator
                    Proxy& operator -- () {
                        valueType value = {};
                        __parent__->FindValue (__key__, &value); // if error or not found we start with 0
                        -- value;

                        return operator = (value);
                    }

                    // postfix -- operator
                    valueType operator -- (int n) {
                        static int result = *this;
                        operator -- ();
                        return (valueType) result;
                    }

                    template<typename T>
                    Proxy& operator += (T other) {
                        valueType value = {};
                        if (__parent__->FindValue (__key__, &value)) // error or not found
                            return *this;

                        if (is_same<valueType, String>::value) {                                                                      // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                            if (!*(String *) &other) {                                                                                 // ... check if parameter construction is valid
                                #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                                    throw err_bad_alloc;
                                #endif
                                __parent__->__errorFlags__ |= err_bad_alloc;
                                return *this;
                            }
                            if (!(*(String *) &value).concat (*(String *) &other)) {
                                __parent__->__errorFlags__ |= err_bad_alloc;
                                return *this;
                            }
                        } else {
                            value += other;
                        }

                        return operator = (value);
                    }


                    template<typename T>
                    Proxy& operator -= (T other) {
                        valueType value = {};
                        if (__parent__->FindValue (__key__, &value)) // error or not found
                            return *this;

                        value -= other;

                        return operator = (value);
                    }                    

                    template<typename T>
                    Proxy& operator *= (T other) {
                        valueType value = {};
                        if (__parent__->FindValue (__key__, &value)) // error or not found
                            return *this;

                        value *= other;

                        return operator = (value);
                    }

                    template<typename T>
                    Proxy& operator /= (T other) {
                        valueType value = {};
                        if (__parent__->FindValue (__key__, &value)) // error or not found
                            return *this;

                        value /= other;

                        return operator = (value);
                    }                    
            };

            // Implementation of operator []
            Proxy operator [] (keyType key) {
                return Proxy (this, &key);
            }


           /*
            *  Truncates key-value pairs, returns OK or one of the error codes.
            */

            signed char Truncate () {
                
                Lock (); 
                    if (__inIteration__) {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_cant_do_it_now;
                        #endif
                        __errorFlags__ |= err_cant_do_it_now;
                        Unlock (); 
                        return err_cant_do_it_now;
                    }

                    if (__dataFile__) __dataFile__.close (); 

                    __dataFile__ = fileSystem.open (__dataFileName__, "w"); // , true);
                    if (__dataFile__) {
                        __dataFile__.close (); 
                    } else {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock ();  
                        return err_file_io;
                    }

                    __dataFile__ = fileSystem.open (__dataFileName__, "r+"); // , false);
                    if (!__dataFile__) {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        Unlock ();  
                        return err_file_io;
                    }

                    __dataFileSize__ = 0; 
                    Map<keyType, uint32_t>::clear ();
                    __freeBlocksList__.clear ();

                Unlock ();  
                return err_ok;
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
            *          keyValueDatabase<int, String>::errorCode e = pkvpA.FindValue (p->key, &value, p->blockOffset);
            *          if (e == pkvpA.OK) 
            *              Serial.println (value);
            *          else
            *              Serial.printf ("Error: %s\n", pkvpA.errorCodeText (e));
            *      }
            */

            struct keyBlockOffsetPair {
                keyType key;          // key
                uint32_t blockOffset; // __dataFile__ offset of block containing both: key-value pair
            };        

            class iterator : public Map<keyType, uint32_t>::iterator {
                public:
            
                    // there are 2 cases when constructor gets called: begin (pointToFirstPair = true) and end (pointToFirstPair = false) 
                    iterator (keyValueDatabase* pkvp, bool pointToFirstPair) : Map<keyType, uint32_t>::iterator (pkvp, pointToFirstPair) {
                        __pkvp__ = pkvp;
                    }

                    ~iterator () {
                        if (__pkvp__) {
                            __pkvp__->__inIteration__ --;
                            // DEBUG: Serial.print ("   stopped itetating, count = "); Serial.println (__pkvp__->__inIteration__);
                            __pkvp__->Unlock (); 
                        }
                    }

                    keyBlockOffsetPair& operator * () { return (keyBlockOffsetPair&) Map<keyType, uint32_t>::iterator::operator *(); }

                    // this will tell if iterator is valid (if there are not elements the iterator can not be valid)
                    operator bool () const { return __pkvp__->size () > 0; }


                private:
          
                    keyValueDatabase* __pkvp__ = NULL;

            };

            iterator begin () { // since only the begin () instance is neede for iteration we'll do the locking here
                Lock (); // Unlock () will be called in instance destructor
                __inIteration__ ++; // -- will be called in instance destructor
                // DEBUG: Serial.print ("   startted itetating (begin), count = "); Serial.println (__inIteration__);
                return iterator (this, true); 
            } 

            iterator end () { 
                Lock (); // Unlock () will be called in instance destructor
                __inIteration__ ++; // -- will be called in instance destructor
                // DEBUG: Serial.print ("   startted itetating (end), count = "); Serial.println (__inIteration__);
                return iterator (this, false); 
            } 


           /*
            *  Finds min and max keys in keyValueDatabase.
            *
            *  Example:
            *  
            *    auto firstElement = pkvpA.first_element ();
            *    if (firstElement) // check if first element is found (if pkvpA is not empty)
            *        Serial.printf ("first element (min key) of pkvpA = %i\n", (*firstElement)->key);
            */

          iterator first_element () { 
              Lock (); // Unlock () will be called in instance destructor
              __inIteration__ ++; // -- will be called in instance destructor
              return iterator (this, this->height ());  // call the 'begin' constructor
          }

          iterator last_element () {
              Lock (); // Unlock () will be called in instance destructor
              __inIteration__ ++; // -- will be called in instance destructor
              return iterator (this->height (), this);  // call the 'end' constructor
          }


           /*
            * Locking mechanism
            */

            void Lock () { 
                #ifdef SEMAPHORE_H // RTOS is running beneath Arduino sketch, multitasking (and semaphores) is supported
                    xSemaphoreTakeRecursive (__semaphore__, portMAX_DELAY); 
                #endif
            } 

            void Unlock () { 
                #ifdef SEMAPHORE_H // RTOS is running beneath Arduino sketch, multitasking (and semaphores) is supported
                    xSemaphoreGiveRecursive (__semaphore__); 
                #endif
            }


        private:

            char __dataFileName__ [255] = "";
            File __dataFile__;
            unsigned long __dataFileSize__ = 0;

            struct freeBlockType {
                uint32_t blockOffset;
                int16_t blockSize;
            };
            vector<freeBlockType> __freeBlocksList__;

            #ifdef SEMAPHORE_H // RTOS is running beneath Arduino sketch, multitasking (and semaphores) is supported
                SemaphoreHandle_t __semaphore__ = xSemaphoreCreateRecursiveMutex (); 
            #endif
            int __inIteration__ = 0;

            // som boards do no thave is_same implemented, so we have to imelement it ourselves: https://stackoverflow.com/questions/15200516/compare-typedef-is-same-type
            template<typename T, typename U> struct is_same { static const bool value = false; };
            template<typename T> struct is_same<T, T> { static const bool value = true; };

            
           /*
            *  Reads the value from __dataFile__.
            *  
            *  Returns success, in case of error it also sets lastErrorCode.
            *
            *  This function does not handle the __semaphore__.
            */


            signed char __readBlock__ (int16_t& blockSize, keyType& key, valueType& value, uint32_t blockOffset, bool skipReadingValue = false) {
                // reposition file pointer to the beginning of a block
                if (!__dataFile__.seek (blockOffset, SeekSet)) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;
                    return err_file_io;
                }

                // read block size
                if (__dataFile__.read ((uint8_t *) &blockSize, sizeof (int16_t)) != sizeof (blockSize)) {
                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                        throw err_file_io;
                    #endif
                    __errorFlags__ |= err_file_io;                    
                    return err_file_io;
                }
                // if block is free the reading is already done
                if (blockSize < 0) { 
                    return err_ok;
                }

                // read key
                if (is_same<keyType, String>::value) { // if key is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                    // read the file until 0 is read
                    while (__dataFile__.available ()) { 
                            char c = (char) __dataFile__.read (); 
                            if (!c) break;
                            if (!((String *) &key)->concat (c)) {
                                #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                                    throw err_bad_alloc;
                                #endif
                                __errorFlags__ |= err_bad_alloc;
                                return err_bad_alloc;
                            }
                    }
                } else {
                    // fixed size key                
                    if (__dataFile__.read ((uint8_t *) &key, sizeof (key)) != sizeof (key)) {
                        #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                            throw err_file_io;
                        #endif
                        __errorFlags__ |= err_file_io;
                        return err_file_io;
                    }                                
                }

                // read value
                if (!skipReadingValue) {
                    if (is_same<valueType, String>::value) { // if value is of type String ... (if anyone knows hot to do this in compile-time a feedback is welcome)
                        // read the file until 0 is read               
                        while (__dataFile__.available ()) { 
                                char c = (char) __dataFile__.read (); 
                                if (!c) break;
                                if (!((String *) &value)->concat (c)) {
                                    #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                                        throw err_bad_alloc;
                                    #endif
                                    __errorFlags__ |= err_bad_alloc;
                                    return err_bad_alloc;     
                                }
                        }
                    } else {
                        // fixed size value               
                        if (__dataFile__.read ((uint8_t *) &value, sizeof (value)) != sizeof (value)) {
                            #ifdef __USE_KEY_VALUE_DATABASE_EXCEPTIONS__
                                throw err_file_io;
                            #endif
                            __errorFlags__ |= err_file_io;
                            return err_file_io;      
                        }                                
                    }
                }

                return err_ok;
            }

    };


    #ifndef __FIRST_LAST_ELEMENT__ 
        #define __FIRST_LAST_ELEMENT__

        template <typename T>
        typename T::iterator first_element (T& obj) { return obj.first_element (); }

        template <typename T>
        typename T::iterator last_element (T& obj) { return obj.last_element (); }

    #endif


#endif
