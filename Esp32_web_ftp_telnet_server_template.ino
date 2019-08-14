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
 *  
 */


#define PROJECT_ID "Esp32_web_ftp_telnet_server_template" // change with the name of your project

#include <WiFi.h>

#include "measurements.hpp"
measurements freeHeap (60);                 // measure free heap each minute for possible memory leaks
measurements connectionCount (60);          // measure how many web connections arrive each minute



// features

#include "real_time_clock.hpp"
real_time_clock rtc ( "1.si.pool.ntp.org",  // first NTP server
                      "3.si.pool.ntp.org",  // second NTP server if the first one is not accessible
                      "3.si.pool.ntp.org"); // third NTP server if the first two are not accessible

#include "file_system.h"

#include "network.h"

#include "user_management.h"

#include "webServer.hpp"
webServer *webSrv;

#include "examples.h" // Example 06, Example 07, Example 08, Example 09, Example 10, Oscilloscope


String httpRequestHandler (String httpRequest, WebSocket *webSocket) {  // - normally httpRequest is HTTP request, webSocket is NULL, function returns a reply in HTML, json, ... formats or "" if request is unhandeled
                                                                        // - for WebSocket httpRequest is WS request, webSocket pointer is set, whatever function returns will be discarded
                                                                        // httpRequestHandler is supposed to be used with smaller replies,
                                                                        // if you want to reply with larger pages you may consider FTP-ing .html files onto the file system (/var/www/html/ by default)
                                                                        // - has to be reentrant!
                                                                        
       connectionCount.increaseCounter ();                              // gether some statistics

  // handling HTTP
       if (httpRequest.substring (0, 20) == "GET /example01.html ") { // Example 01
                                                                      return String ("<HTML>Example 01 - dynamic HTML page<br><br>") + (digitalRead (2) ? "Led is on." : "Led is off.") + String ("</HTML>");
                                                                    }
  else if (httpRequest.substring (0, 16) == "GET /builtInLed ")     { // Example 02, Example 03, Example 04, index.html
                                                                    getBuiltInLed:
                                                                      return "{\"id\":\"esp32\",\"builtInLed\":\"" + (digitalRead (2) ? String ("on") : String ("off")) + "\"}\r\n";
                                                                    }                                                                    
  else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ")  { // Example 03, Example 04
                                                                      digitalWrite (2, HIGH);
                                                                      goto getBuiltInLed;
                                                                    }
  else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ") { // Example 03, Example 04, index.html
                                                                      digitalWrite (2, LOW);
                                                                      goto getBuiltInLed;
                                                                    }
  else if (httpRequest.substring (0, 22) == "PUT /builtInLed/on10s ") { // index.html
                                                                        digitalWrite (2, HIGH);
                                                                        SPIFFSsafeDelay (10000);
                                                                        digitalWrite (2, LOW);
                                                                        goto getBuiltInLed;
                                                                      }
  else if (httpRequest.substring (0, 12) == "GET /upTime ")         { // index.html
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
  else if (httpRequest.substring (0, 14) == "GET /freeHeap ")       { // index.html
                                                                      return freeHeap.measurements2json (5);
                                                                    }
  else if (httpRequest.substring (0, 21) == "GET /connectionCount "){ // index.html
                                                                      return connectionCount.measurements2json (5);
                                                                    }
  // handling WS (WebSockets)
  else if (httpRequest.substring (0, 26) == "GET /example09_WebSockets " && webSocket){ // Example 09
                                                                      example09_webSockets (webSocket);
                                                                      return ""; // doesn't matter what the function returns in case of WebSockets
                                                                    }
  else if (httpRequest.substring (0, 21) == "GET /runOscilloscope "){ // oscilloscope.html
                                                                      example_oscilloscope (webSocket);
                                                                      return "";
                                                                    }

  else                                                              return ""; // HTTP request has not been handled by httpRequestHandler - let the webServer handle it itself
}


#include "ftpServer.hpp"
ftpServer *ftpSrv;

bool ftpAndTelnetFirewall (char *IP) {          // firewall callback function, return true if IP is accepted or false if not
                                                // - has to be reentrant!
  if (!strcmp (IP, "10.0.0.2")) return false;   // block 10.0.0.2 (for some reason) ... please note that this is just an example
  else                          return true;    // ... but let every other client through
}


#include "telnetServer.hpp"
telnetServer *telnetSrv;

String telnetCommandHandler (String command, String parameter, String homeDirectory) { // Example 05
  if (command + " " + parameter  == "led state") {
getBuiltInLed:
    return "Led is " + (digitalRead (2) ? String ("on.\n") : String ("off.\n"));
  } else if (command + " " + parameter == "turn led on") {
    digitalWrite (2, HIGH);
    goto getBuiltInLed;
  } else if (command + " " + parameter  == "turn led off") {
    digitalWrite (2, LOW);
    goto getBuiltInLed;
  }
  
  return ""; // telnetCommand has not been handled by telnetCommandHandler - let the telnetServer handle it itself
}


// setup (), loop () --------------------------------------------------------

  //disable brownout detector - if power asupply is rather poor
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"

void setup () {
  WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  
  Serial.begin (115200);

  mountSPIFFS ();                                     // this is the first thing that you should do

  usersInitialization ();                             // creates user management files with "root", "webadmin" and "webserver" users (only needed for initialization)

  connectNetwork ();                                  // network should be connected after file system is mounted since it reads its configuration from file system

  writeProjectIdOnFlashDrive ();
  listFilesOnFlashDrive ();

  webSrv = new webServer (httpRequestHandler,         // a callback function tht will handle HTTP request that are not handled by webServer itself
                          4096,                       // 4 KB stack size is usually enough, it httpRequestHandler uses more stack increase this value until server is stabile
                          "0.0.0.0",                  // start web server on all available ip adresses
                          80,                         // HTTP port
                          NULL);                      // we won't use firewall callback function for web server

  ftpSrv = new ftpServer ("0.0.0.0",                  // start FTP server on all available ip adresses
                          21,                         // controll connection FTP port
                          ftpAndTelnetFirewall);      // use firewall callback function for FTP server

  telnetSrv = new telnetServer (telnetCommandHandler, // a callback function tht will handle telnet commands that are not handled by telnet server itself
                                4096,                 // 4 KB stack size is usually enough, it telnetCommandHanlder uses more stack increase this value until server is stabile
                                "0.0.0.0",            // start telnt server on all available ip adresses
                                23,                   // telnet port
                                ftpAndTelnetFirewall);// use firewall callback function for telnet server

  pinMode (2, OUTPUT);                                // this is just for demonstration purpose - prepare built-in LED
  digitalWrite (2, LOW);

  // examples
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
  }  
}
