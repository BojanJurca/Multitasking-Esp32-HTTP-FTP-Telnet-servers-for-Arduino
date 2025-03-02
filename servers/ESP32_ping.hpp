/*

    ESP32_ping.hpp

    This file is part of Multitasking Esp32 ping class: https://github.com/BojanJurca/Multitasking-Esp32-ping-class

    The librari is based on Jaume Olivé's work: https://github.com/pbecchi/ESP32_ping
    and D. Varrels's work, which is included in Arduino library list: https://github.com/dvarrel

    Both libraries mentioned above do not support the following use-cases, that are needed in Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
      - multitasking
      - intermediate results display
      - IPv6 (not tested, coudn't get IPv6 fully working on Arduino)
    This is the reason for creating ESP32_ping.hpp


    December 25, 2024, Bojan Jurca
    
*/


#ifndef __ESP32_ping_HPP__
    #define __ESP32_ping_HPP__

    #include <WiFi.h>
    #include <lwip/netdb.h>
    #include "lwip/inet_chksum.h"
    #include "lwip/ip.h"
    #include "lwip/icmp.h"

    #ifndef ICMP6_TYPES_H // missing icmp6_types.h
        #define ICMP6_ECHO_REQUEST 128
        #define ICMP6_ECHO_REPLY 129
    #endif // ICMP6_TYPES_H


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


    #ifndef PING_DEFAULT_COUNT
        #define PING_DEFAULT_COUNT     10
    #endif
    #ifndef PING_DEFAULT_INTERVAL
        #define PING_DEFAULT_INTERVAL  1
    #endif
    #ifndef PING_DEFAULT_SIZE
        #define PING_DEFAULT_SIZE     32
    #endif
    #ifndef PING_DEFAULT_TIMEOUT
        #define PING_DEFAULT_TIMEOUT   1
    #endif


    class esp32_ping {

        private:

            bool __isIPv6__ = false;
            char __pingTargetIp__ [INET6_ADDRSTRLEN] = "";  // target's IP number
            struct sockaddr_in __target_addr_IPv4__ = {};   // target's IPv4 address
            struct sockaddr_in6 __target_addr_IPv6__ = {};  // target's IPv6 address

            const char *__errText__ = NULL;                 // the last error 

            // Operational variables
            int __size__;                                   // packet size
            uint32_t __sent__;                              // number of packets sent
            uint32_t __received__;                          // number of packets receivd
            uint32_t __lost__;                              // number of packets lost
            bool __stopped__;                               // request to stop pinging

            // Statistics: mean and variance are computed in an incremental way
            float __elapsed_time__;
            float __min_time__;
            float __max_time__;
            float __mean_time__;
            float __var_time__;
            float __last_mean_time__;


            const char *__resolveTargetName__ (const char *pingTarget) { // returns error text or NULL if OK
                // Resolve name            
                struct addrinfo hints, *res, *p;
                memset (&hints, 0, sizeof (hints)); 
                hints.ai_family = AF_UNSPEC; // AF_INET for IPv4, AF_INET6 for IPv6, AF_UNSPEC for both 
                hints.ai_socktype = SOCK_DGRAM;

                int e = getaddrinfo (pingTarget, NULL, &hints, &res);
                if (e)
                    return gai_strerror (e);

                // IP addresses for serverName
                for (p = res; p != NULL; p = p->ai_next) { 
                    void* addr;
                    if (p->ai_family == AF_INET) { // IPv4 
                        __isIPv6__ = false;
                        struct sockaddr_in *ipv4 = (struct sockaddr_in*) p->ai_addr; 
                        addr = &(ipv4->sin_addr); 
                    } else { // IPv6
                        __isIPv6__ = true;
                        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
                        addr = &(ipv6->sin6_addr);
                    } 
                    inet_ntop (p->ai_family, addr, __pingTargetIp__, sizeof (__pingTargetIp__)); 
                    // DEBUG: Serial.print ("[ping] target's IP: "); Serial.println (__pingTargetIp__);
                    break; // let's just take the first IP address available
                }
                freeaddrinfo (res);

                if (__isIPv6__) {
                    __target_addr_IPv6__ = {};
                    __target_addr_IPv6__.sin6_family = AF_INET6; 
                    __target_addr_IPv6__.sin6_len = sizeof (__target_addr_IPv6__);
                    if (inet_pton (AF_INET6, __pingTargetIp__, &__target_addr_IPv6__.sin6_addr) <= 0) {
                        // DEBUG: Serial.printf ("[ping] invalid IPv6 address %s for %s\n", __pingTargetIp__, pingTarget);
                        return (const char *) "invalid network address";
                    }
                } else {
                    __target_addr_IPv4__ = {};
                    __target_addr_IPv4__.sin_family = AF_INET;
                    __target_addr_IPv4__.sin_len = sizeof (__target_addr_IPv4__);
                    if (inet_pton (AF_INET, __pingTargetIp__, &__target_addr_IPv4__.sin_addr) <= 0) {          
                        // DEBUG: Serial.printf ("[ping] invalid IPv4 address %s for %s\n", __pingTargetIp__, pingTarget);
                        return (const char *) "invalid network address";
                    }
                }
                return NULL; // OK
            }


            // Data structure that keeps reply information form different (simultaneous) pings
            struct __pingReply_t__ {
                uint16_t seqno;             // packet sequence number
                unsigned long elapsed_time; // in microseconds
            };
            static __pingReply_t__ __pingReplies__ [MEMP_NUM_NETCONN];   // there can only be as much replies as there are sockets available


        public:

            esp32_ping () {}
            esp32_ping (const char *pingTarget) {
                __errText__ = __resolveTargetName__ (pingTarget);
            }


            const char *ping (const char *pingTarget, int count = PING_DEFAULT_COUNT, int interval = PING_DEFAULT_INTERVAL, int size = PING_DEFAULT_SIZE, int timeout = PING_DEFAULT_TIMEOUT) {
                __errText__ = __resolveTargetName__ (pingTarget);
                if (__errText__ != NULL)
                    return __errText__;
                else
                    return ping (count, interval, size, timeout);
            }

            const char *ping (int count = PING_DEFAULT_COUNT, int interval = PING_DEFAULT_INTERVAL, int size = PING_DEFAULT_SIZE, int timeout = PING_DEFAULT_TIMEOUT) {
                // check argument values
                if (count < 0)                        return (const char *) "invalid value"; // note that count = 0 is valid argument value, meaning infinite pinging
                if (interval < 1 || interval > 3600)  return (const char *) "invalid value"; 
                if (size < 4 || size > 256)           return (const char *) "invalid value"; // max ping size is 65535 but ESP32 can't handle it
                if (timeout < 1 || timeout > 30)      return (const char *) "invalid value";
                
                __size__ = size;

                // initialize measuring variables
                __sent__ = 0; 
                __received__ = 0;
                __lost__ = 0;
                __stopped__ = false;
                __min_time__ = 1.E+9; // FLT_MAX;
                __max_time__ = 0.0;
                __mean_time__ = 0.0;
                __var_time__ = 0.0;

                // create socket
                int sockfd;
                if (__isIPv6__) {
                    if ((sockfd = socket (AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
                        return (__errText__ = strerror (errno));
                } else {
                    if ((sockfd = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
                        return (__errText__ = strerror (errno));
                }

                // Make the socket non-blocking, so we can detect time-out later     
                if (fcntl (sockfd, F_SETFL, O_NONBLOCK) == -1) {
                    __errText__ = strerror (errno);
                    close (sockfd);
                    return __errText__;
                }

                // begin ping ...
                for (uint16_t seqno = 1; (seqno <= count || count == 0) && !__stopped__; seqno ++) {
                    unsigned long sendMillis = millis ();

                    __errText__ = __ping_send__ (sockfd, seqno, size);
                    if (__errText__ != NULL) {
                        return __errText__;
                    } else {
                        __sent__ ++;
                        int bytesReceived;
                        __ping_recv__ (sockfd, &bytesReceived, 1000000 * timeout);
                        if (__pingReplies__ [sockfd - LWIP_SOCKET_OFFSET].elapsed_time) { // > 0, meaning that echo reply packet has been received
                            __received__ ++;

                            // Update statistics
                            __elapsed_time__ = (float) __pingReplies__ [sockfd - LWIP_SOCKET_OFFSET].elapsed_time / (float) 1000;
                            // Mean and variance are computed in an incremental way
                            if (__elapsed_time__ < __min_time__)
                                __min_time__ = __elapsed_time__;

                            if (__elapsed_time__ > __max_time__)
                                __max_time__ = __elapsed_time__;

                            __last_mean_time__ = __mean_time__;
                            __mean_time__ = (((__received__ - 1) * __mean_time__) + __elapsed_time__) / __received__;

                            if (__received__ > 1)
                                __var_time__ = __var_time__ + ((__elapsed_time__ - __last_mean_time__) * (__elapsed_time__ - __mean_time__));

                            // print ... Serial.printf ("%d bytes from %s: icmp_seq=%d time=%.3f ms\n", bytesReceived, __pingTargetIp__, seqno, __elapsed_time__);

                        } else {
                            __lost__ ++;
                            __elapsed_time__ = 0;
                        }

                        // report intermediate results 
                        onReceive (bytesReceived);
                    }

                    if (seqno <= count || count == 0) { 
                        while ((millis () - sendMillis < 1000L * interval) && (!__stopped__)) {
                            // report waiting 
                            onWait ();
                            delay (10);
                        }
                    }
                }

                closesocket (sockfd);
                return NULL; // OK
            }

            // returns the target IP beeing pinged
            inline char *target () __attribute__((always_inline)) { return __pingTargetIp__; }

            // returns the size of ping packet
            inline int size () __attribute__((always_inline)) { return __size__; }

            // request to stop pinging
            inline void stop () __attribute__((always_inline)) { __stopped__ = true; }

            // statistics: mean and variance are computed in an incremental way
            inline uint32_t sent () __attribute__((always_inline)) { return __sent__; }
            inline uint32_t received () __attribute__((always_inline)) { return __received__; }
            inline uint32_t lost () __attribute__((always_inline)) { return __lost__; }
            inline float elapsed_time () __attribute__((always_inline)) { return __elapsed_time__; }
            inline float min_time () __attribute__((always_inline)) { return __min_time__; }
            inline float max_time () __attribute__((always_inline)) { return __max_time__; }
            inline float mean_time () __attribute__((always_inline)) { return __mean_time__; }
            inline float var_time () __attribute__((always_inline)) { return __var_time__; }

            // error reporting
            inline const char *errText () __attribute__((always_inline)) { return __errText__; }

            // reporting intermediate results - override (use) these functions if needed
            virtual void onReceive (int bytes) {}
            virtual void onWait () {}


        private:

            err_t __errno__ = ERR_OK;     // the last error number of socket operations

            const char *__ping_send__ (int sockfd, uint16_t seqno, int size) { // returns error text or NULL if OK
                int sent;
                int ping_size;

                if (__isIPv6__) {

                    struct icmp6_echo_hdr *iecho;
                    ping_size = sizeof (struct icmp6_echo_hdr) + size;

                    // Construct ping block
                    // - First there is struct icmp_echo_hdr (https://github.com/ARMmbed/lwip/blob/master/src/include/lwip/prot/icmG.h). We'll use these fields:
                    //    - uint16_t id      - this is where we'll keep the socket number so that we would know from where ping packet has been send when we receive a reply 
                    //    - uint16_t seqno   - each packet gets it sequence number so we can distinguish one packet from another when we receive a reply
                    //    - uint16_t chksum  - needs to be calcualted
                    // - Then we'll add the payload:
                    //    - unsigned long micros  - this is where we'll keep the time packet has been sent so we can calcluate round-trip time when we receive a reply
                    //    - unimportant data, just to fill the payload to the desired length

                    iecho = (struct icmp6_echo_hdr *) mem_malloc ((mem_size_t) ping_size);
                    if (!iecho)
                        return (const char *) "out of memory";
            
                    // initialize structure where the reply information will be stored when it arrives
                    __pingReplies__ [sockfd - LWIP_SOCKET_OFFSET] = { seqno, 0 };

                    // prepare echo packet
                    size_t data_len = ping_size - sizeof (struct icmp6_echo_hdr);
                    iecho->type = ICMP6_ECHO_REQUEST; 
                    iecho->code = 0; 
                    iecho->chksum = 0;
                    iecho->id = sockfd; // id = socket
                    iecho->seqno = seqno; // no need to call htons(seqno), we'll get an echo reply back to the same machine

                    // store micros at send time
                    unsigned long sendMicros = micros ();
                    *(unsigned long *) &((char *) iecho) [sizeof (struct icmp6_echo_hdr)] = sendMicros;

                    // fill the additional data buffer with some data
                    for (int i = sizeof (sendMicros); i < data_len; i++)
                        ((char*) iecho) [sizeof (struct icmp6_echo_hdr) + i] = (char) i;
                    // claculate checksum
                    iecho->chksum = inet_chksum (iecho, ping_size);

                    // send the packet
                    sent = sendto (sockfd, iecho, ping_size, 0, (struct sockaddr*) &__target_addr_IPv6__, sizeof (__target_addr_IPv6__));

                    mem_free (iecho);

                } else {

                    struct icmp_echo_hdr *iecho;
                    ping_size = sizeof (struct icmp_echo_hdr) + size;

                    // Construct ping block
                    // - First there is struct icmp_echo_hdr (https://github.com/ARMmbed/lwip/blob/master/src/include/lwip/prot/icmG.h). We'll use these fields:
                    //    - uint16_t id      - this is where we'll keep the socket number so that we would know from where ping packet has been send when we receive a reply 
                    //    - uint16_t seqno   - each packet gets it sequence number so we can distinguish one packet from another when we receive a reply
                    //    - uint16_t chksum  - needs to be calcualted
                    // - Then we'll add the payload:
                    //    - unsigned long micros  - this is where we'll keep the time packet has been sent so we can calcluate round-trip time when we receive a reply
                    //    - unimportant data, just to fill the payload to the desired length

                    iecho = (struct icmp_echo_hdr *) mem_malloc ((mem_size_t) ping_size);
                    if (!iecho)
                        return (const char *) "out of memory";
            
                    // initialize structure where the reply information will be stored when it arrives
                    __pingReplies__ [sockfd - LWIP_SOCKET_OFFSET] = { seqno, 0 };

                    // prepare echo packet
                    size_t data_len = ping_size - sizeof (struct icmp_echo_hdr);
                    iecho->type = ICMP_ECHO; // ICMPH_TYPE_SET (iecho, ICMP_ECHO);
                    iecho->code = 0; // ICMPH_CODE_SET (iecho, 0);
                    iecho->chksum = 0;
                    iecho->id = sockfd; // id = socket
                    iecho->seqno = seqno; // no need to call htons(seqno), we'll get an echo reply back to the same machine
                    // store micros at send time
                    unsigned long sendMicros = micros ();
                    *(unsigned long *) &((char *) iecho) [sizeof (struct icmp_echo_hdr)] = sendMicros;
                    // fill the additional data buffer with some data
                    for (int i = sizeof (sendMicros); i < data_len; i++)
                        ((char*) iecho) [sizeof (struct icmp_echo_hdr) + i] = (char) i;
                    // claculate checksum
                    iecho->chksum = inet_chksum (iecho, ping_size);

                    // send the packet
                    sent = sendto (sockfd, iecho, ping_size, 0, (struct sockaddr*) &__target_addr_IPv4__, sizeof (__target_addr_IPv4__));

                    mem_free (iecho);
                }

                if (sent != ping_size)
                    return (const char *) "couldn't sendto";
                
                return NULL; // OK
            }


            const char *__ping_recv__ (int sockfd, int *bytes, unsigned long timeoutMicros) { // returns error text or NULL if OK
                char buf [300];

                char fromIp [INET6_ADDRSTRLEN];     // should be the same as __pingTargetIp__
                struct sockaddr_in from_addr_IPv4;  // should be the same as __target_addr_IPv4__
                struct sockaddr_in6 from_addr_IPv6; // should be the same as __target_addr_IPv6__
                int fromlen;

                unsigned long startMicros = micros (); // to calculate time-out

                // receive the echo packet
                while (true) {

                    // did some other process poick up our echo reply and already done the job for us? 
                    if (__pingReplies__ [sockfd - LWIP_SOCKET_OFFSET].elapsed_time) { // > 0
                        // DEBUG: Serial.printf ("Another process already did the job for socket %i, returning\n", sockfd);
                        return NULL; // OK
                    }

                    // read echo packet without waiting
                    if (__isIPv6__)
                        *bytes = recvfrom (sockfd, buf, sizeof (buf), 0, (struct sockaddr*) &from_addr_IPv6, (socklen_t *) &fromlen);
                    else
                        *bytes = recvfrom (sockfd, buf, sizeof (buf), 0, (struct sockaddr*) &from_addr_IPv4, (socklen_t *) &fromlen);

                    if (*bytes <= 0) {
                        if ((errno == EAGAIN || errno == ENAVAIL) && (micros () - startMicros < timeoutMicros)) {
                            delay (1); // 1 ms is too long time for accurte measuring but waiting 0 ms (or yield) would trigger IDLE task's watchdog
                            continue; // not time-out yet
                        } 
                        return (const char *) "timeout";
                    }

                    // get 'from' IP address
                    /*
                    if (__isIPv6__) { 
                        inet_ntop (AF_INET6, &from_addr_IPv6.sin6_addr, fromIp, sizeof (fromIp)); // should be the same as __pingTargetIp__
                        // DEBUG: 
                        Serial.printf ("Received %i IPv6 bytes from %s\n", *bytes, fromIp);
                    } else { 
                        inet_ntop (AF_INET, &from_addr_IPv4.sin_addr, fromIp, sizeof (fromIp)); // should be the same as __pingTargetIp__
                        // DEBUG: 
                        Serial.printf ("Received %i IPv4 bytes from %s\n", *bytes, fromIp);
                    }
                    */

                    // did we get at least all the data that we need?
                    byte type;
                    int id;
                    uint16_t seqno;
                    unsigned long sentMicros;

                    if (__isIPv6__) {

                        if (*bytes < (int) (sizeof (struct ip6_hdr) + sizeof (struct icmp6_echo_hdr)) + sizeof (unsigned long)) {
                            // DEBUG: Serial.printf ("Echo packet received from %s is too short\n", fromIp);
                            continue;
                        }

                        // get echo
                        struct ip6_hdr *ip6hdr;
                        struct icmp6_echo_hdr *iecho = NULL;

                        ip6hdr = (struct ip6_hdr *) buf;
                        iecho = (struct icmp6_echo_hdr *) (buf + 40); // IPv6 internet header has 40 bytes

                        type = iecho->type;
                        id = iecho->id;
                        seqno = iecho->seqno;
                        sentMicros = *(unsigned long *) &((char *) iecho) [sizeof (struct icmp6_echo_hdr)];

                        // subtract IPv6 internet header and struct icmp6_echo_hdr header from *bytes
                        *bytes -= (40 + sizeof (struct icmp6_echo_hdr));

                    } else {

                        if (*bytes < (int) (sizeof (struct ip_hdr) + sizeof (struct icmp_echo_hdr)) + sizeof (unsigned long)) {
                            // DEBUG: Serial.printf ("Echo packet received from %s is too short\n", fromIp);
                            continue;
                        }

                        // get echo
                        struct ip_hdr *iphdr;
                        struct icmp_echo_hdr *iecho = NULL; 

                        iphdr = (struct ip_hdr *) buf;
                        iecho = (struct icmp_echo_hdr *) (buf + (IPH_HL (iphdr) * 4)); // IPH_HL specifies the length of the IPv4 header in 32-bit words

                        type = iecho->type;
                        id = iecho->id;
                        seqno = iecho->seqno;
                        sentMicros = *(unsigned long *) &((char *) iecho) [sizeof (struct icmp_echo_hdr)];

                        // subtract IPv4 internet header and icmp_echo_hdr from *bytes
                        *bytes -= (IPH_HL (iphdr) * 4 + sizeof (struct icmp_echo_hdr));

                    }

                    // check if this is a reply we expected
                    if (id < LWIP_SOCKET_OFFSET || id >= LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN || !(type == ICMP_ER || type == ICMP6_ECHO_REPLY)) {
                        // DEBUG: Serial.printf ("Echo packet received from %s does not have the correct id\n", fromIp);
                        continue;
                    }

                    // now we should consider several options:

                        // did we pick up the echo packet that was send through the socket sockfd?
                        if (id == sockfd) { // we picked up the echo packet that was sent from the same socket

                            // did we pick up the echo packet with the latest sequence number?
                            if (__pingReplies__ [sockfd - LWIP_SOCKET_OFFSET].seqno == seqno) {
                                // DEBUG: Serial.printf ("Echo packet %u received trough socket %i\n", seqno, sockfd);

                                // Write information about the reply in data structure
                                __pingReplies__ [sockfd - LWIP_SOCKET_OFFSET].elapsed_time = micros () - sentMicros; 

                                return NULL; // OK

                            } else { // sequence numbers do not match, ignore this echo packet, its time-out has probably already been reported
                                // DEBUG: Serial.printf ("Echo packet %u arrived too late (%lu us) trough socket %i\n", seqno, micros () - sentMicros, sockfd);
                            }

                        } else { // we picked up an echo packet that was sent from another socket
                            // DEBUG: Serial.printf ("Echo packet %u, that belongs to another socket %i received through socket %i\n", seqno, id, sockfd);

                            // Did we pick up the echo packet with the latest sequence number?
                            if (__pingReplies__ [id - LWIP_SOCKET_OFFSET].seqno == seqno) {

                                // write information about the reply in data structure
                                __pingReplies__ [id - LWIP_SOCKET_OFFSET].elapsed_time = micros () - sentMicros; 

                                // do not return now, continue waiting for the right echo packet
 
                            } else { // sequence numbers do not match, ignore this echo packet, its time-out has probably already been reported
                                // DEBUG: Serial.printf ("Echo packet %u, that belongs to another socket %i arrived too late (%lu us) through socket %i\n", seqno, id, micros () - sentMicros, sockfd);
                            }

                        }

                }

            }

    };

    // declare static member outside of class definition
    esp32_ping::__pingReply_t__ esp32_ping::__pingReplies__ [MEMP_NUM_NETCONN];

#endif