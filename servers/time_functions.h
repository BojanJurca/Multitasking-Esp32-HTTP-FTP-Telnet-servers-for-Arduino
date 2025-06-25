  /*

    time_functions.h
 
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino

    Cron daemon synchronizes the time with NTP server accessible from internet once a day.

    May 22, 2025, Bojan Jurca

    Nomenclature used in time_functions.h - for easier understaning of the code:

      - "buffer size" is the number of bytes that can be placed in a buffer. 

                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".      

      - "max length" is the number of characters that can be placed in a variable.

                  In case of C 0-terminated strings the terminating 0 character is not included so the buffer should be at least 1 byte larger.    
*/


#include <WiFi.h>
#include <lwip/netdb.h>
#include <time.h>
#include <lwip/netdb.h>
#include "std/Cstring.hpp"
#include "std/console.hpp"


#ifndef __TIME_FUNCTIONS__
    #define __TIME_FUNCTIONS__

    #ifndef TZ
        #ifdef TIMEZONE        
            #define TZ TIMEZONE
        #else
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "TZ was not defined previously, #defining the default CET-1CEST,M3.5.0,M10.5.0/3 in time_functions.h"
            #endif
            #define TZ "CET-1CEST,M3.5.0,M10.5.0/3" // default: Europe/Ljubljana, or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
        #endif
    #endif

    #ifndef __FILE_SYSTEM__
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Compiling time_functions.h without file system (fileSystem.hpp), time_functions.h will not use configuration files"
        #endif
    #endif


    // ----- functions and variables in this modul -----

    time_t time ();
    void setTimeOfDay (time_t);
    struct tm gmTime (time_t);
    struct tm localTime (time_t);
    char *ascTime (const struct tm, char *buf, size_t len);
    time_t getUptime ();
    const char *ntpDate (const char *);
    const char *ntpDate ();
    [[deprecated("Use cronTab.insert instead.")]]
    bool cronTabAdd (uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, const char *, bool);
    [[deprecated("Use cronTab.insert instead.")]]
    bool cronTabAdd (const char *, bool);
    [[deprecated("Use cronTab.erase instead.")]]
    int cronTabDel (const char *);
    [[deprecated("Use cronDaemon.start instead.")]]
    void startCronDaemon (void (* cronHandlerCallback) (const char *));


    // TUNNING PARAMETERS

    // NTP servers that ESP32 server is going to sinchronize its time with
    #ifndef DEFAULT_NTP_SERVER_1
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "DEFAULT_NTP_SERVER_1 was not defined previously, #defining the default 1.si.pool.ntp.org in time_finctions.h"
        #endif
        #define DEFAULT_NTP_SERVER_1    "1.si.pool.ntp.org"
    #endif
    #ifndef DEFAULT_NTP_SERVER_2
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "DEFAULT_NTP_SERVER_2 was not defined previously, #defining the default 2.si.pool.ntp.org in time_finctions.h"
        #endif
      #define DEFAULT_NTP_SERVER_2      "2.si.pool.ntp.org"
    #endif
    #ifndef DEFAULT_NTP_SERVER_3
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "DEFAULT_NTP_SERVER_3 was not defined previously, #defining the default 3.si.pool.ntp.org in time_finctions.h"
        #endif
        #define DEFAULT_NTP_SERVER_3    "3.si.pool.ntp.org"
    #endif

    #define MAX_ETC_NTP_CONF_SIZE 1 * 1024              // 1 KB will usually do - initialization reads the whole /etc/ntp.conf file in the memory 
                                                      
    // crontab definitions
    #define CRON_DAEMON_STACK_SIZE 9 * 1024             // a good first estimate how to set this parameter would be to always leave at least 1 KB of cronDaemon stack unused 
    #define CRONTAB_MAX_ENTRIES 32                      // this defines the size of cron table - increase this number if you going to handle more cron events
    #define CRON_COMMAND_MAX_LENGTH 48                  // the number of characters in the longest cron command
    #define ANY 255                                     // byte value that corresponds to * in cron table entries (should be >= 60 so it can be distinguished from seconds, minutes, ...)
    #define MAX_TZ_ETC_NTP_CONF_SIZE CRONTAB_MAX_ENTRIES * (20 + CRON_COMMAND_MAX_LENGTH) // this should do
  

    // ----- CODE -----

    bool __timeHasBeenSet__ = false;
    RTC_DATA_ATTR time_t __startupTime__;

    char __ntpServer1__ [255] = DEFAULT_NTP_SERVER_1; // DNS host name may have max 253 characters
    char __ntpServer2__ [255] = DEFAULT_NTP_SERVER_2; // DNS host name may have max 253 characters
    char __ntpServer3__ [255] = DEFAULT_NTP_SERVER_3; // DNS host name may have max 253 characters 


    // ----- NTP CLIENT -----

    // synchronizes time with NTP server, returns error message
    const char *ntpDate (const char *ntpServer) {
        if (WiFi.status () != WL_CONNECTED) {
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[NTP] not connected to WiFi";
            #endif
            return (const char *) "[NTP] not connected to WiFi";
        }

        // Based on Let's make a NTP Client in C: https://lettier.github.io/posts/2016-04-26-lets-make-a-ntp-client-in-c.html
        // which I'm keeping here as close to the original as possible due to its comprehensive explanation.

        // »Network Time Protocol«

        typedef struct {
            uint8_t li_vn_mode;       // Eight bits. li, vn, and mode.
                                      // li.   Two bits.   Leap indicator.
                                      // vn.   Three bits. Version number of the protocol.
                                      // mode. Three bits. Client will pick mode 3 for client.
            uint8_t stratum;          // Eight bits. Stratum level of the local clock.
            uint8_t poll;             // Eight bits. Maximum interval between successive messages.
            uint8_t precision;        // Eight bits. Precision of the local clock.
            uint32_t rootDelay;       // 32 bits. Total round trip delay time.
            uint32_t rootDispersion;  // 32 bits. Max error aloud from primary clock source.
            uint32_t refId;           // 32 bits. Reference clock identifier.
            uint32_t refTm_s;         // 32 bits. Reference time-stamp seconds.
            uint32_t refTm_f;         // 32 bits. Reference time-stamp fraction of a second.
            uint32_t origTm_s;        // 32 bits. Originate time-stamp seconds.
            uint32_t origTm_f;        // 32 bits. Originate time-stamp fraction of a second.
            uint32_t rxTm_s;          // 32 bits. Received time-stamp seconds.
            uint32_t rxTm_f;          // 32 bits. Received time-stamp fraction of a second.
            uint32_t txTm_s;          // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
            uint32_t txTm_f;          // 32 bits. Transmit time-stamp fraction of a second.
        } ntp_packet;                 // Total: 384 bits or 48 bytes.

        // »The NTP message consists of a 384 bit or 48 byte data structure containing 17 fields. Note that the order of li, vn, and mode 
        //  is important. We could use three bit fields but instead we’ll combine them into a single byte to avoid any implementation-defined 
        //  issues involving endianness, LSB, and/or MSB.«

        // »Populate our Message«

        // Create and zero out the packet. All 48 bytes worth.
        ntp_packet packet = {};
        // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.
        *((char *) &packet + 0) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.

        // »First we zero-out or clear out the memory of our structure and then fill it in with leap indicator zero, version number three, and mode 3. 
        // The rest we can leave blank and still get back the time from the server.«

        // »Setup our Socket and Server Data Structure«

        // Create a UDP socket, convert the host-name to an IP address, set the port number,
        // connect to the server, send the packet, and then read in the return packet.

        struct addrinfo hints, *res, *p;
        char ipstr [INET6_ADDRSTRLEN];
        memset (&hints, 0, sizeof (hints)); 
        hints.ai_family = AF_UNSPEC; // AF_INET for IPv4, AF_INET6 for IPv6, AF_UNSPEC for both 
        hints.ai_socktype = SOCK_DGRAM;

        int status = getaddrinfo (ntpServer, NULL, &hints, &res); 
        if (status != 0)
            return "[NTP] getaddrinfo error"; // + gai_strerror (status);

        // IP addresses for serverName
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
            inet_ntop (p->ai_family, addr, ipstr, sizeof (ipstr)); 
            // DEBUG: Serial.print ("[NTP] NTP server's IP: "); Serial.println (ipstr);
            break; // let's just take the first IP address available
        }
        freeaddrinfo (res);

        // create socket
        int sockfd;
        if (isIPv6)
            sockfd = socket (AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
        else
            sockfd = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sockfd < 0) {
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[NTP] socket error: " << errno << " " << strerror (errno);
            #endif
            return (const char *) "Could not open a socket";
        }

        // Setup socket timeout to 1s, this will limit the time we wait for NTP reply
        struct timeval tout = {1, 0};
        if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof (tout)) < 0) {
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[NTP] setsockopt error: " << errno << strerror (errno);
            #endif
            close (sockfd);
            return (const char *) "Could not set socket time-out";
        }

        // connect to server
        if (isIPv6) {
            struct sockaddr_in6 serv_addr = {}; // zero out the server address structure.
            serv_addr.sin6_family = AF_INET6; 
            serv_addr.sin6_port = htons (123); // convert the port number integer to network big-endian style and save it to the server address structure.
            if (inet_pton (AF_INET6, ipstr, &serv_addr.sin6_addr) <= 0) { 
                close (sockfd);
                return (const char *) "[NTP] invalid or not supported address"; // + ipstr;
            } 

            // »Before we can start communicating we have to setup our socket, server and server address structures. We will be using the 
            //  User Datagram Protocol (versus TCP) for our socket since the server we are sending our message to is listening on port 
            //  number 123 using UDP.«

            // Call up the server using its IP address and port number.
            if (connect (sockfd, (struct sockaddr*) &serv_addr, sizeof (serv_addr)) < 0) { 
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[NTP] connect error: " << errno << " " << strerror (errno);
                #endif          
                close (sockfd);
                return (const char *) "Could not connect to NTP server";
            }

            // »Send our Message to the Server«

            // Send it the NTP packet it wants. If n == -1, it failed.
            int n;
            n = sendto (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
            if (n < 0) {
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[NTP] sendto error: " << errno << " " << strerror (errno);
                #endif
                close (sockfd);
                return (const char *) "Could not sent NTP packet";
            }
      
            // »With our message payload, socket, server and address setup, we can now send our message to the server. 
            //  To do this, we write our 48 byte struct to the socket.«

            // »Read in the Return Message«

            // Wait and receive the packet back from the server. If n == -1, it failed.

            struct sockaddr_in6 from;
            int fromlen = sizeof (from);
            n = recvfrom (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &from, (socklen_t *) &fromlen);
            if (n < 0) {
              #ifdef __DMESG__
                    dmesgQueue << (const char *) "[NTP] recvfrom error: " <<  errno << " " << strerror (errno);
                #endif          
                close (sockfd);
                return (const char *) "Could not receive NTP packet";
            }
        } else {
            struct sockaddr_in serv_addr = {}; // zero out the server address structure.
            serv_addr.sin_family = AF_INET; 
            serv_addr.sin_port = htons (123); // convert the port number integer to network big-endian style and save it to the server address structure.
            if (inet_pton (AF_INET, ipstr, &serv_addr.sin_addr) <= 0) { 
                close (sockfd);
                return (const char *) "[NTP] invalid or not supported address"; // + ipstr;
            }

            // »Before we can start communicating we have to setup our socket, server and server address structures. We will be using the 
            //  User Datagram Protocol (versus TCP) for our socket since the server we are sending our message to is listening on port 
            //  number 123 using UDP.«

            // Call up the server using its IP address and port number.
            if (connect (sockfd, (struct sockaddr*) &serv_addr, sizeof (serv_addr)) < 0) { 
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[NTP] connect error: " << errno << " " << strerror (errno);
                #endif          
                close (sockfd);
                return (const char *) "Could not connect to NTP server";
            } 

            // »Send our Message to the Server«

            // Send it the NTP packet it wants. If n == -1, it failed.
            int n;
            n = sendto (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
            if (n < 0) {
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[NTP] sendto error: " << errno << " " << strerror (errno);
                #endif          
                close (sockfd);
                return (const char *) "Could not sent NTP packet";
            }
      
            // »With our message payload, socket, server and address setup, we can now send our message to the server. 
            //  To do this, we write our 48 byte struct to the socket.«

            // »Read in the Return Message«

            // Wait and receive the packet back from the server. If n == -1, it failed.

            struct sockaddr_in from;
            int fromlen = sizeof (from);
            n = recvfrom (sockfd, (char *) &packet, sizeof (ntp_packet), 0, (struct sockaddr *) &from, (socklen_t *) &fromlen);
            if (n < 0) {
              #ifdef __DMESG__
                    dmesgQueue << (const char *) "[NTP] recvfrom error: " <<  errno << " " << strerror (errno);
                #endif
                close (sockfd);
                return (const char *) "Could not receive NTP packet";
            }

        }

        // »Now that our message is sent, we block or wait for the response by reading from the socket. The message we get back should be the same
        //  size as the message we sent. We will store the incoming message in packet just like we stored our outgoing message.«

        // »Parse the Return Message«

        // These two fields contain the time-stamp seconds as the packet left the NTP server.
        // The number of seconds correspond to the seconds passed since 1900.
        // ntohl() converts the bit/byte order from the network's to host's "endianness".

        packet.txTm_s = ntohl (packet.txTm_s); // Time-stamp seconds.
        packet.txTm_f = ntohl (packet.txTm_f); // Time-stamp fraction of a second.

        // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
        // Subtract 70 years worth of seconds from the seconds since 1900.
        // This leaves the seconds since the UNIX epoch of 1970.
        // (1900)------------------(1970)**************************************(the Time the Packet Left the Server)

        #define NTP_TIMESTAMP_DELTA 2208988800
        time_t txTm = (time_t) (packet.txTm_s - NTP_TIMESTAMP_DELTA);

        // »The message we get back is in network order or big-endian form. Depending on the machine you run this on, ntohl will transform the bits 
        //  from either big to little or big to big-endian. You can think of big or little-endian as reading from left to right or tfel ot thgir respectively.
        //
        //  With the data in the order we need it, we can now subtract the delta and cast the resulting number to a time-stamp number. 
        //  Note that NTP_TIMESTAMP_DELTA = 2208988800ull which is the NTP time-stamp of 1 Jan 1970 or put another way 2,208,988,800 unsigned long long seconds.«

        setTimeOfDay (txTm);
        #ifdef __DMESG__
            dmesgQueue << (const char *) "[time] synchronized with " << ntpServer << (const char *) " (" << ipstr << (const char *) ")";
        #endif
        close (sockfd);
        // cout << (const char *) "[time] synchronized with " << ntpServer << (const char *) " (" << ipstr << (const char *) ")" << endl;

        return (char *) ""; // OK
    }
  
    const char *ntpDate () { // synchronizes time with NTP servers, returns error message
        const char *s;
        if (!*(s = ntpDate (__ntpServer1__))) return ""; 
        #ifdef __DMESG__
            dmesgQueue << (const char *) "[time] " << s; 
        #endif
        cout << "[time] " << s << endl;
        delay (1);
        if (!*(s = ntpDate (__ntpServer2__))) return ""; 
        #ifdef __DMESG__
            dmesgQueue << (const char *) "[time] " << s; 
        #endif
        cout << "[time] " << s << endl;
        delay (1);
        if (!*(s = ntpDate (__ntpServer3__))) return ""; 
        #ifdef __DMESG__
            dmesgQueue << (const char *) "[time] " << s; 
        #endif
        cout << (const char *) "[time] " << s << endl;
        return (const char *) "NTP servers are not available.";
    }


    // ----- CRONTAB -----

    template <int N, int L> class cronTab_t {

        private:

            struct cronTabEntry_t {
                bool readFromFile;                              // true, if this entry is read from /etc/crontab file, false if it is inserted by program code
                bool markForExecution;                          // falg if the command is to be executed
                bool executed;                                  // flag if the command is beeing executed
                uint8_t second;                                 // 0-59, ANY (255) means *
                uint8_t minute;                                 // 0-59, ANY (255) means *
                uint8_t hour;                                   // 0-23, ANY (255) means *
                uint8_t day;                                    // 1-31, ANY (255) means *
                uint8_t month;                                  // 1-12, ANY (255) means *
                uint8_t day_of_week;                            // 0-6 and 7, Sunday to Saturday, 7 is also Sunday, ANY (255) means *
                char cronCommand [L + 1];                       // cronCommand to be passed to cronHandler when time condition is met - it is reponsibility of cronHandler to do with it what is needed
                time_t lastExecuted;                            // the time cronCommand has been executed
            } __cronTabEntry__ [N];        

            int __cronTabSize__ = 0;

            SemaphoreHandle_t __cronTabSemaphore__ = xSemaphoreCreateRecursiveMutex (); // controls access to cronDaemon's variables


        public:

            // inserts new entry into crontab table
            bool insert (uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t day_of_week, const char *cronCommand, bool readFromFile = false) {
                bool b = false;    
                lock ();
                    if (__cronTabSize__ < N - 1) {
                        __cronTabEntry__ [__cronTabSize__] = {readFromFile, false, false, second, minute, hour, day, month, day_of_week, {}, 0};
                        strncpy (__cronTabEntry__ [__cronTabSize__].cronCommand, cronCommand, L);
                        __cronTabEntry__ [__cronTabSize__].cronCommand [L] = 0;
                        __cronTabSize__ ++;
                    }
                    b = true;
                unlock ();
                if (b) 
                    return true;
                
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[cronTab] can't insert cronCommand, cron table is full: " << cronCommand;
                #endif
                cout << (const char *) "[cronTab] can't insert cronCommand, cron table is full: " << cronCommand << endl;
                return false;
            }

            // inserts new entry into crontab table
            bool insert (const char *cronTabLine, bool readFromFile = false) { // parse cronTabLine and then call the function above
                char second [3]; char minute [3]; char hour [3]; char day [3]; char month [3]; char day_of_week [3]; char cronCommand [65];
                if (sscanf (cronTabLine, "%2s %2s %2s %2s %2s %2s %64s", second, minute, hour, day, month, day_of_week, cronCommand) == 7) {
                    int8_t se = strcmp (second, "*")      ? atoi (second)      : ANY; if ((!se && *second != '0')      || se > 59)  { 
                                                                                                                                        #ifdef __DMESG__
                                                                                                                                            dmesgQueue << (const char *) "[cronTab] invalid second: " << second; 
                                                                                                                                        #endif
                                                                                                                                        cout << (const char *) "[cronTab] invalid second: " << second << endl;  
                                                                                                                                        return false; 
                                                                                                                                    }
                    int8_t mi = strcmp (minute, "*")      ? atoi (minute)      : ANY; if ((!mi && *minute != '0')      || mi > 59)  { 
                                                                                                                                        #ifdef __DMESG__
                                                                                                                                            dmesgQueue << (const char *) "[cronTab] invalid minute: " << minute; 
                                                                                                                                        #endif
                                                                                                                                        cout << (const char *) "[cronTab] invalid minute: " << minute << endl;  
                                                                                                                                        return false; 
                                                                                                                                    }
                    int8_t hr = strcmp (hour, "*")        ? atoi (hour)        : ANY; if ((!hr && *hour != '0')        || hr > 23)  {
                                                                                                                                        #ifdef __DMESG__
                                                                                                                                            dmesgQueue << (const char *) "[cronTab] invalid hour: " << hour; 
                                                                                                                                        #endif
                                                                                                                                        cout << (const char *) "[cronTab] invalid hour: " << hour << endl;  
                                                                                                                                        return false; 
                                                                                                                                    }
                    int8_t dm = strcmp (day, "*")         ? atoi (day)         : ANY; if (!dm                          || dm > 31)  { 
                                                                                                                                        #ifdef __DMESG__
                                                                                                                                            dmesgQueue << (const char *) "[cronTab] invalid day: " << day; 
                                                                                                                                        #endif
                                                                                                                                        cout << (const char *) "[cronTab] invalid day: " << day << endl;
                                                                                                                                        return false; 
                                                                                                                                    }
                    int8_t mn = strcmp (month, "*")       ? atoi (month)       : ANY; if (!mn                          || mn > 12)  { 
                                                                                                                                        #ifdef __DMESG__
                                                                                                                                            dmesgQueue << (const char *) "[cronTab] invalid month: " << month; 
                                                                                                                                        #endif
                                                                                                                                        cout << (const char *) "[cronTab] invalid month: " << month << endl;  
                                                                                                                                        return false; 
                                                                                                                                    }
                    int8_t dw = strcmp (day_of_week, "*") ? atoi (day_of_week) : ANY; if ((!dw && *day_of_week != '0') || dw > 7)   {
                                                                                                                                        #ifdef __DMESG__
                                                                                                                                            dmesgQueue << (const char *) "[cronTab] invalid day of week: " << day_of_week; 
                                                                                                                                        #endif
                                                                                                                                        cout << (const char *) "[cronTab] invalid day of week: " << day_of_week << endl;
                                                                                                                                        return false; 
                                                                                                                                    }
                    if (!*cronCommand) { 
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[cronTab] missing cron command";
                        #endif
                        cout << (const char *) "[cronTab] missing cron command" << endl;  
                        return false;
                    }
                    return cronTabAdd (se, mi, hr, dm, mn, dw, cronCommand, readFromFile);
                } else {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[cronTab] invalid cronTabLine: " << cronTabLine;
                    #endif
                    cout << (const char *) "[cronTab] invalid cronTabLine: " << cronTabLine << endl;
                    return false;
                }
            }

            // erases entry(es) from cronTab
            int erase (const char *cronCommand) { // returns the number of cron commands being erased
                int cnt = 0;
                lock ();
                    int i = 0;
                    while (i < __cronTabSize__) {
                        if (!strcmp (__cronTabEntry__ [i].cronCommand, cronCommand)) {        
                            for (int j = i; j < __cronTabSize__ - 1; j ++) { 
                                __cronTabEntry__ [j] = __cronTabEntry__ [j + 1]; 
                            }
                            __cronTabSize__ --;
                            cnt ++;
                        } else {
                            i ++;
                        }
                    }
                unlock ();
                if (!cnt) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[cronTab] erase: cronCommand not found: " << cronCommand;
                    #endif
                    cout << (const char *) "[cronTab] erase: cronCommand not found: " << cronCommand << endl;
                }
                return cnt;
            }

            inline int size () __attribute__((always_inline)) { return __cronTabSize__; }

            void lock () { xSemaphoreTakeRecursive (__cronTabSemaphore__, portMAX_DELAY); }

            void unlock () { xSemaphoreGiveRecursive (__cronTabSemaphore__); }

            // [] operator - returns cronTabEntry
            inline cronTabEntry_t &operator [] (int i) __attribute__((always_inline)) { return __cronTabEntry__ [i]; }

    };

    // create a working instance
    cronTab_t<CRONTAB_MAX_ENTRIES, CRON_COMMAND_MAX_LENGTH> cronTab;

    [[deprecated("Use cronTab.insert instead.")]]
    bool cronTabAdd (uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t day_of_week, const char *cronCommand, bool readFromFile = false) {
        return cronTab.insert (second, minute, hour, day, month, day_of_week, cronCommand, readFromFile);
    }
    
    [[deprecated("Use cronTab.insert instead.")]]
    bool cronTabAdd (const char *cronTabLine, bool readFromFile = false) { 
        return cronTab.insert (cronTabLine, readFromFile);
    }

    [[deprecated("Use cronTab.erase instead.")]]
    int cronTabDel (const char *cronCommand) { 
        return cronTab.erase (cronCommand);
    }


    // ----- CRON DAEMON -----

    // synchronizes time with NTP servers from /etc/ntp.conf, reads /etc/crontab, returns error message. If /etc/ntp.conf or /etc/crontab don't exist (yet) creates the default one
    class cronDaemon_t {

        private:

            void (* __cronHandler__) (const char *);
            bool __runDaemonInItsOwnTask__;

            enum STATE_TYPE { STARTING = 0, FIRST_SYNC = 1, STARTED = 2 } __state__ = STARTING;

            unsigned long __lastNtpSync__ = 0;
            unsigned long __lastMinuteCheck__ = 0;
            unsigned long __lastHourCheck__ = 0;
            unsigned long __lastDayCheck__ = 0;

            time_t __previousRun__;


        public:

            void start (void (* cronHandler) (const char *) = NULL,            // a callback function that would get notified when cron event accurs
                        bool runDaemonInItsOwnTask = true                      // a calling program may repeatedly call run itself to save some memory that cronDaemon task would use
                       ) { 

                // create local copy of the arguments
                __cronHandler__ = cronHandler;
                __runDaemonInItsOwnTask__ = runDaemonInItsOwnTask;

                // create and read configuration files
                #ifdef __FILE_SYSTEM__
                    if (fileSystem.mounted ()) {

                        // read TZ (timezone) configuration from /usr/share/zoneinfo, create a new one if it doesn't exist
                        if (!fileSystem.isFile ("/usr/share/zoneinfo")) {
                            // create directory structure
                            if (!fileSystem.isDirectory ("/usr/share")) {
                                fileSystem.makeDirectory ("/usr"); 
                                fileSystem.makeDirectory ("/usr/share"); 
                            }
                            cout << "[cronDaemon] /usr/share/zoneinfo does not exist, creating default one ... ";
                            bool created = false;
                            File f = fileSystem.open ("/usr/share/zoneinfo", "w");
                            if (f) {
                                String defaultContent = F ( "# timezone configuration - reboot for changes to take effect\r\n"
                                                            "# choose one of available (POSIX) timezones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv\r\n\r\n"
                                                            "TZ " TZ "\r\n");
                                created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                                f.close ();
                                fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                            }
                            cout << (created ? "created\n" : "error\n");
                        }
                        {
                            cout << "[cronDaemon] reading /usr/share/zoneinfo ... ";
                            // parse configuration file if it exists
                            char buffer [MAX_TZ_ETC_NTP_CONF_SIZE] = "\n";
                            if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/usr/share/zoneinfo")) {
                                strcat (buffer, "\n");
                                char __TZ__ [100] = TZ;
                                char *p;
                                if ((p = stristr (buffer, "\nTZ"))) 
                                    sscanf (p + 3, "%*[ =]%98[!-~]", __TZ__);                      
                                setenv ("TZ", __TZ__, 1);
                                tzset ();
                                #ifdef __DMESG__
                                    dmesgQueue << (const char *) "[cronDaemon] TZ set to " << __TZ__;
                                #endif
                                // cout << "OK, TZ set to " << __TZ__ << endl;
                            } else {
                                cout << (const char *) "error\n";
                            }
                        }

                        // read NTP configuration from /etc/ntp.conf, create a new one if it doesn't exist
                        if (!fileSystem.isFile ((char *) "/etc/ntp.conf")) {
                            // create directory structure
                            if (!fileSystem.isDirectory ((char *) "/etc")) 
                                fileSystem.makeDirectory ((char *) "/etc");
                            cout << "[cronDaemon] /etc/ntp.conf does not exist, creating default one ... ";
                            bool created = false;
                            File f = fileSystem.open ((char *) "/etc/ntp.conf", "w");
                            if (f) {
                                String defaultContent = F ( "# configuration for NTP - reboot for changes to take effect\r\n\r\n"
                                                            "server1 " DEFAULT_NTP_SERVER_1 "\r\n"
                                                            "server2 " DEFAULT_NTP_SERVER_2 "\r\n"
                                                            "server3 " DEFAULT_NTP_SERVER_3 "\r\n");
                                created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                                f.close ();
                                fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                            }
                            cout << (created ? "created\n" : "error\n");
                        }
                        {
                            cout << "[cronDaemon] reading /etc/ntp.conf ... ";
                            // parse configuration file if it exists
                            char buffer [MAX_TZ_ETC_NTP_CONF_SIZE] = "\n";
                            if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/ntp.conf")) {
                                strcat (buffer, "\n");
                                *__ntpServer1__ = *__ntpServer2__ = *__ntpServer3__ = 0; 
                                char *p;                    
                                if ((p = stristr (buffer, "\nserver1"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", __ntpServer1__);
                                if ((p = stristr (buffer, "\nserver2"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", __ntpServer2__);
                                if ((p = stristr (buffer, "\nserver3"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", __ntpServer3__);
                                // cout << "OK\n";
                            } else {
                                cout << "error\n";
                            }
                        }

                        // read scheduled tasks from /etc/crontab
                        if (!fileSystem.isFile ((char *) "/etc/crontab")) {
                            // create directory structure
                            if (!fileSystem.isDirectory ((char *) "/etc"))
                                fileSystem.makeDirectory ((char *) "/etc");
                            cout << "[cronDaemon] /etc/crontab does not exist, creating default one ... ";
                            bool created = false;
                            File f = fileSystem.open ((char *) "/etc/crontab", "w");
                            if (f) {
                                String defaultContent = F ("# scheduled tasks (in local time) - reboot for changes to take effect\r\n"
                                                           "#\r\n"
                                                           "# .------------------- second (0 - 59 or *)\r\n"
                                                           "# |  .---------------- minute (0 - 59 or *)\r\n"
                                                           "# |  |  .------------- hour (0 - 23 or *)\r\n"
                                                           "# |  |  |  .---------- day of month (1 - 31 or *)\r\n"
                                                           "# |  |  |  |  .------- month (1 - 12 or *)\r\n"
                                                           "# |  |  |  |  |  .---- day of week (0 - 7 or *; Sunday=0 and also 7)\r\n"
                                                           "# |  |  |  |  |  |\r\n"
                                                           "# *  *  *  *  *  * cronCommand to be executed\r\n"
                                                           "# * 15  *  *  *  * exampleThatRunsQuaterPastEachHour\r\n");
                                created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                                f.close ();
                                fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                            }
                            cout << (created ? "created\n" : "error\n");
                        }
                        {
                            cout << "[cronDaemon] reading /etc/crontab ... ";
                            // parse scheduled tasks if /etc/crontab exists
                            char buffer [MAX_TZ_ETC_NTP_CONF_SIZE] = "\n";
                            if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/crontab")) {
                                strcat (buffer, "\n");
                                char *p, *q;
                                p = buffer + 1;
                                while (*p) {
                                    if ((q = strstr (p, "\n"))) {
                                        *q = 0;
                                        if (*p) cronTabAdd (p, true);
                                        p = q + 1;
                                    }
                                }
                            } else {
                                cout << "error\n";
                            }
                        }

                    } else {
                        cout << "[cronDaemon] file system not mounted, can't read or write configuration files\n";
                    }
                #else
                    // set the default timezone
                    setenv ("TZ", (char *) TZ, 1);
                    tzset ();
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[cronDaemon] TZ set to " << TZ;
                    #endif
                    cout << (const char *) "[cronDaemon] TZ set to " << TZ << endl;

                    cout << (const char *) "[cronDaemon] is starting without a file system\n";
                #endif


                if (__runDaemonInItsOwnTask__) {
                    // run periodic synchronization with NTP servers and execute cron commands in a separate task
                    #define tskNORMAL_PRIORITY 1
                    if (pdPASS != xTaskCreate ([] (void *ths) {
                                                                while (true) {
                                                                    delay (100);
                                                                    ((cronDaemon_t *) ths)->nextRun ();
                                                                }
                                                              }, "cronDaemon", CRON_DAEMON_STACK_SIZE, (void *) this, tskNORMAL_PRIORITY, NULL)) {
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[cronDaemon] xTaskCreate error, could not start";
                        #endif
                        cout << (const char *) "[cronDaemon] xTaskCreate error, could not start" << endl;
                    }
                }

            }


            // (periodic calls to) nextRun functions scans cronTab and executes what needed
            // 1. - it executes cron commands from cron table when the time is right
            // 2. - it synchronizes time with NTP servers once a day
            void nextRun () { 

                // trigger 3 cron events that can be triggerd even if the time is not known (yet) 
                if (__cronHandler__) {
                    if (millis () - __lastMinuteCheck__ >= 60000) { 
                        __lastMinuteCheck__ = millis ();
                        __cronHandler__ ("ONCE_A_MINUTE");

                        if (millis () - __lastHourCheck__ >= 3600000) { 
                            __lastHourCheck__ = millis ();
                            __cronHandler__ ("ONCE_AN_HOUR");

                            if (millis () - __lastDayCheck__ >= 86400000) { 
                                __lastDayCheck__ = millis ();
                                __cronHandler__ ("ONCE_A_DAY");
                            } 
                        }
                    }
                }

                // continue processing depending on the state
                switch (__state__) {
                    case STARTING:      // log success       
                                        #ifdef __DMESG__
                                            dmesgQueue << (const char *) "[cronDaemon] started on core " << xPortGetCoreID () << ", free heap left: " << esp_get_free_heap_size ();
                                        #endif
                                        cout << (const char *) "[cronDaemon] started\n";
                                        __state__ = FIRST_SYNC;
                                        return;

                    case FIRST_SYNC:    // is it time for NTP sync? Try every 15 seconds until first time synchronized
                                        if (millis () - __lastNtpSync__ >= 15000) {
                                            __lastNtpSync__ = millis ();
                                            if (!*ntpDate ()) // success
                                                __state__ = STARTED;
                                        }
                                        if (!__previousRun__)
                                            __previousRun__ = time ();
                                        if (__previousRun__) // if the time is known
                                            goto processCronTab;
                                        return;

                    case STARTED:       // NTP sync once a day and process cronTab
                                        if (millis () - __lastNtpSync__ >= 86400000) {
                                            __lastNtpSync__ = millis ();
                                            ntpDate ();
                                        }
                                        break; // continue with processCronTab;
                }

            processCronTab:
                if (__cronHandler__) {

                    // process cronTab
                    time_t now = time ();
                    if (!now) 
                        return; // should't happen but check anyway
                    for (time_t l = __previousRun__ + 1; l <= now; l++) {
                        delay (1);
                        struct tm slt; 
                        localtime_r (&l, &slt); 
                        //scan through cron entries and find commands that needs to be executed (at time l)
                        cronTab.lock ();
                            // mark cron commands for execution
                            for (int i = 0; i < cronTab.size (); i ++) {
                                if ((cronTab [i].second == ANY      || cronTab [i].second == slt.tm_sec) && 
                                    (cronTab [i].minute == ANY      || cronTab [i].minute == slt.tm_min) &&
                                    (cronTab [i].hour == ANY        || cronTab [i].hour == slt.tm_hour) &&
                                    (cronTab [i].day == ANY         || cronTab [i].day == slt.tm_mday) &&
                                    (cronTab [i].month == ANY       || cronTab [i].month == slt.tm_mon + 1) &&
                                    (cronTab [i].day_of_week == ANY || cronTab [i].day_of_week == slt.tm_wday || cronTab [i].day_of_week == slt.tm_wday + 7) ) {

                                        if (!cronTab [i].executed) {
                                            cronTab [i].markForExecution = true;
                                        }
                                                        
                                } else {

                                        cronTab [i].executed = false;
                    
                                }
                            }          
                            // execute marked cron commands
                            int execCnt = 1;
                            while (execCnt) { // if a command during execution deletes other commands we have to repeat this step
                                execCnt = 0;
                                for (int i = 0; i < cronTab.size (); i ++) {
                                    if (cronTab [i].markForExecution ) {

                                        if (cronTab [i].markForExecution) {
                                            cronTab [i].executed = true;
                                            cronTab [i].lastExecuted = now;
                                            cronTab [i].markForExecution = false;
                                            __cronHandler__ (cronTab [i].cronCommand); // this should be called last in case cronHandler calls insert or erase itself
                                            execCnt ++; 
                                            delay (1);
                                        }

                                        cronTab [i].markForExecution = false;
                                    }
                                }
                            }
                        cronTab.unlock ();
                    }
                    __previousRun__ = now;

                } // if (__cronHandler__)

                // check how much stack did we use
                static UBaseType_t lastHighWaterMark = CRON_DAEMON_STACK_SIZE;
                UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                if (lastHighWaterMark > highWaterMark) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[cronDaemon] new stack high water mark reached: " << highWaterMark << (const char *) " bytes not used";
                    #endif
                    // DEBUG: 
                    Serial.print ((const char *) "[cronDaemon] new stack high water mark reached: "); Serial.print (highWaterMark); Serial.println ((const char *) " bytes not used");
                    lastHighWaterMark = highWaterMark;
                }
            }

    };

    // create a working instance
    cronDaemon_t cronDaemon;

    [[deprecated("Use cronDaemon.start instead.")]]
    void startCronDaemon (void (* cronHandlerCallback) (const char *) = NULL) {
        cronDaemon.start (cronHandlerCallback);
    }

  
    // a more convinient version of time function: returns GMT or 0 if the time is not set
    time_t time () {
        time_t t = time (NULL);
        if (t < 1687461154) return 0; // 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
        return t;
    }

    // returns the number of seconfs ESP32 has been running
    time_t getUptime () {
        time_t t = time (NULL);
        return __timeHasBeenSet__ ? t - __startupTime__ :  millis () / 1000; // if the time has already been set, 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
    }

    // sets internal clock, also sets/corrects __startupTime__ internal variable
    void setTimeOfDay (time_t t) {
        time_t oldTime = time (NULL);
        struct timeval newTime = {t, 0};
        settimeofday (&newTime, NULL);

        // char buf [100];
        if (__timeHasBeenSet__) {
            // ascTime (localTime (oldTime), buf, sizeof (buf));
            // strcat (buf, " to ");
            // ascTime (localTime (t), buf + strlen (buf), sizeof (buf) - strlen (buf));
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[time] time corrected from " << oldTime << (const char *) " to " << t;
            #endif
            cout << (const char *) "[time] time corrected from " << oldTime << (const char *) " to " << t << endl;
        } else {
            __startupTime__ = t - getUptime (); // recalculate            
            __timeHasBeenSet__ = true;
            // ascTime (localTime (t), buf, sizeof (buf));
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[time] time set to " << t; 
            #endif
            cout << (const char *) "[time] time set to " << t << endl;
        }
    }

    // a more convinient version of thread safe gmtime_r function: converts GMT in time_t to GMT in struct tm
    struct tm gmTime (const time_t t) {
        struct tm st;
        return *gmtime_r (&t, &st);
    }

    // a more convinient version of thread safe localtime_r function: converts GMT in time_t to local time in struct tm
    struct tm localTime (const time_t t) {
        struct tm st;
        return *localtime_r (&t, &st);
    }

    // a more convinient version of thread safe asctime_r function: converts struct tm to array of chars, buff should have at least 26 bytes
    char *ascTime (const struct tm st, char *buf, size_t len) {
        strftime (buf, len, "%Y/%m/%d %T", &st);
        return buf;
    }


#endif
