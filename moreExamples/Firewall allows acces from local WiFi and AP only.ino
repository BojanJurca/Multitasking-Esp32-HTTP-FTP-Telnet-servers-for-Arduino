/*
    Allow access only from local WiFi or AP (for example, not from WAN).
*/

#include <WiFi.h>

  #define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
  #define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

  // tell ESP32 how to connect to your WiFi router
  #define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h) 
  #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
  // tell ESP32 to set up its own Access Point
  #define DEFAULT_AP_SSID           HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
  #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
  #include "./servers/network.h"

  // file system holds all configuration and .html files 
  // make sure you selected one of FAT partition schemas first (Tools | Partition scheme | ...)
  #include "./servers/file_system.h"

  // configure the way ESP32 server is going to deal with users, this module also defines where web server will search for .html files
  #define USER_MANAGEMENT NO_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // include webServer.hpp
  #include "./servers/webServer.hpp"

// httpRequestHandler is not mandatory but if we want to handle some HTTP requests ourselves we are going to need it
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  if (httpRequest.substring (0, 6) == "GET / ") {
    return  "<html>Hello " + wsp->connection->getOtherSideIP () + "</html>";
  } 

  return "";  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


bool firewallCallback (String browserIP) {
  int dot;
  dot = browserIP.indexOf ('.');          if (dot == -1) return false; // something is wrong
  dot = browserIP.indexOf ('.', dot + 1); if (dot == -1) return false; // something is wrong
  dot = browserIP.indexOf ('.', dot + 1); if (dot == -1) return false; // something is wrong
  
  if (WiFi.localIP ().toString ().startsWith (browserIP.substring (0, dot + 1))) return true; // a call from station connected to local WiFi (class C IP)
  if (WiFi.softAPIP ().toString ().startsWith (browserIP.substring (0, dot + 1))) return true; // a call from station connected to AP (class C IP)

  return false; // none of above
}


void setup () {
  Serial.begin (115200);
 
  mountFileSystem (true);                                             // get access to configuration files

  initializeUsers ();                                                 // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users

  startWiFi ();                                                       // connect to to WiFi router  

  // start web server
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // <- pass address of your HTTP request handler function to web server 
                                        NULL,                         // no wsRequestHandler 
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        firewallCallback);            // <- pass address of your firewall callback function to web server 
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start."); // insert information into dmesg queue - it can be displayed with dmesg built-in telnet command
}

void loop () {
                
}
