/*
    A minimal HTTP server
*/

#include <WiFi.h>

  // include all .h and .hpp files for full functionality of httpServer
  
  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command
  // #include "./servers/perfMon.h"           // contains performence countrs that can be displayed by using "perfmon" telnet command
  // #include "./servers/file_system.h"       // httpServer can server .html and other files from /var/www/html directory

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

  // define where time functions will get current time from
    // #define TIMEZONE                  CET_TIMEZONE       // see time_functions.h for supported time zonnes
    #define DEFAULT_NTP_SERVER_1      "1.si.pool.ntp.org"
    #define DEFAULT_NTP_SERVER_2      "2.si.pool.ntp.org"
    #define DEFAULT_NTP_SERVER_3      "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"       // httpServer need time functions to correctly calculate expiration time for cookies

  // finally include httpServer.hpp
  #include "./servers/httpServer.hpp"


  String httpRequestHandler (char *httpRequest, httpConnection *hcn) { // note that each HTTP connection runs in its own thread/task, more of them may run simultaneously so this function should be reentrant
    // input: char *httpRequest: the whole HTTP request but without ending \r\n\r\n
    // input example: GET / HTTP/1.1
    //                Host: 192.168.0.168
    //                User-Agent: curl/7.55.1
    //                Accept: */*
    // output: String: content part of HTTP reply, the header will be added by HTTP server before sending it to the client (browser)
    // output example: <html>
    //                   Congratulations, this is working.
    //                 </html>

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

    if (httpRequestStartsWith ("GET / ")) // return a static (for example HTML) content from RAM - static content may be several KB long 
                                          return F ("<html>"
                                                    "  Congratulations, this is working."
                                                    "</html>");

    if (httpRequestStartsWith ("GET /upTime ")) // return calculated (for example JSON) content - we are more limited with calculated content regarding its size
                                                return "{\"upTime\":\"" + String (millis () / 1000) + " seconds\"}";

    // let the HTTP server try to find a file in /var/www/html and send it as a reply
    return "";
  }


  httpServer *myHttpServer;
 
void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                              // to enable httpServer to server .html and other files from /var/www/html directory
  startWiFi ();
  // startCronDaemon (NULL);                              // sichronize ESP32 time with NTP servers if you want to use cookies with expiration time

  myHttpServer = new httpServer (
                                  httpRequestHandler,               // or NULL if httpRequestHandler is not going to be used
                                  NULL,                             // or wsRequestHandler to handle WebSockets
                                  (char *) "0.0.0.0",               // httpServer IP or 0.0.0.0 for all available IPs
                                  80,                               // default HTTP port
                                  NULL                              // or firewallCallback if you want to use firewall
                                );
  if (myHttpServer->state () != httpServer::RUNNING) Serial.printf ("[%10lu] Could not start httpServer\n", millis ());
}

void loop () {

}
