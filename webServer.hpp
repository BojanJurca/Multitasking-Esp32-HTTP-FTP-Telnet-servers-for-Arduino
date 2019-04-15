/*
 * 
 * WebServer.hpp 
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 * 
 *  WebServer is a TcpServer with connectionHandler that handles TcpConnection according to HTTP protocol.
 * 
 *  A connectionHandler handles some HTTP requests by itself but the calling program can provide its own callback
 *  function. In this case connectionHandler will first ask callback function weather is it going to handle the HTTP 
 *  request. If not the connectionHandler will try to resolve it by checking file system for a proper .html file
 *  or it will respond with reply 404 - page not found.
 * 
 *  HTTP 1.1 protocol introduced keep-alive directive which was meant to keep connection alive for handling multiple
 *  consecutive requests thus using less resources. In case of ESP32 it is the other way around, keeping connection
 *  alive would cause running out of free sockets. This is the main reason why webServer replies in HTTP 1.0
 *  manner, meaning it closes the connection as soon as a reply is sent, ingnoring keep-alive directive.
 * 
 * History:
 *          - first release, 
 *            November 19, 2018, Bojan Jurca
 *          - added SPIFFSsemaphore and SPIFFSsafeDelay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca 
 *  
 */

#ifndef __WEB_SERVER__
  #define __WEB_SERVER__
 
  // change this definitions according to your needs

  #define WEB_WITHOUT_FILE_SYSTEM            1     // web server will not search for .html files in file system
  #define WEB_USE_BUILTIN_HOME_DIRECTORY     2     // define home directory that will be hard coded into program
  #define WEB_USE_USER_MANAGEMENT            3     // home directory of webserver system account will be obtained through user_management.h
  // select one of the above methods to obtain web home directory
  #define WEB_HOME_DIRECTORY                 WEB_USE_USER_MANAGEMENT   


  #if WEB_HOME_DIRECTORY == WEB_USE_BUILTIN_HOME_DIRECTORY
    #define WEB_HOME_DIRECTORY_NAME         "/var/www/html/"    // change according to your needs
  #endif
  

  #include "file_system.h"        // webServer.hpp needs file_system.h
  #if WEB_HOME_DIRECTORY == WEB_USE_USER_MANAGEMENT
    #include "user_management.h"  // webServer.hpp needs user_management.h
  #endif
  #include "TcpServer.hpp"        // webServer.hpp is built upon TcpServer.hpp
  

   void __webConnectionHandler__ (TcpConnection *connection, void *httpRequestHandler);
  
  class webServer {                                             
  
    public:
  
      webServer (String (* httpRequestHandler) (String httpRequest),   // httpRequestHandler callback function provided by calling program
                 unsigned int stackSize,                               // stack size of httpRequestHandler thread, usually 4 KB will do 
                 char *serverIP,                                       // web server IP address, 0.0.0.0 for all available IP addresses - 15 characters at most!
                 int serverPort,                                       // web server port
                 bool (* firewallCallback) (char *)                    // a reference to callback function that will be celled when new connection arrives 
                )                               {
                                                  #if   WEB_HOME_DIRECTORY == WEB_WITHOUT_FILE_SYSTEM
                                                    strcpy (this->__webHomeDirectory__, "not available");
                                                  #elif WEB_HOME_DIRECTORY == WEB_USE_BUILTIN_HOME_DIRECTORY
                                                    strcpy (this->__webHomeDirectory__, WEB_HOME_DIRECTORY_NAME);
                                                  #elif WEB_HOME_DIRECTORY == WEB_USE_USER_MANAGEMENT
                                                    char *p = getUserHomeDirectory ("webserver"); 
                                                    if (p && strlen (p) < sizeof (this->__webHomeDirectory__)) strcpy (this->__webHomeDirectory__, p);
                                                  #endif
                                                  
                                                  if (*this->__webHomeDirectory__) { 
                                                  
                                                    // start TCP server
                                                    this->__tcpServer__ = new TcpServer ( __webConnectionHandler__, // worker function
                                                                                          (void *) httpRequestHandler,       // tell TcpServer to pass reference callback function to __webConnectionHandler__
                                                                                          stackSize,                // usually 4 KB will do for webConnectionHandler
                                                                                          5000,                     // close connection if inactive for more than 1,5 seconds
                                                                                          serverIP,                 // accept incomming connections on on specified addresses
                                                                                          serverPort,               // web port
                                                                                          firewallCallback);        // firewall callback function
                                                                                          
                                                  } else {
                                                    Serial.printf ("[web] home directory for webserver system account is not set\n");
                                                  }
                                                  if (this->started ()) Serial.printf ("[web] server started\n");
                                                  else                  Serial.printf ("[web] couldn't start web server\n");
                                                }
      
      ~webServer ()                             { if (this->__tcpServer__) delete (this->__tcpServer__); }
      
      bool started ()                           { return this->__tcpServer__ && this->__tcpServer__->started (); } 

      char *getHomeDirectory ()                 { return this->__webHomeDirectory__; }

    private:

      char __webHomeDirectory__ [33] = {};                                // webServer system account home directory
      TcpServer *__tcpServer__ = NULL;                                    // pointer to (threaded) TcpServer instance
      
  };

       
      void __webConnectionHandler__ (TcpConnection *connection, void *httpRequestHandler) {  // connectionHandler callback function
        log_v ("[Thread:%i][Core:%i] connection has started\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
        char buffer [512];  // make sure there is enough space for each type of use but be modest - this buffer uses thread stack
        int receivedTotal = buffer [0] = 0;
        while (int received = connection->recvData (buffer + receivedTotal, sizeof (buffer) - receivedTotal - 1)) { // this loop may not seem necessary but TCP protocol does not guarantee that a whole request arrives in a single data block althought it usually does
          buffer [receivedTotal += received] = 0; // mark the end of received request
          if (strstr (buffer, "\r\n\r\n")) { // is the end of HTTP request is reached?
            log_v ("[Thread:%i][Core:%i] new request:\n%s", xTaskGetCurrentTaskHandle (), xPortGetCoreID (), buffer);

            // ----- first ask httpRequestHandler (if it is provided by the calling program) if it is going to handle this HTTP request -----

            log_v ("[Thread:%i][Core:%i] trying to get a reply from calling program\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());
            String httpReply;
            unsigned long timeOutMillis = connection->getTimeOut (); connection->setTimeOut (TCP_SERVER_INFINITE_TIMEOUT); // disable time-out checking while proessing httpRequestHandler to allow longer processing times
            if (httpRequestHandler && (httpReply = ((String (*) (String)) httpRequestHandler) (String (buffer))) != "") {
              httpReply = "HTTP/1.0 200 OK\r\nContent-Type:text/html;\r\nCache-control:no-cache\r\nContent-Length:" + String (httpReply.length ()) + "\r\n\r\n" + httpReply; // add HTTP header
              connection->sendData ((char *) httpReply.c_str (), httpReply.length ()); // send everything to the client
              connection->setTimeOut (timeOutMillis); // restore time-out checking before sending reply back to the client
              goto closeWebConnection;
            }
            connection->setTimeOut (timeOutMillis); // restore time-out checking

            // ----- check if request is of type GET filename - if yes then repy with filename content -----

#if WEB_HOME_DIRECTORY > WEB_WITHOUT_FILE_SYSTEM

            char htmlFile [33] = {};
            char fullHtmlFilePath [33];
            if (buffer == strstr (buffer, "GET ")) {
              char *p; if ((p = strstr (buffer + 4, " ")) && (p - buffer) < (sizeof (htmlFile) + 4)) memcpy (htmlFile, buffer + 4, p - buffer - 4);
              if (*htmlFile == '/') strcpy (htmlFile, htmlFile + 1); if (!*htmlFile) strcpy (htmlFile, "index.html");
              if (p = getUserHomeDirectory ("webserver")) {
                if (strlen (p) + strlen (htmlFile) < sizeof (fullHtmlFilePath)) strcat (strcpy (fullHtmlFilePath, p), htmlFile);
                
                xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY);
                
                log_v ("[Thread:%i][Core:%i] trying to find file %s\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID (), fullHtmlFilePath);
                File file;                
                if ((bool) (file = SPIFFS.open (fullHtmlFilePath, FILE_READ))) {
                  if (!file.isDirectory ()) {
                    log_v ("GET %s\n", fullHtmlFilePath);  
                    char *buff = (char *) malloc (2048); // get 2 KB of memory from heap (not from the stack)
                    if (buff) {
                      sprintf (buff, "HTTP/1.0 200 OK\r\nContent-Type:text/html;\r\nContent-Length:%i\r\n\r\n", file.size ());
                      int i = strlen (buff);
                      while (file.available ()) {
                        *(buff + i++) = file.read ();
                        if (i == 2048) { connection->sendData ((char *) buff, 2048); i = 0; }
                      }
                      if (i) { connection->sendData ((char *) buff, i); }
                      free (buff);
                    } else {
                      log_e ("[Thread:%i][Core:%i] malloc error\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());
                    }
                    file.close ();                    
                    
                    xSemaphoreGive (SPIFFSsemaphore);
                    
                    goto closeWebConnection;
                  }
                  file.close ();
                }
                
                xSemaphoreGive (SPIFFSsemaphore);
                
              }
            }
    
            // ----- if request was GET / and index.html couldn't be found then send special reply -----
            
            if (!strcmp (htmlFile, "index.html")) {
              sprintf (buffer + 200, "Please use FTP, loggin as webadmin / webadminpassword and upload *.html and *.png files found in Esp32_web_ftp_telnet_server_template package into webserver home directory.");
              sprintf (buffer, "HTTP/1.0 200 OK\r\nContent-Type:text/html;\r\nCache-control:no-cache\r\nContent-Length:%i\r\n\r\n%s", strlen (buffer + 200), buffer + 200);
              connection->sendData (buffer, strlen (buffer));
              goto closeWebConnection;
            } 

#endif            

reply404:
          
            // ----- 404 page not found reply -----
            
            #define response404 "HTTP/1.0 404 Not found\r\nContent-Type:text/html;\r\nContent-Length:20\r\n\r\nPage does not exist." // HTTP header and content
            connection->sendData (response404, strlen (response404)); // send response
            log_v ("response:\n%s\n", response404);   
            
          } // is the end of HTTP request is reached?
        }
      
      closeWebConnection:
        log_v ("[Thread:%i][Core:%i] connection has ended\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());    
      }

#endif
