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
 *          - changed delay () do SPIFFSsafeDelay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca          
 *          - added support for all European time zones (according to https://www.timeanddate.com/time/europe/)
 *            April 22, 2019, Bojan Jurca
 *  
 */


// change this definitions according to your needs

#define WET_TIMEZONE      0     // Western European Time (GMT + DST)
#define ICELAND_TIMEZONE  100   // same as WET_TIMEZONE but without DST (GMT, no DST)
#define CET_TIMEZONE      1     // Central European Time (GMT + 1 + DST)
#define EET_TIMEZONE      2     // Eastern European Time (GMT + 2 + DST)
#define FET_TIMEZONE      3     // Further-Eastern European Time (GMT + 3, no DST)
// ... add more time zones
#define TIMEZONE          CET_TIMEZONE // one of the above

#define INTERNAL_NTP_PORT 2390  // internal UDP port number used for NTP - you can choose a different port number if you wish
#define NTP_TIME_OUT 100        // number of milliseconds we are going to wait for NTP reply - it the number is too large the time will be less accurate
#define NTP_RETRY_TIME 15000    // number of milliseconds between NTP request retries before it succeds, 15000 = 15 s
#define NTP_SYNC_TIME 86400000  // number of milliseconds between NTP synchronizations, 86400000 = 1 day

  
#ifndef __REAL_TIME_CLOCK__
  #define __REAL_TIME_CLOCK__
 
  #include <WiFi.h>
  #include "TcpServer.hpp"
  
  
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
                                                        newTime.tv_sec = this->getLocalTime ();
                                                        strftime (s, 30, "%d.%m.%y %H:%M:%S", gmtime (&newTime.tv_sec));
                                                        Serial.printf ("[real_time_clock] if (your local time != %s) change TIMEZONE definition or modify getLocalTime () function for your country and location;\n", s);
                                                      } else {
                                                        Serial.printf ("[real_time_clock] time corrected for %li seconds\n", newTime.tv_sec - oldTime.tv_sec);
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
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT time
                                                      // TO DO: modify this function according to your location - it is difficult to
                                                      //        write a generic function since different countries use different time rules
                                                      // the following example is for Westeren European time zone
     
                                                      time_t now = gmtTime; // time in GMT
  
                                                      struct tm *nowStr = gmtime (&now); 
                                                      bool insideDst = false;
                                                      if (nowStr->tm_mon + 1 == 3) // if it is March ...
                                                        if (nowStr->tm_mday - nowStr->tm_wday >= 25) // if it is the last Sunday or a latter day ...
                                                          insideDst = (nowStr->tm_wday > 0 || nowStr->tm_hour >= 1); // if it is a latter day or it is Sunday past 1:00
                                                      if (nowStr->tm_mon + 1 > 3 && nowStr->tm_mon + 1 <= 10) insideDst = true; // let's do similar check for October
                                                      if (nowStr->tm_mon + 1 == 10) // if it is October ...
                                                        if (nowStr->tm_mday - nowStr->tm_wday >= 25) // if it is the last Sunday or a latter day ...
                                                          insideDst = !(nowStr->tm_wday > 0 || nowStr->tm_hour >= 1); // if it is a latter day or it is Sunday past 1:00
                                                          
                                                      if (insideDst) now += 3600; // time in GMT + 1
                                                      return now;
                                                    }
#endif
#if TIMEZONE == ICELAND_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT time
                                                      // TO DO: modify this function according to your location - it is difficult to
                                                      //        write a generic function since different countries use different time rules
                                                      // the following example is for Westeren European time zone
     
                                                      return gmtTime; // time in GMT
                                                    }
#endif
#if TIMEZONE == CET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT time
                                                      // TO DO: modify this function according to your location - it is difficult to
                                                      //        write a generic function since different countries use different time rules
                                                      // the following example is for Central European time zone
     
                                                      time_t now = gmtTime + 3600; // time in GMT + 1
  
                                                      struct tm *nowStr = gmtime (&now); 
                                                      bool insideDst = false;
                                                      if (nowStr->tm_mon + 1 == 3) // if it is March ...
                                                        if (nowStr->tm_mday - nowStr->tm_wday >= 25) // if it is the last Sunday or a latter day ...
                                                          insideDst = (nowStr->tm_wday > 0 || nowStr->tm_hour >= 2); // if it is a latter day or it is Sunday past 2:00
                                                      if (nowStr->tm_mon + 1 > 3 && nowStr->tm_mon + 1 <= 10) insideDst = true; // let's do similar check for October
                                                      if (nowStr->tm_mon + 1 == 10) // if it is October ...
                                                        if (nowStr->tm_mday - nowStr->tm_wday >= 25) // if it is the last Sunday or a latter day ...
                                                          insideDst = !(nowStr->tm_wday > 0 || nowStr->tm_hour >= 2); // if it is a latter day or it is Sunday past 2:00
                                                          
                                                      if (insideDst) now += 3600; // time in GMT + 2
                                                      return now;
                                                    }
#endif
#if TIMEZONE == EET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT time
                                                      // TO DO: modify this function according to your location - it is difficult to
                                                      //        write a generic function since different countries use different time rules
                                                      // the following example is for Eastern European time zone
     
                                                      time_t now = gmtTime + 2 * 3600; // time in GMT + 2
  
                                                      struct tm *nowStr = gmtime (&now); 
                                                      bool insideDst = false;
                                                      if (nowStr->tm_mon + 1 == 3) // if it is March ...
                                                        if (nowStr->tm_mday - nowStr->tm_wday >= 25) // if it is the last Sunday or a latter day ...
                                                          insideDst = (nowStr->tm_wday > 0 || nowStr->tm_hour >= 3); // if it is a latter day or it is Sunday past 3:00
                                                      if (nowStr->tm_mon + 1 > 3 && nowStr->tm_mon + 1 <= 10) insideDst = true; // let's do similar check for October
                                                      if (nowStr->tm_mon + 1 == 10) // if it is October ...
                                                        if (nowStr->tm_mday - nowStr->tm_wday >= 25) // if it is the last Sunday or a latter day ...
                                                          insideDst = !(nowStr->tm_wday > 0 || nowStr->tm_hour >= 3); // if it is a latter day or it is Sunday past 3:00
                                                          
                                                      if (insideDst) now += 3600; // time in GMT + 3
                                                      return now;
                                                    }
#endif
#if TIMEZONE == FET_TIMEZONE 
      time_t localTime (time_t gmtTime)             { // returns local time for a given GMT time
                                                      // TO DO: modify this function according to your location - it is difficult to
                                                      //        write a generic function since different countries use different time rules
                                                      // the following example is for Further Eastern European time zone
     
                                                      return gmtTime + 3 * 3600; // time in GMT + 3
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

#endif
