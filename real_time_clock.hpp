/*
 * 
 * real_time_clock.hpp
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
 *  
 */


// change this definitions according to your needs

// time zones from east to west

// ----- Russia time zones ----- // https://en.wikipedia.org/wiki/Time_in_Russia
#define KAL_TIMEZONE 10                     // GMT + 2
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
// ----- Japan time zone -----
#define JAPAN_TIMEZONE 21                   // GMT + 9
// ----- China time zone -----
#define CHINA_TIMEZONE 22                   // GMT + 8
// ----- Europe time zones -----  https://www.timeanddate.com/time/europe/
#define WET_TIMEZONE      23                // Western European Time (GMT + DST from March to October)
#define ICELAND_TIMEZONE  24                // same as WET_TIMEZONE but without DST (GMT, no DST)
#define CET_TIMEZONE      25                // Central European Time (GMT + 1 + DST from March to October)
#define EET_TIMEZONE      26                // Eastern European Time (GMT + 2 + DST from March to October)
#define FET_TIMEZONE      27                // Further-Eastern European Time (GMT + 3, no DST)
// ----- USA & Canada time zones ---- https://en.wikipedia.org/wiki/Time_in_the_United_States
#define NEWFOUNDLAND_TIMEZONE 28            // GMT - 3.5 + DST from March to November
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

// ... add more time zones or change getLocalTime function yourself

#ifndef TIMEZONE
  #define TIMEZONE          CET_TIMEZONE // one of the above
#endif

 
#ifndef __REAL_TIME_CLOCK__
  #define __REAL_TIME_CLOCK__

  // ----- includes, definitions and supporting functions -----
 
  #ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include "time.h"
    // standard C structures and functions that are missing for ESP8266
    time_t __syncSeconds__ = 0;             // UNIX time obtained from NTP when internal clock has been synchronized with NTP server
    unsigned long __syncMilliseconds__ = 0; // ESP milliseconds when internal clock has been synchronized with NTP server
    struct timeval {
      time_t      tv_sec;
      suseconds_t tv_usec;
    };    
    int gettimeofday (struct timeval *tv, void *tz) {
      if (__syncSeconds__) { // the time has been synchronized with NTP server at __syncSeconds__ - calculate current time
        tv->tv_sec = __syncSeconds__ + (millis () - __syncMilliseconds__) / 1000;
        // we don't care about tv.tv_usec (we don't get this information from NTP anyway) and tz (it is just a legacy)
        return 0; // success
      } else {
        return -1; // failure - the time has not been synchronized with NTP server therefore it is unknown
      }
    }
    int settimeofday (struct timeval *tv, struct timezone *tz) {
      __syncSeconds__ = tv->tv_sec;
      // we don't care about tv.tv_usec and tz
      __syncMilliseconds__ = millis ();
      return 0; // success
    }
    #include <WiFiUdp.h>    
  #endif
  #ifdef ESP32
    #include <WiFi.h>
  #endif  

  #define INTERNAL_NTP_PORT 2390  // internal UDP port number used for NTP - you can choose a different port number if you wish
  #define NTP_TIME_OUT 100        // number of milliseconds we are going to wait for NTP reply - it the number is too large the time will be less accurate
  #define NTP_RETRY_TIME 15000    // number of milliseconds between NTP request retries before it succeds, 15000 = 15 s
  #define NTP_SYNC_TIME 86400000  // number of milliseconds between NTP synchronizations, 86400000 = 1 day

  
  void __rtcDmesg__ (String message) { 
    #ifdef __TELNET_SERVER__ // use dmesg from telnet server if possible
      dmesg (message);
    #else
      Serial.println (message); 
    #endif
  }
  void (* rtcDmesg) (String) = __rtcDmesg__; // use this pointer to display / record system messages

  String timeToString (time_t timeSec) {
    struct tm *timeStr = localtime (&timeSec); // actually gmtime    
    char s [25];
    sprintf (s, "%04i/%02i/%02i %02i:%02i:%02i", 1900 + timeStr->tm_year, 1 + timeStr->tm_mon, timeStr->tm_mday, timeStr->tm_hour, timeStr->tm_min, timeStr->tm_sec);
    return String (s);
  }


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
      
      ~real_time_clock ()                           { if (this->__state__ == real_time_clock::WAITING_FOR_NTP_REPLY) this->__udp__.stop (); }; // destructor
  
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
                                                        rtcDmesg ("[RTC] got GMT from NTP server: " + timeToString (newTime.tv_sec) + ".");
                                                        newTime.tv_sec = this->getLocalTime ();
                                                        rtcDmesg ("[RTC] local time (according to TIMEZONE definition): " + timeToString (newTime.tv_sec) + ".");
                                                        Serial.printf ("[%10d] [RTC] if your local time is different change TIMEZONE definition or modify getLocalTime () function for your country and location.\n", millis ());
                                                      } else {
                                                        rtcDmesg ("[RTC] time corrected for: " + String (newTime.tv_sec - oldTime.tv_sec) + " s.");
                                                      }
  
                                                      this->__gmtTimeSet__ = true;
                                                    }
  
      bool isGmtTimeSet ()                          { return this->__gmtTimeSet__; } // false until we set the time the first time or synchronize with NTP server

      time_t getGmtTime ()                          { // returns current GMT time - calling program should check isGmtTimeSet () first
                                                      struct timeval now;
                                                      gettimeofday (&now, NULL);
                                                      return now.tv_sec;
                                                    }
  
      time_t getLocalTime ()                        { // returns current local time - calling program should check isGmtTimeSet () first
                                                      time_t now;
                                                      // ----- Russia time zones ----- // https://en.wikipedia.org/wiki/Time_in_Russia
                                                      #if TIMEZONE == KAL_TIMEZONE 
                                                        return = this->getGmtTime () + 2 * 3600; // GMT + 2
                                                      #endif
                                                      #if TIMEZONE == MSK_TIMEZONE
                                                        return this->getGmtTime () + 3 * 3600; // GMT + 3
                                                      #endif
                                                      #if TIMEZONE == SAM_TIMEZONE
                                                        return this->getGmtTime () + 4 * 3600; // GMT + 4
                                                      #endif
                                                      #if TIMEZONE == YEK_TIMEZONE 
                                                        return this->getGmtTime () + 5 * 3600; // GMT + 5
                                                      #endif
                                                      #if TIMEZONE == OMS_TIMEZONE
                                                        return this->getGmtTime () + 6 * 3600; // GMT + 6
                                                      #endif
                                                      #if TIMEZONE == KRA_TIMEZONE
                                                        return this->getGmtTime () + 7 * 3600; // GMT + 7
                                                      #endif
                                                      #if TIMEZONE == IRK_TIMEZONE
                                                        return this->getGmtTime () + 8 * 3600; // GMT + 8
                                                      #endif
                                                      #if TIMEZONE == YAK_TIMEZONE
                                                        return this->getGmtTime () + 9 * 3600; // GMT + 9
                                                      #endif
                                                      #if TIMEZONE == VLA_TIMEZONE
                                                        return this->getGmtTime () + 10 * 3600; // GMT + 10
                                                      #endif
                                                      #if TIMEZONE == SRE_TIMEZONE
                                                        return this->getGmtTime () + 11 * 3600; // GMT + 11
                                                      #endif
                                                      #if TIMEZONE == PET_TIMEZONE
                                                        return this->getGmtTime () + 12 * 3600; // GMT + 12
                                                      #endif
                                                      // ----- Japan time zone -----
                                                      #if TIMEZONE == JAPAN_TIMEZONE 
                                                        return this->getGmtTime () + 9 * 3600; // GMT + 9
                                                      #endif
                                                      // ----- China time zone -----
                                                      #if TIMEZONE == CHINA_TIMEZONE 
                                                        return this->getGmtTime () + 8 * 3600; // GMT + 8
                                                      #endif
                                                      // ----- Europe time zones -----  https://www.timeanddate.com/time/europe/
                                                      // The Daylight Saving Time (DST) period in Europe runs from 01:00 Coordinated Universal Time (UTC) 
                                                      // on the last Sunday of March to 01:00 UTC on the last Sunday of October every year.
                                                      #if TIMEZONE == WET_TIMEZONE 
                                                        now = this->getGmtTime (); // GMT
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 23                  //                      && last Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 1))     //                                               && GMT == 1 or more
                                                        if (!(nowStr->tm_mon > 9 || nowStr->tm_mon == 9                                       // NOT November .. December || October 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 23                //                             && last Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                             && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 1)))  //                                                      && GMT == 1 or more
                                                          now += 3600; // DST -> GMT + 1
                                                      #endif
                                                      #if TIMEZONE == ICELAND_TIMEZONE 
                                                        return this->getGmtTime (); // GMT
                                                      #endif
                                                      #if TIMEZONE == CET_TIMEZONE 
                                                        now = this->getGmtTime () + 3600; // GMT + 1
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 23                  //                      && last Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT + 1 == 2 or more
                                                          if (!(nowStr->tm_mon > 9 || nowStr->tm_mon == 9                                       // NOT November .. December || October 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 23                //                             && last Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                             && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 2)))  //                                                      && GMT + 1 == 2 or more
                                                            now += 3600; // DST -> GMT + 2
                                                      #endif
                                                      #if TIMEZONE == EET_TIMEZONE 
                                                        time_t now = this->getGmtTime () + 2 * 3600; // GMT + 2
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                       // April .. December || March 
                                                                                && nowStr->tm_mday - nowStr->tm_wday >= 23                  //                      && last Sunday or latter
                                                                                   && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                              && nowStr->tm_hour >= 3))     //                                               && GMT + 2 == 3 or more
                                                        if (!(nowStr->tm_mon > 9 || nowStr->tm_mon == 9                                       // NOT November .. December || October 
                                                                                    && nowStr->tm_mday - nowStr->tm_wday >= 23                //                             && last Sunday or latter
                                                                                       && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                             && Monday .. Saturday || Sunday
                                                                                                                  && nowStr->tm_hour >= 3)))  //                                                      && GMT + 2 == 3 or more
                                                          now += 3600; // DST -> GMT + 3
                                                      #endif
                                                      #if TIMEZONE == FET_TIMEZONE 
                                                        return this->getGmtTime () + 3 * 3600; // GMT + 3
                                                      #endif
                                                      // ----- USA & Canada time zones ---- https://en.wikipedia.org/wiki/Time_in_the_United_States
                                                      // Daylight saving time (DST) begins on the second Sunday of March and ends on the first Sunday of November.
                                                      // Clocks will be set ahead one hour at 2:00 AM on the start dates and set back one hour at 2:00 AM on ending.
                                                      #if TIMEZONE == NEWFOUNDLAND_TIMEZONE 
                                                        now = this->getGmtTime () - 12600; // GMT - 3.5 
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 2))     //                                               && GMT - 3.5 == 2 or more
                                                              if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                          && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                             && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                        && nowStr->tm_hour >= 1)))  //                                          && GMT - 2.5 == 1 or more (equals GMT - 3.5 == 2 or more)
                                                                now += 3600; // DST -> GMT - 2.5
                                                      #endif
                                                      #if TIMEZONE == ATLANTIC_TIMEZONE 
                                                        now = this->getGmtTime () - 4 * 3600; // GMT - 4
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 4 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 4 == 1 or more (equals GMT - 3 == 2 or more)
                                                            now += 3600; // DST -> GMT - 3
                                                      #endif
                                                      #if TIMEZONE == ATLANTIC_NO_DST_TIMEZONE 
                                                        return this->getGmtTime () - 4 * 3600; // GMT - 4
                                                      #endif
                                                      #if TIMEZONE == EASTERN_TIMEZONE 
                                                        now = this->getGmtTime () - 5 * 3600; // GMT - 5
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
  
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 5 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 5 == 1 or more (equals GMT - 4 == 2 or more)
                                                            now += 3600; // DST -> GMT - 4
                                                      #endif
                                                      #if TIMEZONE == EASTERN_NO_DST_TIMEZONE 
                                                        return this->getGmtTime () - 5 * 3600; // GMT - 5
                                                      #endif
                                                      #if TIMEZONE == CENTRAL_TIMEZONE 
                                                        now = this->getGmtTime () - 6 * 3600; // time in GMT - 6
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 6 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 6 == 1 or more (equals GMT - 5 == 2 or more)
                                                            now += 3600; // DST -> GMT - 5
                                                      #endif
                                                      #if TIMEZONE == CENTRAL_NO_DST_TIMEZONE 
                                                        return this->getGmtTime () - 6 * 3600; // GMT - 6
                                                      #endif
                                                      #if TIMEZONE == MOUNTAIN_TIMEZONE 
                                                        now = this->getGmtTime () - 7 * 3600; // GMT - 7
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 7 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 7 == 1 or more (equals GMT - 6 == 2 or more)
                                                            now += 3600; // DST -> GMT - 6
                                                      #endif
                                                      #if TIMEZONE == MOUNTAIN_NO_DST_TIMEZONE 
                                                        return this->getGmtTime () - 7 * 3600; // GMT - 7
                                                      #endif
                                                      #if TIMEZONE == PACIFIC_TIMEZONE 
                                                        now = this->getGmtTime () - 8 * 3600; // GMT - 8
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 8 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 8 == 1 or more (equals GMT - 7 == 2 or more)
                                                            now += 3600; // DST -> GMT - 7
                                                      #endif
                                                      #if TIMEZONE == ALASKA_TIMEZONE 
                                                        now = this->getGmtTime () - 9 * 3600; // GMT - 9
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 9 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 9 == 1 or more (equals GMT - 8 == 2 or more)
                                                            now += 3600; // DST -> GMT - 8
                                                      #endif
                                                      #if TIMEZONE == HAWAII_ALEUTIAN_TIMEZONE 
                                                        now = this->getGmtTime () - 10 * 3600; // GMT - 10
                                                        // check if now is inside DST interval
                                                        struct tm *nowStr = localtime (&now); // actually gmtime
                                                        if (nowStr->tm_mon > 2 || nowStr->tm_mon == 2                                         // April .. December || March 
                                                                                  && nowStr->tm_mday - nowStr->tm_wday >= 8                   //                      && second Sunday or latter
                                                                                     && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0          //                      && Monday .. Saturday || Sunday
                                                                                                                && nowStr->tm_hour >= 2))     //                                               && GMT - 10 == 2 or more
                                                          if (!(nowStr->tm_mon > 10 || nowStr->tm_mon == 10                                     // NOT December || November 
                                                                                      && nowStr->tm_mday - nowStr->tm_wday >= 1                 //                 && first Sunday or latter
                                                                                         && (nowStr->tm_wday > 0 || nowStr->tm_wday == 0        //                 && Monday .. Saturday || Sunday
                                                                                                                    && nowStr->tm_hour >= 1)))  //                                          && GMT - 10 == 1 or more (equals GMT - 9 == 2 or more)
                                                            now += 3600; // DST -> GMT - 9
                                                      #endif
                                                      #if TIMEZONE == HAWAII_ALEUTIAN_NO_DST_TIMEZONE
                                                        return this->getGmtTime () - 10 * 3600; // GMT - 10
                                                      #endif
                                                      #if TIMEZONE == AMERICAN_SAMOA_TIMEZONE
                                                        return this->getGmtTime () - 11 * 3600; // GMT - 11
                                                      #endif
                                                      #if TIMEZONE == BAKER_HOWLAND_ISLANDS_TIMEZONE
                                                        return this->getGmtTime () - 12 * 3600; // GMT - 12
                                                      #endif
                                                      #if TIMEZONE == WAKE_ISLAND_TIMEZONE
                                                        return this->getGmtTime () + 12 * 3600; // GMT + 12                        }
                                                      #endif
                                                      #if TIMEZONE == CHAMORRO_TIMEZONE
                                                        return this->getGmtTime () + 10 * 3600; // GMT + 10
                                                      #endif
                                                      
                                                      // return the time calculated
                                                      return now;
                                                    }

      time_t getGmtStartupTime ()                   { return this->__startupTime__; } // returns the time ESP32 started
  
      void doThings ()                              { // this is where real_time_clock is getting its processing time
                                                      // real_time_clock can only be in 1 of 2 states:
                                                      // - state 1: it is waiting for a next NTP synchronization to send NTP request or
                                                      // - state 2: NTP request has already been sent and real_time_clock is waiting for a response

                                                      switch (this->__state__) {
                                                        case real_time_clock::WAITING_FOR_NTP_SYNC:

                                                                if ((this->__gmtTimeSet__ && millis () - this->__lastNtpSynchronization__ > NTP_SYNC_TIME) // time already set, synchronize every NTP_SYNC_TIME
                                                                    ||
                                                                    (!this->__gmtTimeSet__ && millis () - this->__lastNtpSynchronization__ > NTP_RETRY_TIME) // time has not been set yet, synchronize every NTP_SYNC_TIME
                                                                    ) { 
                                                                        #ifdef ESP8266
                                                                          // in case of ESP8266, correct synchronization variables - only for the purpose 
                                                                          // if NTP requests do not succeed - internal clock would turn around approximately 
                                                                          // every 50 days - current time calculation would be wrong then
                                                                          // ESP32, on the other hand does not have this kind of problems with internal clock
                                                                          if (this->isGmtTimeSet ()) this->setGmtTime (this->getGmtTime ());
                                                                        #endif
                                                                        // send NTP request - initialize values needed to form a NTP request
                                                                        memset (this->__ntpPacket__, 0, sizeof (this->__ntpPacket__));
                                                                        this->__ntpPacket__ [0] = 0b11100011;  
                                                                        this->__ntpPacket__ [1] = 0;           
                                                                        this->__ntpPacket__ [2] = 6;           
                                                                        this->__ntpPacket__ [3] = 0xEC;  
                                                                        this->__ntpPacket__ [12] = 49;
                                                                        this->__ntpPacket__ [13] = 0x4E;
                                                                        this->__ntpPacket__ [14] = 49;
                                                                        this->__ntpPacket__ [15] = 52;  

                                                                        // if ESP32 is not connected to WiFi UDP can't succeed
                                                                        if (WiFi.status () != WL_CONNECTED) return;
                                                                        // connect to one of three servers
                                                                        IPAddress ntpServerIp;
                                                                        if (!WiFi.hostByName (this->__ntpServer1__, ntpServerIp))
                                                                          if (!WiFi.hostByName (this->__ntpServer1__, ntpServerIp))
                                                                            if (!WiFi.hostByName (this->__ntpServer1__, ntpServerIp)) {
                                                                              rtcDmesg ("[RTC] could not connect to NTP server(s).");
                                                                              return;
                                                                            }
                                                                            // open internal port
                                                                            if (!this->__udp__.begin (INTERNAL_NTP_PORT)) { 
                                                                              rtcDmesg ("[RTC] internal port " + String (INTERNAL_NTP_PORT) + " for NTP is not available.");
                                                                              return;
                                                                            }
                                                                            // change state although we are not sure yet that request will succeed
                                                                            this->__state__ = real_time_clock::WAITING_FOR_NTP_REPLY;
                                                                            this->__ntpRequestMillis__ = millis (); // prepare time-out timing
                                                                            // start UDP over port 123                                                      
                                                                            if (!this->__udp__.beginPacket (ntpServerIp, 123)) { 
                                                                              rtcDmesg ("[RTC] NTP server(s) are not available on port.");
                                                                              return;
                                                                            }
                                                                            // send UDP request
                                                                            if (this->__udp__.write (this->__ntpPacket__, sizeof (this->__ntpPacket__)) != sizeof (this->__ntpPacket__)) { // 48
                                                                              rtcDmesg ("[RTC] could not send NTP request.");
                                                                              return;
                                                                            }
                                                                            // check if UDP request has been sent
                                                                            if (!this->__udp__.endPacket ()) {
                                                                              rtcDmesg ("[RTC] NTP request not sent.");
                                                                              return;
                                                                            }
                                                                            this->__ntpRequestMillis__ = millis (); // correct time-out timing
    
                                                                    this->__lastNtpSynchronization__ = millis ();
                                                                }
                                                                break;
          
                                                        case real_time_clock::WAITING_FOR_NTP_REPLY:
                                                                
                                                                // wait for NTP reply or time-out
                                                                if (millis () - this->__ntpRequestMillis__ > NTP_TIME_OUT) { // time-out, stop waiting for reply
                                                                  rtcDmesg ("[RTC] NTP time-out.");
                                                                  this->__state__ = real_time_clock::WAITING_FOR_NTP_SYNC;
                                                                  return;
                                                                }
                                                                if (this->__udp__.parsePacket () != sizeof (this->__ntpPacket__)) { // keep waiting for reply
                                                                  return; 
                                                                }
                                                                // change state although we are not sure yet that reply is OK
                                                                this->__state__ = real_time_clock::WAITING_FOR_NTP_SYNC;
                                                                // read NTP response back into the same packet we used for NTP request (different bytes are used for request and reply)
                                                                this->__udp__.read (this->__ntpPacket__, sizeof (this->__ntpPacket__));
                                                                if (!this->__ntpPacket__ [40] && !this->__ntpPacket__ [41] && !this->__ntpPacket__ [42] && !this->__ntpPacket__ [43]) { // sometimes we get empty response which is invalid
                                                                  rtcDmesg ("[RTC] got invalid NTP response.");
                                                                  this->__udp__.stop ();
                                                                  return;
                                                                }
                                                                // NTP server counts seconds from 1st January 1900 but ESP32 uses UNIX like format - it counts seconds from 1st January 1970 - let's do the conversion 
                                                                unsigned long highWord;
                                                                unsigned long lowWord;
                                                                unsigned long secsSince1900;
                                                                #define SEVENTY_YEARS 2208988800UL
                                                                time_t currentTime;                                                                

                                                                highWord = word (this->__ntpPacket__ [40], this->__ntpPacket__ [41]); 
                                                                lowWord = word (this->__ntpPacket__ [42], this->__ntpPacket__ [43]);
                                                                secsSince1900 = highWord << 16 | lowWord;
                                                                currentTime = secsSince1900 - SEVENTY_YEARS;
                                                                // Serial.printf ("currentTime = %i\n", currentTime);
                                                                // right now currentTime is 1542238838 (14.11.18 23:40:38), every valid time should be grater then 1542238838
                                                                if (currentTime < 1542238838) { 
                                                                  rtcDmesg ("[RTC] got invalid NTP response..");
                                                                  this->__udp__.stop ();
                                                                  return;
                                                                }
                                                                this->setGmtTime (currentTime);
                                                                this->__udp__.stop ();
                                                                break;
                                                                 
                                                      } // switch                                     
                                                    }
 
    private:

      enum __stateType__ {
        WAITING_FOR_NTP_SYNC = 0, 
        WAITING_FOR_NTP_REPLY = 1
      }; 
      __stateType__ __state__ = real_time_clock::WAITING_FOR_NTP_SYNC;

      char *__ntpServer1__; // initialized in constructor
      char *__ntpServer2__; // initialized in constructor
      char *__ntpServer3__; // initialized in constructor

      WiFiUDP __udp__;                    // initialized at NTP request
      byte __ntpPacket__ [48];            // initialized at NTP request
      unsigned long __ntpRequestMillis__; // initialized at NTP request
      
      bool __gmtTimeSet__ = false; // false until we set it the first time or synchronize with NTP server
      unsigned long __lastNtpSynchronization__ = millis () - NTP_RETRY_TIME; // the time of last NTP synchronization (faked here so the first doThings will start to sync)
      time_t __startupTime__ = 0;
    
  };

 
  // DEBUG - use this function only for testing purposes
  /*
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
  */

#endif
