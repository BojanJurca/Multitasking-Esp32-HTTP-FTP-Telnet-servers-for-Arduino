/*
    A minimal FTP server
*/

#include <WiFi.h>

  // include all .h and .hpp files for full functionality of ftpServer
  
  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command
  // #include "./servers/perfMon.h"           // contains performence countrs that can be displayed by using "perfmon" telnet command
  #include "./servers/file_system.h"          // ftpServer needs a file system 

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

  // define how local time will be calculated and where time functions will get current time from
    #define TIMEZONE                  CET_TIMEZONE          // see time_functions.h for supported time zonnes
    #define DEFAULT_NTP_SERVER_1      "1.si.pool.ntp.org"
    #define DEFAULT_NTP_SERVER_2      "2.si.pool.ntp.org"
    #define DEFAULT_NTP_SERVER_3      "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"       // ftpServer need time functions to correctly display file access time

  // ftpServer is using user_management.h to handle login/logout
    #define USER_MANAGEMENT NO_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // finally include ftpServer.hpp
  #include "./servers/ftpServer.hpp"

  /*
  bool firewallCallback (char *connectingIP) { return true; } // decide based on connectingIP to accept or not to accept the connection
  */

  ftpServer *myFtpServer; 

void setup () {
  Serial.begin (115200);

  mountFileSystem (true);
  // initializeUsers ();                                  // creates defult /etc/passwd and /etc/shadow but only if they don't exist yet
  startWiFi ();                                           // if the file system is mounted prior to startWiFi, startWiFi will create inifial network configuration files
  startCronDaemon (NULL);                                 // sichronize ESP32 clock with NTP servers

  myFtpServer = new ftpServer (
                                (char *) "0.0.0.0",       // ftpServer IP or 0.0.0.0 for all available IPs
                                21,                       // default FTP port
                                NULL                      // or firewallCallback if you want to use firewall
                              );
  if (myFtpServer->state () != ftpServer::RUNNING) Serial.printf ("[%10lu] Could not start ftpServer\n", millis ());
}

void loop () {

}
