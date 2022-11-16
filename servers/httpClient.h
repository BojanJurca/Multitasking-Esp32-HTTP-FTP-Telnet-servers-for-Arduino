/*

    httpClient.hpp 
  
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    HTTP client combines a HTTP request from server, page and method and returns a HTTP reply or "" if there is none.
  
    October, 23, 2022, Bojan Jurca
         
*/


    // ----- includes, definitions and supporting functions -----

    #include <WiFi.h>
    

#ifndef __HTTP_CLIENT__
  #define __HTTP_CLIENT__

  #ifndef __PERFMON__
    #pragma message "Compiling httpClient.h without performance monitors (perfMon.h)"
  #endif  


    // ----- functions and variables in this modul -----

    String httpClient (char *serverName, int serverPort, String httpRequest, String httpMethod, unsigned long timeOut);


    // ----- TUNNING PARAMETERS -----

    #define HTTP_REPLY_TIME_OUT 1500              // 1500 ms = 1,5 sec for HTTP requests
    #define HTTP_REPLY_BUFFER_SIZE 1 * 1024       // reading HTTP reply


    // ----- CODE -----

    #include "dmesg_functions.h"

    #ifndef __STRISTR__
      #define __STRISTR__
      // missing C function in Arduino
      char *stristr (char *haystack, char *needle) { 
        if (!haystack || !needle) return NULL; // nonsense
        int nCheckLimit = strlen (needle);                     
        int hCheckLimit = strlen (haystack) - nCheckLimit + 1;
        for (int i = 0; i < hCheckLimit; i++) {
          int j = i;
          int k = 0;
          while (*(needle + k)) {
              char nChar = *(needle + k ++); if (nChar >= 'a' && nChar <= 'z') nChar -= 32; // convert to upper case
              char hChar = *(haystack + j ++); if (hChar >= 'a' && hChar <= 'z') hChar -= 32; // convert to upper case
              if (nChar != hChar) break;
          }
          if (!*(needle + k)) return haystack + i; // match!
        }
        return NULL; // no match
      }
    #endif


    // ----- HTTP client -----

    String httpClient (char *serverName, int serverPort, String httpRequest, String httpMethod = "GET", unsigned long timeOut = HTTP_REPLY_TIME_OUT) {
      // get server address
      struct hostent *he = gethostbyname (serverName);
      if (!he) {
        dmesg ("[httpClient] gethostbyname() error: ", h_errno, hstrerror (h_errno));
        return "";
      }
      // create socket
      int connectionSocket = socket (PF_INET, SOCK_STREAM, 0);
      if (connectionSocket == -1) {
        dmesg ("[httpClient] socket() error: ", errno, strerror (errno));
        return "";
      }
      // make the socket not-blocking so that time-out can be detected
      if (fcntl (connectionSocket, F_SETFL, O_NONBLOCK) == -1) {
        dmesg ("[httpClient] fcntl() error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // connect to server
      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_port = htons (serverPort);
      serverAddress.sin_addr.s_addr = *(in_addr_t *) he->h_addr; 
      if (connect (connectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
        if (errno != EINPROGRESS) {
          dmesg ("[httpClient] connect() error: ", errno, strerror (errno)); 
          close (connectionSocket);
          return "";
        }
      } // it is likely that socket is not connected yet at this point

      // construct and send minimal HTTP request (or almost minimal, IIS for example, requires Host field)
      char serverIP [46];
      inet_ntoa_r (serverAddress.sin_addr, serverIP, sizeof (serverIP));
      httpRequest = httpMethod + " " + httpRequest + " HTTP/1.0\r\nHost: " + String (serverIP) + "\r\n\r\n"; // 1.0 HTTP does not know keep-alive directive  - we want the server to close the connection immediatelly after sending the reply
      // DEBUG: Serial.print ("[httpClient] httpRequest = " + httpRequest);
      if (sendAll (connectionSocket, (char *) httpRequest.c_str (), httpRequest.length (), timeOut) == -1) {
        dmesg ("[httpClient] send() error: ", errno, strerror (errno)); 
        close (connectionSocket);
        return "";        
      }
      // read HTTP reply  
      char __httpReply__ [HTTP_REPLY_BUFFER_SIZE];
      int receivedThisTime;    
      unsigned long lastActive = millis ();
      String httpReply ("");
      while (true) { // read blocks of incoming data
        switch (receivedThisTime = recv (connectionSocket, __httpReply__, HTTP_REPLY_BUFFER_SIZE - 1, 0)) {
          case -1:
            if ((errno == EAGAIN || errno == ENAVAIL) && HTTP_REPLY_TIME_OUT && millis () - lastActive < HTTP_REPLY_TIME_OUT) break; // not time-out yet
            dmesg ("[httpClient] recv() error: ", errno, strerror (errno));
            close (connectionSocket);  
            return ""; // return empty reply - error
          case 0: // connection closed by peer
            close (connectionSocket);  
            if (stristr ((char *) httpReply.c_str (), (char *) "\nCONTENT-LENGTH:")) return ""; // Content-lenght does not match
            else return httpReply; // return what we have recived without checking if HTTP reply is OK
          default:
            #ifdef __PERFMON__ 
              __perfWiFiBytesReceived__ += receivedThisTime; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
            #endif                        
            __httpReply__ [receivedThisTime] = 0;
            httpReply += __httpReply__;
            // DEBUG: Serial.print ("[httpClient] httpReply = " + httpReply);            
            lastActive = millis ();
            // check if HTTP reply is complete
            char *p = stristr ((char *) httpReply.c_str (), (char *) "\nCONTENT-LENGTH:");
            if (p) {
              p += 16;
              unsigned int contentLength;
              if (sscanf (p, "%u", &contentLength) == 1) {
                p = strstr (p, "\r\n\r\n"); // after this comes the content
                if (p && contentLength == strlen (p + 4)) {
                  close (connectionSocket);
                  return httpReply;
                }
              }
            }
            // else continue reading
        }
      }
      // never executes
      close (connectionSocket);
      return "";
    }
 
#endif
