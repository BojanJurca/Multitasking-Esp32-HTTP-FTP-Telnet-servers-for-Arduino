/*
    Allow access only from local WiFi or AP (for example and notfrom WAN)
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

    if (httpRequestStartsWith ("GET / ")) return F ("<html>"
                                                    "  Congratulations, this is working."
                                                    "</html>");

    // let the HTTP server try to find a file in /var/www/html and send it as a reply
    return "";
  }


  bool firewallCallback (char *connectingIP) {
    String browserIP (connectingIP);
    int dot;
    dot = browserIP.indexOf ('.');          if (dot == -1) return false; // something is wrong
    dot = browserIP.indexOf ('.', dot + 1); if (dot == -1) return false; // something is wrong
    dot = browserIP.indexOf ('.', dot + 1); if (dot == -1) return false; // something is wrong
    
    if (WiFi.localIP ().toString ().startsWith (browserIP.substring (0, dot + 1))) return true; // a call from station connected to local WiFi (class C IP)
    if (WiFi.softAPIP ().toString ().startsWith (browserIP.substring (0, dot + 1))) return true; // a call from station connected to AP (class C IP)
  
    return false; // none of above - a connection from WAN
  }
  

  httpServer *myHttpServer;
 
void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                              // to enable httpServer to server .html and other files from /var/www/html directory
  startWiFi ();
  // startCronDaemon (NULL);                              // sichronize ESP32 clock with NTP servers if you want to use cookies with expiration time

  myHttpServer = new httpServer (
                                  httpRequestHandler,               // or NULL if httpRequestHandler is not going to be used
                                  NULL,                             // or wsRequestHandler to handle WebSockets
                                  (char *) "0.0.0.0",               // httpServer IP or 0.0.0.0 for all available IPs
                                  80,                               // default HTTP port
                                  firewallCallback                  // use firewall
                                );
  if (myHttpServer->state () != httpServer::RUNNING) Serial.printf ("[%10lu] Could not start httpServer\n", millis ());
}

void loop () {

}
