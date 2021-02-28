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
 *          - added SPIFFSsafeDelay () to assure safe muti-threading while using fileSystem functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca
 *          - telnetCommandHandler parameters are now easier to access 
 *            September 4, Bojan Jurca   
 *          - elimination of compiler warnings and some bugst
 *            Jun 10, 2020, Bojan Jurca
 *          - port from SPIFFS to fileSystem, adjustment for Arduino 1.8.13,
 *            improvements of web, FTP and telnet server,
 *            simplification of this template to make it more comprehensive and easier to start working with 
 *            October 10, 2020, Bojan Jurca
 *          - web login/logout example
 *            February 3, 2021, Bojan Jurca
 *  
 */

// Compile this code with Arduino for one of ESP32 boards (Tools | Board) and one of FAT partition schemas (Tools | Partition scheme)!

#include <WiFi.h>

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           HOSTNAME 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
#include "./servers/file_system.h"
#include "./servers/network.h"
// #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"
// define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
// ...
// #define TIMEZONE  EASTERN_TIMEZONE
// (default) #define TIMEZONE  CET_TIMEZONE               
#define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
#define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
#include "./servers/time_functions.h"     
#include "./servers/webServer.hpp"                              // include HTTP Server
#include "./servers/ftpServer.hpp"                              // include FTP server
#include "./servers/telnetServer.hpp"                           // include Telnet server


              // ----- measurements are just for demonstration - delete this code if it is not needed -----

              #include "measurements.hpp"
              measurements freeHeap (60);                 // measure free heap each minute for possible memory leaks
              measurements httpRequestCount (60);         // measure how many web connections arrive each minute
              // ...
              #include "examples.h" // example 08, example 09, example 10, example 11


// ----- HTTP request handler example - if you don't want to handle HTTP requests just delete this function and pass NULL to httpSrv instead of its address -----
//       normally httpRequest is HTTP request, function returns a reply in HTML, json, ... formats or "" if request is unhandeled by httpRequestHandler
//       httpRequestHandler is supposed to be used with smaller replies,
//       if you want to reply with larger pages you may consider FTP-ing .html files onto the file system (into /var/www/html/ directory)
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  // debug: Serial.print (httpRequest);
  // debug: Serial.println (wsp->getHttpRequestHeaderField ("Cookie"));
  // debug: Serial.println (wsp->getHttpRequestCookie ("sessionToken"));
  

              // ----- examples - delete this code if it is not needed -----

              httpRequestCount.increaseCounter ();                            // gether some statistics

              // variables used by example 05
              static String niceSwitch1 = "false";  
              static int niceSlider3 = 3;
              static String niceRadio5 = "fm";

              // ----- handle HTTP protocol requests -----
              
                   if (httpRequest.substring (0, 20) == "GET /example01.html ")       { // used by example 01
                                                                                        return String ("<HTML>Example 01 - dynamic HTML page<br><br><hr />") + (digitalRead (2) ? "Led is on." : "Led is off.") + String ("<hr /></HTML>");
                                                                                      }
              else if (httpRequest.substring (0, 16) == "GET /builtInLed ")           { // used by example 02, example 03, example 04, index.html
                                                                                      getBuiltInLed:
                                                                                        return "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + (digitalRead (2) ? String ("on") : String ("off")) + "\"}\r\n";
                                                                                      }                                                                    
              else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ")        { // used by example 03, example 04
                                                                                        digitalWrite (2, HIGH);
                                                                                        goto getBuiltInLed;
                                                                                      }
              else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ")       { // used by example 03, example 04, index.html
                                                                                        digitalWrite (2, LOW);
                                                                                        goto getBuiltInLed;
                                                                                      }
              else if (httpRequest.substring (0, 12) == "GET /upTime ")               { // used by index.html
                                                                                        time_t t = getUptime ();       // t holds seconds
                                                                                        int seconds = t % 60; t /= 60; // t now holds minutes
                                                                                        int minutes = t % 60; t /= 60; // t now holds hours
                                                                                        int hours = t % 24;   t /= 24; // t now holds days
                                                                                        char c [10];
                                                                                        sprintf (c, "%02i:%02i:%02i", hours, minutes, seconds);
                                                                                        String s = "";
                                                                                        if (t) s += String ((unsigned long) t) + " days, ";
                                                                                        s += String (c);
                                                                                        return "{\"id\":\"" + String (HOSTNAME) + "\",\"upTime\":\"" + s + "\"}";
                                                                                      }                                                                    
              else if (httpRequest.substring (0, 14) == "GET /freeHeap ")             { // used by index.html
                                                                                        return freeHeap.toJson (5);
                                                                                      }
              else if (httpRequest.substring (0, 22) == "GET /httpRequestCount ")     { // used by index.html
                                                                                        return httpRequestCount.toJson (5);
                                                                                      }
              else if (httpRequest.substring (0, 17) == "GET /niceSwitch1 ")          { // used by example 05.html
                                                                                      returnNiceSwitch1State:
                                                                                        return "{\"id\":\"niceSwitch1\",\"value\":\"" + niceSwitch1 + "\"}"; // read switch state from variable or in some other way
                                                                                      }
              else if (httpRequest.substring (0, 17) == "PUT /niceSwitch1/")          { // used by example 05.html
                                                                                        niceSwitch1 = httpRequest.substring (17, 22); // "true " or "false"
                                                                                        niceSwitch1.trim ();
                                                                                        Serial.println ("[Got request from web browser for niceSwitch1]: " + niceSwitch1 + "\n");
                                                                                        goto returnNiceSwitch1State; // return success (or possible failure) back to the client
                                                                                      }
              else if (httpRequest.substring (0, 25) == "PUT /niceButton2/pressed ")  { // used by example 05.html
                                                                                        Serial.printf ("[Got request from web browser for niceButton2]: pressed\n");
                                                                                        return "{\"id\":\"niceButton2\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                                      }
              else if (httpRequest.substring (0, 17) == "GET /niceSlider3 ")          { // used by example 05.html
                                                                                      returnNiceSlider3Value:
                                                                                        return "{\"id\":\"niceSlider3\",\"value\":\"" + String (niceSlider3) + "\"}"; // read slider value from variable or in some other way
                                                                                      }
              else if (httpRequest.substring (0, 17) == "PUT /niceSlider3/")          { // used by example 05.html
                                                                                        niceSlider3 = httpRequest.substring (17, 19).toInt (); // 0 .. 10
                                                                                        Serial.printf ("[Got request from web browser for niceSlider3]: %i\n", niceSlider3);
                                                                                        goto returnNiceSlider3Value; // return success (or possible failure) back to the client
                                                                                      }
              else if (httpRequest.substring (0, 25) == "PUT /niceButton4/pressed ")  { // used by example 05.html
                                                                                        Serial.printf ("[Got request from web browser for niceButton4]: pressed\n");
                                                                                        return "{\"id\":\"niceButton4\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                                      }
              else if (httpRequest.substring (0, 16) == "GET /niceRadio5 ")           { // used by example 05.html
                                                                                      returnNiceRadio5Value:
                                                                                        return "{\"id\":\"niceRadio5\",\"modulation\":\"" + niceRadio5 + "\"}"; // read radio button selection from variable or in some other way
                                                                                      }
              else if (httpRequest.substring (0, 16) == "PUT /niceRadio5/")           { // used by example 05.html
                                                                                        niceRadio5 = httpRequest.substring (16, 18); // "am", "fm"
                                                                                        Serial.printf ("[Got request from web browser for niceRadio5]: %s\n", niceRadio5.c_str ());
                                                                                        goto returnNiceRadio5Value; // return success (or possible failure) back to the client
                                                                                      }
              else if (httpRequest.substring (0, 25) == "PUT /niceButton6/pressed ")  { // used by example 05.html                                                                                        Serial.printf ("[Got request from web browser for niceButton6]: pressed\n");
                                                                                        return "{\"id\":\"niceButton6\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                                      }
              // ----- example 07: cookies
              else if (httpRequest.substring (0, 20) == "GET /example07.html ")       { // used by example 07
                                                                                        String refreshCounter = wsp->getHttpRequestCookie ("refreshCounter");           // get cookie that browser sent in HTTP request
                                                                                        if (refreshCounter == "") refreshCounter = "0";
                                                                                        refreshCounter = String (refreshCounter.toInt () + 1);
                                                                                        wsp->setHttpResponseCookie ("refreshCounter", refreshCounter, getGmt () + 60);  // set 1 minute valid cookie that will be send to browser in HTTP reply
                                                                                        return String ("<HTML>Example 07<br><br>This page has been refreshed " + refreshCounter + " times. Click refresh to see more.</HTML>");
                                                                                      }
              // ----- a very basic login - logout mechanism, that could be improoved in many ways -----
              else if (httpRequest.substring (0, 11) == "GET /login/")                { // GET /login/userName%20password - called from login.html when "Login" button is pressed 
                                                                                        String userName = between (httpRequest, "/login/", "%20");        // get user name from URL
                                                                                        String password = between (httpRequest, "%20", " ");              // get password from URL
                                                                                        if (checkUserNameAndPassword (userName, password)) {              // check if they are OK
                                                                                          wsp->setHttpResponseCookie ("sessionToken", "98376235");        // create (simple, demonstration) sessionToken cookie, path and expiration time (in GMT) can also be set
                                                                                          wsp->setHttpResponseCookie ("userName", userName);              // save user name in a cookie for later use
                                                                                          return "loggedIn";                                              // notify login.html about success  
                                                                                        } else {
                                                                                          wsp->setHttpResponseCookie ("sessionToken", "");                // delete sessionToken cookie if it exists
                                                                                          wsp->setHttpResponseCookie ("userName", "");                    // delete userName cookie if it exists
                                                                                          return "Wrong user name or password.";                          // notify login.html about failure
                                                                                        }
                                                                                      }
              else if (httpRequest.substring (0, 12) == "PUT /logout ")               { // called from logout.html when "Logout" button is pressed 
                                                                                          if (wsp->getHttpRequestCookie ("sessionToken") == "98376235") { // if logged in
                                                                                            wsp->setHttpResponseCookie ("sessionToken", "");              // delete sessionToken cookie if it exists
                                                                                            wsp->setHttpResponseCookie ("userName", "");                  // delete userName cookie if it exists
                                                                                          }
                                                                                          return "LoggedOut.";                                            // notify logout.html
                                                                                      }
              else if (httpRequest.substring (0, 17) == "GET /logout.html ")          { // logout.html may only be accessed if user is logged in
                                                                                        if (wsp->getHttpRequestCookie ("sessionToken") == "98376235")     // check if browser has a valid sessionToken cookie
                                                                                          return "";                                                      // if yes, return "" so web server will continue with transmission of logout.html file
                                                                                         wsp->httpResponseStatus = "307 temporary redirect";              // if no, redirect browser to login.html
                                                                                         wsp->setHttpResponseHeaderField ("Location", "http://" + wsp->getHttpRequestHeaderField ("Host") + "/login.html");
                                                                                         return "Not logged in.";
                                                                                       }

  return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


// ----- WebSocket request handler example - if you don't want to handle WebSocket requests just delete this function and pass NULL to httpSrv instead of its address -----
#include "./servers/oscilloscope.h"
void wsRequestHandler (String& wsRequest, WebSocket *webSocket) { // - must be reentrant!


              // ----- example WebSockets & Oscilloscope - delete this code if it is not needed -----

                   if (wsRequest.substring (0, 21) == "GET /runOscilloscope ")      runOscilloscope (webSocket);      // used by oscilloscope.html
              else if (wsRequest.substring (0, 26) == "GET /example10_WebSockets ") example10_webSockets (webSocket); // used by example10.html
              else if (wsRequest.substring (0, 16) == "GET /rssiReader ") {                                           // data streaming used by index.html
                                                                            char c;
                                                                            do {
                                                                              delay (100);
                                                                              int i = WiFi.RSSI ();
                                                                              c = (char) i;
                                                                              // Serial.printf ("[WebSocket data streaming] sending %i to web client\n", i);
                                                                            } while (webSocket->sendBinary ((byte *) &c,  sizeof (c))); // send RSSI information as long as web browser is willing tzo receive it
                                                                          }
}


// ----- telnet command handler example - if you don't want to handle telnet commands yourself just delete this function and pass NULL to telnetSrv instead of its address -----
String telnetCommandHandler (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) { // - must be reentrant!

              
              // ----- example 06 - delete this code if it is not needed -----
              #define LED_BUILTIN 2                                 
                      if (argc == 2 && argv [0] == "led" && argv [1] == "state") {                    // led state telnet command
                        return "Led is " + (digitalRead (LED_BUILTIN) ? String ("on.") : String ("off."));
              } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {  // turn led on telnet  command
                        digitalWrite (LED_BUILTIN, HIGH);
                        return "Led is on.";
              } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "off") { // turn led off telnet command
                        digitalWrite (LED_BUILTIN, LOW);
                        return "Led is off.";
              }


  return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning "" reply
}


              // ----- firewall example - if you don't need firewall just delete this function and pass NULL to the servers instead of its address -----
          
              bool firewall (char *IP) {                            // firewall callback function, return true if IP is accepted or false if not - must be reentrant!
                if (!strcmp (IP, "10.0.0.2")) return false;         // block 10.0.0.2 (for the purpose of this example) 
                else                          return true;          // ... but let every other client through
              }


// ----- cron command handler example - if you don't want to handle cron tasks just delete this function and pass NULL to startCronDaemon... instead of its address -----
void cronHandler (String& cronCommand) {
  // debug: Serial.printf ("[%10lu] [cronDaemon] %s\n", millis (), cronCommand.c_str ());    

          // handle your cron commands/events here
          
          if (cronCommand == "gotTime") { // triggers only once - when ESP32 reads time from NTP servers for the first time

            time_t t = getLocalTime ();
            struct tm st = timeToStructTime (t);
            Serial.println ("Got time at " + timeToString (t) + " (local time), do whatever needs to be done the first time the time is known.");

          } else if (cronCommand == "newYear'sGreetingsToProgrammer") { // triggers at the beginning of each year
          
                Serial.printf ("[%10lu] [cronDaemon] *** HAPPY NEW YEAR ***!\n", millis ());    

          }
         
}


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  // deleteFile ("/etc/ntp.conf");                                    // contains ntp server names form time sync - deleting this file would cause creating default one
  // deleteFile ("/etc/crontab");                                     // contains cheduled tasks                  - deleting this file would cause creating empty one
  startCronDaemonAndInitializeItAtFirstCall (cronHandler, 8 * 1024);  // creates /etc/ntp.conf with default NTP server names and syncronize ESP32 time with them once a day
                                                                      // creates empty /etc/crontab, reads it at startup and executes cronHandler when the time is right
                                                                      // 3 KB stack size is minimal requirement for NTP time synchronization, add more if your cronHandler requires more

  // deleteFile ("/etc/passwd");                                      // contains users' accounts information     - deleting this file would cause creating default one
  // deleteFile ("/etc/shadow");                                      // contains users' passwords                - deleting this file would cause creating default one
  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  // deleteFile ("/network/interfaces");                              // contation STA(tion) configuration        - deleting this file would cause creating default one
  // deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf");          // contation STA(tion) credentials          - deleting this file would cause creating default one
  // deleteFile ("/etc/dhcpcd.conf");                                 // contains A(ccess) P(oint) configuration  - deleting this file would cause creating default one
  // deleteFile ("/etc/hostapd/hostapd.conf");                        // contains A(ccess) P(oint) credentials    - deleting this file would cause creating default one
  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist
  // start web server 
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // a callback function that will handle HTTP requests that are not handled by webServer itself
                                        wsRequestHandler,             // a callback function that will handle WS requests, NULL to ignore WS requests
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        (char *) "0.0.0.0",           // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // we won't use firewall callback function for HTTP server
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start.");

  // start FTP server
  ftpServer *ftpSrv = new ftpServer ((char *) "0.0.0.0",              // start FTP server on all available ip addresses
                                     21,                              // controll connection FTP port
                                     firewall);                       // use firewall callback function for FTP server (replace with NULL if not needed)
  if (!ftpSrv || (ftpSrv && !ftpSrv->started ())) dmesg ("[ftpServer] did not start.");

  // start telnet server
  telnetServer *telnetSrv = new telnetServer (telnetCommandHandler,   // a callback function that will handle telnet commands that are not handled by telnet server itself
                                              16 * 1024,              // 16 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stable
                                              (char *) "0.0.0.0",     // start telnt server on all available ip addresses
                                              23,                     // telnet port
                                              NULL);                  // use firewall callback function for telnet server (replace with NULL if not needed)
  if (!telnetSrv || (telnetSrv && !telnetSrv->started ())) dmesg ("[telnetServer] did not start.");

  // ----- add your own code here -----
  

              // ----- some examples - delete this code if it is not needed -----

              // crontab examples: you can add entries in crontab from code or you can write them into /etc/crontab file,
              // fromat is in both cases the same: second minute hour day month day_of_week command

              cronTabAdd ("* * * * * * gotTime");  // triggers only once - when ESP32 reads time from NTP servers for the first time
              cronTabAdd ("0 0 0 1 1 * newYear'sGreetingsToProgrammer");  // triggers at the beginning of each year

              // other examples:
              
              #define LED_BUILTIN 2                     // built-in led blinking is used in examples 01, 03 and 04
              pinMode (LED_BUILTIN, OUTPUT);         
              digitalWrite (LED_BUILTIN, LOW);

              if (pdPASS != xTaskCreate ([] (void *) {  // start some of examples in separate thread
                delay (5000);
                Serial.printf ("[%10lu] [example 08] started.\n", millis ());
                example08_time ();                      // example 08
                Serial.printf ("[%10lu] [example 09] started.\n", millis ());
                example09_makeRestCall ();              // example 09
                Serial.printf ("[%10lu] [example 11] started.\n", millis ());
                example11_morseEchoServer ();           // example 11
                Serial.printf ("[%10lu] [examples] finished.\n", millis ());
                vTaskDelete (NULL); // end this thread
              }, "examples", 4069, NULL, tskNORMAL_PRIORITY, NULL)) Serial.printf ("[%10lu] [examples] couldn't start examples\n", millis ());

}

void loop () {

           
              // ----- example: the use of time functions - delete this code if it is not needed -----
              time_t l = getLocalTime ();
              if (l) { // if the time is set                        
                static bool messageAlreadyDispalyed = false;
                if (timeToString (l).substring (11) >= "23:05:00" && !messageAlreadyDispalyed) {
                  messageAlreadyDispalyed = true;
                  Serial.printf ("[%10lu] Working late again?\n", millis ());
                }
              }
            
              // ----- example: do some measurements each minute - delete this code if it is not needed -----
              static unsigned long lastMeasurementTime = -60000; 
              static int lastScale = -1;
              if (millis () - lastMeasurementTime > 60000) {
                lastMeasurementTime = millis ();
                lastScale = (lastScale + 1) % 60;
                freeHeap.addMeasurement (lastScale, ESP.getFreeHeap () / 1024); // take s sample of free heap in KB 
                httpRequestCount.addCounterToMeasurements (lastScale);          // take sample of number of web connections that arrived last minute
                Serial.printf ("[%10lu] [%s] free heap: %6i bytes.\n", millis (), __func__, ESP.getFreeHeap ());
              }
                
}
