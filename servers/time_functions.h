/*
 * 
 * time_functions.h
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 *  and Esp8266_web_ftp_telnet_server_template project:   https://github.com/BojanJurca/Esp8266_web_ftp_telnet_server_template
 * 
 *  Real_time_clock synchronizes its time with NTP server accessible from internet once a day.
 *  Internet connection is necessary for real_time_clock to work.
 * 
 *  Real_time_clock preserves accurate time even when ESP32 goes into deep sleep (which is not the same with ESP8266).
 * 
 * History:
 *          - first release, 
 *            November 14, 2018, Bojan Jurca
 *          - changed delay () to SPIFFSsafeDelay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca          
 *          - added support for all Europe time zones
 *            April 22, 2019, Bojan Jurca
 *          - added support for USA timezones 
 *            September 30, 2019, Bojan Jurca
 *          - added support for Japan, China and Canada time zones  
 *            October 8, 2019, Bojan Jurca
 *          - added support for ESP8266 and Russia time zones
 *            October 14, 2019, Bojan Jurca
 *          - minor changes
 *            November 5, 2019, Bojan Jurca
 *          - added support for ESP REST time servers,
 *            added awareness of timing preserved during ESP32 deep sleep
 *            November 10, 2019, Bojan Jurca
 *          - elimination of compiler warnings and some bugs
 *            Jun 10, 2020, Bojan Jurca     
 *          - Arduino 1.8.13 supported some features (strftime, settimeofday) that have not been supoprted on ESP8266 earyer, 
 *            (but they have been supported on ESP32). These features need no longer beeing implemented in ESP8266 part of this module any more,
 *            added support of /etc/ntp.conf configuration file,
 *            conversion of real_time_clock class into C functions
 *            October 4, 2020, Bojan Jurca
 *  
 */

#ifndef __TIME_FUNCTIONS__
  #define __TIME_FUNCTIONS__

  #include <WiFi.h>


  // DEFINITIONS - change according to your needs

  
  #define KAL_TIMEZONE 10                     // GMT + 2 ----- Russia time zones ----- // https://en.wikipedia.org/wiki/Time_in_Russia
  #define MSK_TIMEZONE 11                     // GMT + 3
  #define SAM_TIMEZONE 12                     // GMT + 4
  #define YEK_TIMEZONE 13                     // GMT + 5
  #define OMS_TIMEZONE 14                     // GMT + 6
  #define KRA_TIMEZONE 15                     // GMT + 7
  #define IRK_TIMEZONE 16                     // GMT + 8
  #define YAK_TIMEZONE 17                     // GMT + 9
  #define VLA_TIMEZONE 18                     // GMT + 10
  #define SRE_TIMEZONE 19                     // GMT + 11
  #define PET_TIMEZONE 20                     // GMT + 12
  #define JAPAN_TIMEZONE 21                   // GMT + 9  ----- Japan time zone -----
  #define CHINA_TIMEZONE 22                   // GMT + 8  ----- China time zone -----
  #define WET_TIMEZONE      23                // Western European Time (GMT + DST from March to October) ----- Europe time zones -----  https://www.timeanddate.com/time/europe/
  #define ICELAND_TIMEZONE  24                // same as WET_TIMEZONE but without DST (GMT, no DST)
  #define CET_TIMEZONE      25                // Central European Time (GMT + 1 + DST from March to October)
  #define EET_TIMEZONE      26                // Eastern European Time (GMT + 2 + DST from March to October)
  #define FET_TIMEZONE      27                // Further-Eastern European Time (GMT + 3, no DST)
  #define NEWFOUNDLAND_TIMEZONE 28            // GMT - 3.5 + DST from March to November ----- USA & Canada time zones ---- https://en.wikipedia.org/wiki/Time_in_the_United_States
  #define ATLANTIC_TIMEZONE 29                // GMT - 4 + DST from March to November
  #define ATLANTIC_NO_DST_TIMEZONE 30         // GMT - 4, no DST
  #define EASTERN_TIMEZONE 31                 // GMT - 5 + DST from March to November
  #define EASTERN_NO_DST_TIMEZONE 32          // GMT - 5
  #define CNTRAL_TIMEZONE 33                  // GMT - 6 + DST from March to November
  #define CNTRAL_NO_DST_TIMEZONE 34           // GMT - 6 
  #define MOUNTAIN_TIMEZONE 35                // GMT - 7 + DST from March to November
  #define MOUNTAIN_NO_DST_TIMEZONE 36         // GMT - 7
  #define PACIFIC_TIMEZONE 37                 // GMT - 8 + DST from March to November
  #define ALASKA_TIMEZNE 38                   // GMT - 9 + DST from March to November
  #define HAWAII_ALEUTIAN_TIMEZONE 39         // GMT - 10 + DST from March to November
  #define HAWAII_ALEUTIAN_NO_DST_TIMEZONE 40  // GMT - 10
  #define AMERICAN_SAMOA_TIMEZONE 41          // GMT - 11
  #define BAKER_HOWLAND_ISLANDS_TIMEZONE 42   // GMT - 12
  #define WAKE_ISLAND_TIMEZONE 43             // GMT + 12
  #define CHAMORRO_TIMEZONE 44                // GMT + 10               
  
  // ... add more time zones or change timeToLocalTime function yourself
  
  #ifndef TIMEZONE
    #define TIMEZONE       CET_TIMEZONE // one of the above
  #endif

  #ifndef DEFAULT_NTP_SERVER_1
    #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"
  #endif
  #ifndef DEFAULT_NTP_SERVER_2
    #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #endif
  #ifndef DEFAULT_NTP_SERVER_3
    #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #endif


  // FUNCTIONS OF THIS MODULE


  void synchronizeTimeAndInitializeItAtFirstCall ();

  time_t getGmt ();                       // returns current GMT or 0 it the time has not been set yet 

  void setGmt (time_t t);

  time_t setgmt ();                    // returns the number of seconds ESP32 has been running

  time_t getLocalTime ();                 // returns current local time or 0 it the time has not been set yet 

  void setLocalTime (time_t t);

  time_t timeToLocalTime (time_t t);

  struct tm timeToStructTime (time_t t);

  String timeToString (time_t t);


  // VARIABLES AND FUNCTIONS TO BE USED INSIDE THIS MODULE

  time_t __readBuiltInClock__ () {
    struct timeval now;
    gettimeofday (&now, NULL);
    return now.tv_sec;    
  }

  RTC_DATA_ATTR bool __timeHasBeenSet__ = false;
  RTC_DATA_ATTR time_t __startupTime__ = 0;

  void __timeDmesg__ (String message) { 
    #ifdef __TELNET_SERVER__ 
      dmesg (message);                                              // use dmesg from telnet server if possible
    #else
      Serial.printf ("[%10lu] %s\n", millis (), message.c_str ());  // just print the message to serial console window otherwise
    #endif
  }
  void (* timeDmesg) (String) = __timeDmesg__;                      // use this pointer to display / record system messages

  portMUX_TYPE __csTime__ = portMUX_INITIALIZER_UNLOCKED;           // controll real_time_clock critical sections in multi-threaded environment

 
  // IMPLEMENTATION OF FUNCTIONS IN THIS MODULE


  String __ntpServer1__ = DEFAULT_NTP_SERVER_1;
  String __ntpServer2__ = DEFAULT_NTP_SERVER_2;
  String __ntpServer3__ = DEFAULT_NTP_SERVER_3;  
                                                          
  void synchronizeTimeAndInitializeItAtFirstCall () {    

    // Serial.println ("---------------------------");
    // Serial.println ("TIME");
    // Serial.println ("__timeHasBeenSet__ = " + String (__timeHasBeenSet__));
    // Serial.println ("__startupTime__    = " + String (__startupTime__));
    // Serial.println ("gettimeofday ()    = " + String (__readBuiltInClock__ ()));
    // Serial.println ("---------------------------");
    
    #ifdef __FILE_SYSTEM__
      if (__fileSystemMounted__) {
        
        // read NTP configuration from /etc/ntp.conf, create a new one if it doesn't exist
        String fileContent = readTextFile ("/etc/ntp.conf");
        if (fileContent == "") {
          timeDmesg ("[time] /etc/ntp.conf does not exist, creating new one.");
          // xSemaphoreTake (fileSystemSemaphore, portMAX_DELAY);
            FFat.mkdir ("/etc"); // location of this file
          // xSemaphoreGive (fileSystemSemaphore);
          
          fileContent = "# configuration for NTP - reboot for changes to take effect\r\n\r\n"
                        "server1 " + (__ntpServer1__ = DEFAULT_NTP_SERVER_1) + "\r\n"
                        "server2 " + (__ntpServer1__ = DEFAULT_NTP_SERVER_2) + "\r\n"
                        "server3 " + (__ntpServer1__ = DEFAULT_NTP_SERVER_3) + "\r\n";
          if (!writeFile (fileContent, "/etc/ntp.conf")) timeDmesg ("[time] unable to create /etc/ntp.conf");
        } else {
          // parse configuration file if it exists
          fileContent += "\n";
          __ntpServer1__ = __insideBrackets__ (fileContent, "server1","\n"); __ntpServer1__.trim ();
          __ntpServer2__ = __insideBrackets__ (fileContent, "server2","\n"); __ntpServer2__.trim ();
          __ntpServer3__ = __insideBrackets__ (fileContent, "server3","\n"); __ntpServer3__.trim ();
        }
      }
    #endif

    // run periodic synchronization with NTP servers in separate thread
    #define tskNORMAL_PRIORITY 1
    #define tskLOWER_PRIORITY tskNORMAL_PRIORITY - 1
    if (pdPASS != xTaskCreate ([] (void *) {  
      timeDmesg ("[timeSyncDeamon] started.");

      unsigned long lastSyncMillis = -86400000 + 10000; // give ESP32 10 s to setup WiFi before first sync
      
      while (true) {
nextSynchronization:

        // wait for next time synchronization in lower priority mode
        vTaskPrioritySet (NULL, tskLOWER_PRIORITY);

        while (millis () - lastSyncMillis < 86400000) delay (1); // wait 1 day

        // start time synchronization normal priority mode 
        vTaskPrioritySet (NULL, tskNORMAL_PRIORITY);

        // prepare NTP request
        byte ntpPacket [48];
        memset (ntpPacket, 0, sizeof (ntpPacket));
        ntpPacket [0] = 0b11100011;  
        ntpPacket [1] = 0;           
        ntpPacket [2] = 6;           
        ntpPacket [3] = 0xEC;  
        ntpPacket [12] = 49;
        ntpPacket [13] = 0x4E;
        ntpPacket [14] = 49;
        ntpPacket [15] = 52;  
        // send NTP request
        IPAddress ntpServerIp;
        WiFiUDP udp;
        if (!WiFi.hostByName (__ntpServer1__.c_str (), ntpServerIp))
          if (!WiFi.hostByName (__ntpServer2__.c_str (), ntpServerIp))
            if (!WiFi.hostByName (__ntpServer3__.c_str (), ntpServerIp))
              { timeDmesg ("[timeSyncDeamon] could not connect to NTP server(s)."); lastSyncMillis += 60000; goto nextSynchronization; } // wait 1 minute before trying again
        // open internal port
        #define INTERNAL_NTP_PORT 2390
        if (!udp.begin (INTERNAL_NTP_PORT))
          { timeDmesg ("[timeSyncDeamon] internal port " + String (INTERNAL_NTP_PORT) + " is not available for NTP."); lastSyncMillis += 60000; goto nextSynchronization; } // wait 1 minute before trying again 
        // start UDP
        #define NTP_PORT 123
        if (!udp.beginPacket (ntpServerIp, NTP_PORT)) 
          { udp.stop (); timeDmesg ("[timeSyncDeamon] NTP server(s) are not available on port " + String (NTP_PORT) + "."); lastSyncMillis += 60000; goto nextSynchronization; } // wait 1 minute before trying again 
        // send UDP
        if (udp.write (ntpPacket, sizeof (ntpPacket)) != sizeof (ntpPacket)) 
          { udp.stop (); timeDmesg ("[timeSyncDeamon] could not send NTP request."); lastSyncMillis += 60000; goto nextSynchronization; } // wait 1 minute before trying again 
        // check if UDP request has been sent
        if (!udp.endPacket ()) 
          { udp.stop (); timeDmesg ("[timeSyncDeamon] NTP request not sent."); lastSyncMillis += 60000; goto nextSynchronization; } // wait 1 minute before trying again 

        // wait for NTP reply or time-out
        unsigned long ntpRequestMillis = millis ();
        while (true) {
          if (millis () - ntpRequestMillis >= 500) 
            { udp.stop (); timeDmesg ("[timeSyncDeamon] NTP request time-out."); lastSyncMillis += 60000; goto nextSynchronization; } // if we don't get reply in less then 1/2 s then time sicprepancy could be too large
          if (udp.parsePacket () != sizeof (ntpPacket)) continue; // keep waiting for NTP reply
          // read NTP reply
          udp.read (ntpPacket, sizeof (ntpPacket));
          if (!ntpPacket [40] && !ntpPacket [41] && !ntpPacket [42] && !ntpPacket [43]) 
            { udp.stop (); timeDmesg ("[timeSyncDeamon] invalid NTP reply."); lastSyncMillis += 60000; goto nextSynchronization; } // sometimes we get empty reply which is invalid
          // convert reply into UNIX time
          unsigned long highWord;
          unsigned long lowWord;
          unsigned long secsSince1900;
          #define SEVENTY_YEARS 2208988800UL
          highWord = word (ntpPacket [40], ntpPacket [41]); 
          lowWord = word (ntpPacket [42], ntpPacket [43]);
          secsSince1900 = highWord << 16 | lowWord;
          time_t currentTime = secsSince1900 - SEVENTY_YEARS;
          if (currentTime < 946684800)
           { udp.stop (); timeDmesg ("[timeSyncDeamon] wrong NTP reply."); lastSyncMillis += 60000; goto nextSynchronization; } // if the time is < this millenium then it must be wrong - check just in case
          udp.stop ();
          timeDmesg ("[timeSyncDeamon] time synchronized with NTP server(s).");
          setGmt (currentTime);
          lastSyncMillis = millis ();
          break;
        }
        
      }           
      //vTaskDelete (NULL); // end this thread
      
    }, "timeSyncDeamon", 2048, NULL, tskNORMAL_PRIORITY, NULL)) timeDmesg ("[timeSyncDeamon] could not start NTP synchronization.");
  }
 
  time_t getGmt () { // returns current GMT or 0 it the time has not been set yet 
    return __timeHasBeenSet__ ? __readBuiltInClock__() : 0;
  }

  time_t getUptime () {
    return __timeHasBeenSet__ ? __readBuiltInClock__ () - __startupTime__ :  millis () / 1000;
  }
  
  void setGmt (time_t t) { // also sets/corrects __startupTime__
    __startupTime__ = t - getUptime (); // recalculate
    
    struct timeval oldTime;
    gettimeofday (&oldTime, NULL);
    struct timeval newTime = {t, 0};
    settimeofday (&newTime, NULL); 

    if (__timeHasBeenSet__) {
      timeDmesg ("[time] time corrected for " + String (newTime.tv_sec - oldTime.tv_sec) + " s to " + timeToString (timeToLocalTime (t))); 
    } else {
      __timeHasBeenSet__ = true;
      timeDmesg ("[time] time set to " + timeToString (timeToLocalTime (t))  + "."); 
    }
  }

  time_t getLocalTime () { // returns current local time or 0 it the time has not been set yet 
    return __timeHasBeenSet__ ? timeToLocalTime (__readBuiltInClock__ ()) : 0;
  }

  void setLocalTime (time_t t) { 
    setGmt (t -= timeToLocalTime (t) - t); // convert local time into GMT, then set GMT
  }

  #if TIMEZONE == KAL_TIMEZONE // GMT + 2
    time_t timeToLocalTime (time_t t) { return t + 2 * 3600; } 
  #endif
  #if TIMEZONE == MSK_TIMEZONE // GMT + 3
    time_t timeToLocalTime (time_t t) { return t + 3 * 3600; } 
  #endif
  #if TIMEZONE == SAM_TIMEZONE // GMT + 4
    time_t timeToLocalTime (time_t t) { return t + 4 * 3600; } 
  #endif
  #if TIMEZONE == YEK_TIMEZONE // GMT + 5
    time_t timeToLocalTime (time_t t) { return t + 5 * 3600; } 
  #endif
  #if TIMEZONE == OMS_TIMEZONE // GMT + 6
    time_t timeToLocalTime (time_t t) { return t + 6 * 3600; } 
  #endif
  #if TIMEZONE == KRA_TIMEZONE // GMT + 7
    time_t timeToLocalTime (time_t t) { return t + 7 * 3600; } 
  #endif
  #if TIMEZONE == IRK_TIMEZONE // GMT + 8
    time_t timeToLocalTime (time_t t) { return t + 8 * 3600; } 
  #endif
  #if TIMEZONE == YAK_TIMEZONE // GMT + 9
    time_t timeToLocalTime (time_t t) { return t + 9 * 3600; } 
  #endif
  #if TIMEZONE == VLA_TIMEZONE // GMT + 10
    time_t timeToLocalTime (time_t t) { return t + 10 * 3600; } 
  #endif
  #if TIMEZONE == SRE_TIMEZONE // GMT + 11
    time_t timeToLocalTime (time_t t) { return t + 11 * 3600; } 
  #endif
  #if TIMEZONE == PET_TIMEZONE // GMT + 12
    time_t timeToLocalTime (time_t t) { return t + 12 * 3600; } 
  #endif

  #if TIMEZONE == JAPAN_TIMEZONE // GMT + 9
    time_t timeToLocalTime (time_t t) { return t + 9 * 3600; } 
  #endif
  
  #if TIMEZONE == CHINA_TIMEZONE  // GMT + 8
    time_t timeToLocalTime (time_t t) { return t + 8 * 3600; } 
  #endif

  // in Europe, time change is defined according to GMT:
  #define PAST_EUROPE_SPRING_TIME_CHANGE (tStr.tm_mon > 2 /* Apr-Dec */ || (tStr.tm_mon == 2 /* Mar */ && tStr.tm_mday - tStr.tm_wday >= 23 /* last Sun or latter */ && (tStr.tm_wday > 0 /* Mon-Sat */ || (tStr.tm_wday == 0 /* Sun */ && tStr.tm_hour >= 1 /* >= 1:00 GMT */))))
  #define PAST_EUROPE_AUTUMN_TIME_CHANGE (tStr.tm_mon > 9 /* Nov-Dec */ || (tStr.tm_mon == 9 /* Oct */ && tStr.tm_mday - tStr.tm_wday >= 23 /* last Sun or latter */ && (tStr.tm_wday > 0 /* Mon-Sat */ || (tStr.tm_wday == 0 /* Sun */ && tStr.tm_hour >= 1 /* >= 1:00 GMT */))))

  #if TIMEZONE == WET_TIMEZONE // GMT + DST 
    time_t timeToLocalTime (time_t t) {
      time_t l = t;
      struct tm tStr = timeToStructTime (t);
      if (PAST_EUROPE_SPRING_TIME_CHANGE && !PAST_EUROPE_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }
  #endif
  #if TIMEZONE == ICELAND_TIMEZONE // GMT
    time_t timeToLocalTime (time_t t) { return t; } 
  #endif
  #if TIMEZONE == CET_TIMEZONE // GMT + 1 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t;
      struct tm tStr = timeToStructTime (t);
      if (PAST_EUROPE_SPRING_TIME_CHANGE && !PAST_EUROPE_AUTUMN_TIME_CHANGE) l += 3600;   
      return l + 3600;
    }
  #endif
  #if TIMEZONE == EET_TIMEZONE // GMT + 2 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t;
      struct tm tStr = timeToStructTime (t);
      if (PAST_EUROPE_SPRING_TIME_CHANGE && !PAST_EUROPE_AUTUMN_TIME_CHANGE) l += 3600;   
      return l + 2 * 3600;
    }
  #endif
  #if TIMEZONE == FET_TIMEZONE // GMT + 3 
    time_t timeToLocalTime (time_t t) { return t + 3 * 3600; } 
  #endif

  // in North America, time change is defined according to local time:
  #define PAST_NORTH_AMERICA_SPRING_TIME_CHANGE (lStr.tm_mon > 2 /* Apr-Dec */ || (lStr.tm_mon == 2 /* Mar */ && lStr.tm_mday - lStr.tm_wday >= 8  /* 2 nd Sun or latter */ && (lStr.tm_wday > 0 /* Mon-Sat */ || (lStr.tm_wday == 0 /* Sun */ && lStr.tm_hour >= 2 /* >= 2:00 */))))  
  #define PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE (lStr.tm_mon > 10 /* Dec */    || (lStr.tm_mon == 10 /* Nov */ && lStr.tm_mday - lStr.tm_wday >= 1 /* 1 st Sun or latter */ && (lStr.tm_wday > 0 /* Mon-Sat */ || (lStr.tm_wday == 0 /* Sun */ && lStr.tm_hour >= 1 /* >= 1:00 (without considering DST) */))))

  #if TIMEZONE == NEWFOUNDLAND_TIMEZONE // GMT - 3.5 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 12600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }
  #endif
  #if TIMEZONE == ATLANTIC_TIMEZONE // GMT -4 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 4 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }
  #endif
  #if TIMEZONE == ATLANTIC_NO_DST_TIMEZONE // GMT - 4
    time_t timeToLocalTime (time_t t) { return t - 4 * 3600; }
  #endif
  #if TIMEZONE == EASTERN_TIMEZONE // GMT - 5 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 5 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }
  #endif
  #if TIMEZONE == EASTERN_NO_DST_TIMEZONE // GMT - 5
    time_t timeToLocalTime (time_t t) { return t - 5 * 3600; }
  #endif
  #if TIMEZONE == CENTRAL_TIMEZONE // GMT - 6 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 6 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }
  #endif
  #if TIMEZONE == CENTRAL_NO_DST_TIMEZONE // GMT - 6
    time_t timeToLocalTime (time_t t) { return t - 6 * 3600; }
  #endif
  #if TIMEZONE == MOUNTAIN_TIMEZONE // GMT - 7 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 7 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }
  #endif
  #if TIMEZONE == MOUNTAIN_NO_DST_TIMEZONE // GMT - 7
    time_t timeToLocalTime (time_t t) { return t - 7 * 3600; }
  #endif  
  #if TIMEZONE == PACIFIC_TIMEZONE // GMT - 8 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 8 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }  
  #endif
  #if TIMEZONE == ALASKA_TIMEZONE // GMT - 9 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 9 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }  
  #endif
  #if TIMEZONE == HAWAII_ALEUTIAN_TIMEZONE // GMT - 10 + DST
    time_t timeToLocalTime (time_t t) {
      time_t l = t - 10 * 3600;
      struct tm lStr = timeToStructTime (l);
      if (PAST_NORTH_AMERICA_SPRING_TIME_CHANGE && !PAST_NORTH_AMERICA_AUTUMN_TIME_CHANGE) l += 3600;   
      return l;
    }  
  #endif
  #if TIMEZONE == HAWAII_ALEUTIAN_NO_DST_TIMEZONE // GMT - 10
    time_t timeToLocalTime (time_t t) { return t - 10 * 3600; }
  #endif  
  #if TIMEZONE == AMERICAN_SAMOA_TIMEZONE // GMT - 11
    time_t timeToLocalTime (time_t t) { return t - 11 * 3600; }
  #endif
  #if TIMEZONE == BAKER_HOWLAND_ISLANDS_TIMEZONE // GMT - 12
    time_t timeToLocalTime (time_t t) { return t - 12 * 3600; }
  #endif
  #if TIMEZONE == WAKE_ISLAND_TIMEZONE // GMT + 12
    time_t timeToLocalTime (time_t t) { return t + 12 * 3600; }          }
  #endif
  #if TIMEZONE == CHAMORRO_TIMEZONE // GMT + 10
    time_t timeToLocalTime (time_t t) { return t + 10 * 3600; }
  #endif

  struct tm timeToStructTime (time_t t) { 
    portENTER_CRITICAL (&__csTime__); // gmtime (&time_t) returns pointer to static structure which may be a problem in multi-threaded environment
      struct tm timeStr = *gmtime (&t);
    portEXIT_CRITICAL (&__csTime__);
    return timeStr;                                         
  }

  String timeToString (time_t t) {
    struct tm timeStr = timeToStructTime (t);
    char s [25];
    sprintf (s, "%04i/%02i/%02i %02i:%02i:%02i", 1900 + timeStr.tm_year, 1 + timeStr.tm_mon, timeStr.tm_mday, timeStr.tm_hour, timeStr.tm_min, timeStr.tm_sec);
    return String (s);
  }


  // TESTING CODE

  void __TEST_DST_TIME_CHANGE__ () {
    // FOR CET_TIMEZONE
    Serial.printf ("\n      Testing end of DST interval in CET_TIMEZONE\n");
    Serial.printf ("      GMT (UNIX) | GMT               | Local             | Local - GMT [s]\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1616893200 - 2; testTime < 1616893200 + 2; testTime ++) { 
      setGmt (testTime);
      time_t localTestTime = getLocalTime ();
      Serial.printf ("      %llu | ", (unsigned long long) testTime);
      char s [50];
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", (int) (localTestTime - testTime));
    }
    for (time_t testTime = 1635642000 - 2; testTime < 1635642000 + 2; testTime ++) {
      setGmt (testTime);
      time_t localTestTime = getLocalTime ();
      Serial.printf ("      %llu | ", (unsigned long long) testTime);
      char s [50];
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", (int) (localTestTime - testTime));
    }
  }  
  
#endif
