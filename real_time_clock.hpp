/*
 * 
 * Real_time_clock.hpp
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 * 
 *  Real_time_clock synchronizes its time with NTP server accessible from internet once a day.
 *  Internet connection is necessary for real_time_clock to work.
 * 
 *  Real_time_clock preserves accurate time even when ESP32 goes into deep sleep.
 * 
 * History:
 *          - first release, 
 *            November 14, 2018, Bojan Jurca
 *          - changed delay () to SPIFFSsafeDelay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca          
 *          - added support for all European time zones (see https://www.timeanddate.com/time/europe/)
 *            April 22, 2019, Bojan Jurca
 *          - added support for USA timezones (see https://en.wikipedia.org/wiki/Time_in_the_United_States)
 *            September 30, 2019, Bojan Jurca
 *  
 */


// change this definitions according to your needs

// ----- European time zones -----  https://www.timeanddate.com/time/europe/
// The Daylight Saving Time (DST) period in Europe runs from 01:00 Coordinated Universal Time (UTC) 
// on the last Sunday of March to 01:00 UTC on the last Sunday of October every year.
#define WET_TIMEZONE      0                 // Western European Time (GMT + DST from March to October)
#define ICELAND_TIMEZONE  1                 // same as WET_TIMEZONE but without DST (GMT, no DST)
#define CET_TIMEZONE      2                 // Central European Time (GMT + 1 + DST from March to October)
#define EET_TIMEZONE      3                 // Eastern European Time (GMT + 2 + DST from March to October)
#define FET_TIMEZONE      4                 // Further-Eastern European Time (GMT + 3, no DST)

// ----- USA time zones ---- https://en.wikipedia.org/wiki/Time_in_the_United_States
// Daylight saving time (DST) begins on the second Sunday of March and ends on the first Sunday of November.
// Clocks will be set ahead one hour at 2:00 AM on the start dates and set back one hour at 2:00 AM on ending.
#define ATLANTIC_TIMEZONE 5                 // GMT - 4, no DST
#define EASTERN_TIMEZONE 6                  // GMT - 5 + DST from March to November
#define CNTRAL_TIMEZONE 7                   // GMT - 6 + DST from March to November
#define MOUNTAIN_TIMEZONE 8                 // GMT - 7 + DST from March to November
#define PACIFIC_TIMEZONE 9                  // GMT - 8 + DST from March to November
#define ALASKA_TIMEZNE 10                   // GMT - 9 + DST from March to November
#define HAWAII_ALEUTIAN_TIMEZONE 11         // GMT - 10 + DST from March to November
#define HAWAII_ALEUTIAN_NO_DST_TIMEZONE 12  // GMT - 10
#define AMERICAN_SAMOA_TIMEZONE 13          // GMT - 11
#define BAKER_HOWLAND_ISLANDS_TIMEZONE 14   // GMT - 12
#define WAKE_ISLAND_TIMEZONE 15             // GMT + 12
#define CHAMORRO_TIMEZONE 16                // GMT + 10               

// ... add more time zones or change getLocalTime function yourself

#ifndef TIMEZONE
  #define TIMEZONE          CET_TIMEZONE // one of the above
#endif


#define INTERNAL_NTP_PORT 2390  // internal UDP port number used for NTP - you can choose a different port number if you wish
#define NTP_TIME_OUT 100        // number of milliseconds we are going to wait for NTP reply - it the number is too large the time will be less accurate
#define NTP_RETRY_TIME 15000    // number of milliseconds between NTP request retries before it succeds, 15000 = 15 s
#define NTP_SYNC_TIME 86400000  // number of milliseconds between NTP synchronizations, 86400000 = 1 day

  
#ifndef __REAL_TIME_CLOCK__
  #define __REAL_TIME_CLOCK__
 
  #include <WiFi.h>
  #include "TcpServer.hpp"

  void __rtcDmesg__ (String message) { 
    #ifdef __TELNET_SERVER__ // use dmesg from telnet server if possible
      dmesg (message);
    #else
      Serial.println (message); 
    #endif
  }
  void (* rtcDmesg) (String) = __rtcDmesg__; // use this pointer to display / record system messages
 
  class real_time_clock {                                             
  
    public:
  
      real_time_clock (char *ntpServer1,        // first NTP server name
                       char* ntpServer2,        // second NTP server name if the first one is not accessible
                       char *ntpServer3)        // third NTP server name if the first two are not accessible
                       
                                                    { // constructor
                                                      // copy parameters into internal structure for latter use
                                                      // note that we only copy pointers to static strings, not the strings themselves
                                                      
                                                      this->__ntpServer1__ = ntpServer1;
                                                      this->__ntpServer2__ = ntpServer2;
                                                      this->__ntpServer3__ = ntpServer3;
                                                    }; 
      
      ~real_time_clock ()                           {}; // destructor - nothing to do here      
  
      void setGmtTime (time_t currentTime)      // currentTime contains the number of seconds elapsed from 1st January 1970 in GMT - ESP32 uses the same time format as UNIX does
                                                // note that we store time in GMT
                                                // also note that we only use time_t part of struct timeval since we get only soconds from NTP servers without microseconds
                                                       
                                                    { // sets current time
                                                      struct timeval oldTime;
                                                      struct timeval newTime = {};
                                                      
                                                      gettimeofday (&oldTime, NULL); // we don't use struct timezone since it is obsolete and useless
                                                      newTime.tv_sec = currentTime;
                                                      settimeofday (&newTime, NULL); // we don't use struct timezone since it is obsolete and useless
  
                                                      if (!this->__startupTime__) {
                                                        this->__startupTime__ = newTime.tv_sec - millis () / 1000;
                                                        char s [30];
                                                        strftime (s, 30, "%a, %d %b %Y %T", gmtime (&newTime.tv_sec));
                                                        Serial.printf ("[real_time_clock] got GMT: %s\n", s);
                                                        rtcDmesg ("[RTC] got GMT from NTP server: " + String (newTime.tv_sec) + ": " + s + ".");
                                                        newTime.tv_sec = this->getLocalTime ();
                                                        strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&newTime.tv_sec));
                                                        Serial.printf ("[real_time_clock] if (your local time != %s) change TIMEZONE definition or modify getLocalTime () function for your country and location;\n", s);
                                                        rtcDmesg ("[RTC] local time (according to TIMEZONE definition): " + String (s) + ".");
                                                      } else {
                                                        Serial.printf ("[real_time_clock] time corrected for %li seconds\n", newTime.tv_sec - oldTime.tv_sec);
                                                        rtcDmesg ("[RTC] time corrected for: " + String (newTime.tv_sec - oldTime.tv_sec) + " s.");
                                                      }
  
                                                      this->__gmtTimeSet__ = true;
                                                    }
  
      bool isGmtTimeSet ()                          { return this->__gmtTimeSet__; } // false until we set the time the first time or synchronize with NTP server
  
      bool synchronizeWithNtp ()                    { // reads current time from NTP server and sets internal clock, returns success
                                                      // NTP is an UDP protocol communicating over port 123: https://stackoverflow.com/questions/14171366/ntp-request-packet
                                                      
                                                      bool success = false;
                                                      IPAddress ntpServerIp;
                                                      WiFiUDP udp;
                                                      int i;
                                                      byte ntpPacket [48] = {};
                                                      // initialize values needed to form a NTP request
                                                      ntpPacket [0] = 0b11100011;  
                                                      ntpPacket [1] = 0;           
                                                      ntpPacket [2] = 6;           
                                                      ntpPacket [3] = 0xEC;  
                                                      ntpPacket [12] = 49;
                                                      ntpPacket [13] = 0x4E;
                                                      ntpPacket [14] = 49;
                                                      ntpPacket [15] = 52;  
                                                      unsigned long highWord;
                                                      unsigned long lowWord;
                                                      unsigned long secsSince1900;
                                                      #define SEVENTY_YEARS 2208988800UL
                                                      time_t currentTime;
  
                                                      // if ESP32 is not connected to WiFi UDP can' succeed
                                                      if (WiFi.status () != WL_CONNECTED) return false; // could also check networkStationWorking variable
  
                                                      // connect to one of three servers
                                                      if (!WiFi.hostByName (this->__ntpServer1__, ntpServerIp))
                                                        if (!WiFi.hostByName (this->__ntpServer1__, ntpServerIp))
                                                          if (!WiFi.hostByName (this->__ntpServer1__, ntpServerIp)) {
                                                            log_e ("could not connect to NTP server(s)\n");
                                                            goto exit1;
                                                          }
                                                      // open internal port
                                                      if (!udp.begin (INTERNAL_NTP_PORT)) { 
                                                        log_e ("internal port %i is not available\n", INTERNAL_NTP_PORT);
                                                        goto exit1;
                                                      }
                                                      // start UDP over port 123                                                      
                                                      if (!udp.beginPacket (ntpServerIp, 123)) { 
                                                        log_e ("NTP server(s) are not available on port 123\n");
                                                        goto exit2;
                                                      }
                                                      // send UDP request
                                                      if (udp.write (ntpPacket, sizeof (ntpPacket)) != sizeof (ntpPacket)) { // sizeof (ntpPacket) = 48
                                                        log_e ("could not send NTP packet\n");
                                                        goto exit2;
                                                      }
                                                      // check if UDP request has been sent
                                                      if (!udp.endPacket ()) {
                                                        log_e ("NTP packet not sent\n");
                                                        goto exit2;
                                                      }
  
                                                      // wait for NTP reply or time-out
                                                      for (i = 0; i < NTP_TIME_OUT && udp.parsePacket () != sizeof (ntpPacket); i++) SPIFFSsafeDelay (1);
                                                      if (i == NTP_TIME_OUT) {
                                                        log_e ("time-out while waiting for NTP response\n");
                                                        goto exit2;
                                                      }
                                                      // read NTP response back into the same packet we used for NTP request (different bytes are used for request and reply)
                                                      udp.read (ntpPacket, sizeof (ntpPacket));
                                                      if (!ntpPacket [40] && !ntpPacket [41] && !ntpPacket [42] && !ntpPacket [43]) { // sometimes we get empty response which is invalid
                                                        log_e ("invalid NTP response\n");
                                                        goto exit2;
                                                      }
  
                                                      // NTP server counts seconds from 1st January 1900 but ESP32 uses UNIX like format - it counts seconds from 1st January 1970 - let's do the conversion 
                                                      highWord = word (ntpPacket [40], ntpPacket [41]); 
                                                      lowWord = word (ntpPacket [42], ntpPacket [43]);
                                                      secsSince1900 = highWord << 16 | lowWord;
                                                      currentTime = secsSince1900 - SEVENTY_YEARS;
                                                      // Serial.printf ("currentTime = %i\n", currentTime);
                                                      // right now currentTime is 1542238838 (14.11.18 23:40:38), every valid time should be grater then 1542238838
                                                      if (currentTime < 1542238838) { 
                                                        log_e ("wrong NTP response\n");
                                                        goto exit2;
                                                      }
                                                      char str [30];
                                                      strftime (str, 30, "%d.%m.%y %H:%M:%S", gmtime (&currentTime));
                                                      log_i ("current GMT = %s\n", str);
  
                                                       this->setGmtTime (currentTime);
                                                       success = true;
                                                      
                                                      // clean up
                                                    exit2:
                                                      udp.stop ();
                                                    exit1:
                                                      return success;
                                                    }
  
      time_t getGmtTime ()                          { // returns current GMT time - calling program should check isGmtTimeSet () first
                                                      struct timeval now;
                                                      gettimeofday (&now, NULL);
                                                      return now.tv_sec;
                                                    }
  
      time_t getLocalTime ()                        { // returns current local time - calling program should check isGmtTimeSet () first
                                                      return localTime (getGmtTime ());     
                                                    }
#if TIMEZONE == WET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime; // time in GMT
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 
                                                      
                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 23                  //                      && last Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 1))     //                                               && GMT == 1 or more
                                                        if (!(nowStr->tm_mon > 9 || nowStr->tm_mon == 9                                       // NOT November .. December || October 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 23                //                             && last Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                             && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                                      && GMT == 1 or more
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == ICELAND_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime; // time in GMT
                                                    }
#endif
#if TIMEZONE == CET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime + 3600; // GMT + 1
                                                      
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 
                                                      
                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 23                  //                      && last Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT + 1 == 2 or more
                                                        if (!(nowStr->tm_mon > 9 || nowStr->tm_mon == 9                                       // NOT November .. December || October 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 23                //                             && last Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                             && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 2)))  //                                                      && GMT + 1 == 2 or more
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == EET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT 
                                                      time_t now = gmtTime + 2 * 3600; // time in GMT + 2
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 
                                                      
                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 23                  //                      && last Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 3))     //                                               && GMT + 2 == 3 or more
                                                        if (!(nowStr->tm_mon > 9 || nowStr->tm_mon == 9                                       // NOT November .. December || October 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 23                //                             && last Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                             && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 3)))  //                                                      && GMT + 2 == 3 or more
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == FET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime + 3 * 3600; // time in GMT + 3
                                                    }
#endif
#if TIMEZONE == ATLANTIC_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime - 4 * 3600; // GMT - 4
                                                    }
#endif
#if TIMEZONE == EASTERN_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime - 5 * 3600; // GMT - 5

                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 

                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT - 5 == 2 or more
                                                        if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                          && GMT - 5 == 1 or more (equals GMT - 4 == 2 or more)
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == CENTRAL_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime - 6 * 3600; // time in GMT - 6
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 

                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT - 6 == 2 or more
                                                        if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                          && GMT - 6 == 1 or more (equals GMT - 5 == 2 or more)
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == MOUNTAIN_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime - 7 * 3600; // time in GMT - 7
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 

                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT - 7 == 2 or more
                                                        if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                          && GMT - 7 == 1 or more (equals GMT - 6 == 2 or more)
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == PACIFIC_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime - 8 * 3600; // time in GMT - 8
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 

                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT - 8 == 2 or more
                                                        if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                          && GMT - 8 == 1 or more (equals GMT - 7 == 2 or more)
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == ALASKA_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime - 9 * 3600; // time in GMT - 9
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 

                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT - 9 == 2 or more
                                                        if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                          && GMT - 9 == 1 or more (equals GMT - 8 == 2 or more)
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == HAWAII_ALEUTIAN_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      time_t now = gmtTime - 10 * 3600; // time in GMT - 10
  
                                                      // check if now is inside DST interval
                                                      struct tm *nowStr = gmtime (&now); 

                                                      if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 2))     //                                               && GMT - 10 == 2 or more
                                                        if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                          && GMT - 10 == 1 or more (equals GMT - 9 == 2 or more)
                                                          now += 3600;                                                                          // DST

                                                      return now;
                                                    }
#endif
#if TIMEZONE == HAWAII_ALEUTIAN_NO_DST_TIMEZONE
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime - 10 * 3600; // time in GMT - 10
                                                    }
#endif
#if TIMEZONE == AMERICAN_SAMOA_TIMEZONE
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime - 11 * 3600; // time in GMT - 11
                                                    }
#endif
#if TIMEZONE == BAKER_HOWLAND_ISLANDS_TIMEZONE
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime - 12 * 3600; // time in GMT - 12
                                                    }
#endif
#if TIMEZONE == WAKE_ISLAND_TIMEZONE
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime + 12 * 3600; // time in GMT + 12
                                                    }
#endif
#if TIMEZONE == CHAMORRO_TIMEZONE
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT
                                                      return gmtTime + 10 * 3600; // time in GMT + 10
                                                    }
#endif

      time_t getGmtStartupTime ()                   { return this->__startupTime__; } // returns the time ESP32 started
  
      void doThings ()                              { // call this function from loop () if you want real_time_clock to automatically synchronize with NTP server(s)
                                                      if (this->__gmtTimeSet__) { // already set, synchronize every NTP_SYNC_TIME s
                                                        if (millis () - this->__lastNtpSynchronization__ > NTP_SYNC_TIME) {
                                                          // use this method:
                                                          this->synchronizeWithNtp (); this->__lastNtpSynchronization__ = millis ();
                                                          // or this method:
                                                          // if (this->synchronizeWithNtp ()) this->__lastNtpSynchronization__ += NTP_RETRY_TIME;
                                                        }
                                                      } else { // not set yet, synchrnoze every NTP_RETRY_TIME s
                                                        if (millis () - this->__lastNtpSynchronization__ > NTP_RETRY_TIME) {
                                                          this->synchronizeWithNtp ();
                                                          this->__lastNtpSynchronization__ = millis ();
                                                        }
                                                      }
                                                    }
  
    private:
  
      char *__ntpServer1__; // initialized in constructor
      char *__ntpServer2__; // initialized in constructor
      char *__ntpServer3__; // initialized in constructor
      
      bool __gmtTimeSet__ = false; // false until we set it the first time or synchronize with NTP server
      unsigned long __lastNtpSynchronization__ = millis () - NTP_RETRY_TIME; // the time of last NTP synchronization
      time_t __startupTime__ = 0;
    
  };

 
  // DEBUG - use this function only for testing purposes
  void __testLocalTime__ () {
    real_time_clock testRtc ( "1.si.pool.ntp.org", "2.si.pool.ntp.org", "3.si.pool.ntp.org");

    // check European start of DST interval
    Serial.printf ("\n      Testing start of DST interval in Europe - one of European time zones has to be #define-d for testing\n");
    Serial.printf ("      GMT (UNIX) | GMT               | Local             | Local - GMT [s]\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1553994000 - 2; testTime < 1553994000 + 2; testTime ++) {
      testRtc.setGmtTime (testTime);
      time_t localTestTime = testRtc.getLocalTime ();
      Serial.printf ("      %i | ", (unsigned long long) testTime);
      char s [30];
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", localTestTime - testTime);
    }

    // check European end of DST interval
    Serial.printf ("\n      Testing end of DST interval in Europe - one of European time zones has to be #define-d for testing\n");
    Serial.printf ("      GMT (UNIX) | GMT               | Local             | Local - GMT [s]\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1572138000 - 2; testTime < 1572138000 + 2; testTime ++) {
      testRtc.setGmtTime (testTime);
      time_t localTestTime = testRtc.getLocalTime ();
      Serial.printf ("      %i | ", (unsigned long long) testTime);
      char s [30];
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", localTestTime - testTime);
    }

    // check Eastern start of DST interval
    Serial.printf ("\n      Testing Eastern start of DST interval - Eastern time zones has to be #define-d for testing\n");
    Serial.printf ("      GMT (UNIX) | GMT               | Local             | Local - GMT [s]\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1552201200 - 2; testTime < 1552201200 + 2; testTime ++) {
      testRtc.setGmtTime (testTime);
      time_t localTestTime = testRtc.getLocalTime ();
      Serial.printf ("      %i | ", (unsigned long long) testTime);
      char s [30];
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", localTestTime - testTime);
    }

    // check Eastern end of DST interval
    Serial.printf ("\n      Testing Eastern end of DST interval - Eastern time zones has to be #define-d for testing\n");
    Serial.printf ("      GMT (UNIX) | GMT               | Local             | Local - GMT [s]\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1572760800 - 2; testTime < 1572760800 + 2; testTime ++) {
      testRtc.setGmtTime (testTime);
      time_t localTestTime = testRtc.getLocalTime ();
      Serial.printf ("      %i | ", (unsigned long long) testTime);
      char s [30];
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 30, "%y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", localTestTime - testTime);
    }    
  }  

#endif
