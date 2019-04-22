/*
 * 
 * Examples.h 
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 * 
 * History:
 *          - first release, 
 *            April 14, 2019, Bojan Jurca
 *  
 */


// Example 01 shows how to access files on SPIFFS and/or perform delays properly in a multi-threaded environment. 
// Why are those two connected? The issue is well described in https://www.esp32.com/viewtopic.php?t=7876: 
// "vTaskDelay() cannot be called whilst the scheduler is disabled (i.e. in between calls of vTaskSuspendAll() 
// and vTaskResumeAll()). The assertion failure you see is vTaskDelay() checking if it was called whilst the scheduler 
// is disabled." Some SPIFFS functions internally call vTaskSuspendAll() hence no other thread should call delay() 
// (which internally calls vTaskDelay()) since we cannot predict if this would happen while the scheduler is disabled.
// Esp32_web_ftp_telnet_server_template introduces a SPIFFSsemaphore mutex that prevents going into delay() and SPIFSS 
// functions simultaneously from different threads. Calling a delay() function is not thread (when used together with 
// SPIFFS) safe and would crash ESP32 occasionally. Use SPIFFSsafeDelay() instead.

void example01_filesAndDelays () {
  for (int i = 0; i < 3; i++) {
    String s = "";
    File file;
  
    xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY); // always take SPIFFSsemaphore before any SPIFSS operations
    
    if ((bool) (file = SPIFFS.open ("/ID", FILE_READ)) && !file.isDirectory ()) {
      while (file.available ()) s += String ((char) file.read ());
      file.close (); 
      Serial.printf ("[example 01] %s\n", s.c_str ());
    } else {
      Serial.printf ("[example 01] can't read file /ID\n");
    }
    
    xSemaphoreGive (SPIFFSsemaphore); // always give SPIFFSsemaphore when SPIFSS operation completes
    
    SPIFFSsafeDelay (1000); // always use SPIFFSsafeDelay() instead of delay()
  }
}


// Example 02 demonstrates the use of rtc instance (real time clock) built into Esp32_web_ftp_telnet_server_template

void example02_realTimeClock () {
  if (rtc.isGmtTimeSet ()) {
    time_t now;
    now = rtc.getGmtTime ();
    Serial.printf ("[example 02] current UNIX time is %lu\n", (unsigned long) now);
  
    char str [30];
    now = rtc.getLocalTime ();
    strftime (str, 30, "%d.%m.%y %H:%M:%S", gmtime (&now));
    Serial.printf ("[example 02] current local time is %s\n", str);
  } else {
    Serial.printf ("[example 02] rtc has not obtained time from NTP server yet\n");
  }
}


// Example 03 shows how we can use TcpServer objects to make HTTP requests

void example03_makeRestCall () {
  char buffer [256]; *buffer = 0; // reserve some space to hold the response
  // create non-threaded TCP client instance
  TcpClient myNonThreadedClient ("127.0.0.1", // server's IP address (local loopback in this example)
                                 80,          // server port (usual HTTP port)
                                 3000);       // time-out (3 s in this example)
  // get reference to TCP connection. Befor non-threaded constructor of TcpClient returns the 
  // connection is established if this is possible
  TcpConnection *myConnection = myNonThreadedClient.connection ();
  
  if (myConnection) { // test if connection is established
    int sendTotal = myConnection->sendData ("GET /upTime \r\n\r\n", strlen ("GET /upTime \r\n\r\n")); // send REST request
    // Serial.printf ("[example 03] a request of %i bytes sent to the server\n", sendTotal);
    int receivedTotal = 0;
    // read response in a loop untill 0 bytes arrive - this is a sign that connection has ended 
    // if the response is short enough it will normally arrive in one data block although
    // TCP does not guarantee that it would
    while (int received = myConnection->recvData (buffer + receivedTotal, sizeof (buffer) - receivedTotal - 1)) {
      buffer [receivedTotal += received] = 0; // mark the end of the string we have just read
    }
    
  } 
  
  // check if the reply is correct - the best way is to parse the response but here we'll just check if 
  // the whole reply arrived - our JSON reponse ends with "}\r\n"
  if (strstr (buffer, "}\r\n")) {
    Serial.printf ("[example 03] %s\n", buffer);
  } else {
    Serial.printf ("[example 03] %s ... the reply didn't arrive (in time) or it is corrupt or too long\n", buffer);
  }
}


// Example 06 - a simple echo server except that it echos Morse code back to the client
    
void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameter); // connection handler callback function

void example06_morseEchoServer () {
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
    Serial.printf ("[example 06] Morse echo server started, try \"telnet <server IP> 24\" to try it\n");
  
    // let the server run for 30 seconds - this much time you have to connect to it to test how it works
    SPIFFSsafeDelay (30000);
  
    // shut down the server - is any connection is still active it will continue to run anyway
    delete (myServer);
    Serial.printf ("[example 06] Morse echo server stopped, already active connections will continue to run anyway\n");
  } else {
    Serial.printf ("[example 06] unable to start Morse echo server\n");
  }
}

void morseEchoServerConnectionHandler (TcpConnection *connection, void *parameterNotUsed) {  // connection handler callback function
  Serial.printf ("[example 06] new connection arrived from %s\n", connection->getOtherSideIP ());
  
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
    Serial.printf ("[example 06] error while sending response\n");
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
          Serial.printf ("[example 06] error while sending response\n");
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
      Serial.printf ("[example 06] error while sending response\n");
      goto endThisConnection;
    }    
    *outputBuffer = 0; // mark outputBuffer as empty
  } // while loop

endThisConnection: // first check if there is still some data in outputBuffer and then just let the function return 
  if (*outputBuffer) {
    bytesToSend = strlen (outputBuffer);
    if (connection->sendData (outputBuffer, bytesToSend) != bytesToSend) 
      Serial.printf ("[example 06] error while sending response\n");
  }
  Serial.printf ("[example 06] connection has just ended\n");
}


void examples (void *notUsed) {
  example01_filesAndDelays ();
  example02_realTimeClock ();
  example03_makeRestCall ();
  
  example06_morseEchoServer ();

  vTaskDelete (NULL); // end this thread
}
