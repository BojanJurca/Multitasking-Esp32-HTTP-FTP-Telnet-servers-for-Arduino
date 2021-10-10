/*
    Control ESP32 pin values with web browser.
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

  // configure the way ESP32 server is going to deal with users, this module also defines where web server will search for .html files  
  #define USER_MANAGEMENT NO_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // include webServer.hpp
  #include "./servers/webServer.hpp"

// httpRequestHandler is not mandatory but if we want to handle some HTTP requests ourselves we are going to need it
#define LED_PIN 2
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  if (httpRequest.substring (0, 6) == "GET / ") {

    return  "<html>\n"
            "  Combining (large) .html files with (short) programmable responses<br><br>\n"
            "  <hr />\n"
            "  Led: <input type='checkbox' disabled id='ledSwitch' onClick='turnLed (this.checked)'>\n"
            "  <hr />\n"
            "  <script type='text/javascript'>\n"
            "\n"
            "    // mechanism that makes HTTP requests\n"
            "    var httpClient = function () {\n" 
            "      this.request = function (url, method, callback) {\n"
            "        var httpRequest = new XMLHttpRequest ();\n"
            "        httpRequest.onreadystatechange = function () {\n"
            "          if (httpRequest.readyState == 4 && httpRequest.status == 200) callback (httpRequest.responseText);\n"
            "        }\n"
            "        httpRequest.open (method, url, true);\n"
            "        httpRequest.send (null);\n"
            "      }\n"
            "    }\n"
            "\n"
            "    // make a HTTP requests and initialize/populate this page\n"
            "    var client = new httpClient ();\n"
            "    client.request ('/builtInLed', 'GET', function (json) {\n"
            "                                                            // json reply will look like: {\"id\":\"MyESP32Server\",\"builtInLed\":\"on\"}\n"
            "                                                            var obj=document.getElementById ('ledSwitch');\n"
            "                                                            obj.disabled = false;\n"
            "                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');\n"
            "                                                          });\n"
            "\n"
            "  function turnLed (switchIsOn) { // sends desired led state to ESP32 server and refreshes ledSwitch state\n"
            "    client.request (switchIsOn ? '/builtInLed/on' : '/builtInLed/off' , 'PUT', function (json) {\n"
            "                                                            var obj = document.getElementById ('ledSwitch');\n"
            "                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');\n"
            "                                                          });\n"
            "  }\n"
            "\n"
            "  </script>\n"
            "</html>\n";

  } else if (httpRequest.substring (0, 16) == "GET /builtInLed ") {
getBuiltInLed:
      return "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + (digitalRead (LED_PIN) == HIGH ? String ("on") : String ("off")) + "\"}";
  } else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ") {
      digitalWrite (LED_PIN, HIGH);
      goto getBuiltInLed;
  } else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ") {
      digitalWrite (LED_PIN, LOW);
      goto getBuiltInLed;
  } 

  return "";  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  mountFileSystem (true);                                             // get access to configuration files

  initializeUsers ();                                                 // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users

  startWiFi ();                                                       // connect to to WiFi router  

  pinMode (LED_PIN, OUTPUT);
  // start web server
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // <- pass address of your HTTP request handler function to web server 
                                        NULL,                         // no wsRequestHandler 
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // no firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start."); // insert information into dmesg queue - it can be displayed with dmesg built-in telnet command
}

void loop () {
                
}
