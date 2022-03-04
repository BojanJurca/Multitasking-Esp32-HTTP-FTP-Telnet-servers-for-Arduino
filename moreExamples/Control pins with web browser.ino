/*
        Control ESP32 pins with web browser
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
    #define LED_BUILTIN 2

    if (httpRequestStartsWith ("GET / ")) return F ("<html>\n"
                                                    "  Combining (large) .html files with (short) programmable (JSON) responses<br><br>\n"
                                                    "  <hr />\n"
                                                    "  Led: <input type='checkbox' disabled id='ledSwitch' onClick='turnLed (this.checked)'>\n"
                                                    "  <hr />\n"
                                                    "  <script type='text/javascript'>\n"
                                                    "\n"
                                                    "    // mechanism that makes HTTP requests from javascript\n"
                                                    "    var httpClient = function () {\n"
                                                    "      this.request = function (url, method, callback) {\n"
                                                    "        var httpRequest = new XMLHttpRequest ();\n"
                                                    "        var httpRequestTimeout;\n"
                                                    "        httpRequest.onreadystatechange = function () {\n"
                                                    "          // console.log (httpRequest.readyState);\n"
                                                    "          if (httpRequest.readyState == 1) { // 1 = OPENED, start timing\n"
                                                    "            httpRequestTimeout = setTimeout (function () { alert ('Server did not reply (in time).'); }, 1500);\n"
                                                    "          }\n"
                                                    "          if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText\n"
                                                    "            clearTimeout (httpRequestTimeout);\n"
                                                    "            if (httpRequest.status == 200) callback (httpRequest.responseText); // 200 = OK\n"
                                                    "            else alert ('Server reported error ' + httpRequest.status + ' ' + httpRequest.responseText); // some other reply status, like 404, 503, ...\n"
                                                    "          }\n"
                                                    "        }\n"
                                                    "        httpRequest.open (method, url, true);\n"
                                                    "        httpRequest.send (null);\n"
                                                    "      }\n"
                                                    "    }\n"
                                                    "\n"
                                                    "    // make a HTTP requests to initialize/populate this page\n"
                                                    "    var client = new httpClient ();\n"
                                                    "    client.request ('/builtInLed', 'GET', function (json) {\n"
                                                    "                                                            // json reply will look like: {\"id\":\"MyESP32Server\",\"builtInLed\":\"on\"}\n"
                                                    "                                                            var obj=document.getElementById ('ledSwitch');\n"
                                                    "                                                            obj.disabled = false;\n"
                                                    "                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');\n"
                                                    "                                                          });\n"
                                                    "\n"
                                                    "  function turnLed (switchIsOn) { // sends desired led state to ESP32 server and refreshes ledSwitch state\n"
                                                    "    client.request (switchIsOn ? '/builtInLed/on' : '/builtInLed/off' , 'PUT', function (json) {\n"
                                                    "                                                            var obj = document.getElementById ('ledSwitch');\n"
                                                    "                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');\n"
                                                    "                                                          });\n"
                                                    "  }\n"
                                                    "\n"
                                                    "  </script>\n"
                                                    "</html>\n");

    if (httpRequestStartsWith ("GET /builtInLed "))
  getBuiltInLed:                                          return "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + (digitalRead (LED_BUILTIN) == HIGH ? String ("on") : String ("off")) + "\"}";

    if (httpRequestStartsWith ("PUT /builtInLed/on "))  { digitalWrite (LED_BUILTIN, HIGH); goto getBuiltInLed; }
                                                        
    if (httpRequestStartsWith ("PUT /builtInLed/off ")) { digitalWrite (LED_BUILTIN, LOW); goto getBuiltInLed; } 

    // let the HTTP server try to find a file in /var/www/html and send it as a reply
    return "";
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
                                  NULL                              // or firewallCallback if you want to use firewall
                                );
  if (myHttpServer->state () != httpServer::RUNNING) Serial.printf ("[%10lu] Could not start httpServer\n", millis ());

  pinMode (LED_BUILTIN, OUTPUT);
}

void loop () {

}
