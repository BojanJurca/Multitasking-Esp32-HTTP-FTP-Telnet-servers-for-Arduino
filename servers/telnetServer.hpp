/*
        String homeDir;
        String workingDir;
        char *prompt;        
        TcpConnection *connection;
        byte clientWindowCol1;
        byte clientWindowCol2;
        byte clientWindowRow1;
        byte clientWindowRow2;
        // feel free to add more
      } telnetSessionParameters;    
  
      telnetServer (String (*telnetCommandHandler) (int argc, String argv [], telnetServer::telnetSessionParameters *tsp),  // httpRequestHandler callback function provided by calling program
                    size_t stackSize,                                                                                       // stack size of httpRequestHandler thread, usually 16 KB will do 
                    String serverIP,                                                                                        // telnet server IP address, 0.0.0.0 for all available IP addresses
                    int serverPort,                                                                                         // telnet server port
                    bool (*firewallCallback) (String connectingIP)                                                          // a reference to callback function that will be celled when new connection arrives 
                   ): TcpServer (__staticTelnetConnectionHandler__, (void *) this, stackSize, (TIME_OUT_TYPE) 300000, serverIP, serverPort, firewallCallback)
                        {
                          __externalTelnetCommandHandler__ = telnetCommandHandler;

                          if (started ()) dmesg ("[telnetServer] started on " + String (serverIP) + ":" + String (serverPort) + (firewallCallback ? " with firewall." : "."));
                          else            dmesg ("[telnetServer] couldn't start.");
                        }

      ~telnetServer ()  { if (started ()) dmesg ("[telnetServer] stopped."); }
      
    private:

      String (* __externalTelnetCommandHandler__) (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) = NULL; // telnet command handler supplied by calling program 

      static void __staticTelnetConnectionHandler__ (TcpConnection *connection, void *ths) {  // connectionHandler callback function
        ((telnetServer *) ths)->__telnetConnectionHandler__ (connection);
      }

      virtual void __telnetConnectionHandler__ (TcpConnection *connection) { // connectionHandler callback function

        // this is where telnet session begins
        telnetSessionParameters tsp = {"", "", (char *) "", NULL, 0, 0, 0, 0};
        String password;
        
        String cmdLine;

        #if USER_MANAGEMENT == NO_USER_MANAGEMENT

          tsp = {"root", "", "", (char *) "\r\n# ", connection, 0, 0, 0, 0};
          tsp.workingDir = tsp.homeDir = getUserHomeDirectory (tsp.userName);     
          // tell the client to go into character mode, not to echo an send its window size, then say hello 
          dmesg ("[telnetServer] " + tsp.userName + " logged in.");
          connection->sendData ("\r\n\nWelcome " + tsp.userName + ",\r\nyour home directory is " + tsp.homeDir + ",\r\nuse \"help\" to display available commands.\r\n" + tsp.prompt);
        
        #else

          tsp = {"", "", "", (char *) "", connection, 0, 0, 0, 0};
          // tell the client to go into character mode, not to echo an send its window size, then say hello 
          // read user name
          if (13 != __readLineFromClient__ (&tsp.userName, true, &tsp)) goto closeTelnetConnection;
          tsp.workingDir = tsp.homeDir = getUserHomeDirectory (tsp.userName);
          tsp.prompt = tsp.userName == "root" ? (char *) "\r\n# " : (char *) "\r\n$ ";
          connection->sendData ((char *) "\r\npassword: ");
          if (13 != __readLineFromClient__ (&password, false, &tsp)) goto closeTelnetConnection;
          if (!checkUserNameAndPassword (tsp.userName, password)) {
            dmesg ("[telnetServer] " + tsp.userName + " entered wrong password.");
            goto closeTelnetConnection;
          }
          
          if (tsp.homeDir != "") { 
            dmesg ("[telnetServer] " + tsp.userName + " logged in.");
            connection->sendData ("\r\n\nWelcome " + tsp.userName + ",\r\nyour home directory is " + tsp.homeDir + ",\r\nuse \"help\" to display available commands.\r\n" + tsp.prompt);          
          } else {
            dmesg ("[telnetServer] " + tsp.userName + " login attempt failed.");
            connection->sendData ((char *) "\r\n\nUser name or password incorrect.");
            delay (100); // TODO: check why last message doesn't get to the client (without delay) if we close the connection immediatelly
            goto closeTelnetConnection;
          }
        
        #endif

        while (13 == __readLineFromClient__ (&cmdLine, true, &tsp)) { // read and process comands in a loop        
          if (cmdLine.length ()) {
            connection->sendData ((char *) "\r\n");

            // ----- parse command line into arguments (max 32) -----
            cmdLine += ' ';
            int argc = 0; String argv [32] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
            bool quotation = false; 
            for (int i = 0; i < cmdLine.length (); i++) {
              switch (char c = cmdLine.charAt (i)) {
                case '\"':  if (cmdLine.charAt (i + 1) == '\"') { argv [argc] += '\"'; i ++; }
                            else quotation = !quotation;
                            break;
                case ' ':   if (quotation) argv [argc] += ' ';
                            else if (argv [argc] > "") argc ++;
                            break;
                default:    argv [argc] += c;
              }
              if (argc >= 32) break;
            }
            if (quotation) {
              connection->sendData ((char *) "Quotation not finished. Missing ending \".");
            } else {
              // debug: for (int i = 0; i < argc; i++) Serial.print ("|" + argv [i] + "|     "); Serial.println ();
  
              // ----- ask telnetCommandHandler (if it is provided by the calling program) if it is going to handle this command, otherwise try to handle it internally -----
      
              String r;
              // unsigned long timeOutMillis = connection->getTimeOut (); connection->setTimeOut (TcpConnection::INFINITE); // disable time-out checking while proessing telnetCommandHandler to allow longer processing times
              if (__externalTelnetCommandHandler__ && (r = __externalTelnetCommandHandler__ (argc, argv, &tsp)) != "") 
                connection->sendData (r); // send reply to telnet client
              else connection->sendData (__internalTelnetCommandHandler__ (argc, argv, &tsp)); // send reply to telnet client

            } // if command line ended out of quotation
          } // if cmdLine is not empty
          connection->sendData (tsp.prompt);
        } // read and process comands in a loop

      #if USER_MANAGEMENT == NO_USER_MANAGEMENT
        goto closeTelnetConnection; // this is just to get rid of compier warning, otherwise it doesn't make sense
      #endif
      closeTelnetConnection:      
        if (tsp.userName != "") dmesg ("[telnetServer] " + tsp.userName + " logged out.");
      }

      // returns last chracter pressed (Enter, Ctrl-C, Ctrl-D (Ctrl-Z)) or 0 in case of error
      static char __readLineFromClient__ (String *line, bool echo, telnetSessionParameters *tsp) {
        *line = "";
        
        char c;
        while (tsp->connection->recvData (&c, 1)) { // read and process incomming data in a loop 
          switch (c) {
              case 3:   // Ctrl-C
                        *line = "";
                        return 0;
              case 10:  // ignore
                        break;
              case 8:   // backspace - delete last character from the buffer and from the screen
              case 127: // in Windows telent.exe this is del key but putty this backspace with this code so let's treat it the same way as backspace
                        if (line->length () > 0) {
                          *line = line->substring (0, line->length () - 1);
                          if (echo) if (!tsp->connection->sendData ((char *) "\x08 \x08")) return 0; // delete the last character from the screen
                        }
                        break;                            
              case 13:  // end of command line
                        return 13;
              case 4:   // EOF - Ctrl-D (UNIX)
              case 26:  // EOF - Ctrl-Z (Windows)
                        return 4;                      
              default:  // fill the buffer 
                        if ((c >= ' ' && c < 240) || c == 9) { // ignore control characters
                          *line += c;
                          if (echo) if (!tsp->connection->sendData (&c, 1)) return 0;
                        } else {
                          // the only reply we are interested in so far is IAC (255) SB (250) NAWS (31) col1 col2 row1 row1 IAC (255) SE (240)

                          // There is a difference between telnet clients. Windows telent.exe for example will only report client window size as a result of
                          // IAC DO NAWS command from telnet server. Putty on the other hand will report client window size as a reply and will
                          // continue sending its window size each time client windows is resized. But won't respond to IAC DO NAWS if client
                          // window size remains the same.
                          
                          if (c == 255) { // IAC
                            if (!tsp->connection->recvData (&c, 1)) return 0;
                            switch (c) {
                              case 250: // SB
                                        if (!tsp->connection->recvData (&c, 1)) return 0;
                                        if (c == 31) { // NAWS
                                          if (!tsp->connection->recvData ((char *) &tsp->clientWindowCol1, 1)) return 0;
                                          if (!tsp->connection->recvData ((char *) &tsp->clientWindowCol2, 1)) return 0;
                                          if (!tsp->connection->recvData ((char *) &tsp->clientWindowRow1, 1)) return 0;
                                          if (!tsp->connection->recvData ((char *) &tsp->clientWindowRow2, 1)) return 0;
                                          // debug: Serial.printf ("[%10lu] [telnet] client reported its window size %i %i   %i %i\n", millis (), tsp->clientWindowCol1, tsp->clientWindowCol2, tsp->clientWindowRow1, tsp->clientWindowRow2);
                                        }
                                        break;
                              // in the following cases the 3rd character is following, ignore this one too
                              case 251:
                              case 252:
                              case 253:
                              case 254:                        
                                        if (!tsp->connection->recvData (&c, 1)) return 0;
                              default: // ignore
                                        break;
                            }
                          }

                          // debug telnet IAC protocol
                          /*
                          switch (c) {
                            case 255: Serial.printf ("IAC "); break;
                            case 254: Serial.printf ("DONT "); break;
                            case 253: Serial.printf ("DO "); break;
                            case 252: Serial.printf ("WONT "); break;
                            case 251: Serial.printf ("WILL "); break;
                            case 250: Serial.printf ("SB "); break;
                            case 240: Serial.printf ("SE "); break;
                            case  31: Serial.printf ("NAWS "); break;
                            case   1: Serial.printf ("ECHO "); break;
                            default:  Serial.printf ("%i ", c); break;
                          }
                          */
                        }
                        break;              
          } // switch
        } // while
        return 0;
      }

      // ----- built-in commands -----

      String __internalTelnetCommandHandler__ (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) {

        if (argv [0] == "help") { //------------------------------------------- HELP
          
          if (argc == 1) return __help__ (tsp);
                         return "Wrong syntax. Use help";

        } else if (argv [0] == "quit") { //------------------------------------ QUIT
          
          if (argc == 1) return __quit__ (tsp);
                         return "Wrong syntax. Use quit";

        } else if (argv [0] == "clear" || argv [0] == "cls") { //-------------- CLEAR, CLS
          
          if (argc == 1) return __clear__ (tsp);
                         return "Wrong syntax. Use clear or cls";
                         
        } else if (argv [0] == "uname") { //----------------------------------- UNAME
    
          if (argc == 1 || (argc == 2 && argv [1] == "-a")) return __uname__ ();
                                                            return "Wrong syntax. Use uname [-a]";

        } else if (argv [0] == "uptime") { //---------------------------------- UPTIME
    
          if (argc == 1) return __uptime__ ();
                         return "Wrong syntax. Use uptime";

        } else if (argv [0] == "reboot") { //---------------------------------- REBOOT
    
          if (argc == 1) return __reboot__ (tsp);
                         return "Wrong syntax. Use reboot";
          
        } else if (argv [0] == "reset") { //----------------------------------- RESET
    
          if (argc == 1) return __reset__ (tsp);
                         return "Wrong syntax. Use reset";
    
        } else if (argc >= 1 && argv [0] == "date") { //----------------------- DATE
          
          if (argc == 1)                     return __getDateTime__ ();
          if (argc == 4 && argv [1] == "-s") return __setDateTime__ (argv [2], argv [3]);
                                             return "Wrong syntax. Use date [-s YYYY/MM/DD hh:mm:ss] (use hh in 24 hours time format)";

        } else if (argc >= 1 && argv [0] == "ntpdate") { //-------------------- NTPDATE
          
          if (argc == 1 || ((argc == 2 || argc == 3) && argv [1] == "-u")) return __ntpdate__ (argv [2]);
                                                                           return "Wrong syntax. Use ntpdate [-u [ntpServer]]";

        } else if (argc >= 1 && argv [0] == "crontab") { //-------------------- CRONTAB
          
          if (argc == 1 || (argc == 2 && argv [1] == "-l")) return cronTab ();
                                                            return "Wrong syntax. Use crontab [-l]";

        } else if (argv [0] == "free") { //------------------------------------ FREE
    
          long n;
          if (argc == 1)                                                               return __free__ (0, tsp);
          if (argc == 3 && argv [1] == "-s" && (n = argv [2].toInt ()) > 0 && n < 300) return __free__ (n, tsp);
                                                                                       return "Wrong syntax. Use free [-s n] (where 0 < n < 300)";
          
        } else if (argv [0] == "dmesg") { //----------------------------------- DMESG

          bool f = false;
          bool t = false;
          for (int i = 1; i < argc; i++) {
            if (argv [i] == "--follow") f = true;
            else if (argv [i] == "-T") t = true;

        } else if (argv [0] == "mkfs.fat") { //-------------------------------- MKFS.FAT
          
          if (argc == 1) {
            if (tsp->userName == "root") return __mkfs__ (tsp);
            else                         return "Only root user may format disk.";
          }
                                         return "Wrong syntax. Use mkfs.fat";
          
        } else if (argv [0] == "fs_info") { // -------------------------------- FS_INFO
          
          if (argc <= 2) return __fs_info__ ();
                         return "Wrong syntax. Use fs_info";          

        } else if (argv [0] == "ls" || argv [0] == "dir") { //----------------- LS, DIR

          if (argc == 1) return __ls__ (tsp->workingDir, tsp);
          if (argc == 2) return __ls__ (argv [1], tsp);
                         return "Wrong syntax. Use ls [directoryName]";

        } else if (argv [0] == "tree") { //------------------------------------ TREE

          if (argc == 1) return __tree__ (tsp->workingDir, tsp);
          if (argc == 2) return __tree__ (argv [1], tsp);
                         return "Wrong syntax. Use tree [directoryName]";
        
        } else if (argv [0] == "cat" || argv [0] == "type") { //--------------- CAT, TYPE
          
          if (argc == 2) return __catFileToClient__ (argv [1], tsp);
          if (argc == 3 && argv [1] == ">") return __catClientToFile__ (argv [2], tsp);
                         return "Wrong syntax. Use cat fileName or cat > fileName";

        } else if (argv [0] == "rm" || argv [0] == "del") { //----------------- RM
          
          if (argc == 2) return __rm__ (argv [1], tsp);
                         return "Wrong syntax. Use rm fileName";

        } else if (argv [0] == "mkdir") { //----------------------------------- MKDIR
          
          if (argc == 2) return __mkdir__ (argv [1], tsp);
                         return "Wrong syntax. Use mkdir directoryName";

        } else if (argv [0] == "rmdir") { //----------------------------------- RMDIR
          
          if (argc == 2) return __rmdir__ (argv [1], tsp);
                         return "Wrong syntax. Use rmdir directoryName";

        } else if (argv [0] == "cd") { //-------------------------------------- CD
          
          if (argc == 2) return __cd__ (argv [1], tsp);
                         return "Wrong syntax. Use cd directoryName";

        } else if (argv [0] == "pwd") { //------------------------------------- PWD
          
          if (argc == 1) return __pwd__ (tsp);
                         return "Wrong syntax. Use pwd";

        } else if (argv [0] == "mv" || argv [0] == "ren") { //----------------- MV
          
          if (argc == 3) return __mv__ (argv [1], argv [2], tsp);
                         return "Wrong syntax. Use mv srcFileName dstFileName or mv srcDirectoryName dstDirectoryName";

        } else if (argv [0] == "cp" || argv [0] == "copy") { //---------------- CP
          
          if (argc == 3) return __cp__ (argv [1], argv [2], tsp);
                         return "Wrong syntax. Use cp srcFileName dstFileName";

        } else if (argv [0] == "vi") { //-------------------------------------- VI

          if (argc == 2)  {
                            TIME_OUT_TYPE timeOutMillis = tsp->connection->getTimeOut (); tsp->connection->setTimeOut (INFINITE); // we don't want TcpConnection to break during editing
                            String s = __vi__ (argv [1], tsp);
                            tsp->connection->setTimeOut (timeOutMillis); // restore original time-out
                            return s;
                          }
                          return "Wrong syntax. Use vi fileName";                         

        #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT

          } else if (argv [0] == "passwd") { //-------------------------------- PASSWD
            
            if (argc == 1)                                              return __passwd__ (tsp->userName, tsp);
            if (argc == 2) {
              if (tsp->userName == "root" || argv [1] == tsp->userName) return __passwd__ (argv [1], tsp); 
              else                                                      return "You may not change password for " + argv [1] + ".";
            }

          } else if (argv [0] == "useradd") { //------------------------------- USERADD

            if (tsp->userName != "root")                           return "Only root may add users.";
            if (argc == 6 && argv [1] == "-u" && argv [3] == "-d") return __userAdd__ (argv [5], argv [2], argv [4]);
                                                                   return "Wrong syntax. Use useradd -u userId -d userHomeDirectory userName (where userId > 1000)";

          } else if (argv [0] == "userdel") { //------------------------------- USERDEL

            if (tsp->userName != "root") return "Only root may delete users.";
            if (argc != 2)               return "Wrong syntax. Use userdel userName";
            if (argv [1] == "root")      return "You don't really want to to this.";
                                         return __userDel__ (argv [1]);

        #endif

        } else if (argv [0] == "ifconfig" || argv [0] == "ipconfig") { //------ IFCONFIG, IPCONFIG
          
          if (argc == 1) return ifconfig ();
                         return "Unknown option.";

        } else if (argv [0] == "iw") { //-------------------------------------- IW
    
          if (argc == 1) return iw (tsp->connection);
                         return "Wrong syntax. Use ifconfig";

        } else if (argv [0] == "arp") { //------------------------------------- ARP
    
          if (argc == 1)                       return arp (); // from network.h
          if ((argc == 2 && argv [1] == "-a")) return arp (); // from network.h
                                               return "Wrong syntax. Use arp [-a]";
    
        } else if (argv [0] == "ping") { //------------------------------------ PING

          if (argc == 2) { ping (argv [1], PING_DEFAULT_COUNT, PING_DEFAULT_INTERVAL, PING_DEFAULT_SIZE, PING_DEFAULT_TIMEOUT, tsp->connection); return ""; }
                         return "Wrong syntax. Use ping ipAddres";
   
        } else if (argv [0] == "telnet") { //---------------------------------- TELENT

          long port;
          if (argc == 2)                                                                 return __telnet__ (argv [1], 23, tsp);
          if (argc == 3 && (port = argv [2].toInt ()) && port > 0 && (int) port == port) return __telnet__ (argv [1], (int) port, tsp);
                                                                                         return "Wrong syntax. Use telnet ipAddress or telnet ipAddress portNumber";

        } else if (argv [0] == "sendmail") { //-------------------------------- SENDMAIL

          String smtpServer = "";
          String smtpPort = "";
          String userName = "";
          String password = "";
          String from = "";
          String to = ""; 
          String subject = ""; 
          String message = "";
          String nonsense = "";
          String *sp = &nonsense;
          const String syntaxError = "Use sendmail [-S smtpServer] [-P smtpPort] [-u userName] [-p password] [-f from address] [t to address list] [-s subject] [-m messsage]"; 

          for (int i = 1; i < argc; i++) {
                 if (argv [i] == "-S") sp = &smtpServer;
            else if (argv [i] == "-P") sp = &smtpPort;
            else if (argv [i] == "-u") sp = &userName;
            else if (argv [i] == "-p") sp = &password;
            else if (argv [i] == "-f") sp = &from;
            else if (argv [i] == "-t") sp = &to;
            else if (argv [i] == "-s") sp = &subject;
            else if (argv [i] == "-m") sp = &message;
            else { if (*sp == "") { *sp = argv [i]; sp = &nonsense; } else return syntaxError; }
          }

          if (nonsense > "") return syntaxError;
          
          return __sendmail__ (message, subject, to, from, password, userName, smtpPort.toInt (), smtpServer);
                
        } else if (argv [0] == "curl") { //------------------------------------ CURL
          
          if (argc == 2) return __curl__ ("GET", argv [1]);
          if (argc == 3) return __curl__ (argv [1], argv [2]);
                         return "Wrong syntax. Use curl http://url or curl method http://url (where method is GET, PUT, ...):";

        }
        
        // -------------------------------------------------------------------- INVALID COMMAND
                      
        return "Invalid command, use help to display available commands.";
      }

      inline String __help__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        String e = __catFileToClient__ (getUserHomeDirectory ("telnetserver") + "help.txt", tsp);
        return e == "" ? "" : e + " Please use FTP, loggin as root and upload help.txt file found in Esp32_web_ftp_telnet_server_template package into " + getUserHomeDirectory ("telnetserver") + " directory.";  
      }

      inline String __quit__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        tsp->connection->closeConnection ();
        return "";
      }       

      inline String __clear__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        return "\x1b[2J"; // ESC[2J
      }       

      inline String __uname__ () __attribute__((always_inline)) {
        return String (UNAME);
      } 

      inline String __uptime__ () __attribute__((always_inline)) {
        String s;
        char cstr [15];
        time_t t = getGmt ();
        time_t uptime;
        if (t) { // if time is set
          struct tm strTime = timeToStructTime (timeToLocalTime (t));
          sprintf (cstr, "%02i:%02i:%02i", strTime.tm_hour, strTime.tm_min, strTime.tm_sec);
          s = String (cstr) + " up ";     
        } else { // time is not set (yet), just read how far clock has come untill now
          s = "Up ";
        }
        uptime = getUptime ();
        int seconds = uptime % 60; uptime /= 60;
        int minutes = uptime % 60; uptime /= 60;
        int hours = uptime % 24;   uptime /= 24; // uptime now holds days
        if (uptime) s += String ((unsigned long) uptime) + " days, ";
        sprintf (cstr, "%02i:%02i:%02i", hours, minutes, seconds);
        s += String (cstr);
        return s;
      }

      inline String __reboot__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        tsp->connection->sendData ((char *) "rebooting ...");
        delay (100);
        tsp->connection->closeConnection ();
        Serial.printf ("\r\n\nreboot requested ...\r\n");
        delay (100);
        ESP.restart ();
        return "";
      }

      inline String __reset__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        tsp->connection->sendData ((char *) "reseting ...");
        delay (100);
        tsp->connection->closeConnection ();
        Serial.printf ("\r\n\nreset requested ...\r\n");
        delay (100);
        // ESP.reset (); // is not supported on ESP32, let's trigger watchdog insttead: https://github.com/espressif/arduino-esp32/issues/1270
        // cause WDT reset
        esp_task_wdt_init (1, true);
        esp_task_wdt_add (NULL);
        while (true);
        return "";
      }
    
      String __getDateTime__ () {
        time_t t = getGmt ();
        if (t > 1604962085) return timeToString (timeToLocalTime (t));  // if the time is > then the time I'm writting this code then it must have been set
        else                return timeToString (0);                    // the time has not been set yet
      }
  
      inline String __setDateTime__ (String& dt, String& tm) __attribute__((always_inline)) {
        int Y, M, D, h, m, s;
        if (sscanf (dt.c_str (), "%i/%i/%i", &Y, &M, &D) == 3 && Y >= 1900 && M >= 1 && M <= 12 && D >= 1 && D <= 31 && sscanf (tm.c_str (), "%i:%i:%i", &h, &m, &s) == 3 && h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59) { // TO DO: we still do not catch all possible errors, for exmple 30.2.1966
          struct tm tm;
          tm.tm_year = Y - 1900; tm.tm_mon = M - 1; tm.tm_mday = D;
          tm.tm_hour = h; tm.tm_min = m; tm.tm_sec = s;
          time_t t = mktime (&tm); // time in local time
          if (t != -1) {
            setLocalTime (t);
            return __getDateTime__ ();          
          }
        }
        return "Wrong format of date/time specified.";
      }         

      inline String __ntpdate__ (String ntpServer) __attribute__((always_inline)) {
        String s = ntpServer == "" ? ntpDate () : ntpDate (ntpServer);
        if (s != "") return s;
        return "Time synchronized, currrent time is " + timeToString (getLocalTime ()) + ".";
      }

      inline String __free__ (int delaySeconds, telnetSessionParameters *tsp) __attribute__((always_inline)) {
        char *nl = (char *) "";
        do {
          char output [50];
          sprintf (output, "%sFree memory:      %10lu K bytes", nl, (unsigned long) ESP.getFreeHeap () / 1024);
          if (!tsp->connection->sendData (output, strlen (output))) return "";
          nl = (char *) "\r\n";
          // delay with Ctrl C checking
          for (int i = 0; i < 880; i++) { // 880 instead of 1000 - a correction for more precise timing
            while (tsp->connection->available () == TcpConnection::AVAILABLE) {
              char c;
              if (!tsp->connection->recvData (&c, sizeof (c))) return "";
              if (c == 3 || c >= ' ') return ""; // return if user pressed Ctrl-C or any key
            }
            delay (delaySeconds); // / 1000
          }
        } while (delaySeconds);
        return "";
      }

      inline String __dmesg__ (bool follow, bool trueTime, telnetSessionParameters *tsp) __attribute__((always_inline)) { // displays dmesg circular queue over telnet connection
        // make a copy of all messages in circular queue in critical section
        xSemaphoreTake (__dmesgSemaphore__, portMAX_DELAY);
        byte i = __dmesgBeginning__;
        String s = "";
        do {
          if (s != "") s+= "\r\n";
          char c [25];
          if (trueTime && __dmesgCircularQueue__ [i].time) {
            struct tm st = timeToStructTime (timeToLocalTime (__dmesgCircularQueue__ [i].time));
            strftime (c, sizeof (c), "[%y/%m/%d %H:%M:%S] ", &st);
          } else {
            sprintf (c, "[%10lu] ", __dmesgCircularQueue__ [i].milliseconds);
          }
          s += String (c) + __dmesgCircularQueue__ [i].message;
        } while ((i = (i + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) != __dmesgEnd__);
        xSemaphoreGive (__dmesgSemaphore__);
        // send everything to the client
        if (!tsp->connection->sendData (s)) return "";
    
        // --follow?
        while (follow) {
          while (i == __dmesgEnd__) {
            while (tsp->connection->available () == TcpConnection::AVAILABLE) {
              char c;
              if (!tsp->connection->recvData (&c, sizeof (c))) return "";
              if (c == 3 || c >= ' ') return ""; // return if user pressed Ctrl C or any key
            }
            delay (10); // wait a while and check again
          }
          // __dmesgEnd__ has changed which means that at least one new message has been inserted into dmesg circular queue menawhile
          xSemaphoreTake (__dmesgSemaphore__, portMAX_DELAY);
          s = "";
          do {
            s += "\r\n";
            char c [25];
            if (trueTime && __dmesgCircularQueue__ [i].time) {
              struct tm st = timeToStructTime (timeToLocalTime (__dmesgCircularQueue__ [i].time));
              strftime (c, sizeof (c), "[%y/%m/%d %H:%M:%S] ", &st);
            } else {
              sprintf (c, "[%10lu] ", __dmesgCircularQueue__ [i].milliseconds);
            }
            s += String (c) + __dmesgCircularQueue__ [i].message;
          } while ((i = (i + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) != __dmesgEnd__);
          xSemaphoreGive (__dmesgSemaphore__);
          // send everything to the client
          if (!tsp->connection->sendData (s)) return "";
        }
        return "";
      }      

      inline String __mkfs__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        tsp->connection->sendData ((char *) "formatting file system with FAT, please wait ... "); 
        FFat.end ();
        if (FFat.format ()) {
                                  tsp->connection->sendData ((char *) "formatted.");
          if (FFat.begin (false)) tsp->connection->sendData ((char *) "\r\nFile system mounted,\r\nreboot now to create default configuration files\r\nor create then yorself before rebooting.");
          else                    tsp->connection->sendData ((char *) "\r\nFile system mounting has failed.");
        } else                    tsp->connection->sendData ((char *) "failed.");
        return "";
      }

      inline String __fs_info__ () __attribute__((always_inline)) {
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";

        char output [500];
        sprintf (output, "FAT file system info.\r\n"
                         "Total space:      %10i K bytes\r\n"
                         "Total space used: %10i K bytes\r\n"
                         "Free space:       %10i K bytes\r\n"
                         "Max path length:  %10i",
                         FFat.totalBytes () / 1024, 
                         (FFat.totalBytes () - FFat.freeBytes ()) / 1024, 
                         FFat.freeBytes () / 1024,
                         FILE_PATH_MAX_LENGTH
              );
        return String (output);
      } 

      inline String __ls__ (String& directory, telnetSessionParameters *tsp) __attribute__((always_inline)) {
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "" || !isDirectory (fp))            return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))        return "Access to " + fp + " denyed.";

        return listDirectory (fp);
      }

      inline String __tree__ (String& directory, telnetSessionParameters *tsp) __attribute__((always_inline)) {
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "" || !isDirectory (fp))            return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))        return "Access to " + fp + " denyed.";

        return recursiveListDirectory (fp);
      }

      inline String __catFileToClient__ (String& fileName, telnetSessionParameters *tsp) __attribute__((always_inline)) {
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullFilePath (fileName, tsp->workingDir);
        if (fp == "" || !isFile (fp))                 return "Invalid file name " + fileName;
        if (!userMayAccess (fp, tsp->homeDir))        return "Access to " + fp + " denyed.";

        File f;
        if ((bool) (f = FFat.open (fp, FILE_READ))) {
          if (!f.isDirectory ()) {
            #define BUFF_SIZE 2048
            char *buff = (char *) malloc (BUFF_SIZE); // get 2 KB of memory from heap (not from the stack)
            if (buff) {
              *buff = 0;
              int i = strlen (buff);
              while (f.available ()) {
                switch (*(buff + i) = f.read ()) {
                  case '\r':  // ignore
                              break;
                  case '\n':  // crlf conversion
                              *(buff + i ++) = '\r'; 
                              *(buff + i ++) = '\n';
                              break;
                  default:
                              i ++;                  
                }
                if (i >= BUFF_SIZE - 2) { tsp->connection->sendData ((char *) buff, i); i = 0; }
              }
              if (i) { tsp->connection->sendData ((char *) buff, i); }
              free (buff);
            } 
          } else {
            f.close ();
            return "Can't read " + fp;
          }
          f.close ();
        } else {
          return "Can't read " + fp;
        }
        return "";
      }

      inline String __catClientToFile__ (String& fileName, telnetSessionParameters *tsp) __attribute__((always_inline)) {
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullFilePath (fileName, tsp->workingDir);
        if (fp == "" || isDirectory (fp))             return "Invalid file name " + fileName;
        if (!userMayAccess (fp, tsp->homeDir))        return "Access to " + fp + " denyed.";

        File f;
        char *s;
        int l;

        if ((bool) (f = FFat.open (fp, FILE_WRITE))) {
          String line;
          while (char c = __readLineFromClient__ (&line, true, tsp)) {
            switch (c) {
              case 0:   
                      f.close ();
                      return fp + " not fully written.";
              case 4:
                      tsp->connection->sendData ((char *) "\r\n", 2);
                      s = (char *) line.c_str (); l = strlen (s);
                      if (l > 0) if (f.write ((uint8_t *) s, l) != l) {
                        f.close ();
                        return "Can't write " + fp;
                      }
                      f.close ();
                      return fp + " written.";
              case 13:
                      tsp->connection->sendData ((char *) "\r\n", 2);
                      line += "\r\n";
                      s = (char *) line.c_str (); l = strlen (s);
                      if (f.write ((uint8_t *) s, l) != l) { 
                        f.close ();
                        return "Can't write " + fp;
                      } 
                      line = "";
                      break;
              default:  
                    break;
            }
          }
        } 
        return "";
      }
      
      inline String __rm__ (String& fileName, telnetSessionParameters *tsp) __attribute__((always_inline)) {
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullFilePath (fileName, tsp->workingDir);
        if (fp == "" || !isFile (fp))                   return "Invalid file name " + fileName;
        if (!userMayAccess (fp, tsp->homeDir))          return "Access to " + fp + " denyed.";

        if (deleteFile (fp))                            return fp + " deleted.";
                                                        return "Can't delete " + fp;
      }

      inline String __mkdir__ (String& directory, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "")                                   return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))          return "Access tp " + removeExtraSlash (fp) + " denyed.";

        if (makeDir (fp))                               return removeExtraSlash (fp) + " made.";
                                                        return "Can't make " + removeExtraSlash (fp);

      }

      inline String __rmdir__ (String& directory, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "" || !isDirectory (fp))              return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))          return "Access tp " + removeExtraSlash (fp) + " denyed.";
        if (fp == tsp->homeDir)                         return "You may not remove your home directory.";
        if (fp == tsp->workingDir)                      return "You can't remove your working directory.";

        if (removeDir (fp))                             return removeExtraSlash (fp) + " removed.";
                                                        return "Can't remove " + removeExtraSlash (fp);

      }      

      inline String __cd__ (String& directory, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";

        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "" || !isDirectory (fp))              return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))          return "Access to " + removeExtraSlash (fp) + " denyed.";

        tsp->workingDir = fp;                           
                                                        return "Your working directory is " + removeExtraSlash (tsp->workingDir);
      }

      inline String __pwd__ (telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        
        return "Your working directory is " + removeExtraSlash (tsp->workingDir);
      }

      inline String __mv__ (String& srcFileOrDirectory, String& dstFileOrDirectory, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp1 = fullFilePath (srcFileOrDirectory, tsp->workingDir);
        if (fp1 == "")                                  return "Invalid file or directory name " + srcFileOrDirectory;
        if (!userMayAccess (fp1, tsp->homeDir))         return "Access to " + fp1 + " denyed.";

        String fp2 = fullFilePath (dstFileOrDirectory, tsp->workingDir);
        if (fp2 == "")                                  return "Invalid file or directory name " + dstFileOrDirectory;
        if (!userMayAccess (fp2, tsp->homeDir))         return "Access to " + fp2 + " denyed.";

        if (FFat.rename (fp1, fp2))                     return "Renamed to " + fp2;
                                                        return "Can't rename " + fp1;
      }

      inline String __cp__ (String& srcFileName, String& dstFileName, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";

        String fp1 = fullFilePath (srcFileName, tsp->workingDir);
        if (fp1 == "" || !isFile (fp1))                 return "Invalid file name " + srcFileName;
        if (!userMayAccess (fp1, tsp->homeDir))         return "Access to " + fp1 + " denyed.";

        String fp2 = fullFilePath (dstFileName, tsp->workingDir);
        if (fp2 == "" || isDirectory (fp2))             return "Invalid file name " + dstFileName;
        if (isFile (fp2))                               return fp2 + " already exists, delete it first.";
        if (!userMayAccess (fp2, tsp->homeDir))         return "Access to " + fp2 + " denyed.";

        File f1, f2;
        String retVal = "File copied.";
        
        if (!(bool) (f1 = FFat.open (fp1, FILE_READ))) { retVal = "Can't read " + fp1; goto out1; }
        if (f1.isDirectory ()) { retVal = "Can't read " + fp1; goto out2; }
        if (!(bool) (f2 = FFat.open (fp2, FILE_WRITE))) { retVal = "Can't write " + fp2; goto out2; }
                  
        while (f1.available ()) {
          char c = f1.read ();
          if (f2.write ((uint8_t *) &c, 1) != 1) { retVal = "Can't write " + fp2; goto out3; }
        }
    out3:
        f2.close ();
    out2:
        f1.close ();
    out1:          
        return retVal;
      }

      #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT

        inline String __passwd__ (String& userName, telnetSessionParameters *tsp) __attribute__((always_inline)) {
          String password1;
          String password2;
                    
          if (tsp->userName == userName) { // user changing password for himself
            // read current password
            tsp->connection->sendData ((char *) "Enter current password: ");
            if (13 != __readLineFromClient__ (&password1, false, tsp))                                    return "Password not changed.";
            // check if password is valid for user
            if (!checkUserNameAndPassword (userName, password1))                                          return "Wrong password.";
          } else {                         // root is changing password for another user
            // check if user exists with getUserHomeDirectory
            if (getUserHomeDirectory (userName) == "")                                                    return "User " +  userName + " does not exist."; 
          }
          // read new password twice
          tsp->connection->sendData ("\r\nEnter new password for " + userName + ": ");
          if (13 != __readLineFromClient__ (&password1, false, tsp) || !password1.length ())              return "\r\nPassword not changed.";
          if (password1.length () > USER_PASSWORD_MAX_LENGTH)                                             return "\r\nNew password too long.";
          tsp->connection->sendData ("\r\nRe-enter new password for " + userName + ": ");
          if (13 != __readLineFromClient__ (&password2, false, tsp))                                      return "\r\nPasswords do not match.";
          // check passwords
          if (password1 != password2)                                                                     return "\r\nPasswords do not match.";
          // change password
          if (passwd (userName, password1))                                                               return "\r\nPassword changed for " + userName + ".";
          else                                                                                            return "\r\nError changing password.";  
        }

        inline String __userAdd__ (String& userName, String& userId, String& userHomeDir) __attribute__((always_inline)) {
          if (userName.length () > USER_PASSWORD_MAX_LENGTH)  return "New user name too long.";  
          if (userHomeDir.length () > FILE_PATH_MAX_LENGTH)   return "User home directory too long.";
          long uid = userId.toInt ();
          if (!uid || uid <= 100)                             return "User Id must be > 1000.";
                                                              return userAdd (userName, userId, userHomeDir);
        }

        inline String __userDel__ (String& userName) __attribute__((always_inline)) {
          return userDel (userName);
        }
        
      #endif

      // ----- TELNET -----
            
            struct __telnetStruct__ {
              TcpConnection *clientConnection;
              bool clientConnectionRunning;
              TcpConnection *otherServerConnection;
              bool otherServerConnectionRunning;
              bool receivedDataFromOtherServer;
            };

      String __telnet__ (String otherServerName, int otherServerPort, telnetSessionParameters *tsp) {
        // open TCP connection to the other server
        TcpClient *otherServer = new TcpClient (otherServerName, otherServerPort, (TIME_OUT_TYPE) 300000); // close also this connection if inactive for more than 5 minutes
        if (!otherServer || !otherServer->connection () || !otherServer->connection ()->started ()) return "Could not connect to " + otherServerName + " on port " + String (otherServerPort) + ".";
    
        struct __telnetStruct__ telnetSessionSharedMemory = {tsp->connection, true, otherServer->connection (), true, false};
        #define tskNORMAL_PRIORITY 1
        if (pdPASS != xTaskCreate ( [] (void *param)  { // other server -> client data transfer  
                                                        struct __telnetStruct__ *telnetSessionSharedMemory = (struct __telnetStruct__ *) param;
                                                        while (telnetSessionSharedMemory->clientConnectionRunning) { // while the other thread is running
                                                            char buff [512];
                                                            if (telnetSessionSharedMemory->otherServerConnection->available () == TcpConnection::AVAILABLE) {
                                                              int received = telnetSessionSharedMemory->otherServerConnection->recvData (buff, sizeof (buff));
                                                              if (!received) break;
                                                              telnetSessionSharedMemory->receivedDataFromOtherServer = true;
                                                              int sent = telnetSessionSharedMemory->clientConnection->sendData (buff, received);
                                                              if (!sent) break;
                                                            } else {
                                                              delay (1);
                                                            }
                                                        }
                                                        telnetSessionSharedMemory->otherServerConnectionRunning = false; // signal that this thread has stopped
                                                        vTaskDelete (NULL);
                                                      }, 
                                    __func__, 
                                    4068, 
                                    &telnetSessionSharedMemory,
                                    tskNORMAL_PRIORITY,
                                    NULL)) {
          delete (otherServer);                               
          return "Could not start telnet session with " + otherServerName + ".";
        } 
        if (pdPASS != xTaskCreate ( [] (void *param)  { // client -> other server data transfer
                                                        struct __telnetStruct__ *telnetSessionSharedMemory = (struct __telnetStruct__ *) param;
                                                        while (telnetSessionSharedMemory->otherServerConnectionRunning) { // while the other thread is running
                                                            char buff [512];
                                                            if (telnetSessionSharedMemory->clientConnection->available () == TcpConnection::AVAILABLE) {
                                                              int received = telnetSessionSharedMemory->clientConnection->recvData (buff, sizeof (buff));
                                                              if (!received) break;
                                                              int sent = telnetSessionSharedMemory->otherServerConnection->sendData (buff, received);
                                                              if (!sent) break;
                                                            } else {
                                                              delay (1);
                                                            }
                                                        }
                                                        telnetSessionSharedMemory->clientConnectionRunning = false; // signal that this thread has stopped
                                                        vTaskDelete (NULL);
                                                      }, 
                                    __func__, 
                                    4068, 
                                    &telnetSessionSharedMemory,
                                    tskNORMAL_PRIORITY,
                                    NULL)) {
          telnetSessionSharedMemory.clientConnectionRunning = false;                            // signal other server -> client thread to stop
          while (telnetSessionSharedMemory.otherServerConnectionRunning) delay (1); // wait untill it stops
          delete (otherServer);                               
          return "Could not start telnet session with " + otherServerName + ".";
        } 
        while (telnetSessionSharedMemory.otherServerConnectionRunning || telnetSessionSharedMemory.clientConnectionRunning) delay (10); // wait untill both threads stop
    
        if (telnetSessionSharedMemory.receivedDataFromOtherServer) {
          // send to the client IAC DONT ECHO again just in case ther server has changed this setting
          tsp->connection->sendData (IAC DONT ECHO "\r\nConnection to " + otherServerName + " lost.");
        } else {
          return "Could not connect to " + otherServerName + ".";
        }
        return "";
      }

      String __sendmail__ (String message, String subject, String to, String from, String password, String userName, int smtpPort, String smtpServer) {
        return sendMail (message, subject, to, from, password, userName, smtpPort, smtpServer);
      }
   
      String __curl__ (String method, String url) {
        if (method == "GET" || method == "PUT" || method == "POST" || method == "DEELTE") {
          if (url.substring (0, 7) == "http://") {
            url = url.substring (7);
            String server = "";
            int port = 80;
            int i = url.indexOf ('/');
            if (i < 0) {
              server = url;
              url = "/";
            } else {
              server = url.substring (0, i);
              url = url.substring (i);
            }
            i = server.indexOf (":");
            if (i >= 0) {
              port = server.substring (i + 1).toInt ();
              if (port <= 0) return "Invalid port number.";
              server = server.substring (0, i);
            } 
            // call webClient
            Serial.printf ("[%10lu] [CURL] %s:%i %s %s.\n", millis (), server.c_str (), port, method.c_str (), url.c_str ());
            String r = webClient ((char *) server.c_str (), port, (TIME_OUT_TYPE) 15000, method + " " + url);
            return r != "" ? r : "Error, check dmesg to get more information.";
          } else {
            return "URL must begin with http://";
          }
        } else {
          return "Use GET, PUT, POST or DELETE methods.";
        }
      }

      // not really a vi but small and simple text editor

      String __vi__ (String& fileName, telnetSessionParameters *tsp) {
        // a good reference for telnet ESC codes: https://en.wikipedia.org/wiki/ANSI_escape_code
        // and: https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullFilePath (fileName, tsp->workingDir);
        if (fp == "" || isDirectory (fp))             return "Invalid file name " + fileName;
        if (!userMayAccess (fp, tsp->homeDir))        return "Access to " + fp + " denyed.";

        // 1. create a new file one if it doesn't exist (yet)
        String emptyContent = "";
        if (!isFile (fp)) { if (!writeFile (emptyContent, fp)) return "Can't create " + fp; else tsp->connection->sendData (fp + " created.\r\n"); }

        // 2. read the file content into internal vi data structure (lines of Strings)
        #define MAX_LINES 600 // may increase up to < 998 but this would require even more stack for telnet server and works slow
        String line [MAX_LINES]; for (int i = 0; i < MAX_LINES; i++) line [i] = "";
        int fileLines = 0;
        bool dirty = false;
        {
          bool e = true;
          File f = FFat.open (fp, FILE_READ);
          if (f) {
            if (!f.isDirectory ()) {
              while (f.available ()) { 
                if (ESP.getFreeHeap () < 32768) { // always leave at least 32 KB free for other things that may be running on ESP32
                  f.close (); 
                  return "Out of memory."; 
                }
                char c = (char) f.read (); 
                switch (c) {
                  case '\r':  break; // ignore
                  case '\n':  if (++fileLines >= MAX_LINES) { 
                                f.close (); 
                                return fp + " has too many lines for this simple version of vi."; 
                              }
                              // tsp->connection->sendData ("\rreading line " + String (fileLines + 1) + " ");
                              break; 
                  case '\t':  line [fileLines] += "  "; break; // treat tab as 2 spaces
                  default:    line [fileLines] += c; break;
                }
              }
              f.close ();
              e = false;              
            } else tsp->connection->sendData ((char *) "Can't edit a directory.");
            f.close ();
          } else tsp->connection->sendData ("Can't read " + fp);
          if (e) return "";
          fileLines++;
        }    

        // 3. discard any already pending characters from client
        char c;
        while (tsp->connection->available () == TcpConnection::AVAILABLE) tsp->connection->recvData (&c, 1);

        // 4. get information about client window size
          // client reply that we are expecting from IAC DO NAWS will be in the form of: IAC (255) SB (250) NAWS (31) col1 col2 row1 row1 IAC (255) SE (240)

          // There is a difference between telnet clients. Windows telent.exe for example will only report client window size as a result of
          // IAC DO NAWS command from telnet server. Putty on the other hand will report client windows size as a reply and will
          // continue sending its window size each time client window is resized. But won't respond to IAC DO NAWS if client

        // 5. edit 
        int textCursorX = 0;  // vertical position of cursor in text
        int textCursorY = 0;  // horizontal position of cursor in text
        int textScrollX = 0;  // vertical text scroll offset
        int textScrollY = 0;  // horizontal text scroll offset

        bool redrawHeader = true;
        bool redrawAllLines = true;
        bool redrawLineAtCursor = false; 
        bool redrawFooter = true;
        String message = " " + String (fileLines) + " lines ";

                              // clear screen
                              if (!tsp->connection->sendData ((char *) "\x1b[2J")) return ""; // ESC[2J = clear screen

        while (true) {
          // a. redraw screen 
          String s = ""; 
       
          if (redrawHeader)   { 
                                s = "---+"; for (int i = 4; i < tsp->clientWindowCol2; i++) s += '-'; 
                                if (!tsp->connection->sendData ("\x1b[H" + s.substring (0, tsp->clientWindowCol2 - 28) + " Save: Ctrl-S, Exit: Ctrl-X")) return "";  // ESC[H = move cursor home
                                redrawHeader = false;
                              }
          if (redrawAllLines) {

                                // Redrawing algorithm: straight forward idea is to scan screen lines with text i ∈ [2 .. tsp->clientWindowRow2 - 1], calculate text line on
                                // this possition: i - 2 + textScrollY and draw visible part of it: line [...].substring (textScrollX, tsp->clientWindowCol2 - 4 + textScrollX), tsp->clientWindowCol2 - 4).
                                // When there are frequent redraws the user experience is not so good since we often do not have enough time to redraw the whole screen
                                // between two key strokes. Therefore we'll always redraw just the line at cursor position: textCursorY + 2 - textScrollY and then
                                // alternate around this line until we finish or there is another key waiting to be read - this would interrupt the algorithm and
                                // redrawing will repeat after ne next character is processed.
                                {
                                  int nextScreenLine = textCursorY + 2 - textScrollY;
                                  int nextTextLine = nextScreenLine - 2 + textScrollY;
                                  bool topReached = false;
                                  bool bottomReached = false;
                                  for (int i = 0; (!topReached || !bottomReached) && !(tsp->connection->available () == TcpConnection::AVAILABLE); i++) { 
                                    if (i % 2 == 0) { nextScreenLine -= i; nextTextLine -= i; } else { nextScreenLine += i; nextTextLine += i; }
                                    if (nextScreenLine == 2) topReached = true;
                                    if (nextScreenLine == tsp->clientWindowRow2 - 1) bottomReached = true;
                                    if (nextScreenLine > 1 && nextScreenLine < tsp->clientWindowRow2) {
                                      // draw nextTextLine at nextScreenLine 
                                      // debug: Serial.printf ("[telnet vi debug] display text line %i at screen position %i\n", nextTextLine + 1, nextScreenLine);
                                      char c [15];
                                      if (nextTextLine < fileLines) {
                                        sprintf (c, "\x1b[%i;0H%3i|", nextScreenLine, nextTextLine + 1);  // display line number - users would count lines from 1 on, whereas program counts them from 0 on
                                        s = String (c) + pad (line [nextTextLine].substring (textScrollX, tsp->clientWindowCol2 - 4 + textScrollX), tsp->clientWindowCol2 - 4); // ESC[line;columnH = move cursor to line;column, it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen)
                                      } else {
                                        sprintf (c, "\x1b[%i;0H   |", nextScreenLine);
                                        s = String (c)  + pad (" ", tsp->clientWindowCol2 - 4); // ESC[line;columnH = move cursor to line;column, it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen);
                                      }
                                      if (!tsp->connection->sendData (s)) return "";
                                    } 
                                  }
                                  if (topReached && bottomReached) redrawAllLines = false; // if we have drown all the lines we don't have to run this code again 
                                }
                                
          } else if (redrawLineAtCursor) {
                                // calculate screen line from text cursor position
                                s = "\x1b[" + String (textCursorY + 2 - textScrollY) + ";5H" + pad (line [textCursorY].substring (textScrollX, tsp->clientWindowCol2 - 4 + textScrollX), tsp->clientWindowCol2 - 4); // ESC[line;columnH = move cursor to line;column (columns go from 1 to clientWindowsColumns - inclusive), it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen)
                                if (!tsp->connection->sendData (s)) return ""; 
                                redrawLineAtCursor = false;
                              }
         
          if (redrawFooter)   {                                  
                                s = "\x1b[" + String (tsp->clientWindowRow2) + ";0H---+"; for (int i = 4; i < tsp->clientWindowCol2; i++) s += '-'; // ESC[line;columnH = move cursor to line;column
                                if (!tsp->connection->sendData (s)) return ""; 
                                redrawFooter = false;
                              }
          if (message != "")  {
                                tsp->connection->sendData ("\x1b[" + String (tsp->clientWindowRow2) + ";2H" + message);         
                                message = ""; redrawFooter = true; // we'll clear the message the next time screen redraws
                              }
          
          // b. restore cursor position - calculate screen coordinates from text coordinates
          if (!tsp->connection->sendData ("\x1b[" + String (textCursorY - textScrollY + 2) + ";" + String (textCursorX - textScrollX + 5) + "H")) return ""; // ESC[line;columnH = move cursor to line;column

          // c. read and process incoming stream of characters
          char c = 0;
          delay (1);
          if (!tsp->connection->recvData (&c, 1)) return "";
          switch (c) {
            case 24:  // Ctrl-X
                      if (dirty) {
                        tsp->connection->sendData ("\x1b[" + String (tsp->clientWindowRow2) + ";2H Save changes (y/n)? ");
                        redrawFooter = true; // overwrite this question at next redraw
                        while (true) {                                                     
                          if (!tsp->connection->recvData (&c, 1)) return "";
                          if (c == 'y') goto saveChanges;
                          if (c == 'n') break;
                        }
                      } 
                      tsp->connection->sendData ("\x1b[" + String (tsp->clientWindowRow2) + ";2H Share and Enjoy ----\r\n");
                      return "";
            case 19:  // Ctrl-S
saveChanges:
                      // save changes into fp
                      {
                        bool e = false;
                        File f = FFat.open (fp, FILE_WRITE);
                        if (f) {
                          if (!f.isDirectory ()) {
                            for (int i = 0; i < fileLines; i++) {
                              int toWrite = strlen (line [i].c_str ()); if (i) toWrite += 2;
                              if (toWrite != f.write (i ? (uint8_t *) ("\r\n" + line [i]).c_str (): (uint8_t *) line [i].c_str (), toWrite)) { e = true; break; }
                            }
                          }
                          f.close ();
                        }
                        if (e) { message = " Could't save changes "; } else { message = " Changes saved "; dirty = 0; }
                      }
                      break;
            case 27:  // ESC [A = up arrow, ESC [B = down arrow, ESC[C = right arrow, ESC[D = left arrow, 
                      if (!tsp->connection->recvData (&c, 1)) return "";
                      switch (c) {
                        case '[': // ESC [
                                  if (!tsp->connection->recvData (&c, 1)) return "";
                                  // debug: Serial.printf ("[telnet vi debug] ESC [ %c (%i)\n", c, c);
                                  switch (c) {
                                    case 'A':  // ESC [ A = up arrow
                                              if (textCursorY > 0) textCursorY--; 
                                              if (textCursorX > line [textCursorY].length ()) textCursorX = line [textCursorY].length ();
                                              break;                          
                                    case 'B':  // ESC [ B = down arrow
                                              if (textCursorY < fileLines - 1) textCursorY++;
                                              if (textCursorX > line [textCursorY].length ()) textCursorX = line [textCursorY].length ();
                                              break;
                                    case 'C':  // ESC [ C = right arrow
                                              if (textCursorX < line [textCursorY].length ()) textCursorX++;
                                              else if (textCursorY < fileLines - 1) { textCursorY++; textCursorX = 0; }
                                              break;
                                    case 'D':  // ESC [ D = left arrow
                                              if (textCursorX > 0) textCursorX--;
                                              else if (textCursorY > 0) { textCursorY--; textCursorX = line [textCursorY].length (); }
                                              break;        
                                    case '1': // ESC [ 1 = home
                                              tsp->connection->recvData (&c, 1); // read final '~'
                                              textCursorX = 0;
                                              break;
                                    case '4': // ESC [ 4 = end
                                              tsp->connection->recvData (&c, 1); // read final '~'
                                              textCursorX = line [textCursorY].length ();
                                              break;
                                    case '5': // ESC [ 5 = pgup
                                              tsp->connection->recvData (&c, 1); // read final '~'
                                              textCursorY -= (tsp->clientWindowRow2 - 2); if (textCursorY < 0) textCursorY = 0;
                                              if (textCursorX > line [textCursorY].length ()) textCursorX = line [textCursorY].length ();
                                              break;
                                    case '6': // ESC [ 6 = pgdn
                                              tsp->connection->recvData (&c, 1); // read final '~'
                                              textCursorY += (tsp->clientWindowRow2 - 2); if (textCursorY >= fileLines) textCursorY = fileLines - 1;
                                              if (textCursorX > line [textCursorY].length ()) textCursorX = line [textCursorY].length ();
                                              break;  
                                    case '3': // ESC [ 3 
                                              if (!tsp->connection->recvData (&c, 1)) return "";
                                              // debug: Serial.printf ("[telnet vi debug] ESC [ 3 %c (%i)\n", c, c);
                                              switch (c) {
                                                case '~': // ESC [ 3 ~ (126) - putty reports del key as ESC [ 3 ~ (126), since it also report backspace key as del key let' treat del key as backspace                                                                 
                                                          goto backspace;
                                                default:  // ignore
                                                          // debug: Serial.printf ("ESC [ 3 %c (%i)\n", c, c);
                                                          break;
                                              }
                                              break;
                                    default:  // ignore
                                              // debug: Serial.printf ("ESC [ %c (%i)\n", c, c);
                                              break;
                                  }
                                  break;
                         default: // ignore
                                  // debug: Serial.printf ("ESC %c (%i)\n", c, c);
                                  break;
                      }
                      break;
            case 8:   // Windows telnet.exe: back-space, putty does not report this code
backspace:
                      if (textCursorX > 0) { // delete one character left of cursor position
                        line [textCursorY] = line [textCursorY].substring (0, textCursorX - 1) + line [textCursorY].substring (textCursorX);
                        dirty = redrawLineAtCursor = true; // we only have to redraw this line
                        textCursorX--;
                      } else if (textCursorY > 0) { // combine 2 lines
                        textCursorY--;
                        textCursorX = line [textCursorY].length (); 
                        line [textCursorY] += line [textCursorY + 1];
                        for (int i = textCursorY + 1; i <= fileLines - 2; i++) line [i] = line [i + 1]; line [--fileLines] = ""; // shift lines up and free memory if the last one used
                        dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)
                      }
                      break; 
            case 127: // Windows telnet.exe: delete, putty: backspace
                      if (textCursorX < line [textCursorY].length ()) { // delete one character at cursor position
                        line [textCursorY] = line [textCursorY].substring (0, textCursorX) + line [textCursorY].substring (textCursorX + 1);
                        dirty = redrawLineAtCursor = true; // we only need to redraw this line
                      } else { // combine 2 lines
                        if (textCursorY < fileLines - 1) {
                          line [textCursorY] += line [textCursorY + 1];
                          for (int i = textCursorY + 1; i < fileLines - 1; i++) line [i] = line [i + 1]; line [--fileLines] = ""; // shift lines up and free memory if the last one used
                          dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)                          
                        }
                      }
                      break;
            case '\n': // enter
                      if (fileLines >= MAX_LINES) {
                        message = " Too many lines ";
                      } else {
                        fileLines++;
                        for (int i = fileLines - 1; i > textCursorY + 1; i--) line [i] = line [i - 1]; // shift lines down, making a free line at textCursorY + 1
                        line [textCursorY + 1] = line [textCursorY].substring (textCursorX); // split current line at cursor into textCursorY + 1 ...
                        line [textCursorY] = line [textCursorY].substring (0, textCursorX); // ... and textCursorY
                        textCursorX = 0;
                        dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)
                        textCursorY++;
                      }
                      break;
            case '\r':  // ignore
                      break;
            default:  // normal character
                      if (c == '\t') s = "    "; else s = String (c); // treat tab as 4 spaces
                      line [textCursorY] = line [textCursorY].substring (0, textCursorX) + s + line [textCursorY].substring (textCursorX); // inser character into line textCurrorY at textCursorX position
                      dirty = redrawLineAtCursor = true; // we only have to redraw this line
                      textCursorX += s.length ();
                      break;
          }         
          // if cursor has moved - should we scroll?
          {
            if (textCursorX - textScrollX >= tsp->clientWindowCol2 - 4) {
              textScrollX = textCursorX - (tsp->clientWindowCol2 - 4) + 1; // scroll left if the cursor fell out of right client window border
              redrawAllLines = true; // we need to redraw all visible lines
            }
            if (textCursorX - textScrollX < 0) {
              textScrollX = textCursorX; // scroll right if the cursor fell out of left client window border
              redrawAllLines = true; // we need to redraw all visible lines
            }
            if (textCursorY - textScrollY >= tsp->clientWindowRow2 - 2) {
              // Serial.println ("scroll up");
              textScrollY = textCursorY - (tsp->clientWindowRow2 - 2) + 1; // scroll up if the cursor fell out of bottom client window border
              redrawAllLines = true; // we need to redraw all visible lines
            }
            if (textCursorY - textScrollY < 0) {
              textScrollY = textCursorY; // scroll down if the cursor fell out of top client window border
              redrawAllLines = true; // we need to redraw all visible lines
            }
          }
        }
        return ""; 
      }
                        
  };
  
  #include "time_functions.h"

#endif
