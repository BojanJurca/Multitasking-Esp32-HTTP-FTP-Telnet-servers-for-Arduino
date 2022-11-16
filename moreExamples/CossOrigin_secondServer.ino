/*
    Example of HTTP replies that can be called by HTML page originating in another server
*/

#include <WiFi.h>

  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command

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

  // we are going to rely on alredy built-in usermanagement
    #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT // NO_USER_MANAGEMENT 
  #include "./servers/user_management.h"

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

    #define httpRequestStartsWith(X) (strstr(httpRequest,X))

    // DEBUG:
    Serial.printf (httpRequest);

    if (httpRequestStartsWith ("GET /ledState ")) {
      
      // return JSON that can be called even from HTML pages originating in other servers
      hcn->setHttpReplyHeaderField ("Access-Control-Allow-Origin", "*"); // this is all you have to do to allow cross-origin calls of GET method
      return "{\"led\":\"" + String (digitalRead (2)) + "\"}";  // built-in led
      
    } else if (httpRequestStartsWith ("OPTIONS /ledState/on ")) {

      // browser will call OPTIONS method prior to PUT method to check if it is OK by the server first ...
      hcn->setHttpReplyHeaderField ("Access-Control-Allow-Origin", "*");  
      hcn->setHttpReplyHeaderField ("Access-Control-Allow-Methods", "*"); 
      return "ok"; // whatever
      
    } else if (httpRequestStartsWith ("PUT /ledState/on ")) {
      
      // ... than a real PUT request will follow
      hcn->setHttpReplyHeaderField ("Access-Control-Allow-Origin", "*");  // ... for methods other than GET you also ...
      hcn->setHttpReplyHeaderField ("Access-Control-Allow-Methods", "*"); // have to specify Access-Control-Allow-Methods HTTP reply header
      digitalWrite (2, HIGH); // built-in led
      return "{\"led\":\"" + String (digitalRead (2)) + "\"}";  // built-in led
      
    }

    // let the HTTP server try to find a file in /var/www/html and send it as a reply
    return "";
  }


  httpServer *myHttpServer;
 
void setup () {
  Serial.begin (115200);

  pinMode (2, OUTPUT); // built-in led

  // mountFileSystem (true);                              // to enable httpServer to server .html and other files from /var/www/html directory and also to enable UNIX_LIKE_USER_MANAGEMENT
  // initializeUsers ();                                  // creates defult /etc/passwd and /etc/shadow but only if they don't exist yet - only for UNIX_LIKE_USER_MANAGEMENT
  startWiFi ();
  // startCronDaemon (NULL);                              // sichronize ESP32 clock with NTP servers if you want to use (login) cookies with expiration time

  myHttpServer = new httpServer (
                                  httpRequestHandler,               // or NULL if httpRequestHandler is not going to be used
                                  NULL,                             // or wsRequestHandler iw webSockets are to be used
                                  (char *) "0.0.0.0",               // httpServer IP or 0.0.0.0 for all available IPs
                                  80,                               // default HTTP port
                                  NULL                              // or firewallCallback if you want to use firewall
                                );
  if (myHttpServer->state () != httpServer::RUNNING) Serial.printf ("[%10lu] Could not start httpServer\n", millis ());
}

void loop () {

}
