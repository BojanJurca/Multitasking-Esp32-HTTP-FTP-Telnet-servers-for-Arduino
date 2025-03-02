/*

    ftpServer.hpp 
 
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
    
    February 6, 2025, Bojan Jurca


    Classes implemented/used in this module:

        ftpServer_t
        ftpServer_t::ftpControlConnection_t
        tcpClient_t for active data connection
        tcpServer_t, tcpConnection_t for pasive data connection

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


    Nomenclature used here for easier understaning of the code:

     - "connection" normally applies to TCP connection from when it was established to when it is terminated

                  There is normally only one control TCP connection per FTP session. Beside control connection FTP client and server open
                  also a data TCP connection when certain commands are involved (like LIST, RETR, STOR, ...).

     - "session" applies to user interaction between login and logout

                  The first thing that the user does when a TCP connection is established is logging in and the last thing is logging out. It TCP
                  connection drops for some reason the user is automatically logged out.

      - "buffer size" is the number of bytes that can be placed in a buffer. 
      
                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".

    Handling FTP commands that always arrive through control TCP connection is pretty straightforward. The data transfer TCP connection, on the other 
    hand, can be initialized eighter by the server or the client. In the first case, the client starts a listener (thus acting as a server) and sends 
    its connection information (IP and port number) to the FTP server through the PORT command. The server then initializes data connection to the 
    client. This is called active data connection. Windows FTP.exe uses this kind of data transfer by default. In the second case, the client sends 
    a PASV command to the FTP server, then the server starts another listener (beside the control connection listener that is already running) and 
    sends its connection information (IP and port number) back to the client as a reply to the PASV command. The client then initializes data 
    connection to the server. This is called passive data connection. Windows Explorer uses this kind of data transfer.

*/


#include "tcpServer.hpp"
#include "tcpConnection.hpp"
#include "tcpClient.hpp"
#include "std/Cstring.hpp"


#ifndef __FTP_SERVER__
    #define __FTP_SERVER__

    #ifdef SHOW_COMPILE_TIME_INFORMATION
        #pragma message "__FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__ __FTP_SERVER__"
    #endif


    // TUNNING PARAMETERS

    #define FTP_CONTROL_CONNECTION_STACK_SIZE (6 * 1024)        // TCP connection
    #define FTP_CMDLINE_BUFFER_SIZE 300                         // reading and temporary keeping FTP command lines
    #define FTP_SESSION_MAX_ARGC 5                              // max number of arguments in command line    
    #define FTP_CONTROL_CONNECTION_TIME_OUT 300                 // 300 s = 5 min, 0 for infinite
    #define FTP_DATA_CONNECTION_TIME_OUT 3                      // 3 s, 0 for infinite 

    #define ftpServiceUnavailableReply (char *) "421 FTP service is currently unavailable\r\n"

    #ifndef HOSTNAME
        #define "MyESP32Server"                                 // use default if not defined previously
    #endif
    

    // ----- CODE -----

    #ifndef __FILE_SYSTEM__
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Implicitly including fileSystem.hpp" 
        #endif
        #include "fileSystem.hpp" // all file name and file info related functions are there
    #endif

    #ifndef __USER_MANAGEMENT__
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Implicitly including userManagement.hpp" // checkUserNameAndPassword and getHomeDirectory functions are declared there
        #endif
        #include "userManagement.hpp"
    #endif


    class ftpServer_t : public tcpServer_t {

        public:

            class ftpControlConnection_t : public tcpConnection_t {

                friend class ftpServer_t;

                private:

                    // FTP session related variables
                    char __cmdLine__ [FTP_CMDLINE_BUFFER_SIZE];
                    char __userName__ [USER_PASSWORD_MAX_LENGTH + 1] = "";
                    #ifdef __FILE_SYSTEM__
                        char __homeDir__ [FILE_PATH_MAX_LENGTH + 1] = "";
                        char __workingDir__ [FILE_PATH_MAX_LENGTH + 1] = "";
                    #endif

                    static UBaseType_t __lastHighWaterMark__;


                public:

                    ftpControlConnection_t (int connectionSocket,                                                                     // socket number
                                            char *clientIP,                                                                           // client's IP
                                            char *serverIP                                                                            // server's IP (the address to which the connection arrived)
                                           ) : tcpConnection_t (connectionSocket, clientIP, serverIP) {
                    }

                    ~ftpControlConnection_t () {
                        __closeDataConnection__ ();
                    }


                    // FTP session related variables

                    inline char *getUserName () __attribute__((always_inline)) { return __userName__; }
                    #ifdef __FILE_SYSTEM__
                        inline char *getHomeDir () __attribute__((always_inline)) { return __homeDir__; }
                        inline char *getWorkingDir () __attribute__((always_inline)) { return __workingDir__; }
                    #endif

                    char recvLine (char *buf, size_t len, bool trim = true) { // reads incoming stream, returns last character read or 0-error, 3-Ctrl-C, 4-EOF, 10-Enter, ...
                        return 0;
                    }


                private:

                    void __runConnectionTask__ () {

                        // ----- this is where FTP control connection starts, the socket is already opened -----
            
                            // send welcome message to the client
                            #if USER_MANAGEMENT == NO_USER_MANAGEMENT

                                if (sendString ((char *) "220-" HOSTNAME " FTP server - everyone is allowed to login\r\n220 \r\n") <= 0)
                                    return;

                            #else

                                if (sendString ((char *) "220-" HOSTNAME " FTP server - please login\r\n220 \r\n") <= 0)
                                    return;

                            #endif

                            while (true) { // endless loop of reading and executing commands

                                // 1. read the request
                                switch (recvString (__cmdLine__, FTP_CMDLINE_BUFFER_SIZE, "\n")) {
                                    case -1:                      // error
                                    case 0:                       // connection closed by peer
                                                                  return;
                                    case FTP_CMDLINE_BUFFER_SIZE: // buffer full but end of request did not arrive           
                                                                  #ifdef __DMESG__
                                                                      dmesgQueue << (const char *) "[ftpCtrlConn] buffer too small";
                                                                  #endif
                                                                  Serial.println ((const char *) "[ftpCtrlConn] buffer too small");
                                                                  return;
                                    default:                      
                                    break; // continue
                                }

                                // 2. parse command line
                                int argc = 0;
                                char *argv [FTP_SESSION_MAX_ARGC];
    
                                char *q = __cmdLine__ - 1;
                                while (true) {
                                    char *p = q + 1; 
                                    while (*p && *p <= ' ') 
                                        p++;
                                    if (*p) { // p points to the beginning of an argument
                                        bool quotation = false;
                                        if (*p == '\"') { 
                                            quotation = true; 
                                            p++; 
                                        }
                                        argv [argc++] = p;
                                        q = p; 
                                        while (*q && (*q > ' ' || quotation)) 
                                            if (*q == '\"') 
                                                break; 
                                            else 
                                                q++; // q points after the end of an argument 
                                        if (*q) 
                                            *q = 0; 
                                        else 
                                            break;
                                    } else { 
                                        break;
                                    }
                                    if (argc == FTP_SESSION_MAX_ARGC) 
                                        break;
                                }

                                // 3. process commandLine
                                if (argc) {
                                    cstring s = __internalCommandHandler__ (argc, argv);
                                    // 4. send the reply back to the client
                                    if (s != "") {
                                        // DEBUG: cout << s;
                                        if (sendString (s) <= 0)
                                            return;
                                    }

                                    if (strstr ((char *) s, "221") == (char *) s) // 221 closing control connection
                                        return;
                                }


                                // check how much stack did we use
                                UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                                if (__lastHighWaterMark__ > highWaterMark) {
                                    #ifdef __DMESG__
                                        dmesgQueue << (const char *) "[ftpCtrlConn] new FTP connection stack high water mark reached: " << highWaterMark << (const char *) " not used bytes";
                                    #endif
                                    // DEBUG: 
                                    Serial.print ((const char *) "[ftpCtrlConn] new FTP connection stack high water mark reached: "); Serial.print (highWaterMark); Serial.println ((const char *) " not used bytes");
                                    __lastHighWaterMark__ = highWaterMark;
                                }

                            } // endless loop of reading and executing commands
                    endConnection:
                            ;
                            #ifdef __DMESG__
                                dmesgQueue << "[ftpCtrlConn] user logged out: " << __userName__;
                            #endif

                        // ----- this is where FTP connection ends, the socket will be closed upon return -----

                    }

                    cstring __internalCommandHandler__ (int argc, char *argv []) {
                        // DEBUG: for (int i = 0; i < argc; i++) Serial.printf ("   argv [%i] = '%s'\n", i, argv [i]);

                        #define ftpArgv0Is(X) (argc > 0 && !stricmp (argv [0], X))
                        #define ftpArgv1Is(X) (argc > 1 && !stricmp (argv [1], X))
                        #define ftpArgv2Is(X) (argc > 2 && !stricmp (argv [2], X))

                             if (ftpArgv0Is ("QUIT"))                                                    return "221 closing connection\r\n";

                        else if (ftpArgv0Is ("OPTS"))                                                    if (argc == 3 && ftpArgv1Is ("UTF8") && ftpArgv2Is ("ON")) return "200 UTF8 enabled\r\n"; // by default, we don't have to do anything, just report to the client that it is ok to use UTF-8
                                                                                                         else                                                       return "502 OPTS arguments not supported\r\n";

                        else if (ftpArgv0Is ("USER"))                                                    return __USER__ (argv [1]); // not entering user name may be OK for anonymous login
                        else if (ftpArgv0Is ("PASS"))                                                    return __PASS__ (argv [1]); // not entering password may be OK for anonymous login
                        else if (ftpArgv0Is ("PWD") || ftpArgv0Is ("XPWD"))                              return __XPWD__ ();

                        else if (ftpArgv0Is ("TYPE"))                                                    return "200 ok\r\n"; // just say OK to whatever type it is

                        else if (ftpArgv0Is ("NOOP"))                                                    return "200 ok\r\n"; // just say OK to whatever type it is

                        else if (ftpArgv0Is ("SYST"))                                                    return "215 UNIX Type: L8\r\n"; // just say this is UNIX OS

                        else if (ftpArgv0Is ("FEAT"))                                                    return "211-Extensions supported:\r\n UTF8\r\n211 end\r\n";

                        else if (ftpArgv0Is ("PORT"))                                                    return __PORT__ (argv [1]); // IPv4
                        else if (ftpArgv0Is ("EPRT"))                                                    return __EPRT__ (argv [1]); // extended for IPv6 (and IPv4)
                        else if (ftpArgv0Is ("PASV"))                                                    return __PASV__ ();         // IPv4
                        else if (ftpArgv0Is ("EPSV"))                                                    return __EPSV__ ();         // extended for IPv6 (and IPv4)

                        // all the following commands take exactly 1 argument which is a file name that can also include spaces - reconstruct the spaces that have been parsed before
                        if (argc > 1) {
                            for (char *p = argv [1]; p < argv [argc - 1]; p++)
                                if (!*p)
                                    *p = ' ';
                        } else {
                            argv [1] = (char *) ""; 
                        }

                             if (ftpArgv0Is ("LIST") || ftpArgv0Is ("NLST"))                             return __NLST__ (argc > 1 ? argv [1] : __workingDir__);
                        else if (ftpArgv0Is ("SIZE"))                                                    return __SIZE__ (argv [1]);
                        else if (ftpArgv0Is ("XMKD") || ftpArgv0Is ("MKD"))                              return __XMKD__ (argv [1]);
                        else if (ftpArgv0Is ("XRMD") || ftpArgv0Is ("RMD") || ftpArgv0Is ("DELE"))       return __XRMD__ (argv [1]);
                        else if (ftpArgv0Is ("CWD"))                                                     return __CWD__  (argv [1]);
                        else if (ftpArgv0Is ("RNFR"))                                                    return __RNFR__ (argv [1]);
                        else if (ftpArgv0Is ("RNTO"))                                                    return __RNTO__ (argv [1]);
                        else if (ftpArgv0Is ("RETR"))                                                    return __RETR__ (argv [1]);
                        else if (ftpArgv0Is ("STOR"))                                                    return __STOR__ (argv [1]);

                        return cstring ("502 command ") + argv [0] + " not implemented\r\n";
                    }


                    bool __userHasRightToAccessFile__ (char *fullPath) { return strstr (fullPath, __homeDir__) == fullPath; }
                    bool __userHasRightToAccessDirectory__ (char *fullPath) { return __userHasRightToAccessFile__ (cstring (fullPath) + "/"); }


                    cstring __USER__ (char *userName) { // save user name and require password
                        #if USER_MANAGEMENT == NO_USER_MANAGEMENT
                            strcpy (__userName__, "root");
                            return "331 enter password\r\n";
                        #else
                            if (strlen (userName) < sizeof (__userName__)) strcpy (__userName__, userName);
                            return "331 enter password\r\n";
                        #endif
                    }

                    cstring __PASS__ (char *password) { // login
                        if (!userManagement.checkUserNameAndPassword (__userName__, password)) { 
                            #ifdef __DMESG__
                                dmesgQueue << "[ftpCtrlConn] user failed login attempt: " << __userName__;
                            #endif
                            delay (100);
                            return "530 user name or password incorrect\r\n"; 
                        }
                        // prepare session defaults
                        userManagement.getHomeDirectory (__homeDir__, sizeof (__homeDir__), __userName__);
                        if (*__homeDir__) { // if logged in
                            strcpy (__workingDir__, __homeDir__);

                            // remove extra /
                            cstring s (__homeDir__);
                            if (s [s.length () - 1] == '/') s [s.length () - 1] = 0; 
                            if (!s [0]) s = "/"; 

                            #ifdef __DMESG__
                                dmesgQueue << "[ftpCtrlConn] user logged in: " << __userName__;
                            #endif
                            return cstring ("230 logged on, your home directory is \"") + s + "\"\r\n";
                        } else { 
                            #ifdef __DMESG__
                                dmesgQueue << "[ftpCtrlConn] user does not have a home directory: " << __userName__;
                            #endif
                            return "530 user does not have a home directory\r\n"; 
                        }
                    }

                    cstring __CWD__ (char *directoryName) { 
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n";
                        cstring fp = fileSystem.makeFullPath (directoryName, __workingDir__);
                        if (fp == "" || !fileSystem.isDirectory (fp))                                   return "501 invalid directory name\r\n";
                        if (!__userHasRightToAccessDirectory__ (fp))                                    return "550 access denyed\r\n";

                        // shoud be OK but check anyway:
                        if (fp.length () < sizeof (__workingDir__)) strcpy (__workingDir__, fp);        return cstring ("250 your working directory is ") + fp + "\r\n";
                    }

                    cstring __XPWD__ () { 
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n";

                        // remove extra /
                        cstring s (__workingDir__);
                        if (s [s.length () - 1] == '/') s [s.length () - 1] = 0; 
                        if (!s [0]) s = "/"; 
                                                                                                        return cstring ("257 \"") + s + "\"\r\n";
                    }

                    const char *__XMKD__ (char *directoryName) { 
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n"; 
                        cstring fp = fileSystem.makeFullPath (directoryName, __workingDir__);
                        if (fp == "")                                                                   return "501 invalid directory name\r\n"; 
                        if (!__userHasRightToAccessDirectory__ (fp))                                    return "550 access denyed\r\n"; 
                
                        if (fileSystem.makeDirectory (fp))                                              return "257 directory created\r\n";
                                                                                                        return "550 could not create directory\r\n"; 
                    }

                    const char *__XRMD__ (char *fileOrDirName) { 
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n"; 
                        cstring fp = fileSystem.makeFullPath (fileOrDirName, __workingDir__);
                        if (fp == "")                                                                   return "501 invalid file or directory name\r\n"; 
                        if (!__userHasRightToAccessDirectory__ (fp))                                    return "550 access denyed\r\n"; 

                        if (fileSystem.isFile (fp)) {
                            if (fileSystem.deleteFile (fp))                                             return "250 file deleted\r\n";
                                                                                                        return "452 could not delete file\r\n";
                        } else {
                            if (fp == __homeDir__)                                                      return "550 you can't remove your home directory\r\n";
                            if (fp == __workingDir__)                                                   return "550 you can't remove your working directory\r\n";
                            if (fileSystem.removeDirectory (fp))                                        return "250 directory removed\r\n";
                                                                                                        return "452 could not remove directory\r\n";
                        }
                    }

                    cstring __SIZE__ (char *fileName) { 
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n";
                        cstring fp = fileSystem.makeFullPath (fileName, __workingDir__);
                        if (fp == "" || !fileSystem.isFile (fp))                                        return "501 invalid file name\r\n";
                        if (!__userHasRightToAccessFile__ (fp))                                         return "550 access denyed\r\n";

                        unsigned long fSize = 0;
                        File f = fileSystem.open (fp, "r");
                        if (f) { fSize = f.size (); f.close (); }
                                                                                                        return cstring ("213 ") + cstring (fSize) + "\r\n";
                    }


                    tcpClient_t *__activeDataClient__ = NULL;
                    tcpServer_t *__passiveDataServer__ = NULL;
                    tcpConnection_t *__dataConnection__ = NULL;

                    void __closeDataConnection__ () {
                        if (__activeDataClient__) {
                            delete __activeDataClient__;
                            __dataConnection__ = __activeDataClient__ = NULL;
                        } else {
                            if (__dataConnection__) {
                                delete __dataConnection__;
                                __dataConnection__= NULL;
                            }
                            if (__passiveDataServer__) {
                                delete __passiveDataServer__;
                                __passiveDataServer__ = NULL;
                            }
                        }
                    }


                    // cycle through set of port numbers when FTP server is working in pasive mode
                    int __pasiveDataPort__ () {
                        static int __lastPasiveDataPort__ = 1024;                                                   // change pasive data port range if needed
                        xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY);
                            int pasiveDataPort = __lastPasiveDataPort__ = (((__lastPasiveDataPort__ + 1) % 16) + 1024); // change pasive data port range if needed
                        xSemaphoreGive (__tcpServerSemaphore__);
                        return pasiveDataPort;
                    }


                    const char *__PORT__ (char *dataConnectionInfo) { // IPv4 PORT command like PORT 10,18,1,26,239,17
                        if (!*__homeDir__)
                            return "530 not logged in\r\n";
                      
                        int ip1, ip2, ip3, ip4, p1, p2; // get IP and port that client used to set up data connection server
                        char activeServerIP [INET6_ADDRSTRLEN] = ""; // FTP client's IP, INET_ADDRSTRLEN should do for IPv4
                        int activeServerPort; // FTP client's port
                        if (6 == sscanf (dataConnectionInfo, "%i,%i,%i,%i,%i,%i", &ip1, &ip2, &ip3, &ip4, &p1, &p2)) {
                            sprintf (activeServerIP, "%i.%i.%i.%i", ip1, ip2, ip3, ip4); 
                            activeServerPort = p1 << 8 | p2;

                            // connect to the FTP client that acts as server now
                            __activeDataClient__ = new (std::nothrow) tcpClient_t (activeServerIP, activeServerPort, FTP_DATA_CONNECTION_TIME_OUT);
                            if (__activeDataClient__ && !(__activeDataClient__->errText ())) {
                                __dataConnection__ = __activeDataClient__;
                                return "200 port ok\r\n";
                            }
                        }
                        return "425 can't open active data connection\r\n";
                    }

                    const char *__EPRT__ (char *dataConnectionInfo) { // extended IPv6 (and IPv4) EPRT command like EPRT |2|fe80::3d98:8793:4cf0:f618|61166|
                        if (!*__homeDir__)
                            return "530 not logged in\r\n";
                      
                        char activeServerIP [INET6_ADDRSTRLEN]; // FTP client's IP
                        int activeServerPort; // FTP client's port

                        if (2 == sscanf (dataConnectionInfo, "|%*d|%39[^|]|%d|", activeServerIP, &activeServerPort)) {

                            // connect to the FTP client that act as server now
                            __activeDataClient__ = new (std::nothrow) tcpClient_t (activeServerIP, activeServerPort, FTP_DATA_CONNECTION_TIME_OUT);
                            if (__activeDataClient__ && !(__activeDataClient__->errText ())) {

                                __dataConnection__ = __activeDataClient__;

                                return "200 port ok\r\n";
                            }
                        }
                        return "425 can't open active data connection\r\n";
                    }

                    const char *__PASV__ () { // IPv4 PASV command
                        __closeDataConnection__ (); // just in case it is still opened

                        if (!*__homeDir__)                                                                
                            return "530 not logged in\r\n";

                        int ip1, ip2, ip3, ip4, p1, p2; // get FTP server IP and next free port
                        if (4 != sscanf (getServerIP (), "%i.%i.%i.%i", &ip1, &ip2, &ip3, &ip4)) { // should always succeed
                            #ifdef __DMESG__
                                dmesgQueue << "[ftpCtrlConn] can't parse server IP: " << getServerIP ();
                            #endif
                            return "425 can't open passive data connection\r\n";
                        }

                        // get next free port
                        int pasiveDataPort = __pasiveDataPort__ ();
                        p2 = pasiveDataPort % 256;
                        p1 = pasiveDataPort / 256;

                        // set up a new server for data connection and send connection information to the FTP client so it can connect to it
                        __passiveDataServer__ = new tcpServer_t (pasiveDataPort, NULL, FTP_DATA_CONNECTION_TIME_OUT); // run tcpServer in singl-task mode
                        if (__passiveDataServer__ && *__passiveDataServer__) {
                            // notify FTP client about data connection IP and port
                            cstring s;
                            sprintf  (s, "227 entering passive mode (%i,%i,%i,%i,%i,%i)\r\n", ip1, ip2, ip3, ip4, p1, p2);
                            if (sendString ((char *) s) <= 0)
                                return "";  // if control connection is closed
                            // DEBUG: cout << s;
                            __dataConnection__ = __passiveDataServer__->accept ();
                            if (__dataConnection__->setTimeout (FTP_DATA_CONNECTION_TIME_OUT) == -1) {
                                delete __dataConnection__;
                                delete __passiveDataServer__;
                                __dataConnection__ = NULL;
                                __passiveDataServer__ = NULL;
                                return "425 can't set passive data connection timeout\r\n";
                            }
                            return "";
                        }
                        return "425 can't open passive data connection\r\n";
                    }

                    const char *__EPSV__ () { // extended IPv6 (and IPv4) EPSV command
                        __closeDataConnection__ (); // just in case it is still opened

                        if (!*__homeDir__)                                     
                            return "530 not logged in\r\n";


                        // get next free port
                        int pasiveDataPort = __pasiveDataPort__ ();

                        // set up a new server for data connection and send connection information to the FTP client so it can connect to it
                        __passiveDataServer__ = new tcpServer_t (pasiveDataPort, NULL, FTP_DATA_CONNECTION_TIME_OUT); // run tcpServer in singl-task mode
                        if (__passiveDataServer__ && *__passiveDataServer__) {
                            // notify FTP client about data connection IP and port
                            cstring s;
                            sprintf  (s, "229 entering passive mode (|||%i|)\r\n", pasiveDataPort);
                            if (sendString ((char *) s) <= 0)
                                return "";  // if control connection is closed
                            // DEBUG: cout << s;
                            __dataConnection__ = __passiveDataServer__->accept ();
                            if (__dataConnection__->setTimeout (FTP_DATA_CONNECTION_TIME_OUT) == -1) {
                                delete __dataConnection__;
                                delete __passiveDataServer__;
                                __dataConnection__ = NULL;
                                __passiveDataServer__ = NULL;
                                return "425 can't set passive data connection timeout\r\n";
                            }
                            return "";
                        }
                        return "425 can't open passive data connection\r\n";
                    }

                    // NLST is preceeded by PORT or PASV so data connection should already be opened
                    const char *__NLST__ (char *directoryName) {         
                        const char *retVal = "";
                        if (!*__homeDir__) {
                            retVal = "530 not logged in\r\n";
                        } else {
                            if (!fileSystem.mounted ()) { 
                                retVal = "421 file system not mounted\r\n"; 
                            } else {
                                cstring fp = fileSystem.makeFullPath (directoryName, __workingDir__);
                                if (fp == "" || !fileSystem.isDirectory (fp)) { 
                                    retVal = "501 invalid directory name\r\n";                                   
                                } else {
                                    if (!__userHasRightToAccessDirectory__ (fp)) {
                                        retVal = "550 access denyed\r\n";
                                    } else {
                                        // notify FTP client about data transfer
                                        if (sendString ((char *) "150 starting data transfer\r\n") > 0) { 
                                            xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY); // Little FS is not very good at handling multiple files in multi-tasking environment
                                                File d = fileSystem.open (fp); 
                                                if (d) {
                                                    for (File f = d.openNextFile (); f; f = d.openNextFile ()) {
                                                        cstring fullFileName = fp;
                                                        if (fullFileName [fullFileName.length () - 1] != '/') fullFileName += '/'; fullFileName += f.name ();
                                                        if (__dataConnection__->sendString (fileSystem.fileInformation (fullFileName) + "\r\n") <= 0) {
                                                            retVal = "426 data transfer error\r\n";
                                                            break;
                                                        }
                                                    }
                                                    d.close ();
                                                }
                                            xSemaphoreGive (__tcpServerSemaphore__);

                                            if (!*retVal)
                                                sendString ((char *) "226 data transfer complete\r\n");
                                        }
                                    }
                                }
                            }
                        }

                        __closeDataConnection__ ();
                        return retVal;
                    }


                    char __rnfrPath__ [FILE_PATH_MAX_LENGTH + 1];
                    char __rnfrIs__;

                    const char *__RNFR__ (char *fileOrDirName) { 
                        __rnfrIs__ = ' ';
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n"; 
                        cstring fp = fileSystem.makeFullPath (fileOrDirName, __workingDir__);
                        if (fp == "")                                                                   return "501 invalid file or directory name\r\n"; 
                        if (fileSystem.isDirectory (fp)) {
                            if (!__userHasRightToAccessDirectory__ (fp))                                return "550 access denyed\r\n"; 
                            __rnfrIs__ = 'd';
                        } else if (fileSystem.isFile (fp)) {
                            if (!__userHasRightToAccessFile__ (fp))                                     return "550 access denyed\r\n"; 
                            __rnfrIs__ = 'f';
                        } else {
                                                                                                        return "501 invalid file or directory name\r\n"; 
                        }

                        // save temporal result
                        strncpy (__rnfrPath__, fp, FILE_PATH_MAX_LENGTH);
                        __rnfrPath__ [FILE_PATH_MAX_LENGTH] = 0;
                                                                                                        return "350 need more information\r\n"; // RNTO command will follow
                    }

                    const char *__RNTO__ (char *fileOrDirName) { 
                        if (!*__homeDir__)                                                              return "530 not logged in\r\n";
                        if (!fileSystem.mounted ())                                                     return "421 file system not mounted\r\n"; 
                        cstring fp = fileSystem.makeFullPath (fileOrDirName, __workingDir__);
                        if (fp == "")                                                                   return "501 invalid file or directory name\r\n"; 
                        if (__rnfrIs__ == 'd') {
                            if (!__userHasRightToAccessDirectory__ (fp))                                return "550 access denyed\r\n"; 
                        } else if (__rnfrIs__ == 'f') {
                            if (!__userHasRightToAccessFile__ (fp))                                     return "550 access denyed\r\n"; 
                        } else {
                                                                                                        return "501 invalid file or directory name\r\n"; 
                        }

                        // rename file from temporal result
                        if (fileSystem.rename (__rnfrPath__, fp))                                       return "250 renamed\r\n";
                                                                                                        return "553 unable to rename\r\n";  
                    }

                    const char * __RETR__ (char *fileName) { 
                        const char *retVal = "";
                        if (!*__homeDir__) {
                            retVal = "530 not logged in\r\n";
                        } else {
                            if (!fileSystem.mounted ()) {
                                retVal = "421 file system not mounted\r\n";
                            } else {
                                cstring fp = fileSystem.makeFullPath (fileName, __workingDir__);
                                if (fp == "" || fileSystem.isDirectory (fp)) { 
                                    retVal = "501 invalid file name\r\n";                                   
                                } else {
                                    if (!__userHasRightToAccessDirectory__ (fp)) {
                                        retVal = "550 access denyed\r\n";
                                    } else {

                                        // notify FTP client about data transfer
                                        if (sendString ((char *) "150 starting data transfer\r\n") > 0) { 

                                            xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY); // Little FS is not very good at handling multiple files in multi-tasking environment
                                                int bytesReadTotal = 0; int bytesSentTotal = 0;
                                                File f = fileSystem.open (fp, "r");
                                                if (f) {
                                                    // read data from file and transfer it through data connection
                                                    char buff [1024]; // MTU = 1500 (maximum transmission unit), TCP_SND_BUF = 5744 (a maximum block size that ESP32 can send), FAT cluster size = n * 512. 1024 seems a good trade-off
                                                    do {
                                                        int bytesReadThisTime = f.read ((uint8_t *) buff, sizeof (buff));
                                                        if (bytesReadThisTime == 0)
                                                            break; // finished, success

                                                        __diskTraffic__.bytesRead += bytesReadThisTime;

                                                        bytesReadTotal += bytesReadThisTime;
                                                        int bytesSentThisTime = __dataConnection__->sendBlock (buff, bytesReadThisTime);
                                                        if (bytesSentThisTime != bytesReadThisTime) {
                                                            retVal = "426 data transfer error\r\n";
                                                            break;
                                                        }
                                                        bytesSentTotal += bytesSentThisTime;
                                                    } while (true);
                                                    f.close ();
                                                } else {
                                                    retVal = "450 can not open the file\r\n";
                                                }
                                            xSemaphoreGive (__tcpServerSemaphore__);

                                            if (!*retVal)
                                                sendString ((char *) "226 data transfer complete\r\n");
                                        }
                                    }
                                }
                            }
                        }

                        __closeDataConnection__ ();
                        return retVal;                      
                    }

                    const char *__STOR__ (char *fileName) { 
                        const char *retVal = "";
                        if (!*__homeDir__) {
                            retVal = "530 not logged in\r\n";
                        } else {
                            if (!fileSystem.mounted ()) {
                                retVal = "421 file system not mounted\r\n";
                            } else {
                                cstring fp = fileSystem.makeFullPath (fileName, __workingDir__);
                                if (fp == "" || fileSystem.isDirectory (fp)) { 
                                    retVal = "501 invalid file name\r\n";                                   
                                } else {
                                    if (!__userHasRightToAccessDirectory__ (fp)) {
                                        retVal = "550 access denyed\r\n";
                                    } else {

                                        // notify FTP client about data transfer
                                        if (sendString ((char *) "150 starting data transfer\r\n") > 0) { 

                                            xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY); // Little FS is not very good at handling multiple files in multi-tasking environment
                                                int bytesRecvTotal = 0; int bytesWrittenTotal = 0;
                                                File f = fileSystem.open (fp, "w");
                                                if (f) {
                                                    // read data from data connection and store it to the file
                                                    char buff [1024]; // MTU = 1500 (maximum transmission unit), TCP_SND_BUF = 5744 (a maximum block size that ESP32 can send), FAT cluster size = n * 512. 1024 seems a good trade-off
                                                    do {
                                                        int bytesRecvThisTime = __dataConnection__->recv (buff, sizeof (buff));
                                                        if (bytesRecvThisTime < 0)  { 
                                                            retVal = "426 data transfer error\r\n";
                                                            break;
                                                        }
                                                        if (bytesRecvThisTime == 0)
                                                            break; // finished, success
                                                        bytesRecvTotal += bytesRecvThisTime;
                                                        int bytesWrittenThisTime = f.write ((uint8_t *) buff, bytesRecvThisTime);
                                                        bytesWrittenTotal += bytesWrittenThisTime;
                                                        if (bytesWrittenThisTime != bytesRecvThisTime) {
                                                            retVal = "450 can not write the file\r\n";
                                                            break;
                                                        } 

                                                        __diskTraffic__.bytesWritten += bytesWrittenThisTime;

                                                    } while (true);
                                                    f.close ();
                                                } else {
                                                    retVal = "450 can not open the file\r\n";
                                                }
                                            xSemaphoreGive (__tcpServerSemaphore__);

                                            if (!*retVal)
                                                sendString ((char *) "226 data transfer complete\r\n");
                                        }
                                    }
                                }
                            }
                        }

                        __closeDataConnection__ ();
                        return retVal;
                    }

            };


        public:

            ftpServer_t (int serverPort = 21,                                                                              // FTP server port
                         bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL                                  // a reference to callback function that will be celled when new connection arrives 
                        ) : tcpServer_t (serverPort, firewallCallback) {

                #ifdef USE_mDNS
                    // register mDNS servise
                    MDNS.addService ("ftp", "tcp", serverPort);
                    __ftpPort__ = serverPort; // remember FTP port so __NETWK__ onEvent handler would call MDNS.addService again when connected
                #endif
            }

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override {

                ftpControlConnection_t *connection = new (std::nothrow) ftpControlConnection_t (connectionSocket, clientIP, serverIP);

                if (!connection) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[ftpServer] can't create connection instance, out of memory?";
                    #endif
                    send (connectionSocket, ftpServiceUnavailableReply, strlen (ftpServiceUnavailableReply), 0);
                    close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
                    return NULL;
                } 

                if (connection->setTimeout (FTP_CONTROL_CONNECTION_TIME_OUT) == -1) {                 
                    #ifdef __DMESG__
                        dmesgQueue << "[ftpCtrlConn] setsockopt error: " << errno << strerror (errno);
                    #endif
                    connection->sendString (ftpServiceUnavailableReply);
                    delete (connection); // normally tcpConnection would do this but if it is not running we have to do it here
                    return NULL;
                } 

                if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                                        ftpControlConnection_t* ths = (ftpControlConnection_t *) thisInstance; // get "this" pointer

                                                                        xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY);
                                                                            __runningTcpConnections__ ++;
                                                                        xSemaphoreGive (__tcpServerSemaphore__);

                                                                        ths->__runConnectionTask__ ();

                                                                        xSemaphoreTake (__tcpServerSemaphore__, portMAX_DELAY);
                                                                            __runningTcpConnections__ --;
                                                                        xSemaphoreGive (__tcpServerSemaphore__);

                                                                        delete ths;
                                                                        vTaskDelete (NULL); // it is connection's responsibility to close itself
                                                                    }
                                            , "ftpCtrlConn", FTP_CONTROL_CONNECTION_STACK_SIZE, connection, tskNORMAL_PRIORITY, NULL)) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[ftpServer] can't create connection task, out of memory?";
                    #endif
                    connection->sendString (ftpServiceUnavailableReply);
                    delete (connection); // normally tcpConnection would do this but if it is not running we have to do it here
                    return NULL;
                }

                return NULL; // success, but don't return connection, since it may already been closed and destructed by now
            }

    };

    // declare static member outside of class definition
    UBaseType_t ftpServer_t::ftpControlConnection_t::__lastHighWaterMark__ = FTP_CONTROL_CONNECTION_STACK_SIZE;

#endif
