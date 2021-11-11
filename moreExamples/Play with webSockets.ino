/*
    Play with websocket
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

  // file system holds all configuration and .html files 
  // make sure you selected one of FAT partition schemas first (Tools | Partition scheme | ...)
  #include "./servers/file_system.h"

  // configure the way ESP32 server is going to deal with users, this module also defines where web server will search for .html files
  #define USER_MANAGEMENT NO_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // include webServer.hpp
  #include "./servers/webServer.hpp"

// httpRequestHandler is not mandatory, we could also put HTTP reply into /var/www/html/index.html file
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  if (httpRequest.substring (0, 6) == "GET / ") { // first: return a HTML page that would later request websocket communication when it si loaded into web browser
    return  "<html>\n"
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
            "</html>\n";
  } 

  return "";  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}

// we will also need wsRequestHandler to handle websocket communication
void wsRequestHandler (String& wsRequest, WebSocket *webSocket) { // - must be reentrant!

  if (wsRequest.substring (0, 23) == "GET /playWithWebsocket ") { // second: handle webscocket communication
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
  errorInCommunication:     
                                        Serial.printf ("[%10lu] error in communication, closing connection\n", millis ());
                                        return; // close this connection
  
        default:
                                      break;
      }
    }
    
  }

}


void setup () {
  Serial.begin (115200);
 
  mountFileSystem (true);                                             // get access to configuration files

  initializeUsers ();                                                 // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users

  startWiFi ();                                                       // connect to to WiFi router  

  // start web server
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // <- pass address of your HTTP request handler function to web server 
                                        wsRequestHandler,             // <- pass address of your websocket request handler function to web server 
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // we are not going to use firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start."); // insert information into dmesg queue - it can be displayed with dmesg built-in telnet command
}

void loop () {
                
}
