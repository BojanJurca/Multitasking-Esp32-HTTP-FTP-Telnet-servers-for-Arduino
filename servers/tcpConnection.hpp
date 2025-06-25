  /*

    tcpConnection.hpp

    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    May 22, 2025, Bojan Jurca


    Classes implemented/used in this module:

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


#ifndef __TCP_CONNECTION__
    #define __TCP_CONNECTION__


    #ifdef MODEST_ESP32_MODEL // care must be taken not to put too much burden on such ESP32
        extern SemaphoreHandle_t __tcpServerSemaphore__;
    #endif


    static struct networkTraffic_t {

            struct networkTrafficData_t {
                unsigned long bytesReceived;
                unsigned long bytesSent;
            };

            unsigned long bytesReceived = 0;
            unsigned long bytesSent = 0;
            networkTrafficData_t perSocket [MEMP_NUM_NETCONN];

            // [] operator - maps socket number into [0, MEMP_NUM_NETCONN) interval
            inline networkTrafficData_t &operator [] (int sockfd) __attribute__((always_inline)) {
                return perSocket [sockfd - LWIP_SOCKET_OFFSET];
            }

    } __networkTraffic__;


    class tcpConnection_t {

        public:

            tcpConnection_t () {}

            tcpConnection_t (int connectionSocket, char *clientIP, char *serverIP) { 
                initialize (connectionSocket, clientIP, serverIP);
            }

            ~tcpConnection_t () {
                close ();
            }

            void initialize (int connectionSocket, char *clientIP, char *serverIP) { 
                // create a local copy of parameters for later use
                __connectionSocket__ = connectionSocket;
                strncpy (__clientIP__, clientIP, sizeof (__clientIP__));
                __clientIP__ [sizeof (__clientIP__) - 1] = 0;
                strncpy (__serverIP__, serverIP, sizeof (__serverIP__));
                __serverIP__ [sizeof (__serverIP__) - 1] = 0;

                // reset network traffic counters for connectionSocket
                __networkTraffic__ [connectionSocket] = { 0, 0 };
            }


            // recv with traffic reccording
            int recv (void *buf, size_t len) {
                int receivedThisTime = ::recv (__connectionSocket__, ((char *) buf), len, 0);
                if (receivedThisTime <= 0)
                    switch (errno) {
                        case   0:   // connection closed by peer
                        case  11:   // EAGAIN
                        case 104:   // ECONNRESET
                        case 128:   // ENOTCONN
                                    return receivedThisTime;
                        default:
                                    #ifdef __DMESG__
                                        dmesgQueue << "[tcpConn] error: " << errno << " " << strerror (errno);
                                    #endif
                                    return receivedThisTime;
                    }

                // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
                __networkTraffic__.bytesReceived += receivedThisTime;
                __networkTraffic__ [__connectionSocket__].bytesReceived += receivedThisTime;

                return receivedThisTime; 
            }

            // reads the whole block of bytes
            // returns len if OK
            //           0 if the peer closed the connection
            //          -1 if error occured
            int recvBlock (void *buf, size_t len) { // reads the whole block of bytes, returns len if OK or -1 if error occured
                int receivedTotal = 0;
                int receivedThisTime;

                while (receivedTotal != len) { // read blocks of incoming data
                    receivedThisTime = ::recv (__connectionSocket__, ((char *) buf) + receivedTotal, len - receivedTotal, 0);
                    if (receivedThisTime <= 0)
                        switch (errno) {
                            case   0:   // connection closed by peer
                            case  11:   // EAGAIN
                            case 104:   // ECONNRESET
                            case 128:   // ENOTCONN
                                        return receivedThisTime;
                            default:
                                        #ifdef __DMESG__
                                            dmesgQueue << "[tcpConn] error: " << errno << " " << strerror (errno);
                                        #endif
                                        return receivedThisTime;
                        }

                    receivedTotal += receivedThisTime;

                    // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
                    __networkTraffic__.bytesReceived += receivedThisTime;
                    __networkTraffic__ [__connectionSocket__].bytesReceived += receivedThisTime;
                }
                return receivedTotal; 
            }

            // reads and fills the buffer until endingString is read (also finishes the string with ending 0)
            // returns the number of characters read up to len - 1 if OK
            //                                                 len it he buffer is too small
            //                                                   0 if the peer closed the connection
            //                                                  -1 if error occured
            int recvString (char *buf, size_t len, const char *endingString) {
                int receivedTotal = 0;
                int receivedThisTime;

                while (receivedTotal != len - 1) { // read blocks of incoming data
                    receivedThisTime = ::recv (__connectionSocket__, buf + receivedTotal, len - receivedTotal - 1, 0);
                    if (receivedThisTime <= 0)
                        switch (errno) {
                            case   0:   // connection closed by peer
                            case  11:   // EAGAIN
                            case 104:   // ECONNRESET
                            case 128:   // ENOTCONN
                                        return receivedThisTime;
                            default:
                                        #ifdef __DMESG__
                                            dmesgQueue << "[tcpConn] error: " << errno << " " << strerror (errno);
                                        #endif
                                        return receivedThisTime;
                        }

                    receivedTotal += receivedThisTime;

                    // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
                    __networkTraffic__.bytesReceived += receivedThisTime;
                    __networkTraffic__ [__connectionSocket__].bytesReceived += receivedThisTime;


                    // the following code assumes that the other side sends command or reply (according to the protocol) that ends with endingString
                    buf [receivedTotal] = 0;
                    if (strstr (buf, endingString))
                        return receivedTotal;
                }
                // we read the whole buffer up to len-1 but the ending string did not arrive - buffer is too small
                return len; 
            }

            // checks (but not actually reads) if bytes are pending to be read
            // returns len if yes (also fills the buffer)
            //           0 if no 
            //          -1 if error occured (including the case if the peer closed the connection)
            int peek (void *buf, size_t len) { 
                // with LWIP MSG_PEEK only works it the socket is non-blocking

                int flags = fcntl (__connectionSocket__, F_GETFL, 0); // get current flags
                if (flags < 0)
                    return -1;

                if (fcntl (__connectionSocket__, F_SETFL, flags | O_NONBLOCK) < 0) // set the socket non-blocking
                    return -1;

                int bytesRead = ::recv (__connectionSocket__, (char *) buf, len, MSG_PEEK);
                if (bytesRead < 0) {
                    if (errno != 11) // EAGAIN
                        return -1;
                    else
                        bytesRead = 0;
                }

                if (fcntl (__connectionSocket__, F_SETFL, flags) < 0) // restore original flags
                    return -1;

                return bytesRead;
            }


            // sends the whole block of charcteers
            // returns len in case of OK
            //           0 if the peer closed the connection
            //          -1 in case of error
            int sendBlock (void *buf, size_t len) {

                #ifdef MODEST_ESP32_MODEL // care must be taken not to put too much burden on such ESP32
                    #define MAX_BLOCK_SIZE 360 // 720 // 1440
                #else 
                    #define MAX_BLOCK_SIZE 1440
                #endif            

                int sentTotal = 0;
                int sentThisTime;
                size_t toSendThisTime;

                while (sentTotal != len) {
                    toSendThisTime = len - sentTotal; 
                    if (toSendThisTime > MAX_BLOCK_SIZE) 
                        toSendThisTime = MAX_BLOCK_SIZE; // 1440 is the largest payload that fits into one packet: MTU = 1500, 20 bytes are used for TCP header and 40 for IPv6 header (for IPv4 sligltly less, but we want a general solution), TCP_SND_BUF = 5744 (maximum ESP32 can send in one row) 
                    sentThisTime = send (__connectionSocket__, ((char *) buf) + sentTotal, toSendThisTime, 0);
                    if (sentThisTime <= 0)
                        switch (errno) {
                            case   0:   // connection closed by peer
                            case  11:   // EAGAIN
                            case 104:   // ECONNRESET
                            case 128:   // ENOTCONN
                                        return sentThisTime;
                            default:
                                        #ifdef __DMESG__
                                            dmesgQueue << "[tcpConn] error: " << errno << " " << strerror (errno);
                                        #endif     
                                        return sentThisTime;
                        }

                    sentTotal += sentThisTime;

                    // since no semaphore is used here network traffic logging may not be completely accurate in multitaskin environment
                    __networkTraffic__.bytesSent += sentThisTime;
                    __networkTraffic__ [__connectionSocket__].bytesSent += sentThisTime;

                    if (sentTotal != len)
                        delay (25);
                }
                return sentTotal;
            }

            // sends the whole string of charcteers
            // returns the number of characters sent in case of OK
            //                                     0 if the peer closed the connection
            //                                    -1 in case of error
            int sendString (const char *buf) { 
                return sendBlock ((void *) buf, strlen (buf));
            }


            void close () {
                // close connection socket
                if (__connectionSocket__ != -1) {

                    // reset network traffic counters for connectionSocket - not really needed here but this would make possible error debugging easier
                    __networkTraffic__ [__connectionSocket__] = { 0, 0 };

                    #ifdef MODEST_ESP32_MODEL // care must be taken not to put too much burden on such ESP32
                        xSemaphoreTakeRecursive (__tcpServerSemaphore__, portMAX_DELAY);
                    #endif

                    ::close (__connectionSocket__);

                    #ifdef MODEST_ESP32_MODEL // care must be taken not to put too much burden on such ESP32
                        xSemaphoreGiveRecursive (__tcpServerSemaphore__);
                    #endif

                    __connectionSocket__ = -1;
                }
            }


            inline int getSocket () __attribute__((always_inline)) { return __connectionSocket__; }

            int getTimeout (time_t *seconds) {
                // read timeout value from the socket
                struct timeval tv;
                socklen_t len = sizeof (tv);         
                if (getsockopt (__connectionSocket__, SOL_SOCKET, SO_RCVTIMEO, &tv, &len) < 0) { // SO_RCVTIMEO and SO_SNDTIMEO are the same so it doesn't matter whic one we read
                    return -1; // error
                }
                
                *seconds = tv.tv_sec;       
                return 0;
            }

            int setTimeout (time_t seconds) {
                // set timeout on the socket
                struct timeval tv = { seconds, 0 };
                // set receive timeout                  
                if (setsockopt (__connectionSocket__, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof (tv)) < 0) {
                    return -1;
                } else {
                    // set send timeout
                    if (setsockopt (__connectionSocket__, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof (tv)) < 0) {              
                        return -1;
                    }
                }             
                return 0;
            }

            inline char *getClientIP () __attribute__((always_inline)) { return __clientIP__; }
            inline char *getServerIP () __attribute__((always_inline)) { return __serverIP__; }


        private:

            int __connectionSocket__ = -1;

            time_t __connectionTimeout__ = 0;

            char __clientIP__ [INET6_ADDRSTRLEN] = {};
            char __serverIP__ [INET6_ADDRSTRLEN] = {};

    };


#endif
