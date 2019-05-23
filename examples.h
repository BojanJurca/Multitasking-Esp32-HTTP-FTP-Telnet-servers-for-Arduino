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


// Example 06 shows how to access files on SPIFFS and/or perform delays properly in a multi-threaded environment. 
// Why are those two connected? The issue is well described in https://www.esp32.com/viewtopic.php?t=7876: 
// "vTaskDelay() cannot be called whilst the scheduler is disabled (i.e. in between calls of vTaskSuspendAll() 
// and vTaskResumeAll()). The assertion failure you see is vTaskDelay() checking if it was called whilst the scheduler 
// is disabled." Some SPIFFS functions internally call vTaskSuspendAll() hence no other thread should call delay() 
// (which internally calls vTaskDelay()) since we cannot predict if this would happen while the scheduler is disabled.
// Esp32_web_ftp_telnet_server_template introduces a SPIFFSsemaphore mutex that prevents going into delay() and SPIFSS 
// functions simultaneously from different threads. Calling a delay() function is not thread (when used together with 
// SPIFFS) safe and would crash ESP32 occasionally. Use SPIFFSsafeDelay() instead.

void example06_filesAndDelays () {
  for (int i = 0; i < 3; i++) {
    String s = "";
    File file;
  
    xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY); // always take SPIFFSsemaphore before any SPIFSS operations
    
    if ((bool) (file = SPIFFS.open ("/ID", FILE_READ)) && !file.isDirectory ()) {
      while (file.available ()) s += String ((char) file.read ());
      file.close (); 
      Serial.printf ("[example 06] the content of file /ID is %s\n", s.c_str ());
    } else {
      Serial.printf ("[example 06] can't read file /ID\n");
    }
    
    xSemaphoreGive (SPIFFSsemaphore); // always give SPIFFSsemaphore when SPIFSS operation completes
    
    SPIFFSsafeDelay (1000); // always use SPIFFSsafeDelay() instead of delay()
  }
}


// Example 07 demonstrates the use of rtc instance (real time clock) built into Esp32_web_ftp_telnet_server_template

void example07_realTimeClock () {
  if (rtc.isGmtTimeSet ()) {
    time_t now;
    now = rtc.getGmtTime ();
    Serial.printf ("[example 07] current UNIX time is %lu\n", (unsigned long) now);
  
    char str [30];
    now = rtc.getLocalTime ();
    strftime (str, 30, "%d.%m.%y %H:%M:%S", gmtime (&now));
    Serial.printf ("[example 07] current local time is %s\n", str);
  } else {
    Serial.printf ("[example 07] rtc has not obtained time from NTP server yet\n");
  }
}


// Example 08 shows how we can use TcpServer objects to make HTTP requests

void example08_makeRestCall () {
  String s = webClient ("127.0.0.1", 80, 5000, "GET /upTime"); // send request to local loopback port 80, wait max 5 s (time-out)
  if (s > "")
    Serial.print ("[example 08] " + s);
  else
    Serial.printf ("[example 08] the reply didn't arrive (in time) or it is corrupt or too long\n");
  return;
}
 

// Example 09 - basic WebSockets demonstration

void example09_webSockets (WebSocket *webSocket) {
  while (true) {
    switch (webSocket->available ()) {
      case WebSocket::NOT_AVAILABLE:  SPIFFSsafeDelay (1);
                                      break;
      case WebSocket::STRING:       { // text received
                                      String s = webSocket->readString ();
                                      Serial.printf ("[example 09] got text from browser over webSocket: %s\n", s.c_str ());
                                      break;
                                    }
      case WebSocket::BINARY:       { // binary data received
                                      char buffer [256];
                                      int bytesRead = webSocket->readBinary (buffer, sizeof (buffer));
                                      Serial.printf ("[example 09] got %i bytes of binary data from browser over webSocket\n"
                                                     "[example 09]", bytesRead);
                                      // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                      // (I know they are 16 bit integers because I have written javascript client example myself but this information can not be obtained from webSocket)
                                      int16_t *i = (int16_t *) buffer;
                                      while ((char *) (i + 1) <= buffer + bytesRead) Serial.printf (" %i", *i ++);
                                      Serial.printf ("\n[example 09] if the sequence is -21 13 -8 5 -3 2 -1 1 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 6765 10946 17711 28657\n"
                                                       "             it means that both, endianness and complements are compatible with javascript client.\n");
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
                                      Serial.printf ("[example 09] error in communication, closing connection\n");
                                      return; // close this connection
    }
  }
}


// Example 10 - a simple echo server except that it echos Morse code back to the client
    
void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameter); // connection handler callback function

void example10_morseEchoServer () {
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
    Serial.printf ("[example 10] Morse echo server started, try \"telnet <server IP> 24\" to try it\n");
  
    // let the server run for 30 seconds - this much time you have to connect to it to test how it works
    SPIFFSsafeDelay (30000);
  
    // shut down the server - is any connection is still active it will continue to run anyway
    delete (myServer);
    Serial.printf ("[example 10] Morse echo server stopped, already active connections will continue to run anyway\n");
  } else {
    Serial.printf ("[example 10] unable to start Morse echo server\n");
  }
}

void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameterNotUsed) {  // connection handler callback function
  Serial.printf ("[example 10] new connection arrived from %s\n", connection->getOtherSideIP ());
  
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
    Serial.printf ("[example 10] error while sending response\n");
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
          Serial.printf ("[example 10] error while sending response\n");
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
      Serial.printf ("[example 10] error while sending response\n");
      goto endThisConnection;
    }    
    *outputBuffer = 0; // mark outputBuffer as empty
  } // while loop

endThisConnection: // first check if there is still some data in outputBuffer and then just let the function return 
  if (*outputBuffer) {
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) 
      Serial.printf ("[example 10] error while sending response\n");
  }
  Serial.printf ("[example 10] connection has just ended\n");
}


void examples (void *notUsed) {
  example06_filesAndDelays ();
  example07_realTimeClock ();
  example08_makeRestCall ();
  example10_morseEchoServer ();

  vTaskDelete (NULL); // end this thread
}


// razvoj

String oscilloscope () {
  measurements samples (256);

/*
  pinMode (36, INPUT);
  unsigned int start = micros ();
  for (int i = 0; i < 256; i++) {
    samples.addMeasurement (micros () - start, digitalRead (36));
    SPIFFSsafeDelayMicroseconds (10 - 4); // !!!!! tole je približno na 10 us, kar je minimum, da še izgleda smiselno, 4 us je za ostalo procesiranje !!!!! cca 100 KZh
  }
*/

/*
  pinMode (36, INPUT);
  unsigned int start = millis ();
  for (int i = 0; i < 256; i++) {
    samples.addMeasurement (millis () - start, digitalRead (36));
    SPIFFSsafeDelay (2); // !!!!! tole je minimum za prehod na ms skalo, da smiselno prikazuje, raje nekaj več !!!!!
  }
*/

  unsigned int start = micros ();
  for (int i = 0; i < 256; i++) {
    samples.addMeasurement (micros () - start, analogRead (36));
    SPIFFSsafeDelayMicroseconds (60 - 12); // !!!!! je približno na 60 us, kar je minimum, da še izgleda smiselno, 12 us je za ostalo procesiranje !!!!! cca 16 KHz
  }
  
  return (samples.measurements2json (1));
}
