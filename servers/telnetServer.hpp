/*

    telnetServer.hpp

    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ft__p_telnet_server_template

    Telnet server can handle some commands itself (for example uname, uptime) but the calling program can also provide its own 
    telnetCommandHandlerCallback function to handle some commands itself. A simple telnet client is also implemented as one of the built-in
    telnet commands but it doesn't expose an application program interface.
  
    February, 6, 2022, Bojan Jurca

    Nomenclature used here for easier understaning of the code:

     - "connection" normally applies to TCP connection from when it was established to when it is terminated

                  There is normally only one TCP connection per telnet session. These terms are pretty much interchangeable when we are talking about telnet.

     - "session" applies to user interaction between login and logout

                  The first thing that the user does when a TCP connection is established is logging in and the last thing is logging out. It TCP
                  connection drops for some reason the user is automatically logged out.

      - "buffer size" is the number of bytes that can be placed in a buffer. 
      
                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".

    Telnet protocol is basically a straightforward non synchronized transfer of text data in both directions, from client to server and vice versa.
    This implementation also adds processing command lines by detecting the end-of-line, parsing command line to arguments and then executing
    some already built-in commands and sending replies back to the client. In addition the calling program can provide its own command handler to handle
    some commands itself. Special telnet commands, so called IAC ("Interpret As Command") commands are placed within text stream and should be
    extracted from it and processed by both, the server and the client. All IAC commands start with character 255 (IAC character) and are followed
    by other characters. The number of characters following IAC characters varies from command to command. For example: the server may request
    the client to report its window size by sending IAC (255) DO (253) NAWS (31) and then the client replies with IAC (255) SB (250) NAWS (31) following
    with 2 bytes of window width and 2 bytes of window height and then IAC (255) SE (240).
    
*/


    // ----- includes, definitions and supporting functions -----

    #include <WiFi.h>
    // hard reset by triggering watchdog
    #include <esp_int_wdt.h>
    #include <esp_task_wdt.h>


#ifndef __TELNET_SERVER__
  #define __TELNET_SERVER__


    // TUNNING PARAMETERS

    #define TELNET_SERVER_STACK_SIZE 2 * 1024                   // TCP listener
    #define TELNET_CONNECTION_STACK_SIZE 16 * 1024              // TCP connection
    #define TELNET_CMDLINE_BUFFER_SIZE 128                      // reading and temporary keeping telnet command lines
    #define TELNET_CONNECTION_TIME_OUT 300000                   // 300000 ms = 5 min 
    #define TELNET_SESSION_MAX_ARGC 24                          // max number of arguments in command line 

    #define telnetServiceUnavailable (char *) "Telnet service is currently unavailable.\r\n"

    #ifndef HOSTNAME
      #define "MyESP32Server"                               // use default if not defined previously
    #endif
    #ifndef MACHINETYPE
      #define MACHINETYPE "ESP32 Dev Module"                // use default if not defined previously
    #endif

    #include "version_of_servers.h"                         // version of this software, used in uname command
    #define UNAME String (MACHINETYPE) + " (" + ESP.getCpuFreqMHz () + " MHz) " + HOSTNAME + " SDK:" + ESP.getSdkVersion () + " " + VERSION_OF_SERVERS // + " IDF:" + String (ESP_IDF_VERSION_MAJOR) + "." + String (ESP_IDF_VERSION_MINOR) + "." + String (ESP_IDF_VERSION_PATCH)


    // ----- CODE -----

    #include "dmesg_functions.h"

    #include "user_management.h"    // for logging in


    // define some IAC telnet commands
    #define __IAC__               0Xff   // 255 as number
    #define IAC                   "\xff" // 255 as string
    #define __DONT__              0xfe   // 254 as number
    #define DONT                  "\xfe" // 254 as string
    #define __DO__                0xfd   // 253 as number
    #define DO                    "\xfd" // 253 as string
    #define __WONT__              0xfc   // 252 as number
    #define WONT                  "\xfc" // 252 as string
    #define __WILL__              0xfb   // 251 as number
    #define WILL                  "\xfb" // 251 as string
    #define __SB__                0xfa   // 250 as number
    #define SB                    "\xfa" // 250 as string
    #define __SE__                0xf0   // 240 as number
    #define SE                    "\xf0" // 240 as string
    #define __LINEMODE__          0x22   //  34 as number
    #define LINEMODE              "\x22" //  34 as string
    #define __NAWS__              0x1f   //  31 as number
    #define NAWS                  "\x1f" //  31 as string
    #define __SUPPRESS_GO_AHEAD__ 0x03   //   3 as number
    #define SUPPRESS_GO_AHEAD     "\x03" //   3 as string
    #define __ECHO__              0x01   //   1 as number
    #define ECHO                  "\x01" //   1 as string 

    // control telnetServer critical sections
    static SemaphoreHandle_t __telnetServerSemaphore__ = xSemaphoreCreateMutex (); 


    // ----- telnetConnection class -----

    class telnetConnection {

      public:

        // telnetConnection state
        enum STATE_TYPE {
          NOT_RUNNING = 0, 
          RUNNING = 2        
        };

        STATE_TYPE state () { return __state__; }

        telnetConnection ( // the following parameters will be handeled by telnetConnection instance
                         int connectionSocket,
                         String (* telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection *tcn), // telnetCommandHandler callback function provided by calling program
                         char *clientIP, char *serverIP
                       )  {
                            // create a local copy of parameters for later use
                            __connectionSocket__ = connectionSocket;
                            __telnetCommandHandlerCallback__ = telnetCommandHandlerCallback;
                            strncpy (__clientIP__, clientIP, sizeof (__clientIP__)); __clientIP__ [sizeof (__clientIP__) - 1] = 0; // copy client's IP since connection may live longer than the server that created it
                            strncpy (__serverIP__, serverIP, sizeof (__serverIP__)); __serverIP__ [sizeof (__serverIP__) - 1] = 0; // copy server's IP since connection may live longer than the server that created it
                            // handle connection in its own thread (task)       
                            #define tskNORMAL_PRIORITY 1
                            if (pdPASS != xTaskCreate (__connectionTask__, "telnetConnection", TELNET_CONNECTION_STACK_SIZE, this, tskNORMAL_PRIORITY, NULL)) {
                              dmesg ("[telnetConnection] xTaskCreate error");
                            } else {
                              __state__ = RUNNING;                            
                            }

                            #ifdef __PERFMON__
                              xSemaphoreTake (__telnetServerSemaphore__, portMAX_DELAY);
                                __perfOpenedTelnetConnections__ ++;
                                __perfConcurrentTelnetConnections__ ++;
                              xSemaphoreGive (__telnetServerSemaphore__);
                            #endif
                          }

        ~telnetConnection ()  {
                                closeConnection ();
                                #ifdef __PERFMON__
                                  xSemaphoreTake (__telnetServerSemaphore__, portMAX_DELAY);
                                    __perfConcurrentTelnetConnections__ --;
                                  xSemaphoreGive (__telnetServerSemaphore__);
                                #endif
                            }

        int getSocket () { return __connectionSocket__; }

        char *getClientIP () { return __clientIP__; }

        char *getServerIP () { return __serverIP__; }

        bool connectionTimeOut () { return __telnet_connection_time_out__ && millis () - __lastActive__ >= __telnet_connection_time_out__; }
        
        void closeConnection () {
                                  int connectionSocket;
                                  xSemaphoreTake (__telnetServerSemaphore__, portMAX_DELAY);
                                    connectionSocket = __connectionSocket__;
                                    __connectionSocket__ = -1;
                                  xSemaphoreGive (__telnetServerSemaphore__);
                                  if (connectionSocket > -1) close (connectionSocket);
                                }

        // telnet session related variables
        char *getUserName () { return __userName__; }
        #ifdef __FILE_SYSTEM__
          char *getHomeDir () { return __homeDir__; }
          char *getWorkingDir () { return __workingDir__; }
        #endif
        char *getPrompt () { return __prompt__; }
        uint16_t getClientWindowWidth () { return __clientWindowWidth__; }
        uint16_t getClientWindowHeight () { return __clientWindowHeight__; }

        // reading input from Telnet client with extraction of IAC commands

        char readTelnetChar (bool onlyPeek = false) { // returns valid character or 0 in case of error, extracts IAC commands from stream
          // if there is a character that has been just peeked previously return it now
          if (__peekedOnlyChar__) { char c = __peekedOnlyChar__; __peekedOnlyChar__ = 0; return c; }
          // if we are just peeking check the socket
          if (onlyPeek) {
            char c; if (recv (__connectionSocket__, &c, sizeof (c), MSG_PEEK) == -1) return 0; // meaning that no character is pending to be read
          }
          // read the next character from the socket
        tryReadingCharAgain:
          // if time-out occured return 0 
          if (__telnet_connection_time_out__ && millis () - __lastActive__ >= __telnet_connection_time_out__) return 0; // time-out

          char charRead = 0;
          char c [2];
          if (recvAll (__connectionSocket__, c, sizeof (c), NULL, __telnet_connection_time_out__) <= 0) return 0;
          
          switch (c [0]) {
              case 3:       charRead = 3; break;    // Ctrl-C
              case 4:                               // Ctrl-D
              case 26:      charRead = 4; break;    // Ctrl-Z
              case 8:       charRead = 8; break;    // BkSpace
              case 127:     charRead = 127; break;  // Del
              case 9:       charRead = 9; break;    // Tab
              case 10:      charRead = 10; break;   // Enter
              case __IAC__: 
                              // read the next character
                              if (recvAll (__connectionSocket__, c, sizeof (c), NULL, __telnet_connection_time_out__) <= 0) return 0;
                              switch (c [0]) {
                                case __SB__: 
                                              // read the next character
                                              if (recvAll (__connectionSocket__, c, sizeof (c), NULL, __telnet_connection_time_out__) <= 0) return 0;
                                              if (c [0] == __NAWS__) { 
                                                // read the next 4 bytes indication client window size
                                                char c [5];
                                                if (recvAll (__connectionSocket__, c, sizeof (c), NULL, __telnet_connection_time_out__) < 4) return 0;
                                                __clientWindowWidth__ = (uint16_t) c [0] << 8 | (uint8_t) c [1]; 
                                                __clientWindowHeight__ = (uint16_t) c [2] << 8 | (uint8_t) c [3];
                                                // DEBUG: 
                                                Serial.printf ("[%10lu] [telnetConnection] client reported its window size: %i x %i\n", millis (), __clientWindowWidth__, __clientWindowHeight__);
                                              }
                                              break;
                                // in the following cases the 3rd character is following, ignore this one too
                                case __WILL__:
                                case __WONT__:
                                case __DO__:
                                case __DONT__:
                                                if (recvAll (__connectionSocket__, c, sizeof (c), NULL, __telnet_connection_time_out__) <= 0) return 0;
                                default:        // ignore
                                                break;
                              } // switch level 2, after IAC
                              break;              
              default:      
                            __lastActive__ = millis (); // reset connection level countdown after sucessful read
                            if ((c [0] >= 19) && (c [0] < 240)) charRead = c [0]; // Ctrl-S <= c < SE, try to ignore control characters
                            break; // ignore other characters
          } // switch level 1

          if (onlyPeek) {
            if (charRead) { // if there is a regular character that we have just read
              __peekedOnlyChar__ = charRead;
            } else { // no regular characer read (jet), check if there is something more waiting
              if (recv (__connectionSocket__, &charRead, sizeof (charRead), MSG_PEEK) == -1) return 0; // meaning that no character is pending to be read
              else goto tryReadingCharAgain;
            }
          } else {
            if (!charRead) goto tryReadingCharAgain;
          }
          
          return charRead;
        }

        // reads incoming stream, returns last character read or 0-error, 3-Ctrl-C, 4-EOF, 10-Enter, ...
        char readTelnetLine (char *buf, size_t len, bool trim = true) {
          if (!len) return 0;
          int characters = 0;
          buf [0] = 0;

          while (true) { // read and process incomming data in a loop 
            char c = readTelnetChar ();
            switch (c) {
                case 0:   return 0; // Error
                case 3:   return 3; // Ctrl-C
                case 4:             // EOF - Ctrl-D (UNIX)
                case 26:  return 4; // EOF - Ctrl-Z (Windows)
                case 8:   // backspace - delete last character from the buffer and from the screen
                case 127: // in Windows telent.exe this is del key, but putty reports backspace with code 127, so let's treat it the same way as backspace
                          if (characters && buf [characters - 1] >= ' ') {
                            buf [-- characters] = 0;
                            if (__echo__ && sendAll (__connectionSocket__, (char *) "\x08 \x08", strlen ("\x08 \x08"), __telnet_connection_time_out__) <= 0) return 0; // delete the last character from the screen
                          }
                          break;                            
                case 10:  // LF
                          if (trim) {
                            while (buf [0] == ' ') strcpy (buf, buf + 1); // ltrim
                            int i; for (i = 0; buf [i] > ' '; i++); buf [i] = 0; // rtrim
                          }
                          if (__echo__ && sendAll (__connectionSocket__, (char *) "\r\n", 2, __telnet_connection_time_out__) <= 0) return 0; // echo CRLF to the screen
                          return 10;
                case 13:  // CR
                          break; // ignore 
                case 9:   // tab - treat as 2 spaces
                          c = ' ';
                          if (characters < len - 1) {
                            buf [characters] = c; buf [++ characters] = 0;
                            if (__echo__ && sendAll (__connectionSocket__, &c, sizeof (c), __telnet_connection_time_out__) <= 0) return 0; // echo last character to the screen
                          }                
                          // continue with default (repeat adding a space):
                default:  // fill the buffer 
                          if (characters < len - 1) {
                            buf [characters] = c; buf [++ characters] = 0;
                            if (__echo__ && sendAll (__connectionSocket__, &c, sizeof (c), __telnet_connection_time_out__) <= 0) return 0; // echo last character to the screen
                          }
                          break;              
            } // switch
          } // while
          return 0;
        }
        
      private:

        STATE_TYPE __state__ = NOT_RUNNING;

        unsigned long __lastActive__ = millis (); // time-out detection        

        unsigned long __telnet_connection_time_out__ = TELNET_CONNECTION_TIME_OUT;
      
        int __connectionSocket__ = -1;
        String (*__telnetCommandHandlerCallback__) (int argc, char *argv [], telnetConnection *tcn) = NULL;
        char __clientIP__ [46] = "";
        char __serverIP__ [46] = "";

        char __peekedOnlyChar__ = 0;
        char __cmdLine__ [TELNET_CMDLINE_BUFFER_SIZE]; // characters cleared of IAC commands, 1 line at a time

        // telnet session related variables
        char __userName__ [USER_PASSWORD_MAX_LENGTH + 1] = "";
        #ifdef __FILE_SYSTEM__
          char __homeDir__ [FILE_PATH_MAX_LENGTH + 1] = "";
          char __workingDir__ [FILE_PATH_MAX_LENGTH + 1] = "";
        #endif
        char *__prompt__ = (char *) "*";
        uint16_t __clientWindowWidth__ = 0;
        uint16_t __clientWindowHeight__ = 0;

        bool __echo__ = true;

        String __pad__ (String s, int toLenght) { while (s.length () < toLenght) s += " "; return s; }
        
        static void __connectionTask__ (void *pvParameters) {
          // get "this" pointer
          telnetConnection *ths = (telnetConnection *) pvParameters;           
          { // code block
            { // login procedure
              #if USER_MANAGEMENT == NO_USER_MANAGEMENT
                // prepare session defaults
                strcpy (ths->__userName__, "root");
                #ifdef __FILE_SYSTEM__
                  getUserHomeDirectory (ths->__homeDir__, sizeof (ths->__homeDir__), ths->__userName__);
                  strcpy (ths->__workingDir__, ths->__homeDir__);
                #endif
                ths->__prompt__ = (char *) "\r\n# ";
                dmesg ("[telnetConnection] user logged in: ", ths->__userName__);
                // tell the client to go into character mode, not to echo and send its window size, then say hello 
                sprintf (ths->__cmdLine__, IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS "Hello %s\r\nWelcome %s, use \"help\" to display available commands.", ths->__clientIP__, ths->__userName__);
                if (sendAll (ths->__connectionSocket__, ths->__cmdLine__, strlen (ths->__cmdLine__), ths->__telnet_connection_time_out__) <= 0) {
                  dmesg ("[telnetConnection] send error: ", errno);
                  goto endOfConnection;
                }
              #else
                // tell the client to go into character mode, not to echo an send its window size, then say hello 
                sprintf (ths->__cmdLine__, IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS "Hello %s\r\nuser: ", ths->__clientIP__);
                if (sendAll (ths->__connectionSocket__, ths->__cmdLine__, strlen (ths->__cmdLine__), ths->__telnet_connection_time_out__) <= 0) {
                  dmesg ("[telnetConnection] send error: ", errno);
                  goto endOfConnection;
                }
                if (ths->readTelnetLine (ths->__userName__, sizeof (ths->__userName__)) != 10) { 
                  goto endOfConnection;                
                }
                // if (!ths->__userName__ [0]) endOfConnection;
                char password [USER_PASSWORD_MAX_LENGTH + 1] = "";
                ths->__echo__ = false;
                if (sendAll (ths->__connectionSocket__, (char *) "password: ", strlen ("password: "), ths->__telnet_connection_time_out__) <= 0) {
                  dmesg ("[telnetConnection] send error: ", errno);
                  goto endOfConnection;
                }
                if (ths->readTelnetLine (password, sizeof (password)) != 10) { 
                  goto endOfConnection;                
                }
                // if (!ths->__password__ [0]) endOfConnection;
                ths->__echo__ = true;
                // check user name and password
                if (!checkUserNameAndPassword (ths->__userName__, password)) { 
                  dmesg ("[telnetConnection] user failed to login: ", ths->__userName__);
                  delay (100);
                  sendAll (ths->__connectionSocket__, (char *) "\r\nUsername and/or password incorrect.", strlen ("\r\nUsername and/or password incorrect."), 1000);
                  goto endOfConnection;                                
                }
                dmesg ("[telnetConnection] user logged in: ", ths->__userName__);
                // prepare session defaults
                #ifdef __FILE_SYSTEM__
                  getUserHomeDirectory (ths->__homeDir__, sizeof (ths->__homeDir__), ths->__userName__);
                  strcpy (ths->__workingDir__, ths->__homeDir__);
                #endif
                ths->__prompt__ = strcmp (ths->__userName__, "root") ? (char *) "\r\n$ " : (char *) "\r\n# ";
                sprintf (ths->__cmdLine__, "\r\nWelcome %s, use \"help\" to display available commands.", ths->__userName__);
                if (sendAll (ths->__connectionSocket__, ths->__cmdLine__, strlen (ths->__cmdLine__), ths->__telnet_connection_time_out__) <= 0) {
                  dmesg ("[telnetConnection] send error: ", errno);
                  goto endOfConnection;
                }
              #endif
            } // login procedure
            // this is where telnet session really starts
            while (true) { // endless loop of reading and executing commands
              
                // write prompt
                if (sendAll (ths->__connectionSocket__, ths->__prompt__, strlen (ths->__prompt__), ths->__telnet_connection_time_out__) <= 0) {
                  dmesg ("[telnetConnection] send error: ", errno);
                  goto endOfConnection;
                }
                // read command line
                switch (ths->readTelnetLine (ths->__cmdLine__, sizeof (ths->__cmdLine__), false)) {
                  case 3:   { // Ctrl-C, end the connection
                              sendAll (ths->__connectionSocket__, (char *) "\r\nCtrl-C", strlen ("\r\nCtrl-C"), 1000);
                              goto endOfConnection;
                            }
                  case 10:  { // Enter, parse command line

                              // DEBUG: Serial.printf ("cmdline = %s\n", ths->__cmdLine__); Serial.println ("-----------");
                              int argc = 0;
                              char *argv [TELNET_SESSION_MAX_ARGC];
  
                              char *q = ths->__cmdLine__ - 1;
                              while (true) {
                                char *p = q + 1; while (*p && *p <= ' ') p++;
                                if (*p) { // p points to the beginning of an argument
                                  bool quotation = false;
                                  if (*p == '\"') { quotation = true; p++; }
                                  argv [argc++] = p;
                                  q = p; 
                                  while (*q && (*q > ' ' || quotation)) if (*q == '\"') break; else q++; // q points after the end of an argument 
                                  if (*q) *q = 0; else break;
                                } else break;
                                if (argc == TELNET_SESSION_MAX_ARGC) break;
                              }
                              // DEBUG: Serial.printf ("argc = %i\n", argc); for (int i = 0; i < argc; i++) Serial.printf ("argv [%i] = '%s'\n", i, argv [i]); Serial.println ("-----------");
  
                              // process commandLine
                              if (argc) {

                                // ask telnetCommandHandler (if it is provided by the calling program) if it is going to handle this command, otherwise try to handle it internally
                                String s ("");
                                if (ths->__telnetCommandHandlerCallback__) s = ths->__telnetCommandHandlerCallback__ (argc, argv, ths);
                                if (s == "") s = ths->internalTelnetCommandHandler (argc, argv, ths);
                                if (ths->__connectionSocket__ == -1) goto endOfConnection; // in case of quit
                                // write reply
                                if (s != "" && sendAll (ths->__connectionSocket__, (char *) s.c_str (), s.length (), ths->__telnet_connection_time_out__) <= 0) {
                                  dmesg ("[telnetConnection] send error: ", errno);
                                  goto endOfConnection;
                                }
                              }
  
                              break;
                            }
                  case 0:   {
                              if (errno == EAGAIN || errno == ENAVAIL) sendAll (ths->__connectionSocket__, (char *) "\r\nTime-out", strlen ("\r\nTime-out"), 1000);
                              goto endOfConnection;                
                            }
                  default:  // ignore
                            break;
                }
            
            } // endless loop of reading and executing commands
            
          } // code block
        endOfConnection:  
          if (*ths->__userName__) dmesg ("[telnetConnection] user logged out: ", ths->__userName__);
          // all variables are freed now, unload the instance and stop the task (in this order)
          delete ths;
          vTaskDelete (NULL);                
        }

        String internalTelnetCommandHandler (int argc, char *argv [], telnetConnection *tcn) {

          #define argv0Is(X) (argc > 0 && !strcmp (argv [0], X))
          #define argv1Is(X) (argc > 1 && !strcmp (argv [1], X))

               if (argv0Is ("help"))      { 
                                            if (argc == 1) return __help__ (); 
                                            return F ("Wrong syntax, use help");              
                                          }
                                          
          else if (argv0Is ("clear"))     { 
                                            if (argc == 1) return __clear__ ();  
                                            return F ("Wrong syntax, use clear");              
                                          }
                                          
          else if (argv0Is ("uname"))     { 
                                            if (argc == 1) return __uname__ ();  
                                            return F ("Wrong syntax, use uname");              
                                          }
                                            
          else if (argv0Is ("free"))      { 
                                            if (argc == 1)                                        return __free__ (0, tcn);
                                            if (argv1Is ("-f") || argv1Is ("-follow")) {
                                              if (argc == 2)                                      return __free__ (1, tcn);
                                              if (argc == 3) {
                                                int n = atoi (argv [2]); if (n > 0 && n <= 3600)  return __free__ (n, tcn);
                                              }
                                            }
                                            return F ("Wrong syntax, use free [-follow [<n>]]   (where 0 < n <= 3600)");
                                          } 

          else if (argv0Is ("nohup"))     { 
                                            if (argc == 1) return __nohup__ (0);
                                            if (argc == 2) {
                                              int n = atoi (argv [1]); if (n > 0 && n < 3600) return __nohup__ (n);
                                            }
                                            return F ("Wrong syntax, use nohup [<n>]   (where 0 < n <= 3600)");
                                          } 
                                        
          else if (argv0Is ("reboot"))    { 
                                            if (argc == 1)                            return __reboot__ (true);
                                            if (argv1Is ("-h") || argv1Is ("-hard"))  return __reboot__ (false);
                                            return F ("Wrong syntax, use reboot [-hard]");              
                                          }  
                                      
          else if (argv0Is ("quit"))      { 
                                            if (argc == 1) return __quit__ ();  
                                            return F ("Wrong syntax, use quit");              
                                          }

        #ifdef __TIME_FUNCTIONS__

          else if (argv0Is ("uptime"))    { 
                                            if (argc == 1) return __uptime__ ();  
                                            return F ("Wrong syntax, use uptime");              
                                          }

          else if (argv0Is ("date"))      {
                                            if (argc == 1)                                          return __getDateTime__ ();
                                            if (argc == 4 && (argv1Is ("-s") || argv1Is ("-set")))  return __setDateTime__ (argv [2], argv [3]);
                                            return F ("Wrong syntax, use date [-set YYYY/MM/DD hh:mm:ss] (use hh in 24 hours time format)");
                                          }

          else if (argv0Is ("ntpdate"))   { 
                                            if (argc == 1) return __ntpdate__ ();
                                            if (argc == 2) return __ntpdate__ (argv [1]);
                                            return F ("Wrong syntax, use ntpdate [ntpServer]");
                                          }

          else if (argv0Is ("crontab"))   { 
                                            if (argc == 1) return cronTab ();  
                                            return F ("Wrong syntax, use crontab");              
                                          }

        #endif

        #ifdef __NETWORK__

          else if (argv0Is ("ping"))      { 
                                            if (argc == 2) { ping (argv [1], PING_DEFAULT_COUNT, PING_DEFAULT_INTERVAL, PING_DEFAULT_SIZE, PING_DEFAULT_TIMEOUT, __connectionSocket__); return ""; }
                                            return F ("Wrong syntax, use ping <target computer>");              
                                          }

          else if (argv0Is ("ifconfig"))  { 
                                            if (argc == 1) return ifconfig ();  
                                            return F ("Wrong syntax, use arp");              
                                          }

          else if (argv0Is ("arp"))       { 
                                            if (argc == 1) return arp ();  
                                            return F ("Wrong syntax, use arp");              
                                          }

          else if (argv0Is ("iw"))        { 
                                            if (argc == 1) return iw (__connectionSocket__);  
                                            return F ("Wrong syntax, use iw");              
                                          }

        #endif

        #ifdef __DMESG__

          else if (argv0Is ("dmesg"))     { 
                                            bool f = false;
                                            bool t = false;
                                            for (int i = 1; i < argc; i++) {
                                                   if (!strcmp (argv [i], (char *) "-f") || !strcmp (argv [i], (char *) "-follow")) f = true;
                                              else if (!strcmp (argv [i], (char *) "-t") || !strcmp (argv [i], (char *) "-time")) t = true;
                                              else return F ("Wrong syntax, use dmesg [-follow] [-time]");
                                            }
                                            return __dmesg__ (f, t, tcn);
                                          }
                                      
        #endif

          else if (argv0Is ("telnet"))    { 
                                            if (argc == 2) return __telnet__ (argv [1], 23, tcn);
                                            if (argc == 3) {
                                              int port = atoi (argv [2]); if (port > 0) return __telnet__ (argv [1], port, tcn);
                                            }
                                            return F ("Wrong syntax, use telnet <server> [port]");
                                          } 

        #ifdef __HTTP_CLIENT__

          else if (argv0Is ("curl"))      { 
                                            if (argc == 2) return __curl__ ((char *) "GET", argv [1]);
                                            if (argc == 3) return __curl__ (argv [1], argv [2]);
                                            return F ("Wrong syntax, use curl [method] http://url");
                                          } 
                                      
        #endif

        #ifdef __SMTP_CLIENT__

          else if (argv0Is ("sendmail"))  { 
                                            char *smtpServer = (char *) "";
                                            char *smtpPort = (char *) "";
                                            char *userName = (char *) "";
                                            char *password = (char *) "";
                                            char *from = (char *) "";
                                            char *to = (char *) ""; 
                                            char *subject = (char *) ""; 
                                            char *message = (char *) "";
                                          
                                            for (int i = 1; i < argc - 1; i += 2) {
                                                   if (!strcmp (argv [i], "-S")) smtpServer = argv [i + 1];
                                              else if (!strcmp (argv [i], "-P")) smtpPort = argv [i + 1];
                                              else if (!strcmp (argv [i], "-u")) userName = argv [i + 1];
                                              else if (!strcmp (argv [i], "-p")) password = argv [i + 1];
                                              else if (!strcmp (argv [i], "-f")) from = argv [i + 1];
                                              else if (!strcmp (argv [i], "-t")) to = argv [i + 1];
                                              else if (!strcmp (argv [i], "-s")) subject = argv [i + 1];
                                              else if (!strcmp (argv [i], "-m")) message = argv [i + 1];
                                              else return F ("Wrong syntax, use sendmail [-S smtpServer] [-P smtpPort] [-u userName] [-p password] [-f from address] [t to address list] [-s subject] [-m messsage]");
                                            }

                                            return sendMail (message, subject, to, from, password, userName, atoi (smtpPort), smtpServer);
                                          }
  
        #endif

        #ifdef __FTP_CLIENT__ 

          else if (argv0Is ("ftpput"))    { 
                                            char *ftpServer = (char *) "";
                                            char *ftpPort = (char *) "";
                                            char *userName = (char *) "";
                                            char *password = (char *) "";
                                            char *localFileName = NULL;
                                            char *remoteFileName = NULL;
                                            bool syntaxError = false;
                                          
                                            for (int i = 1; i < argc && !syntaxError; i++) {
                                                   if (!strcmp (argv [i], "-S")) { if (i < argc - 1) ftpServer = argv [i++ + 1]; else syntaxError = true; }
                                              else if (!strcmp (argv [i], "-P")) { if (i < argc - 1) ftpPort   = argv [i++ + 1]; else syntaxError = true; }
                                              else if (!strcmp (argv [i], "-u")) { if (i < argc - 1) userName  = argv [i++ + 1]; else syntaxError = true; }
                                              else if (!strcmp (argv [i], "-p")) { if (i < argc - 1) password  = argv [i++ + 1]; else syntaxError = true; }
                                              else if (*argv [i] != '-')  {
                                                                            if (!localFileName) localFileName = argv [i];
                                                                            else if (!remoteFileName) remoteFileName = argv [i];
                                                                            else syntaxError = true;
                                                                          }
                                              else syntaxError = true;
                                            }
                                            if (!remoteFileName) remoteFileName = localFileName;
                                            if (!localFileName) syntaxError = true;
                                            if (syntaxError) return F ("Wrong syntax, use ftpput [-S ftpServer] [-P ftpPort] [-u userName] [-p password] <localFileName> <remoteFileName>");
                                            return __ftpPut__ (localFileName, remoteFileName, password, userName, atoi (ftpPort), ftpServer);
                                          }

          else if (argv0Is ("ftpget"))    { 
                                            char *ftpServer = (char *) "";
                                            char *ftpPort = (char *) "";
                                            char *userName = (char *) "";
                                            char *password = (char *) "";
                                            char *localFileName = NULL;
                                            char *remoteFileName = NULL;
                                            bool syntaxError = false;
                                          
                                            for (int i = 1; i < argc && !syntaxError; i++) {
                                                   if (!strcmp (argv [i], "-S")) { if (i < argc - 1) ftpServer = argv [i++ + 1]; else syntaxError = true; }
                                              else if (!strcmp (argv [i], "-P")) { if (i < argc - 1) ftpPort   = argv [i++ + 1]; else syntaxError = true; }
                                              else if (!strcmp (argv [i], "-u")) { if (i < argc - 1) userName  = argv [i++ + 1]; else syntaxError = true; }
                                              else if (!strcmp (argv [i], "-p")) { if (i < argc - 1) password  = argv [i++ + 1]; else syntaxError = true; }
                                              else if (*argv [i] != '-')  {
                                                                            if (!localFileName) localFileName = argv [i];
                                                                            else if (!remoteFileName) remoteFileName = argv [i];
                                                                            else syntaxError = true;
                                                                          }
                                              else syntaxError = true;
                                            }
                                            if (!remoteFileName) remoteFileName = localFileName;
                                            if (!localFileName) syntaxError = true;
                                            if (syntaxError) return F ("Wrong syntax, use ftpget [-S ftpServer] [-P ftpPort] [-u userName] [-p password] <localFileName> <remoteFileName>");
                                            return __ftpGet__ (localFileName, remoteFileName, password, userName, atoi (ftpPort), ftpServer);
                                          }
        #endif

        #ifdef __FILE_SYSTEM__        
      
          #if FILE_SYSTEM == FILE_SYSTEM_FAT

            else if (argv0Is ("mkfs.fat"))  { 
                                            if (argc == 1) {
                                              if (!strcmp (__userName__, "root")) return __mkfs__ (tcn);
                                              else                                return F ("Only root may format disk.");
                                            }                                     return F ("Wrong syntax, use mkfs.fat");
                                          } 
          
          #endif
          #if FILE_SYSTEM == FILE_SYSTEM_LITTLEFS

            else if (argv0Is ("mkfs.littlefs"))  { 
                                            if (argc == 1) {
                                              if (!strcmp (__userName__, "root")) return __mkfs__ (tcn);
                                              else                                return F ("Only root may format disk.");
                                            }                                     return F ("Wrong syntax, use mkfs.littlefs");
                                          } 
          
          #endif

          else if (argv0Is ("fs_info"))   {
                                            if (argc == 1)  return __fs_info__ ();
                                                            return F ("Wrong syntax, use fs_info");          
                                          }

          else if (argv0Is ("ls"))        {
                                            if (argc == 1) return __ls__ (__workingDir__);
                                            if (argc == 2) return __ls__ (argv [1]);
                                                           return F ("Wrong syntax, use ls [<directoryName>]");
                                          }

          else if (argv0Is ("tree"))      {
                                            if (argc == 1) return __tree__ (__workingDir__);
                                            if (argc == 2) return __tree__ (argv [1]);
                                                           return F ("Wrong syntax, use tree [<directoryName>]");
                                          }

          else if (argv0Is ("cat"))       {
                                            if (argc == 2)                  return __catFileToClient__ (argv [1], tcn);
                                            if (argc == 3 && argv1Is (">")) return __catClientToFile__ (argv [2], tcn);
                                            return F ("Wrong syntax, use cat [>] <fileName>");

                                          }

          else if (argv0Is ("tail"))      {
                                            if (argc == 2)                                            return __tail__ (argv [1], false, tcn);
                                            if (argc == 3 && (argv1Is ("-f") || argv1Is ("-follow"))) return __tail__ (argv [2], true, tcn);
                                            return F ("Wrong syntax, use tail [-follow] <fileName>");
                                          }
                                          
          else if (argv0Is ("vi"))        {  
                                            if (argc == 2)  return __vi__ (argv [1], tcn);
                                                            return F ("Wrong syntax, use vi <fileName>");
                                          }

          else if (argv0Is ("mkdir"))     {  
                                            if (argc == 2)  return __mkdir__ (argv [1]);
                                                            return F ("Wrong syntax, use mkdir <directoryName>");
                                          }

          else if (argv0Is ("rmdir"))     {
                                            if (argc == 2)  return __rmdir__ (argv [1]);
                                                            return F ("Wrong syntax, use rmdir <directoryName>");
                                          }

          else if (argv0Is ("cd"))        {
                                            if (argc == 2)  return __cd__ (argv [1]);
                                                            return F ("Wrong syntax, use cd <directoryName>");
                                          }

          else if (argv0Is ("pwd"))       {
                                            if (argc == 1)  return __pwd__ ();
                                                            return F ("Wrong syntax, use pwd");
                                          }

          else if (argv0Is ("mv"))        {
                                            if (argc == 3)  return __mv__ (argv [1], argv [2]);
                                                            return F ("Wrong syntax, use mv <existing fileName or directoryName> <new fileName or directoryName>");
                                          }
                                           
          else if (argv0Is ("cp"))        {
                                            if (argc == 3)  return __cp__ (argv [1], argv [2]);
                                                            return F ("Wrong syntax, use cp <existing fileName> <new fileName>");
                                          } 

          else if (argv0Is ("rm"))        {
                                            if (argc == 2)  return __rm__ (argv [1]);
                                                            return F ("Wrong syntax, use rm <fileName>");
                                          }
                                          
        #endif

        #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT
  
            else if (argv0Is ("useradd")) { 
                                            if (strcmp (__userName__, (char *) "root"))                             return F ("Only root may add users.");
                                            if (argc == 6 && !strcmp (argv [1], "-u") && !strcmp (argv [3], "-d"))  return userAdd (argv [5], argv [2], argv [4]);
                                                                                                                    return F ("Wrong syntax, use useradd -u userId -d userHomeDirectory userName (where userId > 1000)");
                                          }
                                          
            else if (argv0Is ("userdel")) { 
                                            if (strcmp (__userName__, (char *) "root")) return F ("Only root may delete users.");
                                            if (argc != 2)                              return F ("Wrong syntax. Use userdel userName");
                                            if (!strcmp (argv [1], (char *) "root"))    return F ("You don't really want to to this.");
                                                                                        return userDel (argv [1]);
                                          }
  
          else if (argv0Is ("passwd"))    { 
                                            if (argc == 1)                                                                      return __passwd__ (__userName__);
                                            if (argc == 2) {
                                              if (!strcmp (__userName__, (char *) "root") || !strcmp (argv [1], __userName__))  return __passwd__ (argv [1]); 
                                              else                                                                              return F ("You may change only your own password.");
                                            }
                                            return "";
                                          }
                                                      
        #endif

        #ifdef __PERFMON__

          else if (argv0Is ("perfmon"))   { 
                                            bool f = false;
                                            bool r = false;
                                            for (int i = 1; i < argc; i++) {
                                                   if (!strcmp (argv [i], (char *) "-f") || !strcmp (argv [i], (char *) "-follow")) f = true;
                                              else if (!strcmp (argv [i], (char *) "-r") || !strcmp (argv [i], (char *) "-reset")) r = true;
                                              else return F ("Wrong syntax, use perfmon [-follow] [-reset]");
                                            }
                                            return __perfmon__ (f, r, tcn);
                                          }
                                      
        #endif
                                      
          else return F ("Invalid command, use \"help\" to display available commands.");
        }

        String __help__ ()  {
                              return F (  "Suported commands:"
                                          "\r\n      help"
                                          "\r\n      clear"
                                          "\r\n      uname"
                                          "\r\n      free [<n>]      (where 0 < n <= 3600)"
                                          "\r\n      nohup [<n>]     (where 0 < n <= 3600)"
                                          "\r\n      quit"
  
                                        #ifdef __DMESG__
                                          "\r\n  dmesg circular queue:"
                                          "\r\n      dmesg [-follow] [-time]"
                                        #endif

                                        #ifdef __TIME_FUNCTIONS__
                                          "\r\n  time commands:"
                                          "\r\n      uptime"
                                          "\r\n      date [-set <YYYY/MM/DD hh:mm:ss>]"
                                          "\r\n      ntpdate [<ntpServer>]"
                                          "\r\n      crontab"
                                        #endif
                                        
                                          "\r\n  network commands:"

                                        #ifdef __NETWORK__
                                          "\r\n      ping <terget computer>"
                                          "\r\n      ifconfig"
                                          "\r\n      arp"
                                          "\r\n      iw"
                                        #endif

                                          "\r\n      telnet <server> [port]"
                                          
                                        #ifdef __HTTP_CLIENT__
                                          "\r\n      curl [method] http://url"
                                        #endif

                                        #ifdef __FTP_CLIENT__ 
                                          "\r\n      ftpput [-S ftpServer] [-P ftpPort] [-u userName] [-p password] <localFileName> <remoteFileName>"
                                          "\r\n      ftpget [-S ftpServer] [-P ftpPort] [-u userName] [-p password] <localFileName> <remoteFileName>"
                                        #endif

                                        #ifdef __SMTP_CLIENT__
                                          "\r\n      sendmail [-S smtpServer] [-P smtpPort] [-u userName] [-p password] [-f from address] [t to address list] [-s subject] [-m messsage]"
                                        #endif

                                        #ifdef __FILE_SYSTEM__
                                          "\r\n  file commands:"
                                          #if FILE_SYSTEM == FILE_SYSTEM_FAT
                                            "\r\n      mkfs.fat"
                                          #endif
                                          #if FILE_SYSTEM == FILE_SYSTEM_LITTLEFS
                                            "\r\n      mkfs.littlefs"
                                          #endif
                                          "\r\n      fs_info"
                                          "\r\n      ls [<directoryName>]"
                                          "\r\n      tree [<directoryName>]"
                                          "\r\n      mkdir <directoryName>"
                                          "\r\n      rmdir <directoryName>"
                                          "\r\n      cd <directoryName or ..>"
                                          "\r\n      pwd"
                                          "\r\n      cat [>] <fileName>"
                                          "\r\n      vi <fileName>"
                                          "\r\n      cp <existing fileName> <new fileName>"
                                          "\r\n      mv <existing fileName or directoryName> <new fileName or directoryName>"
                                          "\r\n      rm <fileName>"
                                        
                                        #endif

                                        #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT
                                          "\r\n  user management:"                                        
                                          "\r\n       useradd -u <userId> -d <userHomeDirectory> <userName>"
                                          "\r\n       userdel <userName>"
                                          "\r\n       passwd [<userName>]"
                                        #endif
                                        
                                        #ifdef __PERFMON__
                                          "\r\n  performance monitoring:"
                                          "\r\n      perfmon [-follow] [-reset]"                                          
                                        #endif

                                        #ifdef __FILE_SYSTEM__
                                        
                                          "\r\n  configuration files used by system:"
                                          #ifdef __TIME_FUNCTIONS__
                                            "\r\n      /etc/ntp.conf                             (contains NTP time servers names)"
                                            "\r\n      /etc/crontab                              (contains scheduled tasks)"
                                          #endif
                                          #ifdef __NETWORK__
                                            "\r\n      /network/interfaces                       (contains STA(tion) configuration)"
                                            "\r\n      /etc/wpa_supplicant/wpa_supplicant.conf   (contains STA(tion) credentials)"
                                            "\r\n      /etc/dhcpcd.conf                          (contains A(ccess) P(oint) configuration)"
                                            "\r\n      /etc/hostapd/hostapd.conf                 (contains A(ccess) P(oint) credentials)"
                                          #endif
                                          #ifdef __FTP_CLIENT__
                                            "\r\n      /etc/ftp/ftpclient.cf                     (contains ftpput and ftpget default settings)"
                                          #endif
                                          #ifdef __SMTP_CLIENT__
                                            "\r\n      /etc/mail/sendmail.cf                     (contains sendMail default settings)"
                                          #endif
                                          #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT
                                            "\r\n      /etc/passwd                               (contains users' accounts information)"
                                            "\r\n      /etc/shadow                               (contains users' passwords)"
                                          #endif
                                          #ifdef __SYSLOG__
                                            "\r\n      /var/log/syslog                           (contains system log messages)"
                                          #endif
                                          
                                        #endif
                                       );

        }

        String __clear__ () {
          return "\x1b[2J"; // ESC[2J = clear screen
        }

        String __uname__ () {
          return UNAME;
        }

        String __free__ (unsigned long delaySeconds, telnetConnection *tcn) {
          bool firstTime = true;
          int currentLine = 0;
          char s [100];
          do { // follow         
            if (firstTime || (tcn->getClientWindowHeight ()  && currentLine >= tcn->getClientWindowHeight ())) {
              sprintf (s, "%sFree heap       Max free block\r\n------------------------------", firstTime ? "" : "\r\n");
              if (sendAll (tcn->getSocket (), s, strlen (s), __telnet_connection_time_out__) <= 0) return "";
              currentLine = 2; // we have just displayed 2 lines (header)
            }
            if (!firstTime && delaySeconds) { // wait and check if user pressed a key
              unsigned long startMillis = millis ();
              while (millis () - startMillis < (delaySeconds * 1000)) {
                delay (100);
                if (tcn->readTelnetChar (true)) {
                  tcn->readTelnetChar (false); // read pending character
                  return ""; // return if user pressed Ctrl-C or any key
                } 
                if (tcn->connectionTimeOut ()) { sendAll (tcn->getSocket (), (char *) "\r\nTime-out", strlen ("\r\nTime-out"), 1000); tcn->closeConnection (); return ""; }
              }
            }
            sprintf (s, "\r\n%10lu   %10lu  bytes", (unsigned long) ESP.getFreeHeap (), (unsigned long) heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
            if (sendAll (tcn->getSocket (), s, strlen (s), __telnet_connection_time_out__) <= 0) return "";
            firstTime = false;
            currentLine ++; // we have just displayed the next line (data)
          } while (delaySeconds);
          return "";
        }

        String __nohup__ (unsigned long timeOutSeconds) {
          __telnet_connection_time_out__ = timeOutSeconds * 1000;
          return timeOutSeconds ? "Time-out set to " + String (timeOutSeconds) + " seconds." : "Time-out set to infinite.";
        }

        String __reboot__ (bool softReboot) {
          sendAll (__connectionSocket__, (char *) "Rebooting ...", strlen ("Rebooting ..."), 1000);
          delay (250);
          if (softReboot) {
            ESP.restart ();
          } else {
            // cause WDT reset
            esp_task_wdt_init (1, true);
            esp_task_wdt_add (NULL);
            while (true);
          }
          return "";
        }
  
        String __quit__ () {
          closeConnection ();
          return "";  
        }

      #ifdef __TIME_FUNCTIONS__

        String __uptime__ () {
          String s;
          char c [15];
          time_t t = getGmt ();
          time_t uptime;
          if (t) { // if time is set
            struct tm strTime = timeToStructTime (timeToLocalTime (t));
            sprintf (c, "%02i:%02i:%02i", strTime.tm_hour, strTime.tm_min, strTime.tm_sec);
            s = String (c) + " up ";     
          } else { // time is not set (yet), just read how far clock has come untill now
            s = "Up ";
          }
          uptime = getUptime ();
          int seconds = uptime % 60; uptime /= 60;
          int minutes = uptime % 60; uptime /= 60;
          int hours = uptime % 24;   uptime /= 24; // uptime now holds days
          if (uptime) s += String ((unsigned long) uptime) + " days, ";
          sprintf (c, "%02i:%02i:%02i", hours, minutes, seconds);
          s += String (c);
          return s;
        }
  
        String __getDateTime__ () {
          time_t t = getGmt ();
          if (t) return timeToString (timeToLocalTime (t));
          else   return F ("The time has not been set yet.");  
        }
    
        String __setDateTime__ (char *dt, char *tm) {
          int Y, M, D, h, m, s;
          if (sscanf (dt, "%i/%i/%i", &Y, &M, &D) == 3 && Y >= 1900 && M >= 1 && M <= 12 && D >= 1 && D <= 31 && sscanf (tm, "%i:%i:%i", &h, &m, &s) == 3 && h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59) { // TO DO: we still do not catch all possible errors, for exmple 30.2.1966
            struct tm tm;
            tm.tm_year = Y - 1900; tm.tm_mon = M - 1; tm.tm_mday = D;
            tm.tm_hour = h; tm.tm_min = m; tm.tm_sec = s;
            time_t t = mktime (&tm); // time in local time
            if (t != -1) {
              setLocalTime (t);
              return __getDateTime__ ();          
            }
          }
          return F ("Wrong format of date/time specified.");
        }         
  
        String __ntpdate__ (char *ntpServer = NULL) {
          String s = ntpServer ? ntpDate (ntpServer) : ntpDate ();
          if (s != "") return s; // ntpDate reported an error
          return "Time synchronized, currrent time is " + timeToString (getLocalTime ()) + ".";
        }

      #endif

      #ifdef __DMESG__
  
        String __dmesg__ (bool follow, bool trueTime, telnetConnection *tcn) {
          #ifndef __TIME_FUNCTIONS__
            if (trueTime) return "-time option is not supported";
          #endif
          
          byte i = __dmesgBeginning__;
          {         
            // make a copy of all messages in circular queue in critical section
            String s ("");
            xSemaphoreTake (__dmesgSemaphore__, portMAX_DELAY);
              do {
                if (s != "") s+= "\r\n";
                char c [25];
                if (trueTime && __dmesgCircularQueue__ [i].time) {
                  #ifdef __TIME_FUNCTIONS__
                    struct tm st = timeToStructTime (timeToLocalTime (__dmesgCircularQueue__ [i].time));
                  #else
                    struct tm st = {};
                  #endif
                  strftime (c, sizeof (c), "[%y/%m/%d %H:%M:%S] ", &st);
                } else {
                  sprintf (c, "[%10lu] ", __dmesgCircularQueue__ [i].milliseconds);
                }
                s += String (c) + __dmesgCircularQueue__ [i].message;
              } while ((i = (i + 1) % DMESG_CIRCULAR_QUEUE_LENGTH) != __dmesgEnd__);
            xSemaphoreGive (__dmesgSemaphore__);
            // send everything to the client
            if (sendAll (tcn->getSocket (), (char *) s.c_str (), s.length (), __telnet_connection_time_out__) <= 0) return "";
          }

          // -follow?
          while (follow) {
            while (i == __dmesgEnd__) {
              if (tcn->readTelnetChar (true)) {
                tcn->readTelnetChar (false); // read pending character
                return ""; // return if user pressed Ctrl-C or any key
              } 
              if (tcn->connectionTimeOut ()) { sendAll (tcn->getSocket (), (char *) "\r\nTime-out", strlen ("\r\nTime-out"), 1000); tcn->closeConnection (); return ""; }
            }
            // __dmesgEnd__ has changed which means that at least one new message has been inserted into dmesg circular queue menawhile
            {
              String s ("");
              xSemaphoreTake (__dmesgSemaphore__, portMAX_DELAY);
                do {
                  s += "\r\n";
                  char c [25];
                  if (trueTime && __dmesgCircularQueue__ [i].time) {
                    #ifdef __TIME_FUNCTIONS__
                      struct tm st = timeToStructTime (timeToLocalTime (__dmesgCircularQueue__ [i].time));
                    #else
                      struct tm st = {};
                    #endif
                    strftime (c, sizeof (c), "[%y/%m/%d %H:%M:%S] ", &st);
                  } else {
                    sprintf (c, "[%10lu] ", __dmesgCircularQueue__ [i].milliseconds);
                  }
                  s += String (c) + __dmesgCircularQueue__ [i].message;
                } while ((i = (i + 1) % DMESG_CIRCULAR_QUEUE_LENGTH) != __dmesgEnd__);
              xSemaphoreGive (__dmesgSemaphore__);
              // send everything to the client
              if (sendAll (tcn->getSocket (), (char *) s.c_str (), s.length (), __telnet_connection_time_out__) <= 0) return "";
            }
          }
          return "";
        }      

      #endif

      struct __telnetSharedMemory__ {
        int socketTowardsServer;
        int socketTowardsClient;
        unsigned long time_out;
        bool threadTowardsServerFinished;
        bool threadTowardsClientFinished;
        bool clientError;
        bool clientTimeOut;
      };
      
      String __telnet__ (char *server, int port, telnetConnection *tcn) {
        // get server address
        struct hostent *he = gethostbyname (server);
        if (!he) {
          return "gethostbyname() error: " + String (h_errno);
        }
        // create socket
        int connectionSocket = socket (PF_INET, SOCK_STREAM, 0);
        if (connectionSocket == -1) {
          return "socket() error: " + String (errno);
        }
        
        // make the socket not-blocking so that time-out can be detected
        if (fcntl (connectionSocket, F_SETFL, O_NONBLOCK) == -1) {
          close (connectionSocket);
          return "fcntl() error: " + String (errno);
        }
        // connect to server
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons (port);
        serverAddress.sin_addr.s_addr = *(in_addr_t *) he->h_addr; 
        if (connect (connectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
          if (errno != EINPROGRESS) {
            close (connectionSocket);
            return "connect() error: " + String (errno);
          }
        } // it is likely that socket is not connected yet at this point
        // send information about IP used to connect to server back to client
        {
          char serverIP [14 + 46 + 4] = "Connecting to ";
          inet_ntoa_r (serverAddress.sin_addr, serverIP + 14, 46);
          strcat (serverIP, " ...");
          if (sendAll (__connectionSocket__, serverIP, strlen (serverIP), __telnet_connection_time_out__) <= 0) return "";
        }

        // we need 2 tasks, one for data transfer client -> ESP32 -> server and the other client <- ESP32 <- server
        // for client -> ESP32 -> server direction we are going to use current task  
        // for client <- ESP32 <- server direction we'll start a new one
        struct __telnetSharedMemory__ sharedMemory = {connectionSocket, __connectionSocket__, __telnet_connection_time_out__, false, false, false, false};
        #define tskNORMAL_PRIORITY 1
        if (pdPASS != xTaskCreate ( [] (void *param)  { // client <- ESP32 <- server data transfer
                                                        struct __telnetSharedMemory__ *sharedMemory = (struct __telnetSharedMemory__ *) param;
                                                        while (!sharedMemory->threadTowardsServerFinished) { // while the other task is running
                                                            char buffer [512];
                                                            int readTotal = recvAll (sharedMemory->socketTowardsServer, buffer, sizeof (buffer), NULL, sharedMemory->time_out);
                                                            if (readTotal <= 0) {
                                                                // error while reading data from server, we don't care if it is time-out or something else
                                                                break;
                                                            } else {
                                                              if (sendAll (sharedMemory->socketTowardsClient, buffer, readTotal, sharedMemory->time_out) <= 0) {
                                                                // error while writing data to client, we don't care if it is time-out or something else
                                                                break;
                                                              }
                                                            }
                                                            delay (1);
                                                            // DEBUG: Serial.printf ("[%10lu] [__telnet__ server->ESP32->client] %i bytes transfered\n", millis (), strlen (buffer));
                                                        }
                                                        sharedMemory->threadTowardsClientFinished = true; // signal the other task that this task has stopped
                                                        vTaskDelete (NULL);
                                                      }, 
                                    __func__, 
                                    4068, 
                                    &sharedMemory,
                                    tskNORMAL_PRIORITY,
                                    NULL)) {
          close (connectionSocket);
          return "xTaskCreate error";
        } 
                                                      { // client -> ESP32 -> server data transfer
                                                        while (!sharedMemory.threadTowardsClientFinished) { // while the other task is running
                                                            char buffer [512];
                                                            int readTotal = recvAll (sharedMemory.socketTowardsClient, buffer, sizeof (buffer), NULL, __telnet_connection_time_out__);
                                                            if (readTotal <= 0) {
                                                              // error while reading data from client
                                                              if (errno != EAGAIN && errno != ENAVAIL) sharedMemory.clientTimeOut = true;
                                                              else                                     sharedMemory.clientError = true;
                                                            } else {
                                                              if (sendAll (sharedMemory.socketTowardsServer, buffer, readTotal, __telnet_connection_time_out__) <= 0) {
                                                                // error while writing data to server, we don't care if it is time-out or something else
                                                                break;
                                                              }
                                                            }
                                                            delay (1);
                                                            // DEBUG: Serial.printf ("[%10lu] [__telnet__ server<-ESP32<-client] %i bytes transfered\n", millis (), strlen (buffer));
                                                        }
                                                        sharedMemory.threadTowardsServerFinished = true; // signal the other task that this task has stopped
                                                        // wait the other task to finish
                                                        while (!sharedMemory.threadTowardsClientFinished) delay (1);
                                                      }

        close (connectionSocket);
        // DEBUG: Serial.printf ("[%10lu] [__telnet__] disconnected from %s\n", millis (), server);
        if (sharedMemory.clientTimeOut) { tcn->closeConnection (); return ""; }
        if (sharedMemory.clientError)   { sendAll (tcn->getSocket (), (char *) "\r\nTime-out", strlen ("\r\nTime-out"), 1000); tcn->closeConnection (); return ""; }
        // tell the client to go into character mode, not to echo and send its window size, just in case the other server has changed that
        return IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS "\r\nConnection to host lost.";
      }

      #ifdef __HTTP_CLIENT__

        String __curl__ (char *method, char *url) {
          char server [65];
          char addr [128] = "/";
          int port = 80;
          if (sscanf (url, "http://%64[^:]:%i/%126s", server, &port, addr + 1) < 2) { // we haven't got at least server, port and the default address
            if (sscanf (url, "http://%64[^/]/%126s", server, addr + 1) < 1) { // we haven't got at least server and the default address  
              return F ("Wrong url, use form of http://server/address or http://server:port/address");
            }
          }
          if (port <= 0) return F ("Wrong port number, it should be > 0");
          return httpClient (server, port, addr, method);
        }

      #endif

      #ifdef __FILE_SYSTEM__        

        #define telnetUserHasRightToAccess(fullPath) (strstr(fullPath,__homeDir__)==fullPath) // user has a right to access file or directory if it begins with user's home directory

        String __mkfs__ (telnetConnection *tcn) {
          if (sendAll (__connectionSocket__, (char *) "formatting file system, please wait ... ", sizeof ("formatting file system, please wait ... "), __telnet_connection_time_out__) <= 0) return ""; 
          fileSystem.end ();
          if (fileSystem.format ()) {
                                    return F ("formatted.");
            if (fileSystem.begin (false)) return F ("\r\nFile system mounted,\r\nreboot now to create default configuration files\r\nor you can create them yorself before rebooting.");
            else                    return F ("formatted but failed to mount the file system."); 
          } else                    return F ("failed.");
          return "";
        }
  
        String __fs_info__ () {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          char output [500];
          #if FILE_SYSTEM == FILE_SYSTEM_FAT
            sprintf (output, "FAT file system info.\r\n"
                             "Total space:      %10i K bytes\r\n"
                             "Total space used: %10i K bytes\r\n"
                             "Free space:       %10i K bytes\r\n"
                             "Max path length:  %10i",
                             fileSystem.totalBytes () / 1024, 
                             (fileSystem.totalBytes () - fileSystem.freeBytes ()) / 1024, 
                             fileSystem.freeBytes () / 1024,
                             FILE_PATH_MAX_LENGTH
                  );
          #endif
          #if FILE_SYSTEM == FILE_SYSTEM_LITTLEFS
            sprintf (output, "LittleFS file system info.\r\n"
                             "Total space:      %10i K bytes\r\n"
                             "Total space used: %10i K bytes\r\n"
                             "Free space:       %10i K bytes\r\n"
                             "Max path length:  %10i",
                             fileSystem.totalBytes () / 1024, 
                             fileSystem.usedBytes () / 1024, 
                             (fileSystem.totalBytes () - fileSystem.usedBytes ()) / 1024, 
                             FILE_PATH_MAX_LENGTH
                  );
          #endif
          return String (output);
        } 
  
        String __ls__ (char *directory) {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (directory, __workingDir__);
          if (fp == "" || !isDirectory (fp))             return F ("Invalid directory name.");
          if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access denyed.");
          return listDirectory (fp);
        }
  
        String __tree__ (char *directory) {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (directory, __workingDir__);
          if (fp == "" || !isDirectory (fp))             return F ("Invalid directory name.");
          if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access denyed.");
          return recursiveListDirectory (fp);
        }

        String __catFileToClient__ (char *fileName, telnetConnection *tcn) {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (fileName, __workingDir__);
          if (fp == "" || !isFile (fp))                  return F ("Invalid file name.");
          if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access denyed.");

          #define CAT_TO_CLIENT_BUFF_SIZE TCP_SND_BUF // TCP_SND_BUF = 5744, a maximum block size that ESP32 can send 
          // char *buff = (char *) malloc (CAT_TO_CLIENT_BUFF_SIZE); 
          // if (!buff) return F ("Out of memory");
          // *buff = 0;
          char buff [CAT_TO_CLIENT_BUFF_SIZE]; // as long as we have stack memory there's no need for allocating aditional memory from heap
  
          File f;
          if ((bool) (f = fileSystem.open (fp, FILE_READ))) {
            if (!f.isDirectory ()) {
              int i = strlen (buff);
              while (f.available ()) {
                switch (*(buff + i) = f.read ()) {
                  case '\r':  // ignore
                              break;
                  case '\n':  // LF-CRLF conversion
                              *(buff + i ++) = '\r'; 
                              *(buff + i ++) = '\n';
                              break;
                  default:
                              i ++;                  
                }
                if (i >= sizeof (buff) - 2) { 
                                          #ifdef __PERFMON__
                                            __perfFSBytesRead__ += i; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                                          #endif
                                          if (sendAll (__connectionSocket__, buff, i, __telnet_connection_time_out__) <= 0) { f.close (); /* free (buff); */ return ""; }
                                          i = 0; 
                                        }
              }
              if (i)                    { 
                                          #ifdef __PERFMON__
                                            __perfFSBytesRead__ += i; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                                          #endif
                                          if (sendAll (__connectionSocket__, buff, i, __telnet_connection_time_out__) <= 0) { f.close (); /* free (buff); */ return ""; }
                                       }
            } else {
              f.close (); /* free (buff); */ return "Can't read " + fp;
            }
            f.close ();
          } else {
            /* free (buff); */ return "Can't read " + fp;
          }
          /* free (buff); */ return "";
        }

        String __catClientToFile__ (char *fileName, telnetConnection *tcn) {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (fileName, __workingDir__);
          if (fp == "" || isDirectory (fp))              return F ("Invalid file name.");
          if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access denyed.");

          File f;
          if ((bool) (f = fileSystem.open (fp, FILE_WRITE))) {
            while (char c = readTelnetChar ()) { 
              switch (c) {
                case 0: // Error
                case 3: // Ctrl-C
                        f.close (); return fp + " not fully written.";
                case 4: // Ctrl-D or Ctrl-Z
                        f.close (); return "\r\n" + fp + " written.";
                case 13: // ignore
                        break;
                case 10: // LF -> CRLF conversion
                        if (f.write ((uint8_t *) "\r\n", 2) != 2) { 
                          f.close (); return "Can't write " + fp;
                        } 
                        #ifdef __PERFMON__
                          __perfFSBytesWritten__ += 2; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                        #endif
                        // echo
                        if (sendAll (__connectionSocket__, (char *) "\r\n", 2, __telnet_connection_time_out__) <= 0) return "";
                        break;
                default: // character 
                        if (f.write ((uint8_t *) &c, 1) != 1) { 
                          f.close (); return "Can't write " + fp;
                        } 
                        #ifdef __PERFMON__
                          __perfFSBytesWritten__ += 1; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                        #endif
                        // echo
                        if (sendAll (__connectionSocket__, &c, 1, __telnet_connection_time_out__) <= 0) return "";
                        break;
              }
            }
            f.close ();
          } else {
            return "Can't write " + fp;
          }
          return "";
        }

        String __tail__ (char *fileName, bool follow, telnetConnection *tcn) {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (fileName, __workingDir__);
          if (fp == "" || !isFile (fp))                  return F ("Invalid file name.");
          if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access denyed.");

          File f;
          size_t filePosition = 0;
          do {
            if ((bool) (f = fileSystem.open (fp, FILE_READ))) {
              // if this is the first time skip to (almost) the end of file
              if (filePosition == 0) {
                size_t fileSize = f.size ();
                // move to 512 bytes before the end of the file
                f.seek (fileSize > 512 ? fileSize - 512 : 0);
                // move past next \n
                while (f.available () && f.read () != '\n') {
                  #ifdef __PERFMON__
                    __perfFSBytesRead__ += 1; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                  #endif
                  ;
                }
                filePosition = f.position ();
              } else {
                f.seek (filePosition);
              }
              Serial.printf ("filePosition = %i, fileSize = %i\n", filePosition, f.size ()); 
              if (f.size () > filePosition) {
                // display the rest of the file
                while (f.available ()) {
                  char c;
                  switch ((c = f.read ())) {
                    case '\r':  // ignore
                                break;
                    case '\n':  // LF-CRLF conversion
                                if (sendAll (__connectionSocket__, (char *) "\r\n", 2, __telnet_connection_time_out__) <= 0) { f.close (); return ""; }
                                break;
                    default:
                                if (sendAll (__connectionSocket__, &c, 1, __telnet_connection_time_out__) <= 0) { f.close (); return ""; }
                  }
                  #ifdef __PERFMON__
                    __perfFSBytesRead__ += 1; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                  #endif                
                }
              }
              filePosition = f.position (); 
              Serial.printf ("filePosition = %i\n", filePosition);
              f.close ();
            } else {
              return "Can't read " + fp;
            }
            // follow?
            if (follow) {
              unsigned long startMillis = millis ();
              while (millis () - startMillis < 1000) {
                if (tcn->readTelnetChar (true)) {
                  tcn->readTelnetChar (false); // read pending character
                  f.close ();
                  return ""; // return if user pressed Ctrl-C or any key
                }
                if (tcn->connectionTimeOut ()) { sendAll (tcn->getSocket (), (char *) "\r\nTime-out", strlen ("\r\nTime-out"), 1000); tcn->closeConnection (); return ""; } 
                delay (100);
              }
            } else {
              return "";
            }
          } while (true);
          return "";
        }

        String __mkdir__ (char *directoryName) { 
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (directoryName, __workingDir__);
          if (fp == "")                                   return F ("Invalid directory name.");
          if (!telnetUserHasRightToAccess (fp.c_str ()))  return F ("Access denyed.");
  
          if (makeDir (fp))                               return fp + " made.";
                                                          return "Can't make " + fp;
        }
  
        String __rmdir__ (char *directoryName) { 
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (directoryName, __workingDir__);
          if (fp == "" || !isDirectory (fp))              return F ("Invalid directory name.");
          if (!telnetUserHasRightToAccess (fp.c_str ()))  return F ("Access denyed.");
          if (fp == __homeDir__)                          return F ("You can't remove your home directory.");
          if (fp == __workingDir__)                       return F ("You can't remove your working directory.");
  
          if (removeDir (fp))                             return fp + " removed.";
                                                          return "Can't remove " + fp;
        }      

        String __cd__ (char *directoryName) { 
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (directoryName, __workingDir__);
          if (fp == "" || !isDirectory (fp))              return F ("Invalid directory name.");
          if (!telnetUserHasRightToAccess (fp.c_str ()))  return F ("Access denyed.");
  
          strcpy (__workingDir__, fp.c_str ());           return "Your working directory is " + fp;
        }

        String __pwd__ () { 
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          // remove extra /
          String s (__workingDir__);
          if (s.charAt (s.length () - 1) == '/') s = s.substring (0, s.length () - 1); 
          if (s == "") s = "/"; 
          return "Your working directory is " + s;
        }

        String __mv__ (char *srcFileOrDirectory, char *dstFileOrDirectory) { 
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp1 = fullFilePath (srcFileOrDirectory, __workingDir__);
          if (fp1 == "")                                  return F ("Invalid source file or directory name.");
          if (!telnetUserHasRightToAccess (fp1.c_str ())) return F ("Access to source file or directory denyed.");
          String fp2 = fullFilePath (dstFileOrDirectory, __workingDir__);
          if (fp2 == "")                                  return F ("Invalid destination file or directory name.");
          if (!telnetUserHasRightToAccess (fp1.c_str ())) return F ("Access destination file or directory denyed.");
  
          if (fileSystem.rename (fp1, fp2))                     return "Renamed to " + fp2;
                                                          return "Can't rename " + fp1;
        }

        String __cp__ (char *srcFileName, char *dstFileName) { 
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp1 = fullFilePath (srcFileName, __workingDir__);
          if (fp1 == "")                                  return F ("Invalid source file name.");
          if (!telnetUserHasRightToAccess (fp1.c_str ())) return F ("Access to source file denyed.");
          String fp2 = fullFilePath (dstFileName, __workingDir__);
          if (fp2 == "")                                  return F ("Invalid destination file name.");
          if (!telnetUserHasRightToAccess (fp1.c_str ())) return F ("Access destination file denyed.");

          File f1, f2;
          String retVal = "File copied.";
          
          if (!(bool) (f1 = fileSystem.open (fp1, FILE_READ))) { return "Can't read " + fp1; }
          if (f1.isDirectory ()) { f1.close (); return  "Can't read " + fp1; }
          if (!(bool) (f2 = fileSystem.open (fp2, FILE_WRITE))) { f1.close (); return "Can't write " + fp2; }

          int bytesReadTotal = 0;
          int bytesWrittenTotal = 0;
          #define CP_BUFF_SIZE 4 * 1024
          char buff [CP_BUFF_SIZE];
          do {
            int bytesReadThisTime = f1.read ((uint8_t *) buff, sizeof (buff));
            if (bytesReadThisTime == 0) { break; } // finished, success
            bytesReadTotal += bytesReadThisTime;
            #ifdef __PERFMON__
              __perfFSBytesRead__ += bytesReadThisTime; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
            #endif
            int bytesWrittenThisTime = f2.write ((uint8_t *) buff, bytesReadThisTime);
            bytesWrittenTotal += bytesWrittenThisTime;
            #ifdef __PERFMON__
              __perfFSBytesWritten__ += bytesWrittenThisTime; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
            #endif                                                
            if (bytesWrittenThisTime != bytesReadThisTime) { retVal = "Can't write " + fp2; break; } 
          } while (true);
          
          f2.close ();
          f1.close ();
          return retVal;
        }

        String __rm__ (char *fileName) {
          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (fileName, __workingDir__);
          if (fp == "" || !isFile (fp))                   return F ("Invalid file name.");
          if (!telnetUserHasRightToAccess (fp.c_str ()))  return F ("Access denyed.");

          if (deleteFile (fp))                            return fp + " deleted.";
          else                                            return "Can't delete " + fp;
        }


        // not really a vi but small and simple text editor
  
        String __vi__ (char *fileName, telnetConnection *tcn) {
          // a good reference for telnet ESC codes: https://en.wikipedia.org/wiki/ANSI_escape_code
          // and: https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
          if (!__fileSystemMounted__) return "File system not mounted. You may have to format flash disk first.";

          if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
          String fp = fullFilePath (fileName, __workingDir__);
          if (fp == "")                                   return F ("Invalid file name.");
          if (!telnetUserHasRightToAccess (fp.c_str ()))  return F ("Access denyed.");
  
          // 1. create a new file one if it doesn't exist (yet)
          if (!isFile (fp)) {
            File f = fileSystem.open (fp, FILE_WRITE);
            if (f) { f.close (); if (sendAll (__connectionSocket__, (char *) "\r\nFile created.", strlen ("\r\nFile created."), __telnet_connection_time_out__) <= 0) return ""; }
            else return "Can't create " + fp;
          }
  
          // 2. read the file content into internal vi data structure (lines of Strings)
          #define MAX_LINES 600 // may increase up to < 998 but this would require even more stack for telnet server and it is slow
          String line [MAX_LINES]; for (int i = 0; i < MAX_LINES; i++) line [i] = "";
          int fileLines = 0;
          bool dirty = false;
          unsigned long telnet_connection_time_out = __telnet_connection_time_out__;
          {
            File f = fileSystem.open (fp, FILE_READ);
            if (f) {
              if (!f.isDirectory ()) {
                while (f.available ()) { 
                  if (ESP.getFreeHeap () < 32768) { // always leave at least 32 KB free for other things that may be running on ESP32
                    f.close (); 
                    return "Out of memory."; 
                  }
                  char c = (char) f.read ();
                  #ifdef __PERFMON__
                    __perfFSBytesRead__ += 1; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                  #endif 
                  switch (c) {
                    case '\r':  break; // ignore
                    case '\n':  if (++fileLines >= MAX_LINES) { 
                                  f.close (); 
                                  return fp + " has too many lines for this simple text editor."; 
                                }
                                break; 
                    case '\t':  line [fileLines] += "  "; break; // treat tab as 2 spaces
                    default:    line [fileLines] += c; break;
                  }
                }
                f.close ();
              } else return "Can't edit a directory.";
              f.close ();
            } else return "Can't read " + fp;
            fileLines++;
          }    
  
          // 3. discard any already pending characters from client
          while (readTelnetChar (true)) readTelnetChar (false);
  
          // 4. get information about client window size
          if (__clientWindowWidth__) { // we know that client reports its window size, ask for latest information (the user might have resized the window since beginning of telnet session)
            if (sendAll (__connectionSocket__, (char *) IAC DO NAWS, strlen (IAC DO NAWS), __telnet_connection_time_out__) <= 0) return "";
            // client will reply in the form of: IAC (255) SB (250) NAWS (31) col1 col2 row1 row1 IAC (255) SE (240) but this will be handeled internally by readTelnet function
          } else { // just assume the defaults and hope that the result will be OK
            __clientWindowWidth__ = 80; 
            __clientWindowHeight__ = 24;   
          }
          readTelnetChar (true);
          if (__clientWindowWidth__ < 30 || __clientWindowHeight__ < 5) return "Clinent telnet windows is too small for vi.";
  
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
                                if (sendAll (__connectionSocket__, (char *) "\x1b[2J", strlen ("\x1b[2J"), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // ESC[2J = clear screen
  
          while (true) {
            // a. redraw screen 
            String s = ""; 
         
            if (redrawHeader)   { 
                                  s = "---+"; for (int i = 4; i < __clientWindowWidth__; i++) s += '-'; 
                                  String tmp = "\x1b[H" + s.substring (0, __clientWindowWidth__ - 28) + " Save: Ctrl-S, Exit: Ctrl-X";
                                  if (sendAll (__connectionSocket__, (char *) tmp.c_str (), tmp.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // ESC[H = move cursor home
                                  redrawHeader = false;
                                }
            if (redrawAllLines) {
  
                                  // Redrawing algorithm: straight forward idea is to scan screen lines with text i ∈ [2 .. __clientWindowHeight__ - 1], calculate text line on
                                  // this possition: i - 2 + textScrollY and draw visible part of it: line [...].substring (textScrollX, __clientWindowWidth__ - 4 + textScrollX), __clientWindowWidth__ - 4).
                                  // When there are frequent redraws the user experience is not so good since we often do not have enough time to redraw the whole screen
                                  // between two key strokes. Therefore we'll always redraw just the line at cursor position: textCursorY + 2 - textScrollY and then
                                  // alternate around this line until we finish or there is another key waiting to be read - this would interrupt the algorithm and
                                  // redrawing will repeat after ne next character is processed.
                                  {
                                    int nextScreenLine = textCursorY + 2 - textScrollY;
                                    int nextTextLine = nextScreenLine - 2 + textScrollY;
                                    bool topReached = false;
                                    bool bottomReached = false;
                                    for (int i = 0; (!topReached || !bottomReached) && !(readTelnetChar (true)) ; i++) { // if user presses any key redrawing stops (bettwr user experienca while scrolling)
                                      if (i % 2 == 0) { nextScreenLine -= i; nextTextLine -= i; } else { nextScreenLine += i; nextTextLine += i; }
                                      if (nextScreenLine == 2) topReached = true;
                                      if (nextScreenLine == __clientWindowHeight__ - 1) bottomReached = true;
                                      if (nextScreenLine > 1 && nextScreenLine < __clientWindowHeight__) {
                                        // draw nextTextLine at nextScreenLine 
                                        // debug: Serial.printf ("[telnet vi debug] display text line %i at screen position %i\n", nextTextLine + 1, nextScreenLine);
                                        char c [15];
                                        if (nextTextLine < fileLines) {
                                          sprintf (c, "\x1b[%i;0H%3i|", nextScreenLine, nextTextLine + 1);  // display line number - users would count lines from 1 on, whereas program counts them from 0 on
                                          s = String (c) + __pad__ (line [nextTextLine].substring (textScrollX, __clientWindowWidth__ - 4 + textScrollX), __clientWindowWidth__ - 4); // ESC[line;columnH = move cursor to line;column, it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen)
                                        } else {
                                          sprintf (c, "\x1b[%i;0H   |", nextScreenLine);
                                          s = String (c)  + __pad__ (" ", __clientWindowWidth__ - 4); // ESC[line;columnH = move cursor to line;column, it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen);
                                        }
                                        if (sendAll (__connectionSocket__, (char *) s.c_str (), s.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
                                      } 
                                    }
                                    if (topReached && bottomReached) redrawAllLines = false; // if we have drown all the lines we don't have to run this code again 
                                  }
                                  
            } else if (redrawLineAtCursor) {
                                  // calculate screen line from text cursor position
                                  s = "\x1b[" + String (textCursorY + 2 - textScrollY) + ";5H" + __pad__ (line [textCursorY].substring (textScrollX, __clientWindowWidth__ - 4 + textScrollX), __clientWindowWidth__ - 4); // ESC[line;columnH = move cursor to line;column (columns go from 1 to clientWindowsColumns - inclusive), it is much faster to append line with spaces then sending \x1b[0J (delte from cursor to the end of screen)
                                  if (sendAll (__connectionSocket__, (char *) s.c_str (), s.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } 
                                  redrawLineAtCursor = false;
                                }

            if (redrawFooter)   {                                  
                                  s = "\x1b[" + String (__clientWindowHeight__) + ";0H---+"; for (int i = 4; i < __clientWindowWidth__; i++) s += '-'; // ESC[line;columnH = move cursor to line;column
                                  if (sendAll (__connectionSocket__, (char *) s.c_str (), s.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } 
                                  redrawFooter = false;
                                }
            if (message != "")  {
                                  String tmp = "\x1b[" + String (__clientWindowHeight__) + ";2H" + message;
                                  if (sendAll (__connectionSocket__, (char *) tmp.c_str (), tmp.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }        
                                  message = ""; redrawFooter = true; // we'll clear the message the next time screen redraws
                                }
            
            // b. restore cursor position - calculate screen coordinates from text coordinates
            {
              String tmp = "\x1b[" + String (textCursorY - textScrollY + 2) + ";" + String (textCursorX - textScrollX + 5) + "H";
              if (sendAll (__connectionSocket__, (char *) tmp.c_str (), tmp.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // ESC[line;columnH = move cursor to line;column
            }
  
            // c. read and process incoming stream of characters
            char c = 0;
            delay (1);
            if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
            // DEBUG: Serial.printf ("1. %c=%i\n", c, c);
            switch (c) {
              case 24:  // Ctrl-X
                        if (dirty) {
                          String tmp = "\x1b[" + String (__clientWindowHeight__) + ";2H Save changes (y/n)? ";
                          if (sendAll (__connectionSocket__, (char *) tmp.c_str (), tmp.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
                          redrawFooter = true; // overwrite this question at next redraw
                          while (true) {                                                     
                            if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
                            if (c == 'y') goto saveChanges;
                            if (c == 'n') break;
                          }
                        } 
                        {
                          String tmp = "\x1b[" + String (__clientWindowHeight__) + ";2H Share and Enjoy ----\r\n";
                          if (sendAll (__connectionSocket__, (char *) tmp.c_str (), tmp.length (), __telnet_connection_time_out__) <= 0) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
                        }                        
                        { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
              case 19:  // Ctrl-S
        saveChanges:
                        // save changes to fp
                        {
                          bool e = false;
                          File f = fileSystem.open (fp, FILE_WRITE);
                          if (f) {
                            if (!f.isDirectory ()) {
                              for (int i = 0; i < fileLines; i++) {
                                int toWrite = strlen (line [i].c_str ()); if (i) toWrite += 2;
                                if (toWrite != f.write (i ? (uint8_t *) ("\r\n" + line [i]).c_str (): (uint8_t *) line [i].c_str (), toWrite)) { e = true; break; }
                                #ifdef __PERFMON__
                                  __perfFSBytesWritten__ += toWrite; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                                #endif
                              }
                            }
                            f.close ();
                          }
                          if (e) { message = " Could't save changes "; } else { message = " Changes saved "; dirty = false; __telnet_connection_time_out__ = telnet_connection_time_out; }
                        }
                        break;
              case 27:  // ESC [A = up arrow, ESC [B = down arrow, ESC[C = right arrow, ESC[D = left arrow, 
                        if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
                        // DEBUG: Serial.printf ("2.  %c=%i\n", c, c);
                        switch (c) {
                          case '[': // ESC [
                                    if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
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
                                                if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // read final '~'
                                                textCursorX = 0;
                                                break;
                                      case '4': // ESC [ 4 = end
                                                if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // read final '~'
                                                textCursorX = line [textCursorY].length ();
                                                break;
                                      case '5': // ESC [ 5 = pgup
                                                if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // read final '~'
                                                textCursorY -= (__clientWindowHeight__ - 2); if (textCursorY < 0) textCursorY = 0;
                                                if (textCursorX > line [textCursorY].length ()) textCursorX = line [textCursorY].length ();
                                                break;
                                      case '6': // ESC [ 6 = pgdn
                                                if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; } // read final '~'
                                                textCursorY += (__clientWindowHeight__ - 2); if (textCursorY >= fileLines) textCursorY = fileLines - 1;
                                                if (textCursorX > line [textCursorY].length ()) textCursorX = line [textCursorY].length ();
                                                break;  
                                      case '3': // ESC [ 3 
                                                if (!(c = readTelnetChar ())) { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
                                                switch (c) {
                                                  case '~': // ESC [ 3 ~ (126) - putty reports del key as ESC [ 3 ~ (126), since it also report backspace key as del key let' treat del key as backspace                                                                 
                                                            goto backspace;
                                                  default:  // ignore
                                                            // DEBUG: Serial.printf ("ESC [ 3 %c (%i)\n", c, c);
                                                            break;
                                                }
                                                break;
                                      default:  // ignore
                                                // DEBUG: Serial.printf ("ESC [ %c (%i)\n", c, c);
                                                break;
                                    }
                                    break;
                           default: // ignore
                                    // DEBUG: Serial.printf ("ESC %c (%i)\n", c, c);
                                    break;
                        }
                        break;
              case 8:   // Windows telnet.exe: back-space, putty does not report this code
        backspace:
                        if (textCursorX > 0) { // delete one character left of cursor position
                          line [textCursorY] = line [textCursorY].substring (0, textCursorX - 1) + line [textCursorY].substring (textCursorX);
                          dirty = redrawLineAtCursor = true; // we only have to redraw this line
                          __telnet_connection_time_out__ = 0; // infinite
                          textCursorX--;
                        } else if (textCursorY > 0) { // combine 2 lines
                          textCursorY--;
                          textCursorX = line [textCursorY].length (); 
                          line [textCursorY] += line [textCursorY + 1];
                          for (int i = textCursorY + 1; i <= fileLines - 2; i++) line [i] = line [i + 1]; line [--fileLines] = ""; // shift lines up and free memory if the last one used
                          dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)
                          __telnet_connection_time_out__ = 0; // infinite
                        }
                        break; 
              case 127: // Windows telnet.exe: delete, putty: backspace
                        if (textCursorX < line [textCursorY].length ()) { // delete one character at cursor position
                          line [textCursorY] = line [textCursorY].substring (0, textCursorX) + line [textCursorY].substring (textCursorX + 1);
                          dirty = redrawLineAtCursor = true; // we only need to redraw this line
                          __telnet_connection_time_out__ = 0; // infinite
                        } else { // combine 2 lines
                          if (textCursorY < fileLines - 1) {
                            line [textCursorY] += line [textCursorY + 1];
                            for (int i = textCursorY + 1; i < fileLines - 1; i++) line [i] = line [i + 1]; line [--fileLines] = ""; // shift lines up and free memory if the last one used
                            dirty = redrawAllLines = true; // we need to redraw all visible lines (at least lines from testCursorY down but we wont write special cede for this case)                          
                            __telnet_connection_time_out__ = 0; // infinite
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
                          __telnet_connection_time_out__ = 0; // infinite
                          textCursorY++;
                        }
                        break;
              case '\r':  // ignore
                        break;
              default:  // normal character
                        if (c == '\t') s = "    "; else s = String (c); // treat tab as 4 spaces
                        line [textCursorY] = line [textCursorY].substring (0, textCursorX) + s + line [textCursorY].substring (textCursorX); // inser character into line textCurrorY at textCursorX position
                        dirty = redrawLineAtCursor = true; // we only have to redraw this line
                        __telnet_connection_time_out__ = 0; // infinite
                        textCursorX += s.length ();
                        break;
            }         
            // if cursor has moved - should we scroll?
            {
              if (textCursorX - textScrollX >= __clientWindowWidth__ - 4) {
                textScrollX = textCursorX - (__clientWindowWidth__ - 4) + 1; // scroll left if the cursor fell out of right client window border
                redrawAllLines = true; // we need to redraw all visible lines
              }
              if (textCursorX - textScrollX < 0) {
                textScrollX = textCursorX; // scroll right if the cursor fell out of left client window border
                redrawAllLines = true; // we need to redraw all visible lines
              }
              if (textCursorY - textScrollY >= __clientWindowHeight__ - 2) {
                textScrollY = textCursorY - (__clientWindowHeight__ - 2) + 1; // scroll up if the cursor fell out of bottom client window border
                redrawAllLines = true; // we need to redraw all visible lines
              }
              if (textCursorY - textScrollY < 0) {
                textScrollY = textCursorY; // scroll down if the cursor fell out of top client window border
                redrawAllLines = true; // we need to redraw all visible lines
              }
            }
          }
          { __telnet_connection_time_out__ = telnet_connection_time_out; return ""; }
        }
          
        #ifdef __FTP_CLIENT__
          String __ftpPut__ (char *localFileName, char *remoteFileName, char *password, char *userName, int ftpPort, char *ftpServer) {
            if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
            String fp = fullFilePath (localFileName, __workingDir__);
            if (fp == "" || isDirectory (fp))              return F ("Invalid local file name.");
            if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access to local file denyed.");
            return ftpPut ((char *) fp.c_str (), remoteFileName, password, userName, ftpPort, ftpServer);
          }
  
          String __ftpGet__ (char *localFileName, char *remoteFileName, char *password, char *userName, int ftpPort, char *ftpServer) {
            if (!__fileSystemMounted__) return F ("File system not mounted. You may have to format flash disk first.");
            String fp = fullFilePath (localFileName, __workingDir__);
            if (fp == "" || isDirectory (fp))              return F ("Invalid local file name.");
            if (!telnetUserHasRightToAccess (fp.c_str ())) return F ("Access to local file denyed.");
            return ftpGet ((char *) fp.c_str (), remoteFileName, password, userName, ftpPort, ftpServer);
          }
        #endif

      #endif
      
      #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT      

       char *__passwd__ (char *userName) {
          char password1 [USER_PASSWORD_MAX_LENGTH + 1];
          char password2 [USER_PASSWORD_MAX_LENGTH + 1];
                    
          if (!strcmp (__userName__, userName)) { // user changing password for himself
            // read current password
            __echo__ = false;
            if (sendAll (__connectionSocket__, (char *) "Enter current password: ", sizeof ("Enter current password: "), __telnet_connection_time_out__) <= 0) return (char *) ""; 
            if (readTelnetLine (password1, sizeof (password1), true) != 10) { __echo__ = true; return (char *) "\r\nPassword not changed."; }
            __echo__ = true;
            // check if password is valid for user
            if (!checkUserNameAndPassword (userName, password1))                               return (char *) "Wrong password."; 
          } else {                         // root is changing password for another user
            // check if user exists with getUserHomeDirectory
            char homeDirectory [FILE_PATH_MAX_LENGTH + 1];
            if (!getUserHomeDirectory (homeDirectory, sizeof (homeDirectory), userName))        return (char *) "User name does not exist."; 
          }
          // read new password twice
          __echo__ = false;
          if (sendAll (__connectionSocket__, (char *) "\r\nEnter new password: ", sizeof ("\r\nEnter new password: "), __telnet_connection_time_out__) <= 0) return (char *) ""; 
          if (readTelnetLine (password1, sizeof (password1), true) != 10)   { __echo__ = true; return (char *) "\r\nPassword not changed."; }
          if (sendAll (__connectionSocket__, (char *) "\r\nRe-enter new password: ", sizeof ("\r\nRe-enter new password: "), __telnet_connection_time_out__) <= 0) return (char *) ""; 
          if (readTelnetLine (password2, sizeof (password2), true) != 10)   { __echo__ = true; return (char *) "\r\nPassword not changed."; }
          __echo__ = true;
          // check passwords
          if (strcmp (password1, password2))                                                   return (char *) "\r\nPasswords do not match.";
          // change password
          if (passwd (userName, password1))                                                    return (char *) "\r\nPassword changed.";
          else                                                                                 return (char *) "\r\nError changing password.";  
        }

      #endif

      #ifdef __PERFMON__
  
        char *__perfmon__ (bool follow, bool resetCounters, telnetConnection *tcn) {
          bool firstTime = follow;
          do { // follow         
            char s [1024];
            sprintf (s, "%s" // ESC sequence
                        "Performnce counter\r\n"
                        "--------------------------------------------\r\n"
                        "__perfFSBytesRead__:                    %8lu\r\n"  // file system performance counters
                        "__perfFSBytesWritten__:                 %8lu\r\n"
                        "\r\n"                        
                        "__perfWiFiBytesReceived__:              %8lu\r\n"  // WiFi performance counters
                        "__perfWiFiBytesSent__:                  %8lu\r\n"
                        "\r\n"
                        "__perfOpenedHttpConnections__:          %8lu\r\n"  // HTTP performance counters
                        "__perfHttpRequests__:                   %8lu\r\n"
                        "__perOpenedfWebSockets__:               %8lu\r\n"
                        "__perfConcurrentHttpConnections__:      %8lu\r\n"
                        "__perfConcurrentWebSockets__:           %8lu\r\n"
                        "\r\n"
                        "__perfOpenedTelnetConnections__:        %8lu\r\n"  // Telnet performance counters
                        "__perfConcurrentTelnetConnections__:    %8lu\r\n\r\n"
                        "__perfConcurrentOscWebSockets__:        %8lu\r\n"      // oscilloscope performance counters
                        "\r\n"
                        "__perfOpenedFtpControlConnections__:    %8lu\r\n"  // FTP performance counters
                        "__perfConcurrentFtpControlConnections__:%8lu"
                        , firstTime ? "\x1b[2J" : follow ? "\x1b[0;0H" : "" // cleaar screen or move cursor to 0, 0
                        , __perfFSBytesRead__
                        , __perfFSBytesWritten__
                        , __perfWiFiBytesReceived__
                        , __perfWiFiBytesSent__
                        , __perfOpenedHttpConnections__
                        , __perfHttpRequests__
                        , __perOpenedfWebSockets__
                        , __perfConcurrentHttpConnections__
                        , __perfConcurrentWebSockets__
                        , __perfOpenedTelnetConnections__
                        , __perfConcurrentTelnetConnections__
                        , __perfConcurrentOscWebSockets__
                        ,__perfOpenedFtpControlConnections__
                        ,__perfConcurrentFtpControlConnections__  
                        );
            firstTime = false;
            if (resetCounters) {
              __perfFSBytesRead__ = 0;
              __perfFSBytesWritten__ = 0;
              __perfWiFiBytesReceived__ = 0;
              __perfWiFiBytesSent__ = 0;
              __perfOpenedHttpConnections__ = 0;
              __perfHttpRequests__ = 0;
              __perOpenedfWebSockets__ = 0;
              // __perfConcurrentHttpConnections__
              // __perfConcurrentWebSockets__
              __perfOpenedTelnetConnections__ = 0;
              // __perfConcurrentTelnetConnections__
              // __perfConcurrentOscWebSockets__
            }
            // send everything to the client
            if (sendAll (tcn->getSocket (), s, strlen (s), __telnet_connection_time_out__) <= 0) return (char *) "";

            if (follow) { // wait and check if user pressed a key
              unsigned long startMillis = millis ();
              while (millis () - startMillis < (1000)) {
                delay (100);
                if (tcn->readTelnetChar (true)) {
                  tcn->readTelnetChar (false); // read pending character
                  return (char *) ""; // return if user pressed Ctrl-C or any key
                } 
                if (tcn->connectionTimeOut ()) { sendAll (tcn->getSocket (), (char *) "\r\nTime-out", strlen ("\r\nTime-out"), 1000); tcn->closeConnection (); return (char *) ""; }
              }              
            }
          } while (follow);
          return (char *) "";
        }      

      #endif
        
    };


    // ----- telnetServer class -----

    class telnetServer {                                             
    
      public:

        // telnetServer state
        enum STATE_TYPE {
          NOT_RUNNING = 0, 
          STARTING = 1, 
          RUNNING = 2        
        };
        
        STATE_TYPE state () { return __state__; }
    
        telnetServer ( // the following parameters will be pased to each telnetConnection instance
                     String (* telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection *tcn), // telnetCommandHandler callback function provided by calling program
                     // the following parameters will be handeled by telnetServer instance
                     char *serverIP,                                                                // Telnet server IP address, 0.0.0.0 for all available IP addresses
                     int serverPort,                                                                // Telnet server port
                     bool (*firewallCallback) (char *connectingIP)                                  // a reference to callback function that will be celled when new connection arrives 
                   )  { 
                        // create a local copy of parameters for later use
                        __telnetCommandHandlerCallback__ = telnetCommandHandlerCallback;
                        strncpy (__serverIP__, serverIP, sizeof (__serverIP__)); __serverIP__ [sizeof (__serverIP__) - 1] = 0;
                        __serverPort__ = serverPort;
                        __firewallCallback__ = firewallCallback;

                        // start listener in its own thread (task)
                        __state__ = STARTING;                        
                        #define tskNORMAL_PRIORITY 1
                        if (pdPASS != xTaskCreate (__listenerTask__, "telnetServer", TELNET_SERVER_STACK_SIZE, this, tskNORMAL_PRIORITY, NULL)) {
                          dmesg ("[telnetServer] xTaskCreate error");
                        } else {
                          // wait until listener starts accepting connections
                          while (__state__ == STARTING) delay (1); 
                          // when constructor returns __state__ could be eighter RUNNING (in case of success) or NOT_RUNNING (in case of error)
                        }
                      }
        
        ~telnetServer ()  {
                          // close listening socket
                          int listeningSocket;
                          xSemaphoreTake (__telnetServerSemaphore__, portMAX_DELAY);
                            listeningSocket = __listeningSocket__;
                            __listeningSocket__ = -1;
                          xSemaphoreGive (__telnetServerSemaphore__);
                          if (listeningSocket > -1) close (listeningSocket);

                          // wait until listener finishes before unloading so that memory variables are still there while the listener is running
                          while (__state__ != NOT_RUNNING) delay (1);
                        }
 
      private:

        STATE_TYPE __state__ = NOT_RUNNING;

        String (* __telnetCommandHandlerCallback__) (int argc, char *argv [], telnetConnection *tcn) = NULL;
        char __serverIP__ [46] = "0.0.0.0";
        int __serverPort__ = 23;
        bool (* __firewallCallback__) (char *connectingIP) = NULL;

        int __listeningSocket__ = -1;

        static void __listenerTask__ (void *pvParameters) {
          {
            // get "this" pointer
            telnetServer *ths = (telnetServer *) pvParameters;  
    
            // start listener
            ths->__listeningSocket__ = socket (PF_INET, SOCK_STREAM, 0);
            if (ths->__listeningSocket__ == -1) {
              dmesg ("[telnetServer] socket error: ", errno);
            } else {
              // make address reusable - so we won't have to wait a few minutes in case server will be restarted
              int flag = 1;
              if (setsockopt (ths->__listeningSocket__, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag)) == -1) {
                dmesg ("[telnetServer] setsockopt error: ", errno);
              } else {
                // bind listening socket to IP address and port     
                struct sockaddr_in serverAddress; 
                memset (&serverAddress, 0, sizeof (struct sockaddr_in));
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_addr.s_addr = inet_addr (ths->__serverIP__);
                serverAddress.sin_port = htons (ths->__serverPort__);
                if (bind (ths->__listeningSocket__, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
                  dmesg ("[telnetServer] bind error: ", errno);
               } else {
                 // mark socket as listening socket
                 #define BACKLOG 5
                 if (listen (ths->__listeningSocket__, TCP_LISTEN_BACKLOG) == -1) {
                  dmesg ("[tlnetServer] listen error: ", errno);
                 } else {
          
                  // listener is ready for accepting connections
                  ths->__state__ = RUNNING;
                  dmesg ("[telnetServer] started");
                  while (ths->__listeningSocket__ > -1) { // while listening socket is opened
          
                      int connectingSocket;
                      struct sockaddr_in connectingAddress;
                      socklen_t connectingAddressSize = sizeof (connectingAddress);
                      connectingSocket = accept (ths->__listeningSocket__, (struct sockaddr *) &connectingAddress, &connectingAddressSize);
                      if (connectingSocket == -1) {
                        if (ths->__listeningSocket__ > -1) dmesg ("[telnetServer] accept error: ", errno);
                      } else {
                        // get client's IP address
                        char clientIP [46]; inet_ntoa_r (connectingAddress.sin_addr, clientIP, sizeof (clientIP)); 
                        // get server's IP address
                        char serverIP [46]; struct sockaddr_in thisAddress = {}; socklen_t len = sizeof (thisAddress);
                        if (getsockname (connectingSocket, (struct sockaddr *) &thisAddress, &len) != -1) inet_ntoa_r (thisAddress.sin_addr, serverIP, sizeof (serverIP));
                        // port number could also be obtained if needed: ntohs (thisAddress.sin_port);
                        if (ths->__firewallCallback__ && !ths->__firewallCallback__ (clientIP)) {
                          dmesg ("[telnetServer] firewall rejected connection from ", clientIP);
                          close (connectingSocket);
                        } else {
                          // make the socket non-blocking so that we can detect time-out
                          if (fcntl (connectingSocket, F_SETFL, O_NONBLOCK) == -1) {
                            dmesg ("[telnetServer] fcntl error: ", errno);
                            close (connectingSocket);
                          } else {
                                // create telnetConnection instence that will handle the connection, then we can lose reference to it - telnetConnection will handle the rest
                                telnetConnection *tcn = new telnetConnection (connectingSocket, 
                                                                              ths->__telnetCommandHandlerCallback__, 
                                                                              clientIP, serverIP);
                                if (!tcn) {
                                  // dmesg ("[telnetServer] new telnetConnection error");
                                  sendAll (connectingSocket, telnetServiceUnavailable, strlen (telnetServiceUnavailable), TELNET_CONNECTION_TIME_OUT);
                                  close (connectingSocket); // normally telnetConnection would do this but if it is not created we have to do it here
                                } else {
                                  if (tcn->state () != telnetConnection::RUNNING) {
                                    sendAll (connectingSocket, telnetServiceUnavailable, strlen (telnetServiceUnavailable), TELNET_CONNECTION_TIME_OUT);
                                    delete (tcn); // normally telnetConnection would do this but if it is not running we have to do it here
                                  }
                                }
                                                               
                          } // fcntl
                        } // firewall
                      } // accept
                      
                  } // while accepting connections
                  dmesg ("[telnetServer] stopped");
          
                 } // listen
               } // bind
              } // setsockopt
              int listeningSocket;
              xSemaphoreTake (__telnetServerSemaphore__, portMAX_DELAY);
                listeningSocket = ths->__listeningSocket__;
                ths->__listeningSocket__ = -1;
              xSemaphoreGive (__telnetServerSemaphore__);
              if (listeningSocket > -1) close (listeningSocket);
            } // socket
            ths->__state__ = NOT_RUNNING;
          }
          // stop the listening thread (task)
          vTaskDelete (NULL); // it is listener's responsibility to close itself          
        }
          
    };

#endif
