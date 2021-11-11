/*
    Handle web cookies
*/

#include <WiFi.h>

  #define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
  #define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

  // tell ESP32 how to connect to your WiFi router
  #define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h) 
  #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
  // tell ESP32 not to set up AP, we do not need it in this example
  #define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
  #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
  #include "./servers/network.h"

  // file system holds all configuration and .html files 
  // make sure you selected one of FAT partition schemas first (Tools | Partition scheme | ...)
  #include "./servers/file_system.h"

  // correct timing is needed to set cookie expiration time - define your time zone here
  #define TIMEZONE  CET_TIMEZONE               
  // tell time_functions.h where to read current time from
  #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will syncronize its time with
  #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"     
  
  // configure the way ESP32 server is going to deal with users, this module also defines where web server will search for .html files
  #define USER_MANAGEMENT NO_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // include webServer.hpp
  #include "./servers/webServer.hpp"


// we are going to need httpRequestHandler to handle cookies
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  if (httpRequest.substring (0, 6) == "GET / ") {
    String refreshCounter = wsp->getHttpRequestCookie ("refreshCounter");           // get cookie that browser sent in HTTP request
    Serial.println ("refreshCounter cookie from HTTP request: " + refreshCounter);
    if (refreshCounter == "") refreshCounter = "0";
    refreshCounter = String (refreshCounter.toInt () + 1);
    wsp->setHttpResponseCookie ("refreshCounter", refreshCounter, getGmt () + 60);  // set 1 minute valid cookie that will be send to browser in HTTP reply
    Serial.println ("refreshCounter cookie in HTTP reply: " + refreshCounter);    
    return String ("<HTML>Web cookies<br><br>This page has been refreshed " + refreshCounter + " times. Click refresh to see more.</HTML>");
  }

  return "";  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  mountFileSystem (true);                                             // get access to configuration files

  startCronDaemon (NULL, 4 * 1024);                                   // start cronDaemon which will sinchronize ESP32 internal clock with NTP server(s)

  initializeUsers ();                                                 // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users

  startWiFi ();                                                       // connect to to WiFi router    

  // start web server
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // <- pass address of your HTTP request handler function to web server 
                                        NULL,                         // no wsRequestHandler
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // we are not going to use firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start."); // insert information into dmesg queue - it can be displayed with dmesg built-in telnet command
}

void loop () {
                
}
