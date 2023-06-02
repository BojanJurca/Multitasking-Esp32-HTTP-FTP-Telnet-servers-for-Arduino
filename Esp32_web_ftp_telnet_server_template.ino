/*
 
    Esp32_web_ftp_telnet_server_template.ino

        Compile this code with Arduino for:
            - one of ESP32 boards (Tools | Board) and 
            - one of FAT partition schemes (Tools | Partition scheme) and


    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
     
    May 1, 2023, Bojan Jurca
           
*/

#include <WiFi.h>
// --- Please modify this file first! --- This is where you can configure your network credentials, which servers will be included, etc ...
#include "Esp32_servers_config.h"


              // ----- measurements are just for demonstration purpose - delete this code if it is not needed -----
              #include "measurements.hpp"
              measurements freeHeap (60);                 // measure free heap each minute for possible memory leaks
              measurements httpRequestCount (60);         // measure how many web connections arrive each minute

              #undef LED_BUILTIN
              #define LED_BUILTIN 2                       // built-in led blinking is used in examples 01, 03 and 04

 
// ----- HTTP request handler example - if you don't want to handle HTTP requests just delete this function and pass NULL to httpSrv instead of its address -----
//       normally httpRequest is HTTP request, function returns a reply in HTML, json, ... formats or "" if request is unhandeled by httpRequestHandler
//       httpRequestHandler is supposed to be used with smaller replies,
//       if you want to reply with larger pages you may consider FTP-ing .html files onto the file system (into /var/www/html/ directory)

String httpRequestHandler (char *httpRequest, httpConnection *hcn) { 
  // httpServer will add HTTP header to the String that this callback function returns and send everithing to the Web browser (this callback function is suppose to return only the content part of HTTP reply)

  #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

              // ----- examples - delete all or parts of this code if it is not needed -----
              httpRequestCount.increaseCounter ();                            // gether some statistics

              // variables used by example 05
              char niceSwitch1 [6] = "false";
              static int niceSlider3 = 3;
              char niceRadio5 [3] = "fm";

              // ----- handle HTTP protocol requests -----
                   if (httpRequestStartsWith ("GET /example01.html "))      { // used by example 01: Dynamically generated HTML page
                                                                              return "<HTML>Example 01 - dynamic HTML page<br><br><hr />" + String (digitalRead (LED_BUILTIN) ? "Led is on." : "Led is off.") + "<hr /></HTML>";
                                                                            }
              else if (httpRequestStartsWith ("GET /builtInLed "))          { // used by example 02, example 03, example 04, index.html: REST function for static HTML page
                                                                            getBuiltInLed:
                                                                              return "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + String (digitalRead (LED_BUILTIN) ? "on" : "off") + "\"}\r\n";
                                                                            }                                                                    
              else if (httpRequestStartsWith ("PUT /builtInLed/on "))       { // used by example 03, example 04: REST function for static HTML page
                                                                              digitalWrite (LED_BUILTIN, HIGH);
                                                                              goto getBuiltInLed;
                                                                            }
              else if (httpRequestStartsWith ("PUT /builtInLed/off "))      { // used by example 03, example 04, index.html: REST function for static HTML page
                                                                              digitalWrite (LED_BUILTIN, LOW);
                                                                              goto getBuiltInLed;
                                                                            }
              else if (httpRequestStartsWith ("GET /upTime "))              { // used by index.html: REST function for static HTML page
                                                                              char httpResponseContentBuffer [100];
                                                                              time_t t = getUptime ();       // t holds seconds
                                                                              int seconds = t % 60; t /= 60; // t now holds minutes
                                                                              int minutes = t % 60; t /= 60; // t now holds hours
                                                                              int hours = t % 24;   t /= 24; // t now holds days
                                                                              char c [25]; *c = 0;
                                                                              if (t) sprintf (c, "%lu days, ", t);
                                                                              sprintf (c + strlen (c), "%02i:%02i:%02i", hours, minutes, seconds);
                                                                              sprintf (httpResponseContentBuffer, "{\"id\":\"%s\",\"upTime\":\"%s\"}", HOSTNAME, c);
                                                                              return httpResponseContentBuffer;
                                                                            }                                                                    
              else if (httpRequestStartsWith ("GET /freeHeap "))            { // used by index.html: REST function for static HTML page
                                                                              return freeHeap.toJson (5);
                                                                            }
              else if (httpRequestStartsWith ("GET /httpRequestCount "))    { // used by index.html: REST function for static HTML page
                                                                              return httpRequestCount.toJson (5);
                                                                            }
              else if (httpRequestStartsWith ("GET /niceSwitch1 "))         { // used by example 05.html: REST function for static HTML page
                                                                            returnNiceSwitch1State:
                                                                              return "{\"id\":\"niceSwitch1\",\"value\":\"" + String (niceSwitch1) + "\"}";
                                                                            }
              else if (httpRequestStartsWith ("PUT /niceSwitch1/"))         { // used by example 05.html: REST function for static HTML page
                                                                              *(httpRequest + 21) = 0;
                                                                              strcpy (niceSwitch1, strstr (httpRequest + 17, "true") ? "true": "false");
                                                                              goto returnNiceSwitch1State; // return success (or possible failure) back to the client
                                                                            }
              else if (httpRequestStartsWith ("PUT /niceButton2/pressed ")) { // used by example 05.html: REST function for static HTML page
                                                                              return "{\"id\":\"niceButton2\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                            }
              else if (httpRequestStartsWith ("GET /niceSlider3 "))         { // used by example 05.html: REST function for static HTML page
                                                                            returnNiceSlider3Value:
                                                                              return "{\"id\":\"niceSlider3\",\"value\":\"" + String (niceSlider3) + "\"}";
                                                                            }
              else if (httpRequestStartsWith ("PUT /niceSlider3/"))         { // used by example 05.html: REST function for static HTML page
                                                                              niceSlider3 = atoi (httpRequest + 17);
                                                                              Serial.printf ("[Got request from web browser for niceSlider3]: %i\n", niceSlider3);
                                                                              goto returnNiceSlider3Value; // return success (or possible failure) back to the client
                                                                            }
              else if (httpRequestStartsWith ("PUT /niceButton4/pressed ")) { // used by example 05.html: REST function for static HTML page
                                                                              Serial.printf ("[Got request from web browser for niceButton4]: pressed\n");
                                                                              return "{\"id\":\"niceButton4\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                            }
              else if (httpRequestStartsWith ("GET /niceRadio5 "))          { // used by example 05.html: REST function for static HTML page
                                                                            returnNiceRadio5Value:
                                                                              return "{\"id\":\"niceRadio5\",\"modulation\":\"" + String (niceRadio5) + "\"}";
                                                                            }
              else if (httpRequestStartsWith ("PUT /niceRadio5/"))          { // used by example 05.html: REST function for static HTML page
                                                                              httpRequest [18] = 0;
                                                                              Serial.println (httpRequest + 16);
                                                                              strcpy (niceRadio5, strstr (httpRequest + 16, "am") ? "am" : "fm");
                                                                              goto returnNiceRadio5Value; // return success (or possible failure) back to the client
                                                                            }
              else if (httpRequestStartsWith ("PUT /niceButton6/pressed ")) { // used by example 05.html: REST function for static HTML page
                                                                              Serial.printf ("[Got request from web browser for niceButton6]: pressed\n");
                                                                              return "{\"id\":\"niceButton6\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                            }
              
  return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}

// ----- WebSocket request handler example - if you don't want to handle WebSocket requests just delete this function and pass NULL to httpSrv instead of its address -----

              // ----- oscilloscope - delete this code if it is not needed -----
              #include "./servers/oscilloscope.h"

void wsRequestHandler (char *wsRequest, WebSocket *webSocket) {
  // this callback function is supposed to handle WebSocket communication - once it returns WebSocket will be closed
  
  #define wsRequestStartsWith(X) (strstr(wsRequest,X)==wsRequest)
    
              // ----- example WebSockets & Oscilloscope - delete this code if it is not needed -----
                   if (wsRequestStartsWith ("GET /runOscilloscope"))      runOscilloscope (webSocket);      // used by oscilloscope.html
              else if (wsRequestStartsWith ("GET /rssiReader"))           {                                 // data streaming used by index.html
                                                                            unsigned long startMillis = millis ();
                                                                            char c;
                                                                            do {
                                                                              if (millis () - startMillis >= 300000) {
                                                                                webSocket->sendString ("WebScket is automatically closed after 5 minutes for demonstration purpose.");
                                                                                // return;
                                                                              }
                                                                              delay (100);
                                                                              int i = WiFi.RSSI ();
                                                                              c = (char) i;
                                                                              // Serial.printf ("[WebSocket data streaming] sending %i to web client\n", i);
                                                                            } while (webSocket->sendBinary ((byte *) &c, sizeof (c))); // send RSSI information as long as web browser is willing tzo receive it
                                                                          }

}


// ----- telnet command handler example - if you don't want to handle telnet commands yourself just delete this function and pass NULL to telnetSrv instead of its address -----

String telnetCommandHandlerCallback (int argc, char *argv [], telnetConnection *tcn) {
  // the String that this callback function returns will be written to Telnet client console as a response to Telnet command (already parsed to argv)

  #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
  #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
  #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))    
              
              // ----- example 06 - delete this code if it is not needed -----
                      if (argv0is ("led") && argv1is ("state"))                 { // led state telnet command
                                                                                  return "Led is " + String (digitalRead (LED_BUILTIN) ? "on." : "off.");
                                                                                } 
              else if (argv0is ("turn") && argv1is ("led") && argv2is ("on"))   { // turn led on telnet command
                                                                                  digitalWrite (LED_BUILTIN, HIGH);
                                                                                  return "Led is on.";
                                                                                }
              else if (argv0is ("turn") && argv1is ("led") && argv2is ("off"))  { // turn led off telnet command
                                                                                  digitalWrite (LED_BUILTIN, LOW);
                                                                                  return "Led is off.";
                                                                                }

  return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning "" reply
}


              // ----- firewall example - if you don't need firewall just delete this function and pass NULL to the servers instead of its address -----          
              bool firewallCallback (char *connectingIP) {            // firewall callback function, return true if IP is accepted or false if not - must be reentrant!
                if (!strcmp (connectingIP, "10.0.0.2")) return false; // block 10.0.0.2 (for the purpose of this example) 
                return true;                                          // ... but let every other client through
              }


// ----- cron command handler example - if you don't want to handle cron tasks just delete this function and pass NULL to startCronDaemon... instead of its address -----

void cronHandler (char *cronCommand) {
  // this callback function is supposed to handle cron commands/events that occur at time specified in crontab

    #define cronCommandIs(X) (!strcmp (cronCommand, X))  

          // ----- examples: handle your cron commands/events here - delete this code if it is not needed -----
          
          if (cronCommandIs ("gotTime"))                        { // triggers only once - the first time ESP32 sets its clock (when it gets time from NTP server for example)
                                                                  time_t t = getLocalTime ();
                                                                  struct tm st = timeToStructTime (t);
                                                                  Serial.println ("Got time at " + timeToString (t) + " (local time), do whatever needs to be done the first time the time is known.");
                                                                }           
          if (cronCommandIs ("newYear'sGreetingsToProgrammer")) { // triggers at the beginning of each year
                                                                  Serial.printf ("[%10lu] [cronDaemon] *** HAPPY NEW YEAR ***!\n", millis ());    
                                                                }
          if (cronCommandIs ("onMinute"))                       { // triggers each minute
                                                                  struct tm st = timeToStructTime (getLocalTime ()); 
                                                                  freeHeap.addMeasurement (st.tm_min, ESP.getFreeHeap () / 1024); // take s sample of free heap in KB 
                                                                  httpRequestCount.addCounterToMeasurements (st.tm_min);          // take sample of number of web connections that arrived last minute 
                                                                }
          // handle also other cron commands/events you may have

}


void setup () {
    
  Serial.begin (115200);

  Serial.println (string (MACHINETYPE " (") + string ((int) ESP.getCpuFreqMHz ()) + " MHz) " HOSTNAME " SDK: " + ESP.getSdkVersion () + " " VERSION_OF_SERVERS " compiled at: " __DATE__ " " __TIME__); 

  // fileSystem.formatFAT (); Serial.printf ("\nFormatting file system ...\n\n"); // format flash disk to start everithing from the scratch
  #ifdef __FILE_SYSTEM__
    fileSystem.mountLittleFs (true);                                        // this is the first thing to do - all configuration files reside on the file system
    // fileSystem.mountFAT (true);
  #endif

  // fileSystem.deleteFile ("/etc/ntp.conf");                               // contains ntp server names for time sync - deleting this file would cause creating default one
  // fileSystem.deleteFile ("/etc/crontab");                                // contains cheduled tasks                 - deleting this file would cause creating empty one
  startCronDaemon (cronHandler);                                            // creates /etc/ntp.conf with default NTP server names and syncronize ESP32 time with them once a day
                                                                            // creates empty /etc/crontab, reads it at startup and executes cronHandler when the time is right
                                                                            // 3 KB stack size is minimal requirement for NTP time synchronization, add more if your cronHandler requires more

  // fileSystem.deleteFile ("/etc/passwd");                                 // contains users' accounts information    - deleting this file would cause creating default one
  // fileSystem.deleteFile ("/etc/shadow");                                 // contains users' passwords               - deleting this file would cause creating default one
  userManagement.initialize ();                                             // creates user management files with root and webadmin users, if they don't exist yet

  // fileSystem.deleteFile ("/network/interfaces");                         // contation STA(tion) configuration       - deleting this file would cause creating default one
  // fileSystem.deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf");     // contation STA(tion) credentials         - deleting this file would cause creating default one
  // fileSystem.deleteFile ("/etc/dhcpcd.conf");                            // contains A(ccess) P(oint) configuration - deleting this file would cause creating default one
  // fileSystem.deleteFile ("/etc/hostapd/hostapd.conf");                   // contains A(ccess) P(oint) credentials   - deleting this file would cause creating default one
  startWiFi ();                                                             // starts WiFi according to configuration files, creates configuration files if they don't exist

  #ifdef __HTTP_SERVER__
    // start web server 
    httpServer *httpSrv = new (std::nothrow) httpServer (httpRequestHandler,  // a callback function that will handle HTTP requests that are not handled by webServer itself
                                             wsRequestHandler,                // a callback function that will handle WS requests, NULL to ignore WS requests
                                             (char *) "0.0.0.0",              // start HTTP server on all available IP addresses
                                             80,                              // default HTTP port
                                             NULL);                           // we won't use firewallCallback function for HTTP server
    if (!httpSrv && httpSrv->state () != httpServer::RUNNING) dmesg ("[httpServer] did not start.");
  #endif
  
  #ifdef __FTP_SERVER__
    // start FTP server
    ftpServer *ftpSrv = new (std::nothrow) ftpServer ((char *) "0.0.0.0",     // start FTP server on all available ip addresses
                                           21,                                // default FTP port
                                           firewallCallback);                 // let's use firewallCallback function for FTP server
    if (ftpSrv && ftpSrv->state () != ftpServer::RUNNING) dmesg ("[ftpServer] did not start.");
  #endif

  #ifdef __TELNET_SERVER__
    // start telnet server
    telnetServer *telnetSrv = new (std::nothrow) telnetServer (telnetCommandHandlerCallback, // a callback function that will handle Telnet commands that are not handled by telnetServer itself
                                                               (char *) "0.0.0.0",           // start Telnet server on all available ip addresses 
                                                               23,                           // default Telnet port
                                                               firewallCallback);            // let's use firewallCallback function for Telnet server
    if (telnetSrv && telnetSrv->state () != telnetServer::RUNNING) dmesg ("[telnetServer] did not start.");
  #endif

              // ----- some examples - delete this code if it is not needed -----

              // crontab examples: you can add entries in crontab from code or you can write them into /etc/crontab file,
              // fromat is in both cases the same: second minute hour day month day_of_week command
              cronTabAdd ((char *) "* * * * * * gotTime");  // triggers only once - when ESP32 reads time from NTP servers for the first time
              cronTabAdd ((char *) "0 0 0 1 1 * newYear'sGreetingsToProgrammer");  // triggers at the beginning of each year
              cronTabAdd ((char *) "0 * * * * * onMinute");  // triggers each minute at 0 seconds

              // other examples:
              pinMode (LED_BUILTIN, OUTPUT);         
              digitalWrite (LED_BUILTIN, LOW);
}

void loop () {

}
