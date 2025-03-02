/*

    tcpServer.hpp

    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    February 6, 2025, Bojan Jurca


    Classes implemented/used in this module:

        tcpServer_t

    Inheritance diagram:    

             ┌─────────────┐
             │ tcpServer_t ┼┐
             └─────────────┘│
                            │
      ┌─────────────────┐   │
      │ tcpConnection_t ┼┐  │
      └─────────────────┘│  │
                         │  │
                         │  │  ┌─────────────────────────┐
                         │  ├──┼─ httpServer_t           │
                         ├─────┼──── webSocket_t         │
                         │  │  │     └── httpConection_t │
                         │  │  └─────────────────────────┘
                         │  │  logicly webSocket_t would inherit from httpConnection_t but it is easier to implement it the other way around
                         │  │
                         │  │
                         │  │  ┌────────────────────────┐
                         │  ├──┼─ telnetServer_t        │
                         ├─────┼──── telnetConnection_t │
                         │  │  └────────────────────────┘
                         │  │
                         │  │
                         │  │  ┌────────────────────────────┐
                         │  └──┼─ ftpServer_t               │
                         ├─────┼──── ftpControlConnection_t │
                         │     └────────────────────────────┘
                         │  
                         │  
                         │     ┌──────────────┐
                         └─────┼─ tcpClient_t │
                               └──────────────┘


    tcpServer can work in multi-task or single-task mode. Normaly it would work in multi-task mode meaning that separate task will be created for listener and each connection that arrives. 
    The only case when single-task mode is used is for FTP passive data connections when both listener and the connection run in the calling routine task.

    What distinguishes both modes is serverTimeout parameter. If it is 0 (meaning infinite) tcpServer works in multi-tak mode. If it is > 0 tcpServer works in single-task mode. 

*/


#include "tcpConnection.hpp"


// TUNNING PARAMETERS
#define LISTENER_STACK_SIZE (2 * 1024 + 512)


#ifndef __TCP_SERVER__
    #define __TCP_SERVER__


    static SemaphoreHandle_t __tcpServerSemaphore__ = xSemaphoreCreateMutex ();  // control tcpServer and tcpConnection critical sections - initialize it while still in single-task mode

    static int __runningTcpConnections__ = 0; // run time statistics


    class tcpServer_t {                                             

        public:
    
            tcpServer_t (int serverPort, bool (*firewallCallback) (char *clientIP, char *serverIP), time_t serverTimeout = 0) { 
                // create a local copy of parameters for later use
                __serverPort__ = serverPort;
                __firewallCallback__ = firewallCallback;
                __serverTimeout__ = serverTimeout;

                // create listening socket
                __listeningSocket__ = socket (AF_INET6, SOCK_STREAM, 0);
                if (__listeningSocket__ == -1) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] socket error: " << errno << " " << strerror (errno);
                    #endif
                    // DEBUG: Serial.print ((const char *) "[tcpServer] socket error: "); Serial.print (errno); Serial.println (strerror (errno));
                    return;
                }

                // allow both IPv4 and IPv6 connections 
                int opt = 0;
                if (setsockopt (__listeningSocket__, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof (opt)) < 0) { 
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] setsockopt error: " << errno << " " << strerror (errno);
                    #endif
                    // DEBUG: Serial.print ((const char *) "[tcpServer] setsockopt error: "); Serial.print (errno); Serial.println (strerror (errno));
                    close (__listeningSocket__);
                    return;
                }

                // make address reusable - so we won't have to wait a few minutes in case server will be restarted
                int flag = 1;
                if (setsockopt (__listeningSocket__, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag)) == -1) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] setsockopt error: " <<  errno << (const char *) " " << strerror (errno);
                    #endif
                    // DEBUG: Serial.print ((const char *) "[tcpServer] setsockopt error: "); Serial.print (errno); Serial.println (strerror (errno));
                    close (__listeningSocket__);
                    return;
                }
                
                // bind listening socket (to IP address and) port
                struct sockaddr_in6 serverAddress = {}; // memset (&serverAddress, 0, sizeof (serverAddress)); 
                serverAddress.sin6_family = AF_INET6; 
                // inet_pton (AF_INET6, ths->__serverIP__, &serverAddress.sin6_addr); // leave 0 for all available server's addresses
                serverAddress.sin6_port = htons (__serverPort__);
                if (bind (__listeningSocket__, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] bind error: " << errno << " " << strerror (errno);
                    #endif
                    // DEBUG: Serial.print ((const char *) "[tcpServer] bind error: "); Serial.print (errno); Serial.println (strerror (errno));
                    close (__listeningSocket__);
                    return;
                } 

                // make socket a listening socket
                if (listen (__listeningSocket__, 4) == -1) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] listen error: " << errno << " " << strerror (errno);
                    #endif
                    // DEBUG: Serial.print ((const char *) "[tcpServer] listen error: "); Serial.print (errno); Serial.println (strerror (errno));
                    close (__listeningSocket__);
                    return;
                } 
                
                // set timeout on listening socket
                struct timeval tv = { __serverTimeout__, 0 };
                if (setsockopt (__listeningSocket__, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof (tv)) < 0) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] setsockopt error: " << errno << " " << strerror (errno);
                    #endif
                    // DEBUG: Serial.print ((const char *) "[tcpServer] setsockopt error: "); Serial.print (errno); Serial.println (strerror (errno));
                    close (__listeningSocket__);
                    return;
                }

                // listener is ready to start accepting connections now
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[tcpServer] listener on port " << __serverPort__ << (const char *) " is running on core " << xPortGetCoreID ();
                #endif
                __state__ = RUNNING;

                // if in multi-task mode start a new task that would listen for incoming connections
                if (!__serverTimeout__) { // start listener task in its own task
                    #define tskNORMAL_PRIORITY 1
                    BaseType_t taskCreated = xTaskCreate ([] (void *thisInstance) {
                                                                                      tcpServer_t *ths = (tcpServer_t *) thisInstance;

                                                                                      // accept and handle incoming connections
                                                                                      while (ths->__listeningSocket__ > -1) { // while listening socket is opened

                                                                                          // accept new connection then we can loose reference to it - tcpConnection will close and free memory itself when it ends
                                                                                          ths->accept ();

                                                                                          // check how much stack did we use
                                                                                          static UBaseType_t lastHighWaterMark = LISTENER_STACK_SIZE;
                                                                                          UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                                                                                          if (lastHighWaterMark > highWaterMark) {
                                                                                              #ifdef __DMESG__
                                                                                                  dmesgQueue << (const char *) "[tcpServer new listener's stack high water mark reached: " << highWaterMark << (const char *) " bytes not used";
                                                                                              #endif
                                                                                              // DEBUG: 
                                                                                              Serial.print ((const char *) "[tcpServer] new listener's stack high water mark reached: "); Serial.print (highWaterMark); Serial.println ((const char *) " bytes not used");
                                                                                              lastHighWaterMark = highWaterMark;
                                                                                          }

                                                                                      } // while accepting connections

                                                                                      #ifdef __DMESG__
                                                                                          dmesgQueue << (const char *) "[tcpServer] on port " << ths->__serverPort__ << (const char *) " stopped";
                                                                                      #endif

                                                                                      // close listening socket
                                                                                      int tmpListeningSocket;
                                                                                      xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY);
                                                                                          tmpListeningSocket = ths->__listeningSocket__;
                                                                                          ths->__listeningSocket__ = -1;
                                                                                      xSemaphoreGive (__tcpServerSemaphore__);
                                                                                      if (tmpListeningSocket > -1) 
                                                                                          close (tmpListeningSocket);

                                                                                      // stop the listening task
                                                                                      ths->__state__ = NOT_RUNNING;
                                                                                      vTaskDelete (NULL); // it is listener's responsibility to close itself

                                                                                  }, "tcpListener", LISTENER_STACK_SIZE, this, tskNORMAL_PRIORITY, NULL);
                    if (pdPASS != taskCreated) {
                        __state__ == NOT_RUNNING;
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[tcpServer] xTaskCreate error";
                        #endif
                    }
                }

                // when the constructor returns __state__ could be eighter RUNNING (in case of success) or NOT_RUNNING (in case of error)
            }


        ~tcpServer_t () {
            // close listening socket
            int listeningSocket;
            xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY);
                listeningSocket = __listeningSocket__;
                __listeningSocket__ = -1;
            xSemaphoreGive (__tcpServerSemaphore__);
            if (listeningSocket > -1)
                close (listeningSocket);

            // wait until listener task finishes before unloading so that memory variables are still there while it is running
            if (!__serverTimeout__) 
                while (__state__ != NOT_RUNNING) 
                    delay (1);
        }


        // bool () operator to test if tcpServer started sucessfully
        inline operator bool () __attribute__((always_inline)) { return __state__ == RUNNING; }


        // accepts incoming connection
        tcpConnection_t *accept () {
            int connectionSocket;
            char clientIP [INET6_ADDRSTRLEN]; 
            struct sockaddr_storage connectingAddress;
            socklen_t connectingAddressSize = sizeof (connectingAddress); 
            connectionSocket = ::accept (__listeningSocket__, (struct sockaddr *) &connectingAddress, &connectingAddressSize);
            if (connectionSocket == -1) {
                #ifdef __DMESG__
                    if (__listeningSocket__ > -1) 
                        dmesgQueue << (const char *) "[tcpServer] accept error: " << errno << " " << strerror (errno);
                #endif
            } else { // accept OK

                // get client's IP address, determine if connecting address is IPv4 or IPv6 first
                if (connectingAddress.ss_family == AF_INET) { 
                    struct sockaddr_in* s = (struct sockaddr_in *) &connectingAddress; 
                    inet_ntop (AF_INET, &s->sin_addr, clientIP, sizeof (clientIP));
                } else { // AF_INET6 
                    struct sockaddr_in6* s = (struct sockaddr_in6 *) &connectingAddress; 
                    inet_ntop (AF_INET6, &s->sin6_addr, clientIP, sizeof (clientIP));                                            
                }
                // DEBUG: Serial.printf ("Client's IP address: %s\n", clientIP);

                // get server's IP address
                char serverIP [INET6_ADDRSTRLEN] = "";
                struct sockaddr_storage thisAddress = {};
                socklen_t len = sizeof (thisAddress);
                if (getsockname (connectionSocket, (struct sockaddr *) &thisAddress, &len) != -1) {
                    struct sockaddr_in6* s = (struct sockaddr_in6 *) &thisAddress;
                    inet_ntop (AF_INET6, &s->sin6_addr, serverIP, sizeof (serverIP));
                    // this works fine when the connection is IPv6, but when it is a 
                    // IPv4 connection we are getting IPv6 representation of IPv4 address like "::FFFF:10.18.1.140"
                    if (connectingAddress.ss_family == AF_INET) {
                        char *p = serverIP;
                        for (char *q = p; *q; q++)
                            if (*q == ':')
                                p = q;
                        if (p != serverIP)
                            strcpy (serverIP, p + 1);
                    }
                    // DEBUG: Serial.printf ("Server's IP address: %s\n", serverIP);
                }

                // see what firewall thinks about this
                if (__firewallCallback__ && !__firewallCallback__ (clientIP, serverIP)) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpServer] firewall rejected connection from " << clientIP << (const char *) " to " << serverIP;
                    #endif
                    close (connectionSocket);
                } else { // firewall OK   

                    return __createConnectionInstance__ (connectionSocket, clientIP, serverIP);

                } // firewall
            } // accept
            return NULL;
        }


    private:

        int __serverPort__ ;                                            // will be initialized in constructor
        bool (* __firewallCallback__) (char *clientIP, char *serverIP); // will be initialized in constructor

        enum STATE_TYPE { STARTING = 0, NOT_RUNNING = 1 , RUNNING = 2 } __state__ = STARTING;

        int __listeningSocket__ = -1;
        
        time_t __serverTimeout__;                                       // will be initialized in constructor

        virtual tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) { // to be overridded by derived classes
            // DEBUG: Serial.println ("creating tcpConnection");
            tcpConnection_t *connection = new (std::nothrow) tcpConnection_t (connectionSocket, clientIP, serverIP);
            return connection;
        }

    };

#endif
