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
 *            September 4, Bojan Jurca   
 *          - elimination of compiler warnings and some bugs
 *            Jun 10, 2020, Bojan Jurca
 *  
 */


// ----- define basic project information - this informatin will be displayed as a response to uname telnet command -----

#define HOSTNAME    "MyESP32Server" // <- this is just an example, define unique name for each chip here - by default, if you don't change #definitions this will be hostname of STA and AP network interfaces and SSID of your AP
#define MACHINETYPE "ESP32 NodeMCU" // <- this is just an example, describe your hardware here

#include "./servers/webServer.hpp"                      // Web server


#include <WiFi.h>

// ----- include project features -----

#include "./servers/file_system.h"                                  // network and user configuration files are loacted on file sistem - we'll need them

// define default STA_SSID, STA_PASSWORD, STA_IP, STA_SUBNET_MASK, STA_GATEWAY, AP_SSID, AP_PASSWORD, AP_IP, AP_SUBNET_MASK in network.h
#include "./servers/network.h"                                      // we'll need this to set up a network, Telnet server also needs this to execute certain commands such as ifconfig, ping, ... 

// #define USER_MANAGEMENT NO_USER_MANAGEMENT             // Telnet and FTP servers use user management, Web server uses it only to get its home directory
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT      // define the kind of user management project is going to use
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"

// define TIMEZONE  KAL_TIMEZONE
// ...
// #define TIMEZONE  EASTERN_TIMEZONE
// (default) #define TIMEZONE  CET_TIMEZONE               // choose your time zone or modify timeToLocalTime function
#include "./servers/real_time_clock.hpp"                  // Telnet server needs rtc to execute certain commands such as uptime
real_time_clock rtc ((char *) "1.si.pool.ntp.org",        // first NTP server
                     (char *) "2.si.pool.ntp.org",        // second NTP server if the first one is not accessible
                     (char *) "3.si.pool.ntp.org");       // third NTP server if the first two are not accessible

#include "./servers/oscilloscope.h"

// ----- measurements are not necessary, they are here just for demonstration  -----
#include "measurements.hpp"
measurements freeHeap (60);                 // measure free heap each minute for possible memory leaks
measurements httpRequestCount (60);         // measure how many web connections arrive each minute
measurements rssi (60);                     // measure WiFi signal quality
// ...

#include "examples.h" // Example 07, Example 08, Example 09, Example 10, Example 11


#include "./servers/webServer.hpp"                      // Web server
httpServer  *httpSrv = NULL;                            // pointer to Web server
String httpRequestHandler (String& httpRequest);
void wsRequestHandler (String& wsRequest, WebSocket *webSocket);
String startWebServer () {
  if (getWiFiMode () == WIFI_OFF) {
    webDmesg ("Could not start HTTP server since there is no network.");
    return "Could not start HTTP server since there is no network.";
  }
 
  if (!httpSrv) {
    httpSrv = new httpServer (httpRequestHandler,         // a callback function that will handle HTTP requests that are not handled by webServer itself
                              wsRequestHandler,           // a callback function that will handle WS requests, NULL to ignore WS requests
                              8192,                       // 8 KB stack size is usually enough, if httpRequestHandler uses more stack increase this value until server is stable
                              (char *) "0.0.0.0",         // start HTTP server on all available ip addresses
                              80,                         // HTTP port
                              NULL);                      // we won't use firewall callback function for HTTP server
    if (httpSrv)
      if (httpSrv->started ())                                    return "HTTP server started.";  
      else                    { delete (httpSrv); httpSrv = NULL; return "Could not start HTTP server."; }
    else                                                          return "Could not start HTTP server.";  
  } else                                                          return "HTTP server is already running.";    
}
String stopWebServer () {
  if (httpSrv) { delete (httpSrv); httpSrv = NULL; return "HTTP server stopped. Active connections will continue to run anyway."; } 
  else                                             return "HTTP server is not running.";
}
String httpRequestHandler (String& httpRequest) { // - normally httpRequest is HTTP request, webSocket is NULL, function returns a reply in HTML, json, ... formats or "" if request is unhandeled
                                                  // httpRequestHandler is supposed to be used with smaller replies,
                                                  // if you want to reply with larger pages you may consider FTP-ing .html files onto the file system (/var/www/html/ by default)
                                                  // - has to be reentrant!

  // ----- just a demonstration - delete this code if it is not needed -----

  httpRequestCount.increaseCounter ();                            // gether some statistics

  // example05 variables
  static String niceSwitch1 = "false";  
  static int niceSlider3 = 3;
  static String niceRadio5 = "fm";

  // ----- handle HTTP protocol requests -----
  
       if (httpRequest.substring (0, 20) == "GET /example01.html ")       { // used by Example 01
                                                                            return String ("<HTML>Example 01 - dynamic HTML page<br><br><hr />") + (digitalRead (2) ? "Led is on." : "Led is off.") + String ("<hr /></HTML>");
                                                                          }
  else if (httpRequest.substring (0, 16) == "GET /builtInLed ")           { // used by Example 02, Example 03, Example 04, index.html
                                                                          getBuiltInLed:
                                                                            return "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + (digitalRead (2) ? String ("on") : String ("off")) + "\"}\r\n";
                                                                          }                                                                    
  else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ")        { // used by Example 03, Example 04
                                                                            digitalWrite (2, HIGH);
                                                                            goto getBuiltInLed;
                                                                          }
  else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ")       { // used by Example 03, Example 04, index.html
                                                                            digitalWrite (2, LOW);
                                                                            goto getBuiltInLed;
                                                                          }
  else if (httpRequest.substring (0, 12) == "GET /upTime ")               { // used by index.html
                                                                            unsigned long long l = millis () / 1000; // counts up to 50 days
                                                                            if (rtc.isGmtTimeSet ()) l = rtc.getGmtTime () - rtc.getGmtStartupTime (); // correct timing
                                                                            // int s = l % 60;
                                                                            // l /= 60;
                                                                            // int m = l % 60;
                                                                            // l /= 60;
                                                                            // int h = l % 24;
                                                                            // l /= 24;
                                                                            // return "{\"id\":\"" + ESP_NAME_FOR_JSON + "\",\"upTime\":\"" + String ((int) l) + " days " + String (h) + " hours " + String (m) + " minutes " + String (s) + " seconds\"}";
                                                                            return "{\"id\":\"" + String (HOSTNAME) + "\",\"upTime\":\"" + String ((unsigned long) l) + " sec\"}\r\n";
                                                                          }                                                                    
  else if (httpRequest.substring (0, 14) == "GET /freeHeap ")             { // used by index.html
                                                                            return freeHeap.toJson (5);
                                                                          }
  else if (httpRequest.substring (0, 22) == "GET /httpRequestCount ")     { // used by index.html
                                                                            return httpRequestCount.toJson (5);
                                                                          }
  else if (httpRequest.substring (0, 10) == "GET /rssi ")                 { // used by index.html
                                                                            return rssi.toJson (5);
                                                                          }
  else if (httpRequest.substring (0, 17) == "GET /niceSwitch1 ")          { // used by example05.html
                                                                          returnNiceSwitch1State:
                                                                            return "{\"id\":\"niceSwitch1\",\"value\":\"" + niceSwitch1 + "\"}"; // read switch state from variable or in some other way
                                                                          }
  else if (httpRequest.substring (0, 17) == "PUT /niceSwitch1/")          { // used by example05.html
                                                                            niceSwitch1 = httpRequest.substring (17, 22); // "true " or "false"
                                                                            niceSwitch1.trim ();
                                                                            Serial.println ("[Got request from web browser for niceSwitch1]: " + niceSwitch1 + "\n");
                                                                            goto returnNiceSwitch1State; // return success (or possible failure) back to the client
                                                                          }
  else if (httpRequest.substring (0, 25) == "PUT /niceButton2/pressed ")  { // used by example05.html
                                                                            Serial.printf ("[Got request from web browser for niceButton2]: pressed\n");
                                                                            return "{\"id\":\"niceButton2\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                          }
  else if (httpRequest.substring (0, 17) == "GET /niceSlider3 ")          { // used by example05.html
                                                                          returnNiceSlider3Value:
                                                                            return "{\"id\":\"niceSlider3\",\"value\":\"" + String (niceSlider3) + "\"}"; // read slider value from variable or in some other way
                                                                          }
  else if (httpRequest.substring (0, 17) == "PUT /niceSlider3/")          { // used by example05.html
                                                                            niceSlider3 = httpRequest.substring (17, 19).toInt (); // 0 .. 10
                                                                            Serial.printf ("[Got request from web browser for niceSlider3]: %i\n", niceSlider3);
                                                                            goto returnNiceSlider3Value; // return success (or possible failure) back to the client
                                                                          }
  else if (httpRequest.substring (0, 25) == "PUT /niceButton4/pressed ")  { // used by example05.html
                                                                            Serial.printf ("[Got request from web browser for niceButton4]: pressed\n");
                                                                            return "{\"id\":\"niceButton4\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                          }
  else if (httpRequest.substring (0, 16) == "GET /niceRadio5 ")           { // used by example05.html
                                                                          returnNiceRadio5Value:
                                                                            return "{\"id\":\"niceRadio5\",\"modulation\":\"" + niceRadio5 + "\"}"; // read radio button selection from variable or in some other way
                                                                          }
  else if (httpRequest.substring (0, 16) == "PUT /niceRadio5/")           { // used by example05.html
                                                                            niceRadio5 = httpRequest.substring (16, 18); // "am", "fm"
                                                                            Serial.printf ("[Got request from web browser for niceRadio5]: %s\n", niceRadio5.c_str ());
                                                                            goto returnNiceRadio5Value; // return success (or possible failure) back to the client
                                                                          }
  else if (httpRequest.substring (0, 25) == "PUT /niceButton6/pressed ")  { // used by example05.html
                                                                            Serial.printf ("[Got request from web browser for niceButton6]: pressed\n");
                                                                            return "{\"id\":\"niceButton6\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                          }

  // ----- respond to GET /currentTime if you want to have a REST time server -----

  else if (httpRequest.substring (0, 17) == "GET /currentTime ")          { 
                                                                            if (rtc.isGmtTimeSet ()) {
                                                                              time_t tmp = rtc.getGmtTime ();
                                                                              while (tmp == rtc.getGmtTime ()) SPIFFSsafeDelay (1); // wait for second to end
                                                                              time_t now = rtc.getGmtTime ();
                                                                              return "{\"id\":\"" + String (HOSTNAME) + "\",\"currentTime\":\"" + String (now) + "\"}";
                                                                            } else {
                                                                              return ""; // let webServer return 404 - not found
                                                                            }
                                                                          }

  // ----- HTTP request has not been handled by httpRequestHandler - let the webServer try to handle it itself -----

  else                                                                    return ""; 
}
void wsRequestHandler (String& wsRequest, WebSocket *webSocket) { //     // - has to be reentrant!
                                                                    
  // ----- handle WS (WebSockets) protocol requests -----

       if (wsRequest.substring (0, 21) == "GET /runOscilloscope ")      runOscilloscope (webSocket);      // used by oscilloscope.html
  else if (wsRequest.substring (0, 26) == "GET /example10_WebSockets ") example10_webSockets (webSocket); // used by Example 10
}

bool telnetAndFtpFirewall (char *IP) {          // firewall callback function, return true if IP is accepted or false if not
                                                // - has to be reentrant!
  if (!strcmp (IP, "10.0.0.2")) return false;   // block 10.0.0.2 (for some reason) ... please note that this is just an example
  else                          return true;    // ... but let every other client through
}


#define FTP_RTC rtc                                     // tell FTP server where to get time from to report file time
#include "./servers/ftpServer.hpp"                      // SPIFFS doesn't record file creation time so this information will be false anyway
ftpServer *ftpSrv = NULL;                               // pointer to FTP server (it doesn't call external handling function so it doesn't have to be defined)
String startFtpServer () {
  if (getWiFiMode () == WIFI_OFF) {
    ftpDmesg ("Could not start FTP server since there is no network.");
    return "Could not start FTP server since there is no network.";
  }
    
  if (!ftpSrv) {
    ftpSrv = new ftpServer ((char *) "0.0.0.0",         // start FTP server on all available ip addresses
                            21,                         // controll connection FTP port
                            telnetAndFtpFirewall);      // use firewall callback function for FTP server or NULL
    if (ftpSrv)
      if (ftpSrv->started ())                                   return "FTP server started.";  
      else                    { delete (ftpSrv); ftpSrv = NULL; return "Could not start FTP server."; }
    else                                                        return "Could not start FTP server.";  
  } else                                                        return "FTP server is already running.";    
}
String stopFtpServer () {
  if (ftpSrv) { delete (ftpSrv); ftpSrv = NULL; return "FTP server stopped. Active connections will continue to run anyway."; } 
  else                                          return "FTP server is not running.";
}


#define TELNET_RTC rtc                                    // tell Telnet server functions (like uptime, ...) where to get time from
#include "./servers/telnetServer.hpp"                     // Telnet server - if other server are going to use dmesg telnetServer.hpp has to be defined priorly
telnetServer *telnetSrv = NULL; // pointer to Telnet server
String telnetCommandHandler (int argc, String argv [], String homeDirectory);
String startTelnetServer () {
  if (getWiFiMode () == WIFI_OFF) {
    dmesg ("Could not start Telnet server since there is no network.");
    return "Could not start Telnet server since there is no network.";
  }
    
  if (!telnetSrv) {
    telnetSrv = new telnetServer (telnetCommandHandler, // a callback function that will handle telnet commands that are not handled by telnet server itself
                                  8192,                 // 8 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stable
                                  (char *) "0.0.0.0",   // start telnt server on all available ip addresses
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
    if (homeDirectory == "/") return startWebServer (); // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to start web sever.";
  } else if (argc == 3 && argv [0] == "stop" && argv [1] == "web" && argv [2] == "server") {
    if (homeDirectory == "/") return stopWebServer (); // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to stop web sever.";
  } else if (argc == 3 && argv [0] == "start" && argv [1] == "ftp" && argv [2] == "server") {
    if (homeDirectory == "/") return startFtpServer (); // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to start FTP sever.";
  } else if (argc == 3 && argv [0] == "stop" && argv [1] == "ftp" && argv [2] == "server") {
    if (homeDirectory == "/") return stopFtpServer (); // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to stop FTP sever.";
  } else if (argc == 3 && argv [0] == "start" && argv [1] == "telnet" && argv [2] == "server") {
    if (homeDirectory == "/") return startTelnetServer (); // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to start Telnet sever.";
  } else if (argc == 3 && argv [0] == "stop" && argv [1] == "telnet" && argv [2] == "server") {
    if (homeDirectory == "/") return stopTelnetServer (); // note that the level of rights is determined by homeDirecory
    else                      return "You must have root rights to stop Telnet sever.";

  } else if (argc == 3 && argv [0] == "ifconfig" && argv [1] == "AP" && argv [2] == "up") {
                              WiFi.mode (WIFI_AP_STA);
                              return "AP is up.";

  } else if (argc == 3 && argv [0] == "ifconfig" && argv [1] == "AP" && argv [2] == "down") {
                              WiFi.mode (WIFI_STA);
                              return "AP is down";

  } else if (argc == 3 && argv [0] == "ifconfig" && argv [1] == "STA" && argv [2] == "up") {
                              WiFi.mode (WIFI_AP_STA);
                              return "STA is up.";

  } else if (argc == 3 && argv [0] == "ifconfig" && argv [1] == "STA" && argv [2] == "down") {
                              WiFi.mode (WIFI_AP);
                              return "STA is down";


  // ---- led on ESP32 ----
    
  } else if (argc == 2 && argv [0] == "led" && argv [1] == "state") {
  getBuiltInLed:
    return "Led is " + (digitalRead (2) ? String ("on.") : String ("off."));
  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {
    digitalWrite (2, HIGH);
    goto getBuiltInLed;
  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "off") {
    digitalWrite (2, LOW);
    goto getBuiltInLed;

  // ----- digitalRead, analogRead -----
  
  } else if (argc == 2 && argv [0] == "digitalRead") { // digitalRead example
    int pin = argv [1].toInt ();
    if ((!pin && argv [1] != "0") || pin < 0 || pin > 39) return "Invalid pin number";
    else                                                  return "digitalRead (" + String (pin) + ") = " + String (digitalRead (pin));
  } else if (argc == 2 && argv [0] == "analogRead") { // analogRead example
    int pin = argv [1].toInt ();
    if ((!pin && argv [1] != "0") || pin < 0 || pin > 39) return "Invalid pin number";
    else                                                  return "analogRead (" + String (pin) + ") = " + String (analogRead (pin));
  }  

  // ----- telnetCommand has not been handled by telnetCommandHandler - let the telnetServer handle it itself ----
  
  return ""; 
}


// setup (), loop () --------------------------------------------------------

  //disable brownout detector - if power supply is rather poor
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"

void setup () {
  WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector

  // disableCore0WDT ();
  // disableCore1WDT ();

  Serial.begin (115200);
 
  // __testLocalTime__ ();

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause ();
  switch (wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0:     dmesg ("[ESP32] wakeup caused by external signal using RTC_IO."); break;
    case ESP_SLEEP_WAKEUP_EXT1:     dmesg ("[ESP32] wakeup caused by external signal using RTC_CNTL."); break;
    case ESP_SLEEP_WAKEUP_TIMER:    dmesg ("[ESP32] wakeup caused by timer."); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: dmesg ("[ESP32] wakeup caused by touchpad."); break;
    case ESP_SLEEP_WAKEUP_ULP:      dmesg ("[ESP32] wakeup caused by ULP program."); break;
    default:                        dmesg ("[ESP32] wakeup was not caused by deep sleep: " + String (wakeup_reason) + "."); break;
  }

  // SPIFFS.format (); // uncomment this line and run sketch only one time on ESP32 then comment it again 

  mountSPIFFS (true);                             // this is the first thing that you should do

  usersInitialization ();                         // creates user management files with "root", "webadmin" and "webserver" users (only needed for initialization)

  // SPIFFS.remove ("/etc/wpa_supplicant.conf");  // deleting wpa_supplicant.conf willd cause creating a new one with default WiFi settings - see network.h

  connectNetwork ();                              // network should be connected after file system is mounted since it reads its configuration from file system

  // listFilesOnFlashDrive ();

  startWebServer ();
  startTelnetServer ();
  startFtpServer ();

  // ----- examples -----
  
  pinMode (2, OUTPUT);                            // this is just for demonstration purpose - prepare built-in LED
  digitalWrite (2, LOW);
  pinMode (22, INPUT_PULLUP);                     // this is just for demonstration purpose - oscilloscope
  if (pdPASS != xTaskCreate ([] (void *) {        // start examples in separate thread
                                            example07_filesAndDelays ();
                                            example08_realTimeClock ();
                                            example09_makeRestCall ();
                                            example11_morseEchoServer ();
                                            vTaskDelete (NULL); // end this thread
                                         }, "examples", 2048, NULL, tskNORMAL_PRIORITY, NULL)) Serial.printf ("[%10lu] [examples] couldn't start examples\n", millis ());
}

void loop () {  
  SPIFFSsafeDelay (1);  

  // periodicly check network status and reconnect if neccessary
  network_doThings ();
  // give RTC its processing time - clock synchronization
  rtc.doThings ();

  // ----- just a demonstration from now on -----
  
  if (rtc.isGmtTimeSet ()) {                        
    static bool messageAlreadyDispalyed = false;
    if (timeToString (rtc.getLocalTime ()).substring (11) >= "23:05:00" && !messageAlreadyDispalyed) {
      messageAlreadyDispalyed = true;
      Serial.printf ("[%10lu] Working late again?\n", millis ());
    }
  }

  static unsigned long lastMeasurementTime = -60000; 
  static int lastScale = -1;
  if (millis () - lastMeasurementTime > 60000) {
    lastMeasurementTime = millis ();
    lastScale = (lastScale + 1) % 60;
    freeHeap.addMeasurement (lastScale, ESP.getFreeHeap () / 1024); // take s sample of free heap in KB each minute 
    httpRequestCount.addCounterToMeasurements (lastScale);          // take sample of number of web connections that arrived last minute
    rssi.addMeasurement (lastScale, WiFi.RSSI ());                  // take RSSI sample each minute 
    Serial.printf ("[%10lu] [loop ()] %s, free heap: %6i bytes, RSSI: %3i dBm\n", millis (), 
                                                                                  rtc.isGmtTimeSet () ? timeToString (rtc.getLocalTime ()).c_str () : "                   ",
                                                                                  ESP.getFreeHeap (),
                                                                                  WiFi.RSSI ()
                                                                                );
  }  
}
