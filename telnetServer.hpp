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
 *          - telnetCommandHandler parameters are now easyer to access, 
 *            improved user management
 *            September 4th, Bojan Jurca     
 *          - minor structural changes,
 *            the use of dmesg
 *            September 14, 2019, Bojan Jurca        
 *  
 */


#ifndef __TELNET_SERVER__
  #define __TELNET_SERVER__

  void dmesg (String message);
  
  #include "file_system.h"        // telnetServer.hpp needs file_system.h
  #include "user_management.h"    // telnetServer.hpp needs user_management.h
  #include "TcpServer.hpp"        // telnetServer.hpp is built upon TcpServer.hpp

  // TO DO: make the following function memeber functions
  bool __readCommandLine__ (char *buffer, int bufferSize, bool echo, TcpConnection *connection);
  void __trimCstring__ (char *cstring);
  bool __ls__ (TcpConnection *connection, String directory);
  bool __cat__ (TcpConnection *connection, String fileName);
  bool __rm__ (TcpConnection *connection, String fileName);
  bool __ping__ (TcpConnection *, char *, int, int, int, int);
  bool __dmesg__ (TcpConnection *, bool);
  
  class telnetServer {                                             
  
    public:
  
      telnetServer (String (* telnetCommandHandler) (int argc, String argv [], String homeDirectory), // httpRequestHandler callback function provided by calling program
                    unsigned int stackSize,                                                           // stack size of httpRequestHandler thread, usually 4 KB will do 
                    char *serverIP,                                                                   // telnet server IP address, 0.0.0.0 for all available IP addresses - 15 characters at most!
                    int serverPort,                                                                   // telnet server port
                    bool (* firewallCallback) (char *)                                                // a reference to callback function that will be celled when new connection arrives 
                    )                           {
                                                  // start TCP server
                                                  this->__tcpServer__ = new TcpServer ( __telnetConnectionHandler__,    // worker function
                                                                                        (void *) telnetCommandHandler,  // tell TcpServer to pass reference callback function to __telnetConnectionHandler__
                                                                                        stackSize,                      // usually 4 KB will do for telnetConnectionHandler
                                                                                        300000,                         // close connection if inactive for more than 5 minutes
                                                                                        serverIP,                       // accept incomming connections on on specified addresses
                                                                                        serverPort,                     // telnet port
                                                                                        firewallCallback);              // firewall callback function

                                                  if (this->started ()) dmesg ("[TELNET] server started on " + String (serverIP) + ":" + String (serverPort) + (firewallCallback ? " with firewall." : "."));
                                                  else                  dmesg ("[TELNET] couldn't start Telnet server.");                                                                                        
                                                }
      
      ~telnetServer ()                          { 
                                                  if (this->__tcpServer__) {
                                                    dmesg ("[TELNET] server stopped.");
                                                    delete (this->__tcpServer__);
                                                  }
                                                }
      
      bool started ()                           { return this->__tcpServer__ && this->__tcpServer__->started (); } 

    private:

      TcpServer *__tcpServer__ = NULL;                                    // pointer to (threaded) TcpServer instance

      static void __telnetConnectionHandler__ (TcpConnection *connection, void *telnetCommandHandler) {  // connectionHandler callback function
        log_i ("[Thread:%i][Core:%i] connection started\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
        char prompt1 [] = "\r\n# "; // root user's prompt
        char prompt2 [] = "\r\n$ "; // other users' prompt
        char *prompt = prompt1;
        char cmdLine [256];  // make sure there is enough space for each type of use but be modest - this buffer uses thread stack
        #define IAC 255
        #define DONT 254
        #define ECHO 1
        char user [33]; *user = 0;          // store the name of the user that has logged in here 
        char homeDir [33]; *homeDir = 0;    // store home directory of the user that has logged in here

        #if USER_MANAGEMENT == NO_USER_MANAGEMENT
          getUserHomeDirectory (homeDir, "root");
          sprintf (cmdLine, "Hello %s%c%c%c! ", connection->getOtherSideIP (), IAC, DONT, ECHO); // say hello and tell telnet client not to echo, telnet server will do the echoing
          connection->sendData (cmdLine);
          if (*homeDir) { 
            dmesg ("[TELNET] " + String (user) + " logged in.");
            sprintf (cmdLine, "\r\n\nWelcome,\r\nuse \"/\" to refer to your home directory \"%s\",\r\nuse \"help\" to display available commands.\r\n%s", homeDir, prompt);
            connection->sendData (cmdLine);
          } else { 
            connection->sendData ("\r\n\nUser name or password incorrect\r\n");
            goto closeTelnetConnection;
          }
        #else
          sprintf (cmdLine, "Hello %s%c%c%c,\r\n\nuser: ", connection->getOtherSideIP (), IAC, DONT, ECHO); // say hello and tell telnet client not to echo, telnet server will do the echoing
          connection->sendData (cmdLine);
          // read user name
          if (!__readCommandLine__ (user, sizeof (user), true, connection)) goto closeTelnetConnection;
          connection->sendData ("\r\npassword: ");
          char password [33];
          if (!__readCommandLine__ (password, sizeof (password), false, connection)) goto closeTelnetConnection;
          if (checkUserNameAndPassword (user, password)) getUserHomeDirectory (homeDir, user);
          if (*homeDir) { 
            dmesg ("[TELNET] " + String (user) + " logged in.");
            if (strcmp (user, "root")) prompt = prompt2;
            sprintf (cmdLine, "\r\n\nWelcome %s,\r\nuse \"/\" to refer to your home directory \"%s\",\r\nuse \"help\" to display available commands.\r\n%s", user, homeDir, prompt);
            connection->sendData (cmdLine);
          } else {
            dmesg ("[TELNET] " + String (user) + " login attempt failed.");
            connection->sendData ("\r\n\nUser name or password incorrect.");
            SPIFFSsafeDelay (500); // TODO: check why last message doesn't get to the client (without SPIFFSsafeDelay) if we close the connection immediatelly
            goto closeTelnetConnection;
          }
        #endif

        while (__readCommandLine__ (cmdLine, sizeof (cmdLine), true, connection)) { // read and process comands in a loop
          if (*cmdLine) {
            
            connection->sendData ("\r\n");
            log_i ("[Thread:%i][Core:%i] telnet command %s\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID (), cmdLine);

            // ----- prepare command line arguments (max 32 arguments) -----
            
            int telnetArgc = 0; String telnetArgv [32] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""}; 
            telnetArgv [0] = String (cmdLine); telnetArgv [0].trim ();
            if (telnetArgv [0] != "") {
              telnetArgc = 1;
              while (telnetArgc < 32) {
                int l = telnetArgv [telnetArgc - 1].indexOf (" ");
                if (l > 0) {
                  telnetArgv [telnetArgc] = telnetArgv [telnetArgc - 1].substring (l + 1);
                  telnetArgv [telnetArgc].trim ();
                  telnetArgv [telnetArgc - 1] = telnetArgv [telnetArgc - 1].substring (0, l); // no need to trim
                  telnetArgc ++;
                } else {
                  break;
                }
              }
            }

            // ----- ask telnetCommandHandler (if it is provided by the calling program) if it is going to handle this command -----
    
            String telnetReply = ""; // get reply from telnetCommandHandler here
            unsigned long timeOutMillis = connection->getTimeOut (); connection->setTimeOut (TcpConnection::INFINITE); // disable time-out checking while proessing telnetCommandHandler to allow longer processing times
            if (telnetCommandHandler && (telnetReply = ((String (*) (int, String [], String)) telnetCommandHandler) (telnetArgc, telnetArgv, String (homeDir))) != "") {
              connection->sendData (telnetReply); // send everything to the client
            }
            connection->setTimeOut (timeOutMillis); // restore time-out checking
            
            if (telnetReply == "") {  // command has not been handeled by telnetCommandHandler - start handling built-in commands

              // ----- quit -----
              
              if (telnetArgv [0] == "quit") {
                
                if (telnetArgc == 1) goto closeTelnetConnection;
                else                 connection->sendData ("Unknown option.");
              
              // ----- ls or ls directory or dir or dir directory -----
  
              } else if (telnetArgv [0] == "ls" || telnetArgv [0] == "dir") {
                
                if (telnetArgc <= 2) __ls__ (connection, String (homeDir) + (telnetArgv [1].charAt (0) ==  '/' ? telnetArgv [1].substring (1) : telnetArgv [1]));
                else                 connection->sendData ("Unknown option.");

              // ----- cat filename or type filename -----

              } else if (telnetArgv [0] == "cat" || telnetArgv [0] == "type") {
                
                     if (telnetArgc < 2)  connection->sendData ("Missing option.");
                else if (telnetArgc == 2) __cat__ (connection, String (homeDir) + (telnetArgv [1].charAt (0) ==  '/' ? telnetArgv [1].substring (1) : telnetArgv [1]));
                else                      connection->sendData ("Unknown option.");                                

              // ----- rm fileName or del fileName -----

              } else if (telnetArgv [0] == "rm" || telnetArgv [0] == "del") {
                     if (telnetArgc < 2)  connection->sendData ("Missing option.");
                else if (telnetArgc == 2) __rm__ (connection, String (homeDir) + (telnetArgv [1].charAt (0) ==  '/' ? telnetArgv [1].substring (1) : telnetArgv [1]));
                else                      connection->sendData ("Unknown option.");

              // ----- ifconfig or ipconfig -----

              } else if (telnetArgv [0] == "ifconfig" || telnetArgv [0] == "ipconfig") {

                if (telnetArgc == 1) connection->sendData (ifconfig ());
                else                 connection->sendData ("Options are not supported.");

              // ----- arp -a -----

              } else if (telnetArgv [0] == "arp") {

                     if (telnetArgc == 1)                             connection->sendData (arp_a ());
                else if ((telnetArgc == 2 && telnetArgv [1] == "-a")) connection->sendData (arp_a ());
                else                                                  connection->sendData ("Only option -a is supported.");

              // ----- iw dev wlan1 station dump -----

              } else if (telnetArgv [0] == "iw") {

                     if (telnetArgc == 1)                                                                                                 connection->sendData (iw_dev_wlan1_station_dump ());
                else if ((telnetArgc == 5 && telnetArgv [1] + telnetArgv [2] + telnetArgv [3] + telnetArgv [4] == "devwlan1stationdump")) connection->sendData (iw_dev_wlan1_station_dump ());
                else                                                                                                                      connection->sendData ("Unknown option.");
                
              // ----- ping -----

              } else if (telnetArgv [0] == "ping") {

                     if (telnetArgc < 2)  connection->sendData ("Missing target IP.");
                else if (telnetArgc == 2) __ping__ (connection, (char *) telnetArgv [1].c_str (), 4, 1, 32, 1);
                else                      connection->sendData ("Unknown option."); 

              // ----- uptime -----

              } else if (telnetArgv [0] == "uptime") {

                if (telnetArgc == 1) {
                   String s;
                   unsigned long long upTime;
                   char cstr [15];
                   if (rtc.isGmtTimeSet ()) {
                     // get current local time first 
                     time_t rawTime = rtc.getLocalTime ();
                      strftime (cstr, 15, "%H:%M:%S", gmtime (&rawTime));
                      s = String (cstr) + " up ";
                      // now get up time
                      upTime = rtc.getGmtTime () - rtc.getGmtStartupTime ();
                    } else {
                      s = "Current time is not known, up time may not be accurate ";
                      upTime = millis () / 1000;
                    }
                    int seconds = upTime % 60; upTime /= 60;
                    int minutes = upTime % 60; upTime /= 60;
                    int hours = upTime % 60;   upTime /= 24; // upTime now holds days
                    if (upTime) s += String ((unsigned long) upTime) + " days, ";
                    sprintf (cstr, "%02i:%02i:%02i\r\n", hours, minutes, seconds);
                    s += String (cstr);
                    connection->sendData (s);
                } else  {
                  connection->sendData ("Options are not supported.");
                }

                // ----- reboot -----

                } else if (telnetArgv [0] == "reboot") {
                  
                  if (telnetArgc == 1) {
                    connection->sendData ("rebooting ...");
                    connection->closeConnection ();
                    Serial.printf ("\r\n\nreboot requested via telnet ...\r\n");
                    SPIFFSsafeDelay (100);
                    ESP.restart ();
                  } else {
                    connection->sendData ("Options are not supported.");
                  }

                // ----- help -----

                } else if (telnetArgv [0] == "help") {

                  if (telnetArgc == 1) {
                    *cmdLine = 0;
                    char homeDir [41]; // 33 for home directory and 8 for help.txt
                    if (getUserHomeDirectory (homeDir, "telnetserver")) {
                      if (!__cat__ (connection, String (homeDir) + "help.txt")) // send special reply
                        connection->sendData ("Please use FTP, loggin as root/rootpassword and upload help.txt file found in Esp32_web_ftp_telnet_server_template package into " + String (homeDir) + " directory.");
                    } else {
                      connection->sendData ("Unexpected error - telnetserver home directory not found.");
                    }                      
                  } else {
                    connection->sendData ("Options are not supported.");
                  }

                #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT

                  // ----- passwd -----

                  } else if (telnetArgv [0] == "passwd" && (telnetArgc == 1 || (telnetArgc == 2 && telnetArgv [1] == String (user)))) {

                    // read current password
                    connection->sendData ("Enter current password: ");
                    char password1 [33];
                    if (!__readCommandLine__ (password1, sizeof (password1), false, connection)) goto closeTelnetConnection;
                    // check if password is valid for user
                    if (checkUserNameAndPassword (user, password1)) {
                      // read new password twice
                      connection->sendData ("\r\nEnter new password: ");
                      if (!__readCommandLine__ (password1, sizeof (password1), false, connection)) goto closeTelnetConnection;
                      if (*password1) {
                        connection->sendData ("\r\nRe-enter new password: ");
                        char password2 [33];
                        if (!__readCommandLine__ (password2, sizeof (password2), false, connection)) goto closeTelnetConnection;
                          // check passwords
                          if (!strcmp (password1, password2)) // change password
                            if (passwd (String (user), String (password1))) 
                              connection->sendData ("\r\nPassword changed.\r\n");
                            else 
                              connection->sendData ("\r\nError changing password.");  
                          else  // different passwords
                            connection->sendData ("\r\nPasswords do not match.");    
                      } else { // first password is empty
                        connection->sendData ("\r\nPassword not changed.\r\n");  
                      }
                    } else { // wrong current pasword
                      connection->sendData ("\r\nWrong password.");  
                    }

                  } else if (telnetArgv [0] == "passwd" && telnetArgc == 2 && telnetArgv [1] != "root") {
                    
                    if (!strcmp (user, "root")) {
                      // check if user exists with getUserHomeDirectory
                      char userHomeDir [33];
                      if (getUserHomeDirectory (userHomeDir, (char *) telnetArgv [1].c_str ())) {
                        // read new password
                        sprintf (cmdLine, "Enter new password for %s: ", telnetArgv [1].c_str ());
                        connection->sendData (cmdLine);
                        char password1 [33];
                        if (!__readCommandLine__ (password1, sizeof (password1), false, connection)) goto closeTelnetConnection;
                        if (*password1) {
                          sprintf (cmdLine, "\r\nRe-enter new password for %s: ", telnetArgv [1].c_str ());
                          connection->sendData (cmdLine);
                          char password2 [33];
                          if (!__readCommandLine__ (password2, sizeof (password2), false, connection)) goto closeTelnetConnection;
                            // check passwords
                            if (!strcmp (password1, password2)) // change password
                              if (passwd (telnetArgv [1], String (password1)))
                                connection->sendData ("\r\nPassword changed.\r\n");
                              else
                                connection->sendData ("\r\nError changing password.");  
                            else // different passwords
                              connection->sendData ("\r\nPasswords do not match.");    
                        } else { // first password is empty
                          connection->sendData ("\r\nPassword not changed.");  
                        }
                      } else {
                        sprintf (cmdLine, "User %s does not exist.", telnetArgv [1].c_str ());
                        connection->sendData (cmdLine);
                      }
                    } else {
                      connection->sendData ("Only root may change password for another user.");
                    }

                  // ----- useradd -u userID -d homeDirectory userName -----

                  } else if (telnetArgv [0] == "useradd") { 

                    if (!strcmp (user, "root")) {
                      int userId;
                      if (telnetArgc == 6 && telnetArgv [1] == "-u" && (userId = atoi (telnetArgv [2].c_str ())) && userId >= 1000 && telnetArgv [3] == "-d")
                        if (userAdd (telnetArgv [5], telnetArgv [2], telnetArgv [4])) 
                          connection->sendData ("User created with defaul password changeimmediatelly. You may want to change it now.\r\n");
                        else
                          connection->sendData ("Error creating user. Maybe userName or userId already exist or userId is lower then 1000.");
                      else 
                        connection->sendData ("The only useradd syntax supported is useradd -u <userId> -d <userHomeDirectory> <userName>.");
                    } else {
                      connection->sendData ("Only root may add users.");
                    }

                  // ----- userdel  -----

                  } else if (telnetArgv [0] == "userdel") { 

                    if (!strcmp (user, "root")) 
                      if (telnetArgc == 2) 
                        if (telnetArgv [1] != "root") 
                          if (userDel (telnetArgv [1])) 
                            connection->sendData ("User deleted.\r\n");
                          else
                            connection->sendData ("Error deleting user.");
                        else
                          connection->sendData ("A really bad idea."); // deleting root
                      else
                        connection->sendData ("The only userdel syntax supported is userdel <userName>.");
                    else
                      connection->sendData ("Only root may delete users.");
                   
                #endif

                // ----- dmesg  -----

                  } else if (telnetArgv [0] == "dmesg") {

                         if (telnetArgc == 1)                                 __dmesg__ (connection, false);
                    else if (telnetArgc == 2 && telnetArgv [1] == "--follow") __dmesg__ (connection, true);
                    else                                                      connection->sendData ("The only dmesg syntax supported is dmesg (--follow).");
                                                  
                  } else {

                    // ----- invalid command -----
                    
                    connection->sendData ("Invalid command, use \"help\" to display available commands.");
                  }
              
            } // end of handling built-in comands

          } // if cmdLine is not empty
          connection->sendData (prompt);
        }
      
      closeTelnetConnection:
          if (*homeDir) dmesg ("[TELNET] " + String (user) + " logged out.");
          log_i ("[Thread:%i][Core:%i] connection ended\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
      }
            
  };

       
  // returns true if command line is read, false if connection is closed
  bool __readCommandLine__ (char *buffer, int bufferSize, bool echo, TcpConnection *connection) {
    unsigned char c;
    int i = 0;
    *buffer = 0; 
    while (connection->recvData ((char *) &c, 1)) { // read and process incomming data in a loop
      switch (c) {
          case 3:   // Ctrl-C
                    return false;
          
          case 127: // ignore
          case 10:  // ignore
                    break;

          case 8:   // backspace - delete last character from the buffer and from the screen
                    if (i) {
                      buffer [i--] = 0; // delete last character from buffer
                      if (echo) if (!connection->sendData ("\x08 \x08")) false; // delete the last character from the screen
                    }
                    break;                        

          case 13:  // end of command line
                    __trimCstring__ (buffer);
                    return true;
          
          default:  // fill buffer if the character is a valid character and there is still space in a buffer
                    if (c >= ' ' && c < 240 && i < bufferSize - 1) { // ignore control characters
                      buffer [i++] = c; // insert character into buffer
                      buffer [i] = 0;
                      if (echo) if (!connection->sendData ((char *) &c, 1)) return false; // write character to the screen
                    }
                    break;
      } // switch
    } // while
    return false;
  }

  void __trimCstring__ (char *cstring) {
    // ltrim
    int i = 0;
    while (cstring [i] == ' ' || cstring [i] == '\t') i++;
    if (i) strcpy (cstring, cstring + i);
    // rtrim
    i = strlen (cstring) - 1;
    while ((cstring [i] == ' ' || cstring [i] == '\t') && i >= 0) cstring [i--] = 0;
  }

  bool __ls__ (TcpConnection *connection, String directory) {
    char d [33]; *d = 0; 
    if (directory.length () < sizeof (d)) strcpy (d, directory.c_str ()); 
    if (*d && *(d + strlen (d) - 1) == '/') *(d + strlen (d) - 1) = 0;
    if (!*d) *d = '/';
    String s = "";

    xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY);
    File dir = SPIFFS.open (d);
    if (!dir) { // TO DO: debug - this doesn't work like expected
      xSemaphoreGive (SPIFFSsemaphore);
      connection->sendData ("Failed to open directory.");
      return false;
    }
    if (!dir.isDirectory ()) {
      xSemaphoreGive (SPIFFSsemaphore);
      connection->sendData (directory); connection->sendData (" is a file, not a directory."); 
      return false;
    }
    File file = dir.openNextFile ();
    while (file) {
      if(!file.isDirectory ()) {
        char c [10];
        sprintf (c, "  %6i ", file.size ());
        s += String (c) + String (file.name ()) + "\r\n";
      }
      file = dir.openNextFile ();
    }
    xSemaphoreGive (SPIFFSsemaphore);
    connection->sendData (s);
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
          *buff = 0;
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
        connection->sendData ("Failed to open "); connection->sendData (fileName);
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
      connection->sendData (fileName); connection->sendData (" deleted.\r\n");
      return true;
    } else {
      xSemaphoreGive (SPIFFSsemaphore);
      connection->sendData ("Failed to delete "); connection->sendData (fileName);
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

  struct __pingDataStructure__ {
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

  static void __pingPrepareEcho__ (__pingDataStructure__ *pds, struct icmp_echo_hdr *iecho, uint16_t len) {
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

  static err_t __pingSend__ (__pingDataStructure__ *pds, int s, ip4_addr_t *addr, int pingSize) {
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    size_t ping_size = sizeof (struct icmp_echo_hdr) + pingSize;
    int err;
  
    if (!(iecho = (struct icmp_echo_hdr *) mem_malloc ((mem_size_t) ping_size))) return ERR_MEM;
  
    __pingPrepareEcho__ (pds, iecho, (uint16_t) ping_size);
  
    to.sin_len = sizeof (to);
    to.sin_family = AF_INET;
    inet_addr_from_ipaddr (&to.sin_addr, addr);
  
    if ((err = sendto (s, iecho, ping_size, 0, (struct sockaddr*) &to, sizeof (to)))) pds->transmitted ++;
  
    return (err ? ERR_OK : ERR_VAL);
  }

  bool __pingRecv__ (__pingDataStructure__ *pds, TcpConnection *telnetConnection, int s) {
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
          if (!telnetConnection->sendData (cstr)) return false;
          
          return true;
        }
        else {
          // TODO: 
        }
      }
    }
  
    if (len < 0) {
      sprintf (cstr, "Request timeout for icmp_seq %d\r\n", pds->pingSeqNum);
      telnetConnection->sendData (cstr);
    }
  }  

  bool __ping__ (TcpConnection *telnetConnection, char *targetIP, int pingCount = PING_DEFAULT_COUNT, int pingInterval = PING_DEFAULT_INTERVAL, int pingSize = PING_DEFAULT_SIZE, int timeOut = PING_DEFAULT_TIMEOUT) {
    struct sockaddr_in address;
    ip4_addr_t pingTarget;
    int s;
    char cstr [256];
  
    // Create socket
    if ((s = socket (AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
      return false; // Error creating socket.
    }
  
    pingTarget.addr = inet_addr (targetIP); 
  
    // Setup socket
    struct timeval tOut;
  
    // Timeout
    tOut.tv_sec = timeOut;
    tOut.tv_usec = 0;
  
    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tOut, sizeof (tOut)) < 0) {
      closesocket (s);
      return false; // Error setting socket options
    }

    __pingDataStructure__ pds = {};
    pds.ID = random (0, 0xFFFF); // each consequently running ping command should have its own unique ID otherwise we won't be able to distinguish packets 
    pds.minTime = 1.E+9; // FLT_MAX;
  
    // Begin ping ...
  
    sprintf (cstr, "ping %s: %d data bytes\r\n",  targetIP, pingSize);
    if (!telnetConnection->sendData (cstr)) return false;
    
    while ((pds.pingSeqNum < pingCount) && (!pds.stopped)) {
      if (__pingSend__ (&pds, s, &pingTarget, pingSize) == ERR_OK) if (!__pingRecv__ (&pds, telnetConnection, s)) return false;
      SPIFFSsafeDelay (pingInterval * 1000L);
    }
  
    closesocket (s);
  
    sprintf (cstr, "%d packets transmitted, %d packets received, %.1f%% packet loss\r\n", pds.transmitted, pds.received, ((((float) pds.transmitted - (float) pds.received) / (float) pds.transmitted) * 100.0));
    if (!telnetConnection->sendData (cstr)) return false;
  
    if (pds.received) {
      sprintf (cstr, "round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\r\n", pds.minTime, pds.meanTime, pds.maxTime, sqrt (pds.varTime / pds.received));
      if (!telnetConnection->sendData (cstr)) return false;
      return true;
    }
    return false;
  }

  // ----- dmesg data structure and functions -----
  
  typedef struct __dmesgType__ {
    unsigned long milliseconds;    
    String        message;
  };

  #define __DMESG_CIRCULAR_QUEUE_LENGTH__ 64
  __dmesgType__ __dmesgCircularQueue__ [__DMESG_CIRCULAR_QUEUE_LENGTH__] = {{millis (), String ("[ESP32] started.")}}; // there is always at lease 1 message in the queue which makes things a little simper
  byte __dmesgBeginning__ = 0; // first used location
  byte __dmesgEnd__ = 1;       // the location next to be used
  portMUX_TYPE __csDmesg__ = portMUX_INITIALIZER_UNLOCKED;

  // displays dmesg circular queue over telnet connection
  bool __dmesg__ (TcpConnection *connection, bool follow) {
    // make a copy of all messages in circular queue in critical section
    portENTER_CRITICAL (&__csDmesg__);  
    byte i = __dmesgBeginning__;
    String s = "";
    do {
      if (s != "") s+= "\r\n";
      char c [15];
      sprintf (c, "[%10d] ", __dmesgCircularQueue__ [i].milliseconds);
      s += String (c) + __dmesgCircularQueue__ [i].message;
    } while ((i = (i + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) != __dmesgEnd__);
    portEXIT_CRITICAL (&__csDmesg__);
    // send everything to the client
    if (!connection->sendData (s + "\r\n")) return false;

    // -- follow?
    while (follow) {
      while (i == __dmesgEnd__) {
        while (connection->available () == TcpConnection::AVAILABLE) {
          char c;
          if (!connection->recvData (&c, sizeof (c))) return false;
          if (c == 3 || c >= ' ') return true; // return if user pressed Ctrl-C or any key
        }
        SPIFFSsafeDelay (10); // wait a while and check again
      }
      // __dmesgEnd__ has changed which means that at least one new message has been inserted into dmesg circular queue menawhile
      portENTER_CRITICAL (&__csDmesg__);
      s = "";
      do {
        if (s != "") s+= "\r\n";
        char c [15];
        sprintf (c, "[%10d] ", __dmesgCircularQueue__ [i].milliseconds);
        s += String (c) + __dmesgCircularQueue__ [i].message;
      } while ((i = (i + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) != __dmesgEnd__);
      portEXIT_CRITICAL (&__csDmesg__);
      // send everything to the client
      if (!connection->sendData (s + "\r\n")) return false;
    }
    return true;
  }

  // adds message into dmesg circular queue
  void dmesg (String message) {
    portENTER_CRITICAL (&__csDmesg__); 
    __dmesgCircularQueue__ [__dmesgEnd__].milliseconds = millis ();
    __dmesgCircularQueue__ [__dmesgEnd__].message = message;
    if ((__dmesgEnd__ = (__dmesgEnd__ + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) == __dmesgBeginning__) __dmesgBeginning__ = (__dmesgBeginning__ + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__;
    portEXIT_CRITICAL (&__csDmesg__);
  }

  // redirect other moduls' dmesg here before setup () begins
  bool __redirectDmesg__ () {
    #ifdef __NETWORK__
      networkDmesg = dmesg;
    #endif
    #ifdef __FTP_SERVER__
      ftpDmesg = dmesg;
    #endif    
    #ifdef __WEB_SERVER__
      webDmesg = dmesg;
    #endif  
    return true;
  }
  bool __redirectedDmesg__ = __redirectDmesg__ ();
  
#endif
