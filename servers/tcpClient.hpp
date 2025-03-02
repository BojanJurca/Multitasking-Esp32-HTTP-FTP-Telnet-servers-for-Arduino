/*

    tcpClient.hpp

    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino

    February 6, 2025, Bojan Jurca

    Classes implemented/used in this module:

        tcpServer_t
        tcpConnection_t

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

*/


#include <lwip/netdb.h>
#include "tcpConnection.hpp"


#ifndef __TCP_CLIENT__
    #define __TCP_CLIENT__


    #ifndef __NETWK__

        #define EAGAIN 11
        #define ENAVAIL 119

        // missing gai_strerror function
        #include <lwip/netdb.h>
        const char *gai_strerror (int err) {
            switch (err) {
                case EAI_AGAIN:     return (const char *) "temporary failure in name resolution";
                case EAI_BADFLAGS:  return (const char *) "invalid value for ai_flags field";
                case EAI_FAIL:      return (const char *) "non-recoverable failure in name resolution";
                case EAI_FAMILY:    return (const char *) "ai_family not supported";
                case EAI_MEMORY:    return (const char *) "memory allocation failure";
                case EAI_NONAME:    return (const char *) "name or service not known";
                // not in lwip: case EAI_OVERFLOW:  return (const char *) "argument buffer overflow";
                case EAI_SERVICE:   return (const char *) "service not supported for ai_socktype";
                case EAI_SOCKTYPE:  return (const char *) "ai_socktype not supported";
                // not in lwip: case EAI_SYSTEM:    return (const char *) "system error returned in errno";
                default:            return (const char *) "invalid gai_errno code";
            }
        }
          
    #endif    


    class tcpClient_t : public tcpConnection_t {

        private:

            const char *__errText__;


        public:


            tcpClient_t (const char *serverName, int serverPort, long connectionTimeout) : tcpConnection_t () {

                __errText__ = NULL; // assume OK

                if (WiFi.status () != WL_CONNECTED) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpClient] " << (const char *) "not connected to WiFi";
                    #endif
                    __errText__ = "Not connected to WiFi";
                    return;
                }

                // resolve server name
                struct addrinfo hints, *res, *p;
                char serverIP [INET6_ADDRSTRLEN] = "";
                memset (&hints, 0, sizeof (hints)); 
                hints.ai_family = AF_UNSPEC; // AF_INET for IPv4, AF_INET6 for IPv6, AF_UNSPEC for both 
                hints.ai_socktype = SOCK_STREAM;

                int status = getaddrinfo (serverName, NULL, &hints, &res); 
                if (status != 0) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpClient] " << gai_strerror (status);
                    #endif
                    __errText__ = gai_strerror (status);
                    return;
                }

                // get IP addresses for serverName
                bool isIPv6;
                for (p = res; p != NULL; p = p->ai_next) {
                    void* addr; 
                    if (p->ai_family == AF_INET) { // IPv4 
                        isIPv6 = false;
                        struct sockaddr_in *ipv4 = (struct sockaddr_in*) p->ai_addr; 
                        addr = &(ipv4->sin_addr); 
                    } else { // IPv6 
                        isIPv6 = true;
                        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr; 
                        addr = &(ipv6->sin6_addr); 
                    } 
                    inet_ntop (p->ai_family, addr, serverIP, sizeof (serverIP)); 
                    // DEBUG: Serial.print ("[tcpClient] server's IP: "); Serial.println (serverIP);
                    break; // let's just take the first IP address available
                }
                freeaddrinfo (res);

                // create socket
                int connectionSocket;
                if (isIPv6)
                    connectionSocket = socket (AF_INET6, SOCK_STREAM, 0);
                else
                    connectionSocket = socket (AF_INET, SOCK_STREAM, 0);
                if (connectionSocket < 0) {
                    __errText__ = strerror (errno);                  
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[tcpClient] " << __errText__;
                    #endif
                    return;
                }


                    // get client's IP address
                    char clientIP [INET6_ADDRSTRLEN] = "";
                    struct sockaddr_storage thisAddress = {};
                    socklen_t len = sizeof (thisAddress);
                    if (getsockname (connectionSocket, (struct sockaddr *) &thisAddress, &len) != -1) {
                        struct sockaddr_in6* s = (struct sockaddr_in6 *) &thisAddress;
                        inet_ntop (AF_INET6, &s->sin6_addr, clientIP, sizeof (clientIP));
                        // this works fine when the connection is IPv6, but when it is a 
                        // IPv4 connection we are getting IPv6 representation of IPv4 address like "::FFFF:10.18.1.140"
                        if (!isIPv6) {
                            char *p = clientIP;
                            for (char *q = p; *q; q++)
                                if (*q == ':')
                                    p = q;
                            if (p != clientIP)
                                strcpy (clientIP, p + 1);
                        }
                        // DEBUG: Serial.printf ("Server's IP address: %s\n", serverIP);
                    }


                tcpConnection_t::initialize (connectionSocket, clientIP, serverIP);

                tcpConnection_t::setTimeout (connectionTimeout);


                // connect to the server
                if (isIPv6) {
                      struct sockaddr_in6 server_addr = {};
                      server_addr.sin6_family = AF_INET6;
                      server_addr.sin6_len = sizeof (server_addr);
                      if (inet_pton (AF_INET6, serverIP, &server_addr.sin6_addr) <= 0) {
                          __errText__ = (const char *) "Invalid network address";
                          #ifdef __DMESG__
                              dmesgQueue << (const char *) "[tcpClient] " << "invalid network address " << serverIP;
                          #endif
                          return;
                      }
                      server_addr.sin6_port = htons (serverPort);
                      if (connect (connectionSocket, (struct sockaddr*) &server_addr, sizeof (server_addr)) < 0) {
                          if (errno != EINPROGRESS) {
                              __errText__ = strerror (errno);
                              #ifdef __DMESG__
                                  dmesgQueue << (const char *) "[tcpClient] " << __errText__;
                              #endif
                              return;
                          }
                      }
                } else {
                    struct sockaddr_in server_addr = {};
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_len = sizeof (server_addr);
                    if (inet_pton (AF_INET, serverIP, &server_addr.sin_addr) <= 0) {          
                        __errText__ =  (const char *) "Invalid network address";
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[tcpClient] " << "invalid network address " << serverIP;
                        #endif
                        return;
                    }
                    server_addr.sin_port = htons (serverPort);                  
                    if (connect (connectionSocket, (struct sockaddr*) &server_addr, sizeof (server_addr)) < 0) {
                        __errText__ =  __errText__ = strerror (errno);
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[tcpClient] " << __errText__;
                        #endif
                        return;
                    }
                }
            }


            // error reporting
            inline operator bool () __attribute__((always_inline)) { return __errText__ != NULL; }
            inline const char *errText () __attribute__((always_inline)) { return __errText__; }

    };

#endif
