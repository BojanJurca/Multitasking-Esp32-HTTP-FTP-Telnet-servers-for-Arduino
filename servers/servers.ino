// this file is here just to make development of server files easier, it is not part of any project

#include <WiFi.h>

#define HOSTNAME    "MyESP32Server" 
#define MACHINETYPE "TTGO T1"
#define DEFAULT_STA_SSID          "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           HOSTNAME 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"

#include "file_system.h"
#include "network.h"
#define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "user_management.h"
// define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
// ...
// #define TIMEZONE  EASTERN_TIMEZONE
// (default) #define TIMEZONE  CET_TIMEZONE               
#define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
#define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
#include "time_functions.h"     
#include "webServer.hpp"                              // include HTTP Server
#include "ftpServer.hpp"                              // include FTP server
#include "telnetServer.hpp"                           // include Telnet server
#include "smtpClient.h"                               // include SMTP client (sendMail function)


#include "oscilloscope.h"

  String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { return "<html><body>Is this working?</body></html>"; }

  void wsRequestHandler (String& wsRequest, WebSocket *webSocket) { return; }

  String telnetCommandHandler (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) { return ""; }

  bool firewall (String connectingIP) { return true; }

void setup () {  
  Serial.begin (115200);

  // __TEST_DST_TIME_CHANGE__ ();

  // FFat.format ();
  mountFileSystem (true);

  // deleteFile ("/etc/ntp.conf");
  // deleteFile ("/etc/crontab");
  startCronDaemon (NULL, 8 * 1024); 

  // deleteFile ("/etc/passwd");
  // deleteFile ("/etc/shadow");
  initializeUsers ();

  // deleteFile ("/network/interfaces");
  // deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf");
  // deleteFile ("/etc/dhcpcd.conf");
  // deleteFile ("/etc/hostapd/hostapd.conf");
  startWiFi ();

  // start telnet server
  telnetServer *telnetSrv = newTelnetServer (telnetCommandHandler, 16 * 1024, "0.0.0.0", 23, firewall);
  if (!telnetSrv) Serial.println ("[telnetServer] did not start.");
  
  // start web server
  httpServer *httpSrv = newHttpServer (httpRequestHandler, wsRequestHandler, 8 * 1024, "0.0.0.0", 80, NULL); if (!httpSrv) dmesg ("[httpServer] did not start.");

  // start FTP server
  ftpServer *ftpSrv = newFtpServer ("0.0.0.0", 21, firewall); if (!ftpSrv) dmesg ("[ftpServer] did not start.");

}

void loop () {  

}
