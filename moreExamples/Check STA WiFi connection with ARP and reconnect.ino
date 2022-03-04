/*
    Check each hour if ESP32 is still connected to the WiFi router, try reconnecting if it is not.
    
    You can check the WiFi connection more often but it wouldn't make sense to go under 20 minutes since ARP records
    stay in the ARP table that long before they are being deleted. If a shorter period is required you may consider
    rather pinging the WiFi router.   
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
      delay (1 * 60 * 60 * 1000); // 1 hour
      Serial.printf ("[%10lu] [checkRouter] checking ...\n", millis ());
      // send ARP request to all working netifs
      if (WiFi.localIP ().toString () >  "0.0.0.0") {
        struct netif *netif;
        ip_addr_t ipAddrTo; inet_aton (WiFi.gatewayIP ().toString ().c_str (), &ipAddrTo); 

        for (netif = netif_list; netif; netif = netif->next) {
          if (netif_is_up (netif)) {
            if (netif->hwaddr_len) { 
              // make ARP request
              etharp_request (netif, (const ip4_addr_t *) &ipAddrTo);  
              // give ARP reply some time to arrive
              delay (1000); 
            }
          }
        }
      }

      // check if the WiFi router's IP exists in ARP table
      if (arp ().indexOf (WiFi.gatewayIP ().toString ()) < 0) {
        Serial.printf ("[%10lu] [checkRouter] lost WiFi connection.\n", millis ());
        startWiFi (); // try reconnecting                                       
      }
    } // while
  }, "checkRouter", 8 * 1024, NULL, tskNORMAL_PRIORITY, NULL)) Serial.printf ("[%10lu] [checkRouter] couldn't start new task.\n", millis ());

}

void loop () {

}