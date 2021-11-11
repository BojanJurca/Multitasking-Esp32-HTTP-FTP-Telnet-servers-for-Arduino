/*
    Echo TCP server but it echoes back in Morse code. The server listens for incoming connections on port 24 so you can test
    it just by telneting to it: telnet <your ESP32 IP> 24
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

  // include TcpServer.hpp
  #include "./servers/TcpServer.hpp"

// Morse echo server connectionHandler callback function
void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameterNotUsed) {
  Serial.printf ("[%10lu] [Morse echo server] new connection arrived from %s\n", millis (), connection->getOtherSideIP ().c_str ());
  
  char inputBuffer [256] = {0}; // reserve some stack memory for incomming packets
  char outputBuffer [256] = {0}; // reserve some stack memory for output buffer 
  int bytesToSend;
  // construct Morse table. Make it static so it won't use the stack
  static const char *morse [43] = {"----- ", ".---- ", "..--- ", "...-- ", "....- ", // 0, 1, 2, 3, 4
                                   "..... ", "-.... ", "--... ", "---.. ", "----. ", // 5, 6, 7, 8, 9
                                   "   ", "", "", "", "", "", "",                    // space and some characters not in Morse table
                                   ".- ", "-... ", "-.-. ", "-.. ", ". ",            // A, B, C, D, E
                                   "..-. ", "--. ", ".... ", ".. ", ".--- ",         // F, G, H, I, J
                                   "-.- ", ".-.. ", "-- ", "-. ", "--- ",            // K, L, M, N, O
                                   ".--. ", "--.- ", ".-. ", "... ", "- ",           // P, Q, R, S, T
                                   "..- ", "...- ", ".-- ", "-..- ", "-.-- ",        // U, V, W, X, Y
                                   "--.. "};                                         // Z
  unsigned char c;
  int index;  
  
  // send welcome reply first as soon as new connection arrives - in a readable form
  sprintf (outputBuffer, "Type anything except Ctrl-C - this would end the connection.\xff\xfe\x01\r\n");  // IAC DONT ECHO
  // IAC DONT ECHO is not really necessary. It is a part of telnet protocol. Since we'll be using a telnet client
  // to test this example it is a good idea to communicate with it in the way it understands
  bytesToSend = strlen (outputBuffer);
  if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
    *outputBuffer = 0; // mark outputBuffer as empty
    Serial.printf ("[%10lu] [Morse echo server] error while sending response\n", millis ());
    goto endThisConnection;
  }
  *outputBuffer = 0; // mark outputBuffer as empty
  
  // Read and process input stream in endless loop, detect Ctrl-C 
  // If "quit" substring is present then end this connection.
  // If 0 bytes arrive then the client has ended the connection or there are problems in communication.
  while (int received = connection->recvData (inputBuffer, sizeof (inputBuffer))) {
    for (int i = 0; i < received; i ++) {
      // calculate index of morse table entry
      c = inputBuffer [i];
      if (c == 3) goto endThisConnection; // Ctrl-C
      index = 11;                                     // no character in morse table
      if (c == ' ') index = 10;                       // space in morse table
      else if (c >= '0' && c <= 'Z') index = c - '0'; // letter in morse table
      else if (c >= 'a' && c <= 'z') index = c - 80;  // letter converted to upper case in morse table

      // fill outputBuffer if there is still some space left otherwise empty it
      if (strlen (outputBuffer) + 7 > sizeof (outputBuffer)) {
        bytesToSend = strlen (outputBuffer);
        if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
          *outputBuffer = 0; // mark outputBuffer as empty
          Serial.printf ("[%10lu] [Morse echo server] error while sending response\n", millis ());
          goto endThisConnection;
        }
        strcpy (outputBuffer, morse [index]); // start filling outputBuffer with morse letter
      } else {
        strcat (outputBuffer, morse [index]); // append morse letter to outputBuffer
      }

    } // for loop
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
      *outputBuffer = 0; // mark outputBuffer as empty
      Serial.printf ("[%10lu] [Morse echo server] error while sending response\n", millis ());
      goto endThisConnection;
    }    
    *outputBuffer = 0; // mark outputBuffer as empty
  } // while loop

endThisConnection: // first check if there is still some data in outputBuffer and then just let the function return 
  if (*outputBuffer) {
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) 
      Serial.printf ("[%10lu] [Morse echo server] error while sending response\n", millis ());
  }
  Serial.printf ("[%10lu] [Morse echo server] connection has just ended\n", millis ());
}

void setup () {
  Serial.begin (115200);
  
  startWiFi ();                             // connect to to WiFi router
   
  // start new TCP server on all available IP addresses on port 24 and provide our own connectionHandler callback function that will do the echoing
  TcpServer *myServer = new TcpServer (morseEchoServerConnectionHandler,  // function that is going to handle the connections
                                       NULL,                              // no additional parameter will be passed to morseEchoServerConnectionHandler function
                                       4096,                              // 4 KB stack for morseEchoServerConnectionHandler is usually enough
                                       (TIME_OUT_TYPE) 180000,            // time-out - close connection if it is inactive for more than 3 minutes
                                       "0.0.0.0",                         // serverIP, 0.0.0.0 means that the server will accept connections on all available IP addresses
                                       24,                                // server port number, 
                                       NULL);                             // don't use firewall in this example
  // check success
  if (myServer && myServer->started ()) Serial.printf ("[%10lu] [Morse echo server] started, type telnet serverIP 24 to try it.\n", millis ());
  else                                  Serial.printf ("[%10lu] [Morse echo server] did not start.\n", millis ());
}

void loop () {
                
}
