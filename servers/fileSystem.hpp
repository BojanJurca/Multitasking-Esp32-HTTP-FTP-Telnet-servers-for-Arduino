/*
  
    fileSystem.hpp 
    
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
    
    May 22, 2025, Bojan Jurca

    LittleFS or FFAT (and/or SD) for ESP32 built-in flash disk

        https://github.com/espressif/arduino-esp32/blob/master/libraries/FS/src/FS.cpp
        https://github.com/espressif/arduino-esp32/blob/master/libraries/FFat/src/FFat.h
        https://github.com/espressif/arduino-esp32/tree/master/libraries/SD

*/


    // ----- includes, definitions and supporting functions -----

    #include <FS.h>
    // fixed size strings
    #include "std/Cstring.hpp"
    #include "std/console.hpp"


#ifndef __FILE_SYSTEM__
    #define __FILE_SYSTEM__

    #ifdef SHOW_COMPILE_TIME_INFORMATION
        #pragma message "__FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__ __FILE_SYSTEM__"
    #endif


    // TUNNING PARAMETERS

    #define FILE_PATH_MAX_LENGTH (256 - 1) // the number of characters of longest full file path on FAT (not counting closing 0) 

    // choose file system (it must correspond to Tools | Partition scheme setting)
        #define FILE_SYSTEM_FAT      1   
        #define FILE_SYSTEM_LITTLEFS 2
        #define FILE_SYSTEM_SD_CARD  4
    // #define FILE_SYSTEM  FILE_SYSTEM_LITTLEFS // one of the above or combination (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)

    #ifndef FILE_SYSTEM
        // try guessing which file system to use (if some libraries are already included)
        #ifdef LFS_H 
            // LittleFS file system library already included
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "FILE_SYSTEM was not defined previously, #defining the FILE_SYSTEM_LITTLEFS in fileSystem.hpp"
            #endif
            #define FILE_SYSTEM FILE_SYSTEM_LITTLEFS // by default
        #else
            #ifdef _FFAT_H
                // FFat file system library already included
                #ifdef _SD_H_ 
                    // SD file system library already included (besides Ffat)
                    #ifdef SHOW_COMPILE_TIME_INFORMATION
                        #pragma message "FILE_SYSTEM was not defined previously, #defining the (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD) in fileSystem.hpp"
                    #endif
                    #define FILE_SYSTEM (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD) 
                #else
                    // use only Ffat
                    #ifdef SHOW_COMPILE_TIME_INFORMATION
                        #pragma message "FILE_SYSTEM was not defined previously, #defining the default FILE_SYSTEM_FAT in fileSystem.hpp"
                    #endif
                    #define FILE_SYSTEM FILE_SYSTEM_FAT
                #endif
            #else
                #ifdef _SD_H_ 
                    // SD file system library already included
                    #ifdef SHOW_COMPILE_TIME_INFORMATION
                        #pragma message "FILE_SYSTEM was not defined previously, #defining the FILE_SYSTEM_SD_CARD in fileSystem.hpp"
                    #endif
                    #define FILE_SYSTEM FILE_SYSTEM_SD_CARD
                #else
                    // no file system library already included
                    #ifdef SHOW_COMPILE_TIME_INFORMATION
                        #pragma message "FILE_SYSTEM was not defined previously, #defining the default FILE_SYSTEM_LITTLEFS in fileSystem.hpp"
                    #endif
                    #define FILE_SYSTEM FILE_SYSTEM_LITTLEFS // by default
                #endif
            #endif
        #endif
    #endif

    #if (FILE_SYSTEM & (FILE_SYSTEM_FAT | FILE_SYSTEM_LITTLEFS)) == (FILE_SYSTEM_FAT | FILE_SYSTEM_LITTLEFS)
        #pragma error "FILE_SYSTEM can't be both, FILE_SYSTEM_FAT and FILE_SYSTEM_LITTLEFS, at the same time"
    #endif

    #if FILE_SYSTEM == FILE_SYSTEM_FAT
        #include <FFat.h>
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Compiling fileSystem.hpp for FAT file system"
        #endif
        #define __fileSystem__ FFat
    #endif
    #if FILE_SYSTEM == FILE_SYSTEM_LITTLEFS
        #include <LittleFS.h>
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Compiling fileSystem.hpp for LittleFS file system"
        #endif
        #define __fileSystem__ LittleFS
    #endif
    #if (FILE_SYSTEM & FILE_SYSTEM_SD_CARD) == FILE_SYSTEM_SD_CARD // FILE_SYSTEM == FILE_SYSTEM_SD_CARD or FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
        #include <SD.h>
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Compiling fileSystem.hpp for SD card"
        #endif
        #define __fileSystem__ SD
    #endif


    // ----- CODE -----


    class fileSystem_t { 

        public:

            // log disk Traffic information
            struct diskTraffic_t {
                unsigned long bytesRead;
                unsigned long bytesWritten;
            };
            diskTraffic_t diskTraffic = {}; // measure disk Traffic on ESP32 level


            fileSystem_t () {} // constructor

            // format (only the built-in flash disk can be formated with LittleFs or FAT, SD card can not be formatted)

            #if (FILE_SYSTEM & FILE_SYSTEM_FAT) == FILE_SYSTEM_FAT
                inline bool formatFAT () __attribute__((always_inline)) { return __fileSystem__.format (); }
            #endif
            #if (FILE_SYSTEM & FILE_SYSTEM_LITTLEFS) == FILE_SYSTEM_LITTLEFS
                inline bool formatLittleFs () __attribute__((always_inline)) { return __fileSystem__.format (); }
            #endif


            // s built-in flash disk

            #if (FILE_SYSTEM & FILE_SYSTEM_FAT) == FILE_SYSTEM_FAT // FILE_SYSTEM == FILE_SYSTEM_SD_CARD or FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                bool mountFAT (bool formatIfUnformatted) { 
                    if (mounted ()) return false; // already mounted

                    if (!__fileSystem__.begin (false)) {
                        if (formatIfUnformatted) {
                            cout << "[fileSystem] formatting FAT, please wait ... ";
                            if (__fileSystem__.format ()) {
                                cout << "[fileSystem] ... formatted\n";
                                #ifdef __DMESG__
                                    dmesgQueue << "[fileSystem] formatted";
                                #endif
                                __fileSystem__.begin (false);
                            } else {
                                cout << "[fileSystem] ... formatting failed\n";
                                #ifdef __DMESG__
                                    dmesgQueue << "[fileSystem] formatting failed";
                                #endif
                            }
                        }
                    }

                    if (mounted ()) {
                        cout << "[fileSystem] FAT mounted\n";
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] FAT mounted, free heap left: " << esp_get_free_heap_size ();
                        #endif
                        return true;
                    } else { 
                        cout << "[fileSystem] failed to mount FAT\n";
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] failed to mount FAT";
                        #endif
                        return false;
                    }
                }

            #endif
            #if FILE_SYSTEM == FILE_SYSTEM_LITTLEFS
                bool mountLittleFs (bool formatIfUnformatted) { 
                    if (mounted ()) return false; // already mounted
                    
                    if (!__fileSystem__.begin (false)) {
                        if (formatIfUnformatted) {
                            cout << "[fileSystem] formatting LittleFS, please wait ... ";
                            if (__fileSystem__.format ()) {
                                cout << "[fileSystem] ... formatted\n";
                                #ifdef __DMESG__
                                    dmesgQueue << "[fileSystem] formatted";
                                #endif
                                __fileSystem__.begin (false);
                            } else {
                                cout <<  "[fileSystem] ... formatting failed\n";
                                #ifdef __DMESG__
                                    dmesgQueue << "[fileSystem] formatting failed";
                                #endif
                            }
                        }
                    }

                    if (mounted ()) {
                        cout << "[fileSystem] LittleFS mounted\n";
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] LittleFS mounted, free heap left: " << esp_get_free_heap_size ();
                        #endif
                        return true;
                    } else {
                        cout << "[fileSystem] failed to mount LittleFS\n";
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] failed to mount LittleFS";
                        #endif
                        return false;
                    }
                }

            #endif

            // mounts SD card, provide pin connected to CS and a mount point for SD card to the fileSystem
            #if FILE_SYSTEM  == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                bool mountSD (const char *mountPoint = "/SD", uint8_t pinCS = 5) {
                    if (SDmounted ()) return false; // already mounted

                    if (!mountPoint || *mountPoint != '/' || strlen (mountPoint) >= sizeof (__SDcardMountPoint__) - 1) {
                        cout << "[fileSystem] invalid SD card mount point: " << mountPoint << endl;
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] invalid SD card mount point: " << mountPoint; 
                        #endif
                        return false;
                    }

                    strcpy (__SDcardMountPoint__, mountPoint); // we've made the length checking before
                    if (__SDcardMountPoint__ [strlen (__SDcardMountPoint__) - 1] != '/') strcat (__SDcardMountPoint__, "/"); // __SDcardMountPoint__ now always ends with /


                    // if builtin flash disk file system is already mounted then create mountPoint directory on builtin flash disk which would make operations like list directory much easier 
                    // note that SD card is not mounted yet at this point so mountPOint will not get redirected to SD
                    if (mounted ()) {
                        if (!isDirectory (mountPoint)) {
                            // create directory tree
                            for (int i = 1; i < strlen (__SDcardMountPoint__); i++) {
                                if (__SDcardMountPoint__ [i] == '/') {
                                    __SDcardMountPoint__ [i] = 0;
                                    makeDirectory (__SDcardMountPoint__);
                                    __SDcardMountPoint__ [i] = '/';
                                }
                            }
                            // check success
                            if (!isDirectory (mountPoint)) {
                                cout << "[fileSystem] can't make the SD mount point directory: " << mountPoint << endl;
                                #ifdef __DMESG__
                                    dmesgQueue << "[fileSystem] can't make the SD mount point directory: " << mountPoint; 
                                #endif
                                return false;
                            }
                        }  
                    }
                     
                    if (!SD.begin (pinCS)) {
                        cout <<  "[fileSystem] failed to mount SD card\n";
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] failed to mount SD card"; 
                        #endif
                        return false;
                    }

                    if (SD.cardType () == CARD_NONE) {
                        cout <<  "[fileSystem] no SD card attached\n";
                        #ifdef __DMESG__
                            dmesgQueue << "[fileSystem] no SD card attached"; 
                        #endif
                        return false;
                    }

                    cout << "[fileSystem] SD card mounted to " << mountPoint << endl;
                    #ifdef __DMESG__
                        dmesgQueue << "[fileSystem] SD card mounted to " << mountPoint << ", free heap left: " << esp_get_free_heap_size ();
                    #endif

                    return true;
                } 
            #endif

            #if FILE_SYSTEM  == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                inline char *SDcardMountPoint () __attribute__((always_inline)) { return __SDcardMountPoint__; }
            #endif            

            // unmounts file system

            void unmount () { __fileSystem__.end (); }

            #if FILE_SYSTEM  == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                void unmountSD () { SD.end (); }
            #endif     


            // tells is a file system is mounted - this covers also the case when SD is used as main and only file system
            bool mounted () { // if file system is mounted or not 
                #if FILE_SYSTEM == FILE_SYSTEM_LITTLEFS
                    return LittleFS.totalBytes (); // if LittleFS is not mounted totalBytes returns 0
                #elif FILE_SYSTEM == FILE_SYSTEM_FAT
                    return FFat.totalBytes (); // if FFat is not mounted totalBytes returns 0 
                #elif FILE_SYSTEM == FILE_SYSTEM_SD_CARD
                    return SD.totalBytes (); // if SD is not mounted totalBytes returns 0 
                #elif FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    return FFat.totalBytes (); // if FFat is not mounted totalBytes returns 0 
                #elif
                    #pragma error "FILE_SYSTEM is not correctly #defined" 
                #endif
            }

            // is SD card mounted to FAT mount point - only in case when SD is used besides FAT, which is the main file system
            #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                bool SDmounted () { return SD.totalBytes (); } // if SD is not mounted totalBytes returns 0
            #endif


            // tells wether the path points to builtin flash disk or to SD card mount point
            #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                bool pointsToSDcard (const char *fullPath) {
                    if (!SDmounted ()) return false;
                    if (strstr (fullPath, __SDcardMountPoint__) == fullPath) return true; // fullPath points to SD card like mountPoint = /SD/ and fullPath = /SD/a.txt
                    if (strstr (__SDcardMountPoint__, fullPath) == __SDcardMountPoint__ && strlen (fullPath) == strlen (__SDcardMountPoint__) - 1) return true; // fullPath points to SD card like mountPoint = /SD/ and fullPath = /SD
                    return false;
                }
            #endif     


            // calculates path to SD card from full path ant SD card mount point
            #if (FILE_SYSTEM & FILE_SYSTEM_SD_CARD) == FILE_SYSTEM_SD_CARD
                const char *SDcardPath (const char *fullPath) {
                    return strlen (fullPath) < strlen (__SDcardMountPoint__) ? "/" : fullPath + strlen (__SDcardMountPoint__) - 1;
                }
            #endif                 


            // opens the file

            File open (const char* path) { 
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (path)) return SD.open (SDcardPath (path));  
                #endif

                // directory points to main file system
                return __fileSystem__.open (path); 
            }

            File open (const char* path, const char* mode) { 
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (path)) return SD.open (SDcardPath (path), mode);  
                #endif

                // directory points to main file system
                return __fileSystem__.open (path, mode); 
            }

            File open (const char* path, const char* mode, bool create) { 
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (path)) return SD.open (SDcardPath (path), mode, create);  
                #endif

                // directory points to main file system
                return __fileSystem__.open (path, mode, create); 
            }


            // reads entire configuration file in the buffer - returns success, it also removes \r characters, double spaces, comments, ...
            bool readConfigurationFile (char *buffer, size_t bufferSize, const char *fileName) {
                *buffer = 0;
                int i = 0; // index in the buffer
                bool beginningOfLine = true;  // beginning of line
                bool inComment = false;       // if reading comment text
                char lastCharacter = 0;       // the last character read from the file
            
                File f = open (fileName, "r");
                if (f) {
                    if (!f.isDirectory ()) {
                      
                      while (f.available ()) { 
                          char c = (char) f.read (); 
                          switch (c) {
                              case '\r':  break; // igonore \r
                              case '\n':  inComment = false; // \n terminates comment
                                          if (beginningOfLine) break; // ignore 
                                          if (i > 0 && buffer [i - 1] == ' ') i--; // right trim (we can not reach the beginning of the line - see left trim)
                                          goto processNormalCharacter;
                              case '=':
                              case '\t':
                              case ' ':   if (inComment) break; // ignore
                                          if (beginningOfLine) break; // left trim - ignore
                                          if (lastCharacter == ' ') break; // trim in the middle - ignore
                                          c = ' ';
                                          goto processNormalCharacter;
                              case '#':   if (beginningOfLine) inComment = true; // mark comment and ignore
                                          goto processNormalCharacter;
                              default:   
            processNormalCharacter:
                                          if (inComment) break; // ignore
                                          if (i > bufferSize - 2) { f.close (); return false; } // buffer too small
                                          buffer [i++] = lastCharacter = c; // copy space to the buffer                       
                                          beginningOfLine = (c == '\n');
                                          break;
                            }
                        }
                        buffer [i] = 0; 
                        f.close ();
                        return true;
                    }
                    f.close ();
                }             
                return false; // can't open the file or it is a directory
            }

            // deletes file

            bool deleteFile (const char *fileName) {
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (fileName)) {
                        if (!SD.remove (SDcardPath (fileName))) {
                            #ifdef __DMESG__
                                dmesgQueue << "[fileSystem][SD] unable to delete " << fileName; 
                            #endif
                            return false;
                        }
                        return true;
                    }
                #endif

                // main file system
                if (!__fileSystem__.remove (fileName)) {
                    #ifdef __DMESG__
                        dmesgQueue << "[fileSystem] unable to delete " << fileName; 
                    #endif
                    return false;
                }
                return true;    
            }

            // makes directory

            bool makeDirectory (const char *directory) {
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (directory)) {
                        if (!SD.mkdir (SDcardPath (directory))) {
                            #ifdef __DMESG__
                                dmesgQueue << "[fileSystem][SD] unable to make " << directory; 
                            #endif
                            return false;
                        }
                        return true;
                    }
                #endif

                // main file system
                if (!__fileSystem__.mkdir (directory)) {
                    #ifdef __DMESG__
                        dmesgQueue << "[fileSystem] unable to make " << directory;
                    #endif
                    return false;
                }
                return true;    
            }

            // removes directory

            bool removeDirectory (const char *directory) {
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (directory)) {
                        // is it a SD card mount point?
                        if (strlen (directory) == strlen (__SDcardMountPoint__) - 1) {
                            #ifdef __DMESG__
                                dmesgQueue << "[fileSystem][SD] can not remove SC card mount point " << directory; 
                            #endif
                            return false;
                        }

                        if (!SD.rmdir (SDcardPath (directory))) {
                            #ifdef __DMESG__
                                dmesgQueue << "[fileSystem][SD] unable to remove " << directory; 
                            #endif
                            return false;
                        }
                        return true;
                    }
                #endif

                // main file system
                if (!__fileSystem__.rmdir (directory)) {
                    #ifdef __DMESG__
                        dmesgQueue << "[fileSystem] unable to remove " << directory;
                    #endif
                    return false;
                }
                return true;    
            }


            // renames file or directory

            bool rename (const char* pathFrom, const char* pathTo) {
                // builtin flash disk or SD card?
                #if FILE_SYSTEM == (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                    if (pointsToSDcard (pathFrom)) {
                        if (pointsToSDcard (pathTo)) {
                            if (!SD.rename (SDcardPath (pathFrom), SDcardPath (pathTo))) {
                                #ifdef __DMESG__
                                    dmesgQueue << "[fileSystem][SD] unable to rename " << pathFrom;
                                #endif
                                return false;
                            }
                            return true;
                        } else {
                            #ifdef __DMESG__
                                dmesgQueue << "[fileSystem][SD] can't mix builtin flash disk and SD card with rename";
                            #endif
                            return false;
                        }
                    } 
                #endif
                
                // both paths point to main file system
                if (!__fileSystem__.rename (pathFrom, pathTo)) {
                    #ifdef __DMESG__
                        dmesgQueue << "[fileSystem] unable to rename " << pathFrom; 
                    #endif
                    return false;
                }
                return true; 
            }


            // makes the full path out of relative path and working directory, for example if working directory is /usr and relative patj is a.txt then full path is /usr/a.txt

            cstring makeFullPath (const char *relativePath, const char *workingDirectory) { 
                char *p = relativePath [0] ? (char *) relativePath : (char *) "/"; // relativePath should never be empty

                cstring s;
                if (p [0] == '/') { // if path begins with / then it is already supposed to be fullPath
                    s = p; 
                } else if (!strcmp (p, ".")) { // . means the working directory
                    s = workingDirectory;
                } else if (p [0] == '.' && p [1] == '/') { // if path begins with ./ then fullPath = workingDirectory + the rest of paht
                    s = workingDirectory;
                    if (s [s.length () - 1] != '/') s += '/'; 
                    s += (p + 2); 
                } else { // else fullPath = workingDirectory + path
                    s = workingDirectory;
                    if (s [s.length () - 1] != '/') s += '/'; 
                    s += p; 
                }

                if (s [1] && s [s.length () - 1] == '/') s [s.length () - 1] = 0; // remove trailing '/' if exists

                // resolve (possibly multiple) ..
                int i = 0;
                while (i >= 0) {
                    switch ((i = s.indexOf ((char *) "/.."))) {
                        case -1:  return s;  // no .. found, nothing to resolve any more
                        case  0:  return ""; // s begins with /.. - error in path
                        default:             // restructure path
                                  int j = s.substring (0, i - 1).lastIndexOf ('/');
                                  if (j < 0) return ""; // error in path - should't happen
                                  s = s.substring (0, j) + s.substring (i + 3);
                                  if (s == "") s = "/";
                    }
                }

                return ""; // never executes
            }

            // is full path a directory?

            bool isDirectory (const char *fullPath) {
                bool b = false;
                File f = open (fullPath, "r");
                if (f) {
                    b = f.isDirectory ();
                    f.close ();
                }     
                return b;
            }

            // is full path a file?
  
            bool isFile (const char *fullPath) {
                bool b = false;
                File f = open (fullPath, "r");
                if (f) {
                    b = !f.isDirectory ();
                    f.close ();
                } 
                return b;
            }

            // returns information about file or directory in UNIX like text line

            cstring fileInformation (const char *fileOrDirectory, bool showFullPath = false) { // returns UNIX like text with file information
                cstring s;
                File f = open (fileOrDirectory, "r");
                if (f) { 
                    unsigned long fSize = 0;
                    struct tm fTime = {};
                    time_t lTime = f.getLastWrite ();
                    localtime_r (&lTime, &fTime);
                    sprintf ((char *) s.c_str (), "%crw-rw-rw-   1 root     root          %7lu ", f.isDirectory () ? 'd' : '-', f.size ());  // cstring = Cstring<350> so we have enough space
                    strftime ((char *) s.c_str () + strlen (s.c_str ()), 25, " %b %d %H:%M      ", &fTime);  
                    if (showFullPath || !strcmp (fileOrDirectory, "/")) {
                        s += fileOrDirectory;
                    } else {
                        int lastSlash = 0;
                        for (int i = 1; fileOrDirectory [i]; i++) if (fileOrDirectory [i] == '/') lastSlash = i;
                        s += fileOrDirectory + lastSlash + 1;
                    }
                    f.close ();
                }
                return s;
            }

        private:

            #if (FILE_SYSTEM & FILE_SYSTEM_SD_CARD) == FILE_SYSTEM_SD_CARD
                char __SDcardMountPoint__ [FILE_PATH_MAX_LENGTH] = "";
            #endif

    };


    // create a working instance before including time_functions.h, time_functions.h will use the fileSystem instance
    fileSystem_t fileSystem;


#endif
