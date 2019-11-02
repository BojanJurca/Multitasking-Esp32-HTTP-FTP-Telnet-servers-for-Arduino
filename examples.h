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
 *  
 */


// Example 07 shows how to access files on SPIFFS and/or perform delays properly in a multi-threaded environment. 
// Why are those two connected? The issue is well described in https://www.esp32.com/viewtopic.php?t=7876: 
// "vTaskDelay() cannot be called whilst the scheduler is disabled (i.e. in between calls of vTaskSuspendAll() 
// and vTaskResumeAll()). The assertion failure you see is vTaskDelay() checking if it was called whilst the scheduler 
// is disabled." Some SPIFFS functions internally call vTaskSuspendAll() hence no other thread should call delay() 
// (which internally calls vTaskDelay()) since we cannot predict if this would happen while the scheduler is disabled.
// Esp32_web_ftp_telnet_server_template introduces a SPIFFSsemaphore mutex that prevents going into delay() and SPIFSS 
// functions simultaneously from different threads. Calling a delay() function is not thread (when used together with 
// SPIFFS) safe and would crash ESP32 occasionally. Use SPIFFSsafeDelay() instead.

void example07_filesAndDelays () {
  for (int i = 0; i < 3; i++) {
    String s = "";
    File file;
  
    xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY); // always take SPIFFSsemaphore before any SPIFSS operations
    
    if ((bool) (file = SPIFFS.open ("/etc/wpa_supplicant.conf", FILE_READ)) && !file.isDirectory ()) {
      while (file.available ()) s += String ((char) file.read ());
      file.close (); 
      Serial.printf ("[%10d] [example 07] the content of file /etc/wpa_supplicant.conf is\r\n%s\n", millis (), s.c_str ());
    } else {
      Serial.printf ("[%10d] [example 07] can't read file /etc/wpa_supplicant.conf\n", millis ());
    }
    
    xSemaphoreGive (SPIFFSsemaphore); // always give SPIFFSsemaphore when SPIFSS operation completes
    
    SPIFFSsafeDelay (1000); // always use SPIFFSsafeDelay() instead of delay()
  }
}


// Example 08 demonstrates the use of rtc instance (real time clock) built into Esp32_web_ftp_telnet_server_template

void example08_realTimeClock () {
  // since we are running this example at the very start of ESP we have to give it time to connect
  // and get current time from NTP server first - wait max 10 seconds
  unsigned long startTime = millis ();
  while (millis () - startTime < 10000 && !rtc.isGmtTimeSet ()) SPIFFSsafeDelay (1);
  
  if (rtc.isGmtTimeSet ()) { // success, got time
    time_t now;
    now = rtc.getGmtTime ();
    Serial.printf ("[%10d] [example 08] current UNIX time is %lu\n", millis (), (unsigned long) now);
  
    struct tm nowStr = rtc.getLocalStructTime ();
    Serial.printf ("[%10d] [example 08] current local time is %02i.%02i.%02i %02i:%02i:%02i\n", millis (), 1900 + nowStr.tm_year, 1 + nowStr.tm_mon, nowStr.tm_mday, nowStr.tm_hour, nowStr.tm_min, nowStr.tm_sec);
  } else { // faild to get current time so far
    Serial.printf ("[%10d] [example 08] rtc has not obtained time from NTP server yet\n", millis ());
  }
}


// Example 09 shows how we can use TcpServer objects to make HTTP requests

void example09_makeRestCall () {
  String s = webClient ("127.0.0.1", 80, 5000, "GET /upTime"); // send request to local loopback port 80, wait max 5 s (time-out)
  // alternatively, you can use webClientCallMAC if you prefer to address stations connected to the AP network interface
  // by MAC rather then IP addresses - for example webClientCallMAC ("a0:20:a6:0c:ea:a9", 80, 5000, "GET /upTime"); 
  if (s > "")
    Serial.printf ("[%10d] [example 09] %s\r\n", millis (), s.c_str ());
  else
    Serial.printf ("[%10d] [example 09] the reply didn't arrive (in time) or it is corrupt or too long\n", millis ());
  return;
}
 

// Example 10 - basic WebSockets demonstration

void example10_webSockets (WebSocket *webSocket) {
  while (true) {
    switch (webSocket->available ()) {
      case WebSocket::NOT_AVAILABLE:  SPIFFSsafeDelay (1);
                                      break;
      case WebSocket::STRING:       { // text received
                                      String s = webSocket->readString ();
                                      Serial.printf ("[%10d] [example 09] got text from browser over webSocket: %s\n", millis (), s.c_str ());
                                      break;
                                    }
      case WebSocket::BINARY:       { // binary data received
                                      byte buffer [256];
                                      int bytesRead = webSocket->readBinary (buffer, sizeof (buffer));
                                      Serial.printf ("[%10d] [example 10] got %i bytes of binary data from browser over webSocket\n", millis (), bytesRead);
                                      // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                      // (I know they are 16 bit integers because I have written javascript client example myself but this information can not be obtained from webSocket)
                                      int16_t *i = (int16_t *) buffer;
                                      while ((byte *) (i + 1) <= buffer + bytesRead) Serial.printf (" %i", *i ++);
                                      Serial.printf ("\n[%10d] [example 10] if the sequence is -21 13 -8 5 -3 2 -1 1 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 6765 10946 17711 28657\n"
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
                                      Serial.printf ("[%10d] [example 10] error in communication, closing connection\n", millis ());
                                      return; // close this connection
    }
  }
}


// Example 11 - a simple echo server except that it echos Morse code back to the client
    
void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameter); // connection handler callback function

void example11_morseEchoServer () {
  // start new TCP server
  TcpServer *myServer = new TcpServer (morseEchoServerConnectionHandler, // function that is going to handle the connections
                                       NULL,      // no additional parameter will be passed to morseEchoServerConnectionHandler function
                                       4096,      // 4 KB stack for morseEchoServerConnectionHandler is usually enough
                                       180000,    // time-out - close connection if it is inactive for more than 3 minutes
                                       "0.0.0.0", // serverIP, 0.0.0.0 means that the server will accept connections on all available IP addresses
                                       24,        // server port number, 
                                       NULL);     // don't use firewall in this example
  // check success
  if (myServer->started ()) {
    Serial.printf ("[%10d] [example 11] Morse echo server started, try \"telnet <server IP> 24\" to try it\n", millis ());
  
    // let the server run for 30 seconds - this much time you have to connect to it to test how it works
    SPIFFSsafeDelay (30000);
  
    // shut down the server - is any connection is still active it will continue to run anyway
    delete (myServer);
    Serial.printf ("[%10d] [example 11] Morse echo server stopped, already active connections will continue to run anyway\n", millis ());
  } else {
    Serial.printf ("[%10d] [example 11] unable to start Morse echo server\n", millis ());
  }
}

void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameterNotUsed) {  // connection handler callback function
  Serial.printf ("[%10d] [example 11] new connection arrived from %s\n", millis (), connection->getOtherSideIP ());
  
  char inputBuffer [256] = {0}; // reserve some stack memory for incomming packets
  char outputBuffer [256] = {0}; // reserve some stack memory for output buffer 
  int bytesToSend;
  // construct Morse table. Make it static so it won't use the stack
  static char *morse [43] = {"----- ", ".---- ", "..--- ", "...-- ", "....- ", // 0, 1, 2, 3, 4
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
  #define IAC 255
  #define DONT 254
  #define ECHO 1
  sprintf (outputBuffer, "Type anything except quit. Quit will end the connection.%c%c%c\r\n", IAC, DONT, ECHO); 
  // IAC DONT ECHO is not really necessary. It is a part of telnet protocol. Since we'll be using a telnet client
  // to test this example it is a good idea to communicate with it in the way it understands
  bytesToSend = strlen (outputBuffer);
  if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) {
    *outputBuffer = 0; // mark outputBuffer as empty
    Serial.printf ("[%10d] [example 11] error while sending response\n", millis ());
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
          Serial.printf ("[%10d] [example 11] error while sending response\n", millis ());
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
      Serial.printf ("[%10d] [example 11] error while sending response\n", millis ());
      goto endThisConnection;
    }    
    *outputBuffer = 0; // mark outputBuffer as empty
  } // while loop

endThisConnection: // first check if there is still some data in outputBuffer and then just let the function return 
  if (*outputBuffer) {
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) 
      Serial.printf ("[%10d] [example 11] error while sending response\n", millis ());
  }
  Serial.printf ("[%10d] [example 11] connection has just ended\n", millis ());
}
