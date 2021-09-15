/*
  
    examples.h 
  
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    September, 15, 2021, Bojan Jurca
         
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
  String s = webClient ("127.0.0.1", 80, (TIME_OUT_TYPE) 5000, "GET /builtInLed"); // send request to local loopback port 80, wait max 5 s (time-out)
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
