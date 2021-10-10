/*
    Control and monitor ESP32 pin values with telnet.
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

  // file system holds all configuration files and also some telnet commands (cp, rm ...) deal with files so this is necessary (don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme |  ... first)
  // make sure you selected one of FAT partition schemas first (Tools | Partition scheme | ...)
  #include "./servers/file_system.h"

  // configure the way ESP32 server is going to deal with users - telnet sever may require loging in first, this module also defines the location of telnet help.txt file
  #define USER_MANAGEMENT NO_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // configure local time zone - this affects the way telnet commands display your local time
  #define TIMEZONE  CET_TIMEZONE               
  // define NTP servers ESP32 server is going to use for time sinchronization - some built-in telnet commands need to be aware of it
  #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
  #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"     

  // include telnetServer.hpp
  #include "./servers/telnetServer.hpp"

// telnetCommandHandler is not mandatory but if we want to handle some commands (beside those already built-in) ourselves we are going to need it,
// all built-in commands will work without it 
#define LED_PIN 2
String telnetCommandHandler (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) { // - must be reentrant!

  // process 'turn led on' and 'turn led off' commands - these commands return immediately with a response for the user
  if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {
    digitalWrite (LED_PIN, HIGH);
    return "LED is on.";  // response for the user, a response different from "" will also tell telnet server not to try to handle the command itself
  } 
  if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "off") {
    digitalWrite (LED_PIN, LOW);
    return "LED is on.";  // response for the user, a response different from "" will also tell telnet server not to try to handle the command itself
  } 

  // process 'monitor led' command - this command runs for a longer time, it should display intermediate resuts and the user should be able to stop execution anytime
  if (argc == 2 && argv [0] == "monitor" && argv [1] == "led") {
      if (!tsp->connection->sendData ("Press Ctrl-C to stop monitoring.\r\n")) // display intermediate result
        return "Connection is closed.";                             // stop execution since telnet connection is already closed
    while (true) {
      String s = "LED is " + String (digitalRead (LED_PIN) ? "on" : "off") + ".\r\n";
      if (!tsp->connection->sendData (s)) return "Connection is closed."; // display intermediate result, stop execution if telnet connection is already closed

      unsigned long startDelay = millis ();
      while (millis () - startDelay <= 1000) { // wait 1 second
        delay (1);
        while (tsp->connection->available () == TcpConnection::AVAILABLE) { // stop execution of the user presses Ctrl-C
          char c;
          if (!tsp->connection->recvData (&c, sizeof (c))) return "Connection is closed.";
          if (c == 3) return "User pressed Ctrl-C."; // Ctrl-C, stop execution
        }
      }    
    }
  } 

  return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnet server to try to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  mountFileSystem (true);                                             // get access to configuration files

  initializeUsers ();                                                 // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users

  startWiFi ();                                                       // connect to to WiFi router  

  pinMode (LED_PIN, OUTPUT);
  // start telnet server
  telnetServer *telnetSrv = new telnetServer (telnetCommandHandler,   // <- pass address of your telnet command handler function to telnet server 
                                              16 * 1024,              // 16 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stable
                                              "0.0.0.0",              // start telnet server on all available ip addresses
                                              23,                     // telnet port
                                              NULL);                  // use firewall callback function for telnet server (replace with NULL if not needed)
  if (!telnetSrv || (telnetSrv && !telnetSrv->started ())) 
    dmesg ("[telnetServer] did not start.");                          // insert information into dmesg queue - it can be displayed with dmesg built-in telnet command
}

void loop () {
                
}
