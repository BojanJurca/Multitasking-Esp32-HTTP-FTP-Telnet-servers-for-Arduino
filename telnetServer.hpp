/*
 * 
 * TelnetServer.hpp
 * 
 *  This file is part of A_kind_of_esp32_OS_template.ino project: https://github.com/BojanJurca/A_kind_of_esp32_OS_template
 * 
 *  TelnetServer is built upon TcpServer with connectionHandler that handles TcpConnection according to telnet protocol.
 * 
 *  A connectionHandler handles some telnet commands by itself but the calling program can provide its own callback
 *  function. In this case connectionHandler will first ask callback function wheather is it going to handle the telnet 
 *  request. If not, the connectionHandler will try to process it.
 * 
 * History:
 *          - first release, November 29, 2018, Bojan Jurca
 *          - added ifconfig, arp -a, December 9, 2018, Bojan Jurca
 *          - added iw dev wlan1 station dump, December 11, 2018, Bojan Jurca          
 *          - adjusted buffer size to default MTU size (1500), December 12, 2018, Bojan Jurca          
 *  
 */


#ifndef __TELNET_SERVER__
  #define __TELNET_SERVER__
 
  // change this definitions according to your needs

  #define TELNET_HELP_FILE    "help.txt" // content of this file (licated in telnetserver home directory, /var/telnet by default) will be displayed when user types "help" command
  
  // DEBUG_LEVEL 5 // error messges
  // DEBUG_LEVEL 6 // main telnetServer activities
  #define DEBUG_LEVEL 5
  #include "debugmsgs.h"          // telnetServer.hpp needs debugmsgs.h
  #include "file_system.h"        // telnetServer.hpp needs file_system.h
  #include "user_management.h"    // telnetServer.hpp needs user_management.h
  #include "TcpServer.hpp"        // telnetServer.hpp is built upon TcpServer.hpp

  #ifndef MTU
    #define MTU 1500 // default MTU size
  #endif


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
                                                                                        180000,                         // close connection if inactive for more than 3 minutes
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
       
      void __telnetConnectionHandler__ (TcpConnection *connection, void *telnetCommandHandler) {  // connectionHandler callback function
        dbgprintf63 ("[telnet][Thread %i][Core %i] connection has started\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
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
                          dbgprintf64 ("[telnet][Thread %lu][Core %i] new command = %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), cmdLine);

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
                                                   delay (1000);
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
                                    String s = "\r\n\nPlease use FTP, loggin as root / rootpassword and upload help.txt file found in a_kind_of_esp32_OS_template package into " + String (cmdLine) + " directory.\r\n"; 
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
          dbgprintf63 ("[telnet][Thread %i][Core %i] connection has ended\n", xTaskGetCurrentTaskHandle (), xPortGetCoreID ());  
      }

      bool __ls__ (TcpConnection *connection, String directory) {
        char d [33]; *d = 0; if (directory.length () < sizeof (d)) strcpy (d, directory.c_str ()); if (strlen (d) > 1 && *(d + strlen (d) - 1) == '/') *(d + strlen (d) - 1) = 0;
        String s = "\r\n\n";
        File dir = SPIFFS.open (d);
        if (!dir) {
          connection->sendData ("\r\n\nFailed to open directory.\r\n", strlen ("\r\n\nFailed to open directory.\r\n"));
          return false;
        }
        if (!dir.isDirectory ()) {
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
        connection->sendData ((char *) s.c_str (), s.length ());
        return true;
      }

      bool __cat__ (TcpConnection *connection, String fileName) {
        bool retVal = false;
        File file;
        if ((bool) (file = SPIFFS.open (fileName, FILE_READ))) {
          if (!file.isDirectory ()) {
            char *buff = (char *) malloc (MTU); // get 2 KB of memory from heap (not from the stack)
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
                if (i >= MTU - 1) { connection->sendData ((char *) buff, i); i = 0; }
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
        return retVal;
      }

      bool __rm__ (TcpConnection *connection, String fileName) {
        if (SPIFFS.remove (fileName)) {
          String s = "\r\n\n" + fileName + " deleted.\r\n";
          connection->sendData ((char *) s.c_str (), s.length ()); 
          return true;
        } else {
          String s = "\r\n\nFailed to delete " + fileName + ".\r\n";
          connection->sendData ((char *) s.c_str (), s.length ()); 
          return false;          
        }
      }

#endif
