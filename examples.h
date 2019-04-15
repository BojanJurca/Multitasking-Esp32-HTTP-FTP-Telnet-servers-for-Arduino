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
    
    xSemaphoreGive (SPIFFSsemaphore); // alwys give SPIFFSsemaphore when SPIFSS operation completes
    
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


void examples (void *notUsed) {
  example01_filesAndDelays ();
  example02_realTimeClock ();
  example03_makeRestCall ();

  vTaskDelete (NULL); // end this thread
}
