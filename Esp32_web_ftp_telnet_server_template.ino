/*
 *
 * Esp32_web_ftp_telnet_server_template.ino
 *
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 *
 *  File contains a working template for some operating system functionalities that can support your projects.
 *
 *  Copy all files in the package into Esp32_web_ftp_telnet_server_template directory, compile them with Arduino and run on ESP32.
 *   
 * History:
 *          - first release, 
 *            December 5, 2018, Bojan Jurca
 *          - added SPIFFSsafeDelay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca
 *          - telnetCommandHandler parameters are now easyer to access 
 *            September 4th, Bojan Jurca   
 *  
 */


#include <WiFi.h>

// ----- include project features - order may be important -----

#include "file_system.h"                                  // network and user configuration files are loacted on file sistem - we'll need them

#include "network.h"                                      // we'll need this to set up a network, Telnet server also needs this to execute certain commands such as ifconfig, ping, ... 
                                                          
// #define USER_MANAGEMENT NO_USER_MANAGEMENT             // Telnet and FTP servers use user management, Web server uses it only to get its home directory
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT      // define the kind of user management project is going to use
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "user_management.h"
                    
#include "real_time_clock.hpp"                            // Telnet server needs rtc to execute certain commands such as uptime
real_time_clock rtc ( "1.si.pool.ntp.org",  // first NTP server
                      "3.si.pool.ntp.org",  // second NTP server if the first one is not accessible
                      "3.si.pool.ntp.org"); // third NTP server if the first two are not accessible

#include "telnetServer.hpp"                               // Telnet server - if other server are going to use dmesg telnetServer.hpp has to be defined priorly

#include "webServer.hpp"                                  // Web server

#define FTP_FILE_TIME rtc.getLocalTime()                  // define this if FTP is going to use current time to report file creation time 
#include "ftpServer.hpp"                                  // SPIFFS doesn't record file creation time so this information will be false anyway

// ----- add what this template needs for demonstration purpose -----

#include "measurements.hpp"
measurements freeHeap (60);                 // measure free heap each minute for possible memory leaks
measurements connectionCount (60);          // measure how many web connections arrive each minute

#include "examples.h" // Example 06, Example 07, Example 08, Example 09, Example 10, Oscilloscope

// ----- use features in the project -----

webServer *webSrv; // pointer to Web server
String httpRequestHandler (String httpRequest, WebSocket *webSocket);
String startWebServer () {
  if (!webSrv) {
    webSrv = new webServer (httpRequestHandler,         // a callback function tht will handle HTTP request that are not handled by webServer itself
                            8192,                       // 8 KB stack size is usually enough, if httpRequestHandler uses more stack increase this value until server is stabile
                            "0.0.0.0",                  // start web server on all available ip addresses
                            80,                         // HTTP port
                            NULL);                      // we won't use firewall callback function for web server
    if (webSrv)
      if (webSrv->started ())                                   return "Web server started.";  
      else                    { delete (webSrv); webSrv = NULL; return "Could not start web server."; }
    else                                                        return "Could not start web server.";  
  } else                                                        return "Web server is already running.";    
}
String stopWebServer () {
  if (webSrv) { delete (webSrv); webSrv = NULL; return "Web server stopped. Active connections will continue to run anyway."; } 
  else                                          return "Web server is not running.";
}

String httpRequestHandler (String httpRequest, WebSocket *webSocket) {  // - normally httpRequest is HTTP request, webSocket is NULL, function returns a reply in HTML, json, ... formats or "" if request is unhandeled
                                                                        // - for WebSocket httpRequest is WS request, webSocket pointer is set, whatever function returns will be discarded
                                                                        // httpRequestHandler is supposed to be used with smaller replies,
                                                                        // if you want to reply with larger pages you may consider FTP-ing .html files onto the file system (/var/www/html/ by default)
                                                                        // - has to be reentrant!
                                                                        
       connectionCount.increaseCounter ();                              // gether some statistics

  // ----- handle HTTP protocol requests -----
  
       if (httpRequest.substring (0, 20) == "GET /example01.html ") { // used in Example 01
                                                                      return String ("<HTML>Example 01 - dynamic HTML page<br><br><hr />") + (digitalRead (2) ? "Led is on." : "Led is off.") + String ("<hr /></HTML>");
                                                                    }
  else if (httpRequest.substring (0, 16) == "GET /builtInLed ")     { // used in Example 02, Example 03, Example 04, index.html
                                                                    getBuiltInLed:
                                                                      return "{\"id\":\"esp32\",\"builtInLed\":\"" + (digitalRead (2) ? String ("on") : String ("off")) + "\"}\r\n";
                                                                    }                                                                    
  else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ")  { // used in Example 03, Example 04
                                                                      digitalWrite (2, HIGH);
                                                                      goto getBuiltInLed;
                                                                    }
  else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ") { // used in Example 03, Example 04, index.html
                                                                      digitalWrite (2, LOW);
                                                                      goto getBuiltInLed;
                                                                    }
  else if (httpRequest.substring (0, 22) == "PUT /builtInLed/on10s ") { // used in index.html
                                                                        digitalWrite (2, HIGH);
                                                                        SPIFFSsafeDelay (10000);
                                                                        digitalWrite (2, LOW);
                                                                        goto getBuiltInLed;
                                                                      }
  else if (httpRequest.substring (0, 12) == "GET /upTime ")         { // used in index.html
                                                                      if (rtc.isGmtTimeSet ()) {
                                                                        unsigned long long l = rtc.getGmtTime () - rtc.getGmtStartupTime ();
                                                                        // int s = l % 60;
                                                                        // l /= 60;
                                                                        // int m = l % 60;
                                                                        // l /= 60;
                                                                        // int h = l % 60;
                                                                        // l /= 24;
                                                                        // return "{\"id\":\"esp32\",\"upTime\":\"" + String ((int) l) + " days " + String (h) + " hours " + String (m) + " minutes " + String (s) + " seconds\"}";
                                                                        return "{\"id\":\"esp32\",\"upTime\":\"" + String ((unsigned long) l) + " sec\"}\r\n";
                                                                      } else {
                                                                        return "{\"id\":\"esp32\",\"upTime\":\"unknown\"}\r\n";
                                                                      }
                                                                    }                                                                    
  else if (httpRequest.substring (0, 14) == "GET /freeHeap ")       { // used in index.html
                                                                      return freeHeap.measurements2json (5);
                                                                    }
  else if (httpRequest.substring (0, 21) == "GET /connectionCount "){ // used in index.html
                                                                      return connectionCount.measurements2json (5);
                                                                    }
  // ----- handle WS (WebSockets) protocol requests -----
  
  else if (httpRequest.substring (0, 26) == "GET /example09_WebSockets " && webSocket){ // used in Example 09
                                                                      example09_webSockets (webSocket);
                                                                      return ""; // it doesn't matter what the function returns in case of WebSockets
                                                                    }
  else if (httpRequest.substring (0, 21) == "GET /runOscilloscope " && webSocket){ // used in oscilloscope.html
                                                                      example_oscilloscope (webSocket);
                                                                      return ""; // it doesn't matter what the function returns in case of WebSockets
                                                                    }

  // ----- HTTP request has not been handled by httpRequestHandler - let the webServer handle it itself -----

  else                                                              return ""; 
}

bool telnetAndFtpFirewall (char *IP) {          // firewall callback function, return true if IP is accepted or false if not
                                                // - has to be reentrant!
  if (!strcmp (IP, "10.0.0.2")) return false;   // block 10.0.0.2 (for some reason) ... please note that this is just an example
  else                          return true;    // ... but let every other client through
}

ftpServer *ftpSrv; // pointer to FTP server (it doesn't call external handling function so it doesn't have to be defined)
String startFtpServer () {
  if (!ftpSrv) {
    ftpSrv = new ftpServer ("0.0.0.0",                  // start FTP server on all available ip addresses
                            21,                         // controll connection FTP port
                            telnetAndFtpFirewall);      // use firewall callback function for FTP server or NULL
    if (ftpSrv)
      if (ftpSrv->started ())                                   return "FTP server started.";  
      else                    { delete (webSrv); webSrv = NULL; return "Could not start FTP server."; }
    else                                                        return "Could not start FTP server.";  
  } else                                                        return "FTP server is already running.";    
}
String stopFtpServer () {
  if (ftpSrv) { delete (ftpSrv); ftpSrv = NULL; return "FTP server stopped. Active connections will continue to run anyway."; } 
  else                                          return "FTP server is not running.";
}

telnetServer *telnetSrv; // pointer to Telnet server
String telnetCommandHandler (int argc, String argv [], String homeDirectory);
String startTelnetServer () {
  if (!telnetSrv) {
    telnetSrv = new telnetServer (telnetCommandHandler, // a callback function tht will handle telnet commands that are not handled by telnet server itself
                                  8192,                 // 8 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stabile
                                  "0.0.0.0",            // start telnt server on all available ip addresses
                                  23,                   // telnet port
                                  telnetAndFtpFirewall);// use firewall callback function for telnet server or NULL
    if (telnetSrv)
      if (telnetSrv->started ())                                        return "Telnet server started.";  
      else                      { delete (telnetSrv); telnetSrv = NULL; return "Could not start Telnet server."; }
   else                                                                 return "Could not start Telnet server.";  
  } else                                                                return "Telnet server is already running.";    
}
String stopTelnetServer () {
  if (telnetSrv) { delete (telnetSrv); telnetSrv = NULL; return "Telnet server stopped. Active connections will continue to run anyway, includin this one.\r\n"
                                                                "Once the connections are closed you won't be able to run Telnet server again."; }
  else                                                   return "Telnet server is not running.";
}



String telnetCommandHandler (int argc, String argv [], String homeDirectory) { // Example 05
  
  // ----- start and stop servers -----
  
  if (argc == 3 && argv [0] == "start" && argv [1] == "web" && argv [2] == "server") {

    if (homeDirectory == "/") return startWebServer () + "\r\n"; // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to start web sever.\r\n";
    
  } else if (argc == 3 && argv [0] == "stop" && argv [1] == "web" && argv [2] == "server") {
    
    if (homeDirectory == "/") return stopWebServer () + "\r\n"; // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to stop web sever.\r\n";

  } else if (argc == 3 && argv [0] == "start" && argv [1] == "ftp" && argv [2] == "server") {

    if (homeDirectory == "/") return startFtpServer () + "\r\n"; // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to start FTP sever.\r\n";

  } else if (argc == 3 && argv [0] == "stop" && argv [1] == "ftp" && argv [2] == "server") {
    
    if (homeDirectory == "/") return stopFtpServer () + "\r\n"; // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to stop FTP sever.\r\n";

  } else if (argc == 3 && argv [0] == "start" && argv [1] == "telnet" && argv [2] == "server") {

    if (homeDirectory == "/") return startTelnetServer () + "\r\n"; // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to start Telnet sever.\r\n";
    
  } else if (argc == 3 && argv [0] == "stop" && argv [1] == "telnet" && argv [2] == "server") {
    
    if (homeDirectory == "/") return stopTelnetServer () + "\r\n"; // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to stop Telnet sever.\r\n";

  // ---- led on ESP32 ----
    
  } else if (argc == 2 && argv [0] == "led" && argv [1] == "state") {
    
getBuiltInLed:
    return "Led is " + (digitalRead (2) ? String ("on.\n") : String ("off.\n"));
    
  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {
    
    digitalWrite (2, HIGH);
    goto getBuiltInLed;
    
  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "off") {
    
    digitalWrite (2, LOW);
    goto getBuiltInLed;
    
  }

  // ----- telnetCommand has not been handled by telnetCommandHandler - let the telnetServer handle it itself ----
  
  return ""; 
}

// setup (), loop () --------------------------------------------------------

  //disable brownout detector - if power asupply is rather poor
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"

void setup () {
  WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector

  Serial.begin (115200);

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause ();
  switch (wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0:     dmesg ("[ESP32] Wakeup caused by external signal using RTC_IO."); break;
    case ESP_SLEEP_WAKEUP_EXT1:     dmesg ("[ESP32] Wakeup caused by external signal using RTC_CNTL."); break;
    case ESP_SLEEP_WAKEUP_TIMER:    dmesg ("[ESP32] Wakeup caused by timer."); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: dmesg ("[ESP32] Wakeup caused by touchpad."); break;
    case ESP_SLEEP_WAKEUP_ULP:      dmesg ("[ESP32] Wakeup caused by ULP program."); break;
    default:                        dmesg ("[ESP32] Wakeup was not caused by deep sleep: " + String (wakeup_reason) + "."); break;
  }
  
  if (mountSPIFFS ()) dmesg ("[SPIFFS] mounted.");      // this is the first thing that you should do
  else                dmesg ("[SPIFFS] mount failed.");

  usersInitialization ();                             // creates user management files with "root", "webadmin" and "webserver" users (only needed for initialization)

  connectNetwork ();                                  // network should be connected after file system is mounted since it reads its configuration from file system
  // if (networkAccesPointWorking) dmesg ("wlan1 is working in Access Point mode.");
  // else                          dmesg ("wlan1 could not mount Access Point.");
  // if (networkStationWorking)    dmesg ("wlan2 is connected in Station mode.");
  // else                          dmesg ("wlan2 could not connect as a station.");
  
  listFilesOnFlashDrive ();

  startWebServer ();
  startFtpServer ();
  startTelnetServer ();

  // examples
  pinMode (2, OUTPUT);                                // this is just for demonstration purpose - prepare built-in LED
  digitalWrite (2, LOW);
  pinMode (22, INPUT_PULLUP);                         // this is just for demonstration purpose - oscilloscope
  if (pdPASS != xTaskCreate (examples, "examples", 2048, NULL, tskNORMAL_PRIORITY, NULL)) Serial.printf ("[examples] couldn't start examples\n");
}

void loop () {
  SPIFFSsafeDelay (1);  

  rtc.doThings ();                                  // automatically synchronize real_time_clock with NTP server(s) once a day

  if (rtc.isGmtTimeSet ()) {                        // this is just for demonstration purpose - how to use real time clock
    static bool messageAlreadyDispalyed = false;
    time_t now = rtc.getLocalTime ();
    char s [9];
    strftime (s, 9, "%H:%M:%S", gmtime (&now));
    if (strcmp (s, "23:05:00") >= 0 && !messageAlreadyDispalyed && (messageAlreadyDispalyed = true)) Serial.printf ("Working late again?\n");
    if (strcmp (s, "06:00:00") <= 0) messageAlreadyDispalyed = false;
  }

  static unsigned long lastFreeHeapSampleTime = -60000;
  static int lastScale = -1;
  if (millis () - lastFreeHeapSampleTime > 60000) {
    lastFreeHeapSampleTime = millis ();
    lastScale = (lastScale + 1) % 60;
    freeHeap.addMeasurement (lastScale, ESP.getFreeHeap () / 1024); // take s asmple of free heap in KB each minute 
    connectionCount.addCounterToMeasurements (lastScale);           // take sample of number of web connections that arrived last minute
    Serial.printf ("[loop ()][Thread:%lu][Core:%i] free heap:   %6i bytes\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), ESP.getFreeHeap ());
    dmesg ("Free heap: " + String (ESP.getFreeHeap () / 1024) + " KB.");    
  }  
}
