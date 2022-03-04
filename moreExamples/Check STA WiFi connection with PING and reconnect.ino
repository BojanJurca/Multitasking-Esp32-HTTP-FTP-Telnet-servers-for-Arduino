/*
    Check each 15 minutes if ESP32 is still connected to the WiFi router by pinging it, try reconnecting if it is not
*/

#include <WiFi.h>

  // define how network.h will connect to the router
    #define HOSTNAME                  "MyESP32Server"       // define the name of your ESP32 here
    #define MACHINETYPE               "ESP32 NodeMCU"       // describe your hardware here
    // tell ESP32 how to connect to your WiFi router if this is needed
    #define DEFAULT_STA_SSID          "YOUR_STA_SSID"       // define default WiFi settings  
    #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
    // tell ESP32 not to set up AP if it is not needed
    #define DEFAULT_AP_SSID           ""  // HOSTNAME       // set it to "" if you don't want ESP32 to act as AP 
    #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"    // must have at leas 8 characters
  #include "./servers/network.h"
  
void setup () {
  Serial.begin (115200);

  startWiFi ();

  #define tskNORMAL_PRIORITY 1
  if (pdPASS != xTaskCreate ([] (void *) {  // start checking in a separate task
    while (true) {
      delay (15 * 60 * 1000); // 15 minutes

      Serial.printf ("[%10lu] [checkRouter] checking ...\n", millis ());
      // ping router
      if (WiFi.localIP ().toString () >  "0.0.0.0") {
        if (ping ((char *) WiFi.gatewayIP ().toString ().c_str (), 1) != 1) {
          Serial.printf ("[%10lu] [checkRouter] lost WiFi connection.\n", millis ());
          startWiFi (); // try reconnecting                                       
        }
      }
            
    } // while
  }, "checkRouter", 8 * 1024, NULL, tskNORMAL_PRIORITY, NULL)) Serial.printf ("[%10lu] [checkRouter] couldn't start new task.\n", millis ());

}

void loop () {

}