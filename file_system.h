/*
 * 
 * File_system.h 
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template.ino project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 *
 *  Only flash disk formating and mounting of SPIFFS file system is defined here, a documentation on SPIFFS
 *  can be found here: http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html
 * 
 * INSTRUCTIONS
 * 
 *  Run Esp32_web_ftp_telnet_server_template the first time with FILE_SYSTEM_MOUNT_METHOD == FORMAT_IF_MOUNT_FAILS or FORMAT_BEFORE_MOUNT
 *  Once the file system is formatted there's no need of formatting it again, you can switch
 *  FILE_SYSTEM_MOUNT_METHOD to DO_NOT_FORMAT
 * 
 * History:
 *          - first release, November 15, 2018, Bojan Jurca
 *  
 */


#ifndef __FILE_SYSTEM__
  #define __FILE_SYSTEM__
  
  
  // change this definitions according to your needs
  
  #define FORMAT_BEFORE_MOUNT       1                             // this option is normally used ony the first time you run Esp32_web_ftp_telnet_server_template on ESP32
  #define FORMAT_IF_MOUNT_FAILS     2                             // this option is normally used ony the first time you run Esp32_web_ftp_telnet_server_template on ESP32
  #define DO_NOT_FORMAT             3                             // once the formatting is complete, this option is preferable
  // select one of the above file system mount method
  #define FILE_SYSTEM_MOUNT_METHOD  FORMAT_BEFORE_MOUNT

  
  bool SPIFFSmounted = false;                                     // use this variable to check if file system has mounted before I/O operations
  
  
  #include <FS.h>
  #include <SPIFFS.h>
  
  
  bool mountSPIFFS () {                                           // mount file system by calling this function
    #if FILE_SYSTEM_MOUNT_METHOD == FORMAT_BEFORE_MOUNT
      Serial.printf ("[file system] formatting, please wait ... "); 
      if (SPIFFS.format ()) {
        Serial.printf ("formatted\n");
        if (SPIFFS.begin (false)) {
          Serial.printf ("[file system] SPIFFS mounted\n");
          return SPIFFSmounted = true;
        } else {
          Serial.printf ("[file system] SPIFFS mount failed\n");
          return false;      
        }
      } else {
        Serial.printf ("[file system] SPIFFS formatting failed\n");
        return false;
      }
    #elif FILE_SYSTEM_MOUNT_METHOD == FORMAT_IF_MOUNT_FAILS
      if (SPIFFS.begin (false)) {
        Serial.printf ("[file system] SPIFFS mounted\n");
        return SPIFFSmounted = true;
      } else {
        Serial.printf ("[file system] formatting, please wait ... "); 
        if (SPIFFS.format ()) {
          Serial.printf ("formatted\n");
          if (SPIFFS.begin (false)) {
            Serial.printf ("[file system] SPIFFS mounted\n");
            return SPIFFSmounted = true;
          } else {
            Serial.printf ("[file system] SPIFFS mount failed\n");
            return false;      
          }
        } else {
          Serial.printf ("[file system] SPIFFS formatting failed\n");
          return false;
        }
      }
    #else // DO_NOT_FORMAT
      if (SPIFFS.begin (false)) {
        Serial.printf ("[file system] SPIFFS mounted\n");
        return SPIFFSmounted = true;
      } else {
        Serial.printf ("[file system] SPIFFS mount failed\n");
        return false;      
      }
    #endif
  }

  
  String readEntireTextFile (char *fileName) { // reads entire file into String (ignoring \r) - it is supposed to be used for small files
    String s = "";
    char c;
    File file;
    if ((bool) (file = SPIFFS.open (fileName, FILE_READ)) && !file.isDirectory ()) {
      while (file.available ()) if ((c = file.read ()) != '\r') s += String (c);
      file.close (); 
    } else {
      Serial.printf ("[file system] can't read %s\n", fileName);
    }
    return s;  
  }

#endif
