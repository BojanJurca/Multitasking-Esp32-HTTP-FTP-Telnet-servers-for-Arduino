/*
 
    syslog.h

    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template

      - writing system messages to /var/log/syslog

    February, 14, 2022, Bojan Jurca
    
*/


    // ----- includes, definitions and supporting functions -----
    
    #include <WiFi.h>


#ifndef __SYSLOG__
  #define __SYSLOG__


    // ----- functions and variables in this modul -----

    void syslog (String message);
    

    // ----- TUNNING PARAMETERS -----

    #define SYSLOG_MAX_LENGTH 256 * 1024 // max size of /var/log/syslog so that it wouldn't occupy the whole disk space


    // ----- CODE -----

    #include "file_system.h"


    void syslog (String message) {
      if (!__fileSystemMounted__) {
        #ifdef __DMESG__
          dmesg ("[sylog] is not working: ", (char *) message.c_str ());
          return;
        #endif
      }
      File f = FFat.open ("/var/log/syslog", FILE_APPEND);
      if (!f) {
        if (!isDirectory ("/var/log")) { FFat.mkdir ("/var"); FFat.mkdir ("/var/log"); } // location of /var/log/syslog
      }
      // try opening /var/log/syslog max 4 more times
      int i = 4;
      while (!f && i --) {
        delay (10);
        f = FFat.open ("/var/log/syslog", FILE_APPEND);
      }
      if (!f) {
        #ifdef __DMESG__
          dmesg ("[sylog] can't open syslog: ", (char *) message.c_str ());
          return;
        #endif          
      }

      // check /var/log/syslog size
      if (f.size () >= SYSLOG_MAX_LENGTH) {
        #ifdef __DMESG__
          dmesg ("[sylog] syslog is full: ", (char *) message.c_str ());
        #endif          
        f.close ();
        return;
      }

      // write the message and close
      char c [25];
      #ifdef __TIME_FUNCTIONS__
        time_t now = getGmt ();
        if (now) {
          struct tm st = timeToStructTime (timeToLocalTime (now));
          strftime (c, sizeof (c), "[%y/%m/%d %H:%M:%S] ", &st);
        } else {
          sprintf (c, "[%10lu] ", millis ());
        }
      #else
        sprintf (c, "[%10lu] ", millis ());
      #endif
      
      if (f.println (String (c) + message) <= 0) {
        #ifdef __DMESG__
          dmesg ("[sylog] can't write syslog: ", (char *) message.c_str ());
        #endif          
      }
      #ifdef __PERFMON__
        __perfFSBytesWritten__ += strlen (c) + message.length () + 2; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
      #endif                                          
      // DEUG:
      Serial.println (String (c) + message);
      size_t fileSize = f.size ();
      f.close ();
      if (fileSize >= SYSLOG_MAX_LENGTH) { // try switching syslog
        FFat.remove ("/var/log/syslog.bkp");
        FFat.rename ("/var/log/syslog", "/var/log/syslog.bkp");
      }
    }

#endif
