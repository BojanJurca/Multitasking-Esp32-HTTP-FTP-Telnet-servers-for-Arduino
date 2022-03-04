/*
    Play with websocket
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

    if (httpRequestStartsWith ("GET / ")) // return a HTML page that would later request websocket communication when it si loaded into web browser
                                          return F ("<html>\n"
                                                    "  <body>\n"
                                                    "    Play with websocket\n"
                                                    "\n"
                                                    "    <script type='text/javascript'>\n"
                                                    "      if ('WebSocket' in window) {\n"
                                                    "\n"
                                                    "        var ws = new WebSocket ('ws://' + self.location.host + '/playWithWebsocket'); // open webSocket\n" // <- this request will later be handeled by websocket handler                      
                                                    "\n"
                                                    "        ws.onopen = function () {\n"
                                                    "          // ----- send text data -----\n"
                                                    "          ws.send ('Hello webSocket server, after this text I am sending 32 16 bit binary integers.');\n" 
                                                    "          // ----- send binary data -----\n"
                                                    "          const fibonacciSequence = new Uint16Array (32);\n"
                                                    "          fibonacciSequence [0] = -21;\n"
                                                    "          fibonacciSequence [1] = 13;\n"
                                                    "          for (var i = 2; i < 32; i ++) fibonacciSequence [i] = fibonacciSequence [i - 1] + fibonacciSequence [i - 2];\n" 
                                                    "          ws.send (fibonacciSequence);\n"
                                                    "        };\n"
                                                    "\n"
                                                    "        ws.onmessage = function (evt) {\n" 
                                                    "          if (typeof(evt.data) === 'string' || evt.data instanceof String) { // UTF-8 formatted string data\n"
                                                    "            // ----- receive text data -----\n"
                                                    "            alert ('Got text from server over webSocket: ' + evt.data);\n"
                                                    "          }\n"
                                                    "          if (evt.data instanceof Blob) { // binary data\n"
                                                    "            // ----- receive binary data as blob and then convert it into array buffer -----\n"
                                                    "            var myFloat32Array = null;\n"
                                                    "            var myArrayBuffer = null;\n"
                                                    "            var myFileReader = new FileReader ();\n"
                                                    "            myFileReader.onload = function (event) {\n"
                                                    "              myArrayBuffer = event.target.result;\n"
                                                    "              myFloat32Array = new Float32Array (myArrayBuffer); // <= our data is now here, in the array of 32 bit floating point numbers\n"
                                                    "              var myMessage = 'Got ' + myArrayBuffer.byteLength + ' bytes of binary data from server over webSocket\\n';\n"
                                                    "              for (var i = 0; i < myFloat32Array.length; i++) myMessage += " " + myFloat32Array [i];\n"
                                                    "                alert (myMessage);\n"
                                                    "                // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 32 bit floating point numbers\n"
                                                    "                // (I know they are 32 bit floating point numbers because I have written server C++ code myself but this information can not be obtained from webSocket)\n"
                                                    "                alert ('If the sequence is 1 0.5 0.25 0.125 0.0625 0.03125 0.015625 0.0078125\\nit means that 32 bit floating point format is compatible with ESP32 C++ server.');\n"
                                                    "                ws.close (); // this is where webSocket connection ends - in our simple 'protocol' browser closes the connection but it could be the server as well\n"
                                                    "            };\n"
                                                    "            myFileReader.readAsArrayBuffer (evt.data);\n"
                                                    "          }\n"
                                                    "        };\n"
                                                    "\n"
                                                    "        ws.onclose = function () {\n" 
                                                    "           alert ('WebSocket connection is closed.');\n"
                                                    "        };\n"
                                                    "\n"
                                                    "      } else {\n"
                                                    "        alert ('WebSockets are not supported by your browser.');\n"
                                                    "      }\n"
                                                    "\n"
                                                    "    </script>\n"
                                                    "  </body>\n"
                                                    "</html>");
    
    // let the HTTP server try to find a file in /var/www/html and send it as a reply
    return "";
  }

  void wsRequestHandler (char *wsRequest, WebSocket *webSocket) {
    
    #define wsRequestStartsWith(X) (strstr(wsRequest,X)==wsRequest)
      
    if (wsRequestStartsWith ("GET /playWithWebsocket ")) { // handle webSocket called from HTML page
      while (true) {
        switch (webSocket->available ()) {
          case WebSocket::NOT_AVAILABLE:  delay (1);
                                          break;
          case WebSocket::STRING:       { // text received
                                          String s = webSocket->readString ();
                                          Serial.printf ("[%10lu] got text from browser over webSocket: %s\n", millis (), s.c_str ());
                                          break;
                                        }
          case WebSocket::BINARY:       { // binary data received
                                          byte buffer [256];
                                          int bytesRead = webSocket->readBinary (buffer, sizeof (buffer));
                                          Serial.printf ("[%10lu] got %i bytes of binary data from browser over webSocket\n", millis (), bytesRead);
                                          // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                          // (I know they are 16 bit integers because I have written javascript client myself but this information can not be obtained from webSocket)
                                          int16_t *i = (int16_t *) buffer;
                                          while ((byte *) (i + 1) <= buffer + bytesRead) Serial.printf (" %i", *i ++);
                                          Serial.printf ("\n[%10lu] if the sequence is -21 13 -8 5 -3 2 -1 1 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 6765 10946 17711 28657\n"
                                                           "        it means that both, endianness and complements are compatible with javascript client.\n", millis ());
                                          // send text data
                                          if (!webSocket->sendString ("Thank you webSocket client, I'm sending back 8 32 bit binary floats.")) goto errorInCommunication;
                                          // send binary data
                                          float geometricSequence [8] = {1.0}; for (int i = 1; i < 8; i++) geometricSequence [i] = geometricSequence [i - 1] / 2;
                                          if (!webSocket->sendBinary ((byte *) geometricSequence, sizeof (geometricSequence))) goto errorInCommunication;
                                          break; // this is where webSocket connection ends - in our simple "protocol" browser closes the connection but it could be the server as well ...
                                                 // ... just "return;" in this case (instead of break;)
                                        }
          case WebSocket::ERROR:          
          case WebSocket::TIME_OUT: 
    errorInCommunication:     
                                        Serial.printf ("[%10lu] error in communication, closing connection\n", millis ());
                                        return; // close this connection
              default:                  break; // does not happen
        }
      }
    }
  }


  httpServer *myHttpServer;
 
void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                              // to enable httpServer to server .html and other files from /var/www/html directory
  startWiFi ();
  // startCronDaemon (NULL);                              // sichronize ESP32 clock with NTP servers if you want to use cookies with expiration time

  myHttpServer = new httpServer (
                                  httpRequestHandler,               // or NULL if httpRequestHandler is not going to be used
                                  wsRequestHandler,                 // or NULL if wsRequestHandler is not going to be used
                                  (char *) "0.0.0.0",               // httpServer IP or 0.0.0.0 for all available IPs
                                  80,                               // default HTTP port
                                  NULL                              // or firewallCallback if you want to use firewall
                                );
  if (myHttpServer->state () != httpServer::RUNNING) Serial.printf ("[%10lu] Could not start httpServer\n", millis ());
}

void loop () {

}
