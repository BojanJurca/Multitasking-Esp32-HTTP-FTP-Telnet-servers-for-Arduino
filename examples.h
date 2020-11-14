/*
 * 
 * Examples.h 
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 * 
 * History:
 *          - first release, 
 *            April 14, 2019, Bojan Jurca
 *          - added WebSocket example,
 *            May, 20, 2019, Bojan Jurca
  *          - elimination of compiler warnings and some bugs
 *            Jun 10, 2020, Bojan Jurca            
 *  
 */


// Example 08 demonstrates the use of rtc instance (real time clock) built into Esp32_web_ftp_telnet_server_template

void example08_time () {
  // since we are running this example at the very start of ESP we have to give it time to connect
  // and get current time from NTP server first - wait max 10 seconds
  time_t t;
  unsigned long startTime = millis ();
  while (millis () - startTime < 10000 && !(t = getGmt ())) delay (1);
  
  if (t) { // success, time has been set
    Serial.printf ("[%10lu] [example 08] current UNIX time is %lu\n", millis (), (unsigned long) t);
    struct tm localTime = timeToStructTime (timeToLocalTime (t));
    Serial.printf ("[%10lu] [example 08] current local time is %02i.%02i.%02i %02i:%02i:%02i\n", millis (), 1900 + localTime.tm_year, 1 + localTime.tm_mon, localTime.tm_mday, localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
  } else { // faild to get current time
    Serial.printf ("[%10lu] [example 08] couldn't get current time from NTP servers (yet)\n", millis ());
  }
}


// Example 09 shows how we can use TcpServer objects to make HTTP requests

void example09_makeRestCall () {
  String s = webClient ((char *) "127.0.0.1", 80, 5000, "GET /builtInLed"); // send request to local loopback port 80, wait max 5 s (time-out)
  if (s > "")
    Serial.printf ("[%10lu] [example 09] %s\r\n", millis (), s.c_str ());
  else
    Serial.printf ("[%10lu] [example 09] the reply didn't arrive (in time) or it is corrupt or too long\n", millis ());
  return;
}
 

// Example 10 - basic WebSockets demonstration

void example10_webSockets (WebSocket *webSocket) {
  while (true) {
    switch (webSocket->available ()) {
      case WebSocket::NOT_AVAILABLE:  delay (1);
                                      break;
      case WebSocket::STRING:       { // text received
                                      String s = webSocket->readString ();
                                      Serial.printf ("[%10lu] [example 10] got text from browser over webSocket: %s\n", millis (), s.c_str ());
                                      break;
                                    }
      case WebSocket::BINARY:       { // binary data received
                                      byte buffer [256];
                                      int bytesRead = webSocket->readBinary (buffer, sizeof (buffer));
                                      Serial.printf ("[%10lu] [example 10] got %i bytes of binary data from browser over webSocket\n", millis (), bytesRead);
                                      // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                      // (I know they are 16 bit integers because I have written javascript client example myself but this information can not be obtained from webSocket)
                                      int16_t *i = (int16_t *) buffer;
                                      while ((byte *) (i + 1) <= buffer + bytesRead) Serial.printf (" %i", *i ++);
                                      Serial.printf ("\n[%10lu] [example 10] if the sequence is -21 13 -8 5 -3 2 -1 1 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 6765 10946 17711 28657\n"
                                                       "             it means that both, endianness and complements are compatible with javascript client.\n", millis ());
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
                                      Serial.printf ("[%10lu] [example 10] error in communication, closing connection\n", millis ());
                                      return; // close this connection

      default:
                                    break;
    }
  }
}


// Example 11 - a simple echo server except that it echos Morse code back to the client
    
void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameter); // connection handler callback function

void example11_morseEchoServer () {
  if (getWiFiMode () == WIFI_OFF) {
    Serial.printf ("[%10lu] [example 11] Could not start Morse server since there is no network.\n", millis ());
    return;
  }
  
  // start new TCP server
  TcpServer *myServer = new TcpServer (morseEchoServerConnectionHandler,  // function that is going to handle the connections
                                       NULL,                              // no additional parameter will be passed to morseEchoServerConnectionHandler function
                                       4096,                              // 4 KB stack for morseEchoServerConnectionHandler is usually enough
                                       180000,                            // time-out - close connection if it is inactive for more than 3 minutes
                                       (char *) "0.0.0.0",                // serverIP, 0.0.0.0 means that the server will accept connections on all available IP addresses
                                       24,                                // server port number, 
                                       NULL);                             // don't use firewall in this example
  // check success
  if (myServer->started ()) {
    Serial.printf ("[%10lu] [example 11] Morse echo server started, type telnet serverIP 24 to try it\n", millis ());
  
    // let the server run for 30 seconds - this much time you have to connect to it to test how it works
    delay (30000);
  
    // shut down the server - is any connection is still active it will continue to run anyway
    delete (myServer);
    Serial.printf ("[%10lu] [example 11] Morse echo server stopped, already active connections will continue to run anyway\n", millis ());
  } else {
    Serial.printf ("[%10lu] [example 11] unable to start Morse echo server\n", millis ());
  }
}

void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameterNotUsed) {  // connection handler callback function
  Serial.printf ("[%10lu] [example 11] new connection arrived from %s\n", millis (), connection->getOtherSideIP ());
  
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
  char finiteState = ' '; // finite state machine to detect quit, valid states are ' ', 'q', 'u', 'i', 't'
  unsigned char c;
  int index;  
  
  // send welcome reply first as soon as new connection arrives - in a readable form
  sprintf (outputBuffer, "Type anything except quit. Quit will end the connection.\xff\xfe\x01\r\n");  // IAC DONT ECHO
  // IAC DONT ECHO is not really necessary. It is a part of telnet protocol. Since we'll be using a telnet client
  // to test this example it is a good idea to communicate with it in the way it understands
  bytesToSend = strlen (outputBuffer);
  if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
    *outputBuffer = 0; // mark outputBuffer as empty
    Serial.printf ("[%10lu] [example 11] error while sending response\n", millis ());
    goto endThisConnection;
  }
  *outputBuffer = 0; // mark outputBuffer as empty
  
  // Read and process input stream in endless loop, detect "quit" substring. 
  // If "quit" substring is present then end this connection.
  // If 0 bytes arrive then the client has ended the connection or there are problems in communication.
  while (int received = connection->recvData (inputBuffer, sizeof (inputBuffer))) {
    for (int i = 0; i < received; i ++) {
      // calculate index of morse table entry
      c = inputBuffer [i];

      index = 11;                                     // no character in morse table
      if (c == ' ') index = 10;                       // space in morse table
      else if (c >= '0' && c <= 'Z') index = c - '0'; // letter in morse table
      else if (c >= 'a' && c <= 'z') index = c - 80;  // letter converted to upper case in morse table

      // fill outputBuffer if there is still some space left otherwise empty it
      if (strlen (outputBuffer) + 7 > sizeof (outputBuffer)) {
        bytesToSend = strlen (outputBuffer);
        if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
          *outputBuffer = 0; // mark outputBuffer as empty
          Serial.printf ("[%10lu] [example 11] error while sending response\n", millis ());
          goto endThisConnection;
        }
        strcpy (outputBuffer, morse [index]); // start filling outputBuffer with morse letter
      } else {
        strcat (outputBuffer, morse [index]); // append morse letter to outputBuffer
      }

      // calculat finite machine state to detect if "quit" has been entered
      switch (c /* inputBuffer [i] */) {
        case 'Q':
        case 'q': finiteState = 'q';
                  break;
        case 'U':
        case 'u': if (finiteState == 'q') finiteState = 'u'; else finiteState = ' ';
                  break;
        case 'I':
        case 'i': if (finiteState == 'u') finiteState = 'i'; else finiteState = ' ';
                  break;
        case 'T':
        case 't': if (finiteState == 'i') goto endThisConnection; // quit has been entered
                  else finiteState = ' ';
                  break; 
        default:  finiteState = ' ';
                  break;
      }
    } // for loop
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
      *outputBuffer = 0; // mark outputBuffer as empty
      Serial.printf ("[%10lu] [example 11] error while sending response\n", millis ());
      goto endThisConnection;
    }    
    *outputBuffer = 0; // mark outputBuffer as empty
  } // while loop

endThisConnection: // first check if there is still some data in outputBuffer and then just let the function return 
  if (*outputBuffer) {
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) 
      Serial.printf ("[%10lu] [example 11] error while sending response\n", millis ());
  }
  Serial.printf ("[%10lu] [example 11] connection has just ended\n", millis ());
}
