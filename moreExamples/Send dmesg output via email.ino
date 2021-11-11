/*
    Send dmesg output via email.
    
    Instead of using telnet dmesg command send dmesg report via email.   
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

  // we'll only use dmesg structure from telnetServer.hpp, not the server itself
  #include "./servers/telnetServer.hpp" 

  // include SMTP client
  #include "./servers/smtpClient.h"


void sendDmesg () {
  String s = "";

  // this is almost exact copy of dmesg command code from telnetServer.hpp
  xSemaphoreTake (__dmesgSemaphore__, portMAX_DELAY);
  byte i = __dmesgBeginning__;
  do {
    if (s != "") s += "<br>\r\n";
    char c [15];
    sprintf (c, "[%10lu] ", __dmesgCircularQueue__ [i].milliseconds);
    s += String (c) + __dmesgCircularQueue__ [i].message;
  } while ((i = (i + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) != __dmesgEnd__);
  xSemaphoreGive (__dmesgSemaphore__);
  // end of almost exact copy
  
  sendMail ("<html><p style=\"font-family:'Courier New'\">" + s + "</p></html>", String (HOSTNAME) + " - report", "\"You\"<your.name@somewhere.com>", "\"" + String (HOSTNAME) + "\"<your.name@somewhere.com>", "yourSmtpPassword", "yourSmtpUsername", 25, "smtp.something.net");
}

void setup () {
  Serial.begin (115200);

  startWiFi ();                             // connect to to WiFi router

  while (WiFi.localIP ().toString () ==  "0.0.0.0") delay (10); // wait until ESP32 gets its IP address from router

  dmesg ("[setup ()] so far do good.");     // add your own message
  
  sendDmesg ();                             // send all dmesg messages via email
}

void loop () {

}
