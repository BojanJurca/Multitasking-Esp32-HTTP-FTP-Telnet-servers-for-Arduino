/*
 * 
 * TelnetServer.hpp
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 * 
 *  TelnetServer is built upon TcpServer with connectionHandler that handles TcpConnection according to telnet protocol.
 * 
 *  A connectionHandler handles some telnet commands by itself but the calling program can provide its own callback
 *  function. In this case connectionHandler will first ask callback function wheather is it going to handle the telnet 
 *  request. If not, the connectionHandler will try to process it.
 * 
 * History:
 *          - first release, 
 *            November 29, 2018, Bojan Jurca
 *          - added ifconfig, arp -a, 
 *            December 9, 2018, Bojan Jurca
 *          - added iw dev wlan1 station dump, 
 *            December 11, 2018, Bojan Jurca          
 *          - added SPIFFSsemaphore and SPIFFSsafeDelay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca
 *  
 */


#ifndef __TELNET_SERVER__
  #define __TELNET_SERVER__
 
  // change this definitions according to your needs

  #define TELNET_HELP_FILE    "help.txt" // content of this file (licated in telnetserver home directory, /var/telnet by default) will be displayed when user types "help" command
  
  #include "file_system.h"        // telnetServer.hpp needs file_system.h
  #include "user_management.h"    // telnetServer.hpp needs user_management.h
  #include "TcpServer.hpp"        // telnetServer.hpp is built upon TcpServer.hpp


    void __telnetConnectionHandler__ (TcpConnection *connection, void *telnetCommandHandler);
  
  class telnetServer {                                             
  
    public:
  
      telnetServer (String (* telnetCommandHandler) (String command, String parameter, String homeDirectory), // httpRequestHandler callback function provided by calling program
                    unsigned int stackSize,                                                                   // stack size of httpRequestHandler thread, usually 4 KB will do 
                    char *serverIP,                                                                           // telnet server IP address, 0.0.0.0 for all available IP addresses - 15 characters at most!
                    int serverPort,                                                                           // telnet server port
                    bool (* firewallCallback) (char *)                                                        // a reference to callback function that will be celled when new connection arrives 
                    )                           {
                                                  // start TCP server
                                                  this->__tcpServer__ = new TcpServer ( __telnetConnectionHandler__,    // worker function
                                                                                        (void *) telnetCommandHandler,  // tell TcpServer to pass reference callback function to __telnetConnectionHandler__
                                                                                        stackSize,                      // usually 4 KB will do for telnetConnectionHandler
                                                                                        300000,                         // close connection if inactive for more than 5 minutes
                                                                                        serverIP,                       // accept incomming connections on on specified addresses
                                                                                        serverPort,                     // telnet port
                                                                                        firewallCallback);              // firewall callback function
                                                  if (this->started ()) Serial.printf ("[telnet] server started\n");
                                                  else                  Serial.printf ("[telnet] couldn't start telnet server\n");
                                                }
      
      ~telnetServer ()                          { if (this->__tcpServer__) delete (this->__tcpServer__); }
      
      bool started ()                           { return this->__tcpServer__ && this->__tcpServer__->started (); } 

    private:

      TcpServer *__tcpServer__ = NULL;                                    // pointer to (threaded) TcpServer instance
      
  };


      bool __ls__ (TcpConnection *connection, String directory);
      bool __cat__ (TcpConnection *connection, String fileName);
      bool __rm__ (TcpConnection *connection, String fileName);

      String ping (TcpConnection *, char *, int, int, int, int);
       
      void __telnetConnectionHandler__ (TcpConnection *connection, void *telnetCommandHandler) {  // connectionHandler callback function
        log_i ("[Thread:%i][Core:%i] connection started\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
        char prompt1 [] = "\r\n# "; // root user's prompt
        char prompt2 [] = "\r\n$ "; // other users' prompt
        char *prompt = prompt1;
        char buffer [256];   // make sure there is enough space for each type of use but be modest - this buffer uses thread stack
        char cmdLine [256];  // make sure there is enough space for each type of use but be modest - this buffer uses thread stack
        #define IAC 255
        #define DONT 254
        #define ECHO 1
        char user [33]; *user = 0;          // store the name of the user that has logged in here 
        char password [33];                 // store password while it is beeing retyped here
        #define NOT_LOGGED_IN       1
        #define LOGGED_IN           2
        #define READING_PASSWORD    3
        #define CHANGING_PASSWORD_1 4
        #define CHANGING_PASSWORD_2 5
        #define CHANGING_PASSWORD_3 6
        
        byte loggedIn = NOT_LOGGED_IN;      // "logged in" flag
        char homeDir [33]; *homeDir = 0;    // store home directory of the user that has logged in here
        #if USER_MANAGEMENT == NO_USER_MANAGEMENT
          loggedIn = LOGGED_IN;
          char *p = getUserHomeDirectory ("root"); if (p && strlen (p) < sizeof (homeDir)) strcpy (homeDir, p);
          sprintf (buffer, "Hello %s%c%c%c! ", connection->getOtherSideIP (), IAC, DONT, ECHO); // say hello and tell telnet client not to echo, telnet server will do the echoing
          connection->sendData (buffer, strlen (buffer));
          if (*homeDir) { 
            loggedIn = LOGGED_IN; 
            sprintf (buffer, "\r\n\nWelcome,\r\nuse \"/\" to refer to your home directory \"%s\",\r\nuse \"help\" to display available commands.\n%s", homeDir, prompt);
            connection->sendData (buffer, strlen ( buffer));
          } else { 
            connection->sendData ("\r\n\nUser name or password incorrect\r\n", strlen ("\r\nUser name or password incorrect\r\n"));
            connection->closeConnection (); 
          }
        #else
          sprintf (buffer, "Hello %s%c%c%c,\r\n\nuser: ", connection->getOtherSideIP (), IAC, DONT, ECHO); // say hello and tell telnet client not to echo, telnet server will do the echoing        
          connection->sendData (buffer, strlen (buffer));
        #endif          
        
        int k = 0; 
        while (int received = connection->recvData (buffer, sizeof (buffer) - 1)) { // read and process incomming data in a loop
          buffer [received] = 0; // mark the end of received characters
          int j = 0;
          for (int i = 0; buffer [i]; i++) { // scan through received characters
            switch (buffer [i]) {
              case 127: // ignore
              case 10:  // ignore
                        buffer [i] = 0;
                        break;
              case 13:  // try to execute the command
                        buffer [i] = 0; if (buffer [j] && loggedIn < READING_PASSWORD) connection->sendData (buffer + j, strlen (buffer + j)); j = i + 1; // echo back to client characters from j do i
                        if (k) {
                          cmdLine [k] = 0;
                          char *p; while (p = strstr (cmdLine, "  ")) strcpy (p, p + 1); // compact cmdLine
                          log_v ("[Thread:%lu][Core:%i] new command = %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), cmdLine);

                          // ----- if the user has not logged in yet then the first command that arrives holds user name -----

                          if (loggedIn == NOT_LOGGED_IN) {            // ----- user name -----
                            
                            if (strlen (cmdLine) < sizeof (user)) strcpy (user, cmdLine); else *user = 0;
                            connection->sendData ("\r\npassword: ", strlen ("\r\npassword: "));
                            loggedIn = READING_PASSWORD;

                          } else if (loggedIn == READING_PASSWORD) {  // ----- password -----

                            if (checkUserNameAndPassword (user, cmdLine)) { char *p = getUserHomeDirectory (user); if (p && strlen (p) < sizeof (homeDir)) strcpy (homeDir, p); }
                            if (*homeDir) { 
                              loggedIn = LOGGED_IN; 
                              if (*(homeDir + 1) != 0) prompt = prompt2;
                              sprintf (cmdLine, "\r\n\nWelcome %s,\r\nuse \"/\" to refer to your home directory \"%s\",\r\nuse \"help\" to display available commands.%s", user, homeDir, prompt);
                              connection->sendData (cmdLine, strlen (cmdLine));
                            } else { 
                              connection->sendData ("\r\n\nUser name or password incorrect\r\n", strlen ("\r\nUser name or password incorrect\r\n"));
                              connection->closeConnection (); 
                            }
                            
                        #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT
                        
                          } else if (loggedIn == CHANGING_PASSWORD_1) {  // ----- changing password (current password entered) -----
                            if (checkUserNameAndPassword (user, cmdLine)) { 
                              strcpy (cmdLine, "\r\nenter new password: ");
                              loggedIn = CHANGING_PASSWORD_2;
                            } else {
                              sprintf (cmdLine, "\r\n\nWrong password.%s", prompt);
                              loggedIn = LOGGED_IN;
                            }
                            connection->sendData (cmdLine, strlen (cmdLine));

                          } else if (loggedIn == CHANGING_PASSWORD_2) {  // ----- changing password (new password entered) -----
                            if (strlen (cmdLine) <= sizeof (password) - 1) { 
                              strcpy (password, cmdLine);
                              strcpy (cmdLine, "\r\nre-enter new password: ");
                              loggedIn = CHANGING_PASSWORD_3;
                            } else {
                              sprintf (cmdLine, "\r\n\nPassword too long (max %i characters).%s", strlen (password) - 1, prompt);
                              loggedIn = LOGGED_IN;
                            }   
                            connection->sendData (cmdLine, strlen (cmdLine));                           

                          } else if (loggedIn == CHANGING_PASSWORD_3) {  // ----- changing password (new password re-entered) -----
                            if (!strcmp (password, cmdLine)) { 
                              if (changeUserPassword (user, password)) {
                                sprintf (cmdLine, "\r\n\nPassword changed.%s", prompt);
                              } else {
                                sprintf (cmdLine, "\r\n\nError changing password.%s", prompt);  
                              }
                            } else {
                              sprintf (cmdLine, "\r\n\nPasswords do not match.%s", prompt);
                            } 
                            connection->sendData (cmdLine, strlen (cmdLine));
                            loggedIn = LOGGED_IN;      
                            
                        #endif                     

                          } else {                                  // ----- other telnet command -----

                            // ----- ask telnetCommandHandler (if it is provided by the calling program) if it is going to handle this command -----

                            String telnetReply = ""; String cmd = String (cmdLine); cmd.trim (); String param; int l = cmd.indexOf (" "); if (l > 0) {param = cmd.substring (l + 1); cmd = cmd.substring (0, l);} else {param = "";}
                            unsigned long timeOutMillis = connection->getTimeOut (); connection->setTimeOut (TCP_SERVER_INFINITE_TIMEOUT); // disable time-out checking while proessing telnetCommandHandler to allow longer processing times
                            if (telnetCommandHandler && (telnetReply = ((String (*) (String, String, String)) telnetCommandHandler) (cmd, param, String (homeDir))) != "") {
                              sprintf (cmdLine, "\r\n\n%s%s", telnetReply.substring (0, sizeof (cmdLine) - strlen (prompt) - 4).c_str (), prompt); 
                              connection->sendData (cmdLine, strlen (cmdLine)); // send everything to the client
                            }
                            connection->setTimeOut (timeOutMillis); // restore time-out checking
                            if (telnetReply == "") {  // command has not been handeled by telnetCommandHandler

                              // ----- quit -----
                              
                              if (cmd == "quit") { 
                                goto closeTelnetConnection;
                              
                              // ----- ls or ls directory or dir or dir directory -----

                              } else if (cmd == "ls" || cmd == "dir") {
                                __ls__ (connection, String (homeDir) + (param.charAt (0) ==  '/' ? param.substring (1) : param));
                                connection->sendData (prompt, strlen (prompt));

                              // ----- rm fileName or del fileName -----

                              } else if (cmd == "rm" || cmd == "del") {   

                                __rm__ (connection, String (homeDir) + (param.charAt (0) ==  '/' ? param.substring (1) : param));
                                connection->sendData (prompt, strlen (prompt));

                              // ----- ifconfig or ipconfig -----

                              } else if (cmd == "ifconfig" || cmd == "ipconfig") {
                                String s;
                                if (param == "")  s = "\r\n\n" + ifconfig ();
                                else              s = "\r\n\nOptions are not supported.\r\n";
                                s += String (prompt);
                                connection->sendData ((char *) s.c_str (), s.length ());

                              // ----- arp -a -----

                              } else if (cmd == "arp") {
                                String s;
                                if (param == "-a" || param == "")  s = "\r\n\n" + arp_a ();
                                else                               s = "\r\n\nOnly arp -a is supported.\r\n";
                                s += String (prompt);
                                connection->sendData ((char *) s.c_str (), s.length ());

                              // ----- iw dev wlan1 station dump -----

                              } else if (cmd == "iw") {
                                String s;
                                if (param == "dev wlan1 station dump" || param == "")  s = "\r\n\n" + iw_dev_wlan1_station_dump ();
                                else                                                   s = "\r\n\nOnly iw dev wlan1 station dump is supported.\r\n";
                                s += String (prompt);
                                connection->sendData ((char *) s.c_str (), s.length ());

                              // ----- ping -----

                              } else if (cmd == "ping") {
                                String s;
                                if (param != "")  s = ping (connection, (char *) param.c_str (), 4, 1, 32, 1);
                                else              s = "\r\n\nPing target must be specified.\r\n";
                                s += String (prompt);
                                connection->sendData ((char *) s.c_str (), s.length ());                                

                              // ----- uptime -----

                              } else if (cmd == "uptime") {
                                String s;
                                if (param == "")  { 
                                                    unsigned long long upTime;
                                                    char cstr [15];
                                                    if (rtc.isGmtTimeSet ()) {
                                                      // get current local time first 
                                                      time_t rawTime = rtc.getLocalTime ();
                                                      strftime (cstr, 15, "%H:%M:%S", gmtime (&rawTime));
                                                      s = "\r\n\n" + String (cstr) + " up ";
                                                      // now get up time
                                                      upTime = rtc.getGmtTime () - rtc.getGmtStartupTime ();
                                                    } else {
                                                      s = "\r\n\nCurrent time is not known, up time may not be accurate ";
                                                      upTime = millis () / 1000;
                                                    }
                                                    int seconds = upTime % 60; upTime /= 60;
                                                    int minutes = upTime % 60; upTime /= 60;
                                                    int hours = upTime % 60;   upTime /= 24; // upTime now holds days
                                                    if (upTime) s += String ((unsigned long) upTime) + " days, ";
                                                    sprintf (cstr, "%02i:%02i:%02i\r\n", hours, minutes, seconds);
                                                    s += String (cstr);
                                                  }
                                else              s = "\r\n\nOnly uptime without options is supported.\r\n";
                                s += String (prompt);
                                connection->sendData ((char *) s.c_str (), s.length ());                                

                              // ----- reboot -----

                              } else if (cmd == "reboot") {
                                if (param != "") connection->sendData ("\r\n\nOnly reboot without options is supported.\r\n", strlen ("\r\n\nOnly reboot without options is supported.\r\n"));
                                else             {
                                                   connection->sendData ("\r\n\nrebooting ...", strlen ("\r\n\nrebooting ..."));
                                                   connection->closeConnection ();
                                                   Serial.printf ("\r\n\nreboot requested via telnet ...\r\n\n");
                                                   SPIFFSsafeDelay (1000);
                                                   ESP.restart ();
                                                 }

                              // ----- help -----

                              } else if (cmd == "help") {
                                *cmdLine = 0;
                                char *p = getUserHomeDirectory ("telnetserver"); if (p && strlen (p) < sizeof (cmdLine)) strcpy (cmdLine, p);
                                if (*cmdLine) {
                                  param = String (cmdLine) + "help.txt";
                                  if (!__cat__ (connection, param)) {
                                    // send special reply
                                    String s = "\r\n\nPlease use FTP, loggin as root / rootpassword and upload help.txt file found in Esp32_web_ftp_telnet_server_template package into " + String (cmdLine) + " directory.\r\n"; 
                                     connection->sendData ((char *) s.c_str (), s.length ());
                                  }
                                  connection->sendData (prompt, strlen (prompt));
                                }

                              // ----- cat filename or type filename -----

                              } else if (cmd == "cat" || cmd == "type") {
                                __cat__ (connection, String (homeDir) + (param.charAt (0) ==  '/' ? param.substring (1) : param));
                                connection->sendData (prompt, strlen (prompt));

                              // ----- passwd -----

                            #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT
                            
                              } else if (cmd == "passwd") {
                                if (param != "") {
                                  sprintf (cmdLine, "\r\n\nCan't change password for user %s.%s", param.c_str (), prompt);
                                } else {
                                  sprintf (cmdLine, "\r\n\nenter current password for user %s: ", user);
                                  loggedIn = CHANGING_PASSWORD_1;
                                }
                                connection->sendData (cmdLine, strlen (cmdLine));

                            #endif
                                                              
                              } else {

                                // ----- invalid command -----
                                
                                sprintf (cmdLine, "\r\n\nInvalid command, use \"help\" to display available commands.\n%s", prompt);
                                connection->sendData (cmdLine, strlen (cmdLine));
                              }
                            }
                          }
                          k = 0; 
                        } 
                        break;
              case 8:   // backspace
                        strcpy (buffer + i, buffer + i-- + 1);  // ignore backspace in echo response ...
                        if (k) {k--; if (loggedIn < READING_PASSWORD) connection->sendData ("\x08 \x08", 3);} // ... delete the last character from the screen instead
                        break;
              default:  // if buffer [i] is a valid character then fill up cmdLine else ignore it
                        if (buffer [i] >= ' ' && buffer [i] < 240 && k < sizeof (cmdLine) - 1) cmdLine [k++] = buffer [i]; // ignore control characters
                        else strcpy (buffer + i, buffer + i-- + 1); 
            }
          } // scan through received characters
          if (buffer [j] && loggedIn < READING_PASSWORD) connection->sendData (buffer + j, strlen (buffer + j)); // echo back to client characters from j further
        } // read and process incomming data in a loop
      closeTelnetConnection:
          log_i ("[Thread:%i][Core:%i] connection ended\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
      }

      bool __ls__ (TcpConnection *connection, String directory) {
        char d [33]; *d = 0; if (directory.length () < sizeof (d)) strcpy (d, directory.c_str ()); if (strlen (d) > 1 && *(d + strlen (d) - 1) == '/') *(d + strlen (d) - 1) = 0;
        String s = "\r\n\n";

        xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY);
        
        File dir = SPIFFS.open (d);
        if (!dir) {
          
          xSemaphoreGive (SPIFFSsemaphore);
          
          connection->sendData ("\r\n\nFailed to open directory.\r\n", strlen ("\r\n\nFailed to open directory.\r\n"));
          return false;
        }
        if (!dir.isDirectory ()) {

          xSemaphoreGive (SPIFFSsemaphore);
          
          connection->sendData ("\r\n\nInvalid directory.", strlen ("\r\n\nInvalid directory.")); 
          return false;
        }
        File file = dir.openNextFile ();
        while (file) {
          if(!file.isDirectory ()) {
            char c [10];
            sprintf (c, "  %6i ", file.size ());
            s += String (c) + String (file.name ()) + String ("\r\n");
          }
          file = dir.openNextFile ();
        }

        xSemaphoreGive (SPIFFSsemaphore);
        
        connection->sendData ((char *) s.c_str (), s.length ());
        return true;
      }

      bool __cat__ (TcpConnection *connection, String fileName) {
        bool retVal = false;
        File file;

        xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY);
        
        if ((bool) (file = SPIFFS.open (fileName, FILE_READ))) {
          if (!file.isDirectory ()) {
            char *buff = (char *) malloc (2048); // get 2 KB of memory from heap (not from the stack)
            if (buff) {
              strcpy (buff, "\r\n\n");
              int i = strlen (buff);
              while (file.available ()) {
                switch (*(buff + i) = file.read ()) {
                  case '\r':  // ignore
                              break;
                  case '\n':  // crlf conversion
                              *(buff + i ++) = '\r'; 
                              *(buff + i ++) = '\n';
                              break;
                  default:
                              i ++;                  
                }
                if (i >= 2048 - 1) { connection->sendData ((char *) buff, i); i = 0; }
              }
              if (i) { connection->sendData ((char *) buff, i); }
              free (buff);
              retVal = true;
            } 
            file.close ();
          } else {
            String s = "\r\n\nFailed to open " + fileName + ".\r\n";
            connection->sendData ((char *) s.c_str (), s.length ()); 
          }
          file.close ();
        } 

        xSemaphoreGive (SPIFFSsemaphore);
        
        return retVal;
      }

      bool __rm__ (TcpConnection *connection, String fileName) {
        
        xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY);
        
        if (SPIFFS.remove (fileName)) {

          xSemaphoreGive (SPIFFSsemaphore);
          
          String s = "\r\n\n" + fileName + " deleted.\r\n";
          connection->sendData ((char *) s.c_str (), s.length ()); 
          return true;
        } else {

          xSemaphoreGive (SPIFFSsemaphore);
          
          String s = "\r\n\nFailed to delete " + fileName + ".\r\n";
          connection->sendData ((char *) s.c_str (), s.length ()); 
          return false;          
        }
      }

  // ----- ping ----- according to: https://github.com/pbecchi/ESP32_ping

  #include "lwip/inet_chksum.h"
  #include "lwip/ip.h"
  #include "lwip/ip4.h"
  #include "lwip/err.h"
  #include "lwip/icmp.h"
  #include "lwip/sockets.h"
  #include "lwip/sys.h"
  #include "lwip/netdb.h"
  #include "lwip/dns.h"

  #define PING_DEFAULT_COUNT     4
  #define PING_DEFAULT_INTERVAL  1
  #define PING_DEFAULT_SIZE     32
  #define PING_DEFAULT_TIMEOUT   1

  struct pingDataStructure {
    uint16_t ID;
    uint16_t pingSeqNum;
    uint8_t stopped = 0;
    uint32_t transmitted = 0;
    uint32_t received = 0;
    float minTime = 0;
    float maxTime = 0;
    float meanTime = 0;
    float lastMeanTime = 0;
    float varTime = 0;
  };

  static void pingPrepareEcho (pingDataStructure *pds, struct icmp_echo_hdr *iecho, uint16_t len) {
    size_t i;
    size_t data_len = len - sizeof (struct icmp_echo_hdr);
  
    ICMPH_TYPE_SET (iecho, ICMP_ECHO);
    ICMPH_CODE_SET (iecho, 0);
    iecho->chksum = 0;
    iecho->id = pds->ID;
    iecho->seqno = htons (++pds->pingSeqNum);
  
    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++) ((char*) iecho)[sizeof (struct icmp_echo_hdr) + i] = (char) i;
  
    iecho->chksum = inet_chksum (iecho, len);
  }

  static err_t pingSend (pingDataStructure *pds, int s, ip4_addr_t *addr, int pingSize) {
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    size_t ping_size = sizeof (struct icmp_echo_hdr) + pingSize;
    int err;
  
    if (!(iecho = (struct icmp_echo_hdr *) mem_malloc ((mem_size_t) ping_size))) return ERR_MEM;
  
    pingPrepareEcho (pds, iecho, (uint16_t) ping_size);
  
    to.sin_len = sizeof (to);
    to.sin_family = AF_INET;
    inet_addr_from_ipaddr (&to.sin_addr, addr);
  
    if ((err = sendto (s, iecho, ping_size, 0, (struct sockaddr*) &to, sizeof (to)))) pds->transmitted ++;
  
    return (err ? ERR_OK : ERR_VAL);
  }

  static void pingRecv (pingDataStructure *pds, TcpConnection *telnetConnection, int s) {
    char buf [64];
    int fromlen, len;
    struct sockaddr_in from;
    struct ip_hdr *iphdr;
    struct icmp_echo_hdr *iecho = NULL;
    char ipa[16];
    struct timeval begin;
    struct timeval end;
    uint64_t microsBegin;
    uint64_t microsEnd;
    float elapsed;

    char cstr [255];    
  
    // Register begin time
    gettimeofday (&begin, NULL);
  
    // Send
    while ((len = recvfrom (s, buf, sizeof (buf), 0, (struct sockaddr *) &from, (socklen_t *) &fromlen)) > 0) {
      if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
        // Register end time
        gettimeofday (&end, NULL);
  
        /// Get from IP address
        ip4_addr_t fromaddr;
        inet_addr_to_ipaddr (&fromaddr, &from.sin_addr);
  
        strcpy (ipa, inet_ntoa (fromaddr));
  
        // Get echo
        iphdr = (struct ip_hdr *) buf;
        iecho = (struct icmp_echo_hdr *) (buf + (IPH_HL(iphdr) * 4));
  
        // Print ....
        if ((iecho->id == pds->ID) && (iecho->seqno == htons (pds->pingSeqNum))) {
          pds->received ++;
  
          // Get elapsed time in milliseconds
          microsBegin = begin.tv_sec * 1000000;
          microsBegin += begin.tv_usec;
  
          microsEnd = end.tv_sec * 1000000;
          microsEnd += end.tv_usec;
  
          elapsed = (float) (microsEnd - microsBegin) / (float) 1000.0;
  
          // Update statistics
          // Mean and variance are computed in an incremental way
          if (elapsed < pds->minTime) pds->minTime = elapsed;
          if (elapsed > pds->maxTime) pds->maxTime = elapsed;
  
          pds->lastMeanTime = pds->meanTime;
          pds->meanTime = (((pds->received - 1) * pds->meanTime) + elapsed) / pds->received;
  
          if (pds->received > 1) pds->varTime = pds->varTime + ((elapsed - pds->lastMeanTime) * (elapsed - pds->meanTime));
  
          // Print ...
          sprintf (cstr, "%d bytes from %s: icmp_seq=%d time=%.3f ms\r\n", len, ipa, ntohs (iecho->seqno), elapsed);
          telnetConnection->sendData (cstr, strlen (cstr));
          
            return;
        }
        else {
          // TODO
        }
      }
    }
  
    if (len < 0) {
      sprintf (cstr, "Request timeout for icmp_seq %d\r\n", pds->pingSeqNum);
      telnetConnection->sendData (cstr, strlen (cstr));
    }
  }  

  String ping (TcpConnection *telnetConnection, char *targetIP, int pingCount = PING_DEFAULT_COUNT, int pingInterval = PING_DEFAULT_INTERVAL, int pingSize = PING_DEFAULT_SIZE, int timeOut = PING_DEFAULT_TIMEOUT) {
    struct sockaddr_in address;
    ip4_addr_t pingTarget;
    int s;
    char cstr [256];
  
    // Create socket
    if ((s = socket (AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
      return "Error creating socket.\r\n";
    }
  
    pingTarget.addr = inet_addr (targetIP); 
  
    // Setup socket
    struct timeval tOut;
  
    // Timeout
    tOut.tv_sec = timeOut;
    tOut.tv_usec = 0;
  
    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tOut, sizeof (tOut)) < 0) {
      closesocket (s);
      return "Error setting socket options.\r\n";
    }

    pingDataStructure pds = {};
    pds.ID = random (0, 0xFFFF); // each consequently running ping commanf should have its own unique ID otherwise we won't be able to distinguish packets 
    pds.minTime = 1.E+9; // FLT_MAX;
  
    // Begin ping ...
  
    sprintf (cstr, "\r\n\nping %s: %d data bytes\r\n",  targetIP, pingSize);
    telnetConnection->sendData (cstr, strlen (cstr));
    
    while ((pds.pingSeqNum < pingCount) && (!pds.stopped)) {
      if (pingSend (&pds, s, &pingTarget, pingSize) == ERR_OK) pingRecv (&pds, telnetConnection, s);
      SPIFFSsafeDelay (pingInterval * 1000L);
    }
  
    closesocket (s);
  
    sprintf (cstr, "%d packets transmitted, %d packets received, %.1f%% packet loss\r\n", pds.transmitted, pds.received, ((((float) pds.transmitted - (float) pds.received) / (float) pds.transmitted) * 100.0));
    telnetConnection->sendData (cstr, strlen (cstr));
  
    if (pds.received) {
      sprintf (cstr, "round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\r\n", pds.minTime, pds.meanTime, pds.maxTime, sqrt (pds.varTime / pds.received));
      telnetConnection->sendData (cstr, strlen (cstr));
      return ""; // true
    }
    return ""; // false
  }

#endif
