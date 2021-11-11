/*
    Calculate and then execute daily plan - calculate Sun rise and Sun set at the beginning of each day and do something about it.
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

  // webClient function is defined in webServer.hpp
  #include "./servers/webServer.hpp"     


void setup () {
  Serial.begin (115200);

  startWiFi ();                                   // connect to to WiFi router  
}

void loop () {
  delay (1);
  
  static bool alreadyRetrieved = false;

  if (!alreadyRetrieved) { // we'll request web page only once
    if (WiFi.status() != WL_CONNECTED) { // if ESP32 is already connected to WiFi ...
      if (WiFi.localIP ().toString () != "0.0.0.0") { // ... and has already got its IP address
          String s = webClient ("www.google.com", 80, (TIME_OUT_TYPE) 5000, "GET /");
          if (s > "") {
            alreadyRetrieved = true;
            Serial.println (s);
            Serial.println ("\nReceived: " + String (strlen (s.c_str ())) + " bytes");
          } else {
            delay (10000);                
          }
      }
    }
  }
}
