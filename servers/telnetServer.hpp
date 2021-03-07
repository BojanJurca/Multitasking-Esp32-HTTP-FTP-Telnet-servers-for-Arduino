/*
 * 
 * telnetServer.hpp
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
 *          - added ifconfig and arp commands, 
 *            December 9, 2018, Bojan Jurca
 *          - added iw (dev wlan1 station dump) command, 
 *            December 11, 2018, Bojan Jurca          
 *          - added fileSystemSemaphore and delay () to assure safe muti-threading while using SPIFSS functions (see https://www.esp32.com/viewtopic.php?t=7876), 
 *            April 13, 2019, Bojan Jurca
 *          - telnetCommandHandler parameters are now easyer to access, 
 *            improved user management,
 *            September 4th, Bojan Jurca     
 *          - added dmesg (--follow) command,
 *            September 14, 2019, Bojan Jurca        
 *          - added free command,
 *            October 2, 2019, Bojan Jurca
 *          - added mkfs command,
 *            replaced gmtime () function that returns ponter to static structure with reentrant solution
 *            October 29, 2019, Bojan Jurca
 *          - added uname and telnet commands
 *            November 10, 2019, Bojan Jurca
 *          - added curl command
 *            December 22, 2019, Bojan Jurca
 *          - telnetServer is now inherited from TcpServer
 *            February 27, 2020, Bojan Jurca
 *          - elimination of compiler warnings and some bugs
 *            Jun 11, 2020, Bojan Jurca 
 *          - port from SPIFFS to FAT file system, 
 *            adjustment for Arduino 1.8.13,
 *            added telnet commands (pwd, cd, mkdir, rmdir, clear, vi, ...)
 *            October 10, 2020, Bojan Jurca
 *          - putty support,
 *            bug fixes
 *            March 1, 2021, Bojan Jurca
 *            
 */


#ifndef __TELNET_SERVER__
  #define __TELNET_SERVER__

  #include <WiFi.h>
  #include "common_functions.h"   // pad, cpuMHz
  #include "TcpServer.hpp"        // telnetServer.hpp is built upon TcpServer.hpp  
  #include "user_management.h"    // telnetServer.hpp needs user_management.h for login, home directory, ...
  #include "network.h"            // telnetServer.hpp needs network.h to process network commands such as arp, ...
  #include "file_system.h"        // telnetServer.hpp needs file_system.h to process file system commands sucn as ls, ...
  #include "version_of_servers.h" // version of this software used in uname command
  #include "webServer.hpp"        // webClient needed for curl command
  // needed for ping command
  #include "lwip/inet_chksum.h"
  #include "lwip/ip.h"
  #include "lwip/ip4.h"
  #include "lwip/err.h"
  #include "lwip/icmp.h"
  #include "lwip/sockets.h"
  #include "lwip/sys.h"
  #include "lwip/netdb.h"
  #include "lwip/dns.h"
  // needed for (hard) reset command
  #include "esp_int_wdt.h"
  #include "esp_task_wdt.h"
  // functions from real_time_clock.hpp
  time_t getGmt ();                       
  void setGmt (time_t t);
  time_t getUptime ();                    
  time_t timeToLocalTime (time_t t);


  // DEFINITIONS - change according to your needs

  #ifndef HOSTNAME
    #define "MyESP32Server" // WiFi.getHostname() // use default if not defined
  #endif
  #ifndef MACHINETYPE
    #define MACHINETYPE "ESP32"
  #endif
  #define ESP_SDK_VERSION ESP.getSdkVersion ()


  // FUNCTIONS OF THIS MODULE

  void dmesg (String message);  


  // VARIABLES AND FUNCTIONS TO BE USED INSIDE THIS MODULE
  
  #define UNAME String (MACHINETYPE) + " (" + String (cpuMHz) + " MHz) " + String (HOSTNAME) + " SDK " + String (ESP_SDK_VERSION) + " " + String (VERSION_OF_SERVERS)

  // find and report reset reason (this may help with debugging)
  #include <rom/rtc.h>
  String resetReasonAsString (RESET_REASON reason) {
    switch (reason) {
      case 1:  return "POWERON_RESET - 1, Vbat power on reset";
      case 3:  return "SW_RESET - 3, Software reset digital core";
      case 4:  return "OWDT_RESET - 4, Legacy watch dog reset digital core";
      case 5:  return "DEEPSLEEP_RESET - 5, Deep Sleep reset digital core";
      case 6:  return "SDIO_RESET - 6, Reset by SLC module, reset digital core";
      case 7:  return "TG0WDT_SYS_RESET - 7, Timer Group0 Watch dog reset digital core";
      case 8:  return "TG1WDT_SYS_RESET - 8, Timer Group1 Watch dog reset digital core";
      case 9:  return "RTCWDT_SYS_RESET - 9, RTC Watch dog Reset digital core";
      case 10: return "INTRUSION_RESET - 10, Instrusion tested to reset CPU";
      case 11: return "TGWDT_CPU_RESET - 11, Time Group reset CPU";
      case 12: return "SW_CPU_RESET - 12, Software reset CPU";
      case 13: return "RTCWDT_CPU_RESET - 13, RTC Watch dog Reset CPU";
      case 14: return "EXT_CPU_RESET - 14, for APP CPU, reseted by PRO CPU";
      case 15: return "RTCWDT_BROWN_OUT_RESET - 15, Reset when the vdd voltage is not stable";
      case 16: return "RTCWDT_RTC_RESET - 16, RTC Watch dog reset digital core and rtc module";
      default: return "RESET REASON UNKNOWN";
    }
  } 

  // find and report reset reason (this may help with debugging)
  String wakeupReasonAsString () {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause ();
    switch (wakeup_reason){
      case ESP_SLEEP_WAKEUP_EXT0:     return "ESP_SLEEP_WAKEUP_EXT0 - wakeup caused by external signal using RTC_IO.";
      case ESP_SLEEP_WAKEUP_EXT1:     return "ESP_SLEEP_WAKEUP_EXT1 - wakeup caused by external signal using RTC_CNTL.";
      case ESP_SLEEP_WAKEUP_TIMER:    return "ESP_SLEEP_WAKEUP_TIMER - wakeup caused by timer.";
      case ESP_SLEEP_WAKEUP_TOUCHPAD: return "ESP_SLEEP_WAKEUP_TOUCHPAD - wakeup caused by touchpad.";
      case ESP_SLEEP_WAKEUP_ULP:      return "ESP_SLEEP_WAKEUP_ULP - wakeup caused by ULP program.";
      default:                        return "WAKEUP REASON UNKNOWN - wakeup was not caused by deep sleep: " + String (wakeup_reason) + ".";
    }   
  }
    
  struct __dmesgType__ {
    unsigned long milliseconds;    
    time_t        time;
    String        message;
  };

  #define __DMESG_CIRCULAR_QUEUE_LENGTH__ 256
  RTC_DATA_ATTR unsigned int bootCount = 0;
  __dmesgType__ __dmesgCircularQueue__ [__DMESG_CIRCULAR_QUEUE_LENGTH__] = {{0, 0, "[ESP32] CPU0 reset reason: " + resetReasonAsString (rtc_get_reset_reason (0))}, 
                                                                            {0, 0, "[ESP32] CPU1 reset reason: " + resetReasonAsString (rtc_get_reset_reason (1))}, 
                                                                            {millis (), 0, "[ESP32] wakeup reason: " + wakeupReasonAsString ()},
                                                                            {millis (), 0, String (__timeHasBeenSet__ ? "[ESP32] " + UNAME + " (re)started " + String (++bootCount) + " times at: " + timeToString (getLocalTime ()) + "." : "[ESP32] " + UNAME + " (re)started " + String (++bootCount) + ". time and has not obtained current time yet.")}
                                                                           }; // there are always at least 4 messages in the queue which makes things a little simper - after reboot or deep sleep the time is preserved
  byte __dmesgBeginning__ = 0; // first used location
  byte __dmesgEnd__ = 4;       // the location next to be used
  portMUX_TYPE __csDmesg__ = portMUX_INITIALIZER_UNLOCKED;

  // adds message into dmesg circular queue
  void dmesg (String message) {
    portENTER_CRITICAL (&__csDmesg__); 
    __dmesgCircularQueue__ [__dmesgEnd__].milliseconds = millis ();
    __dmesgCircularQueue__ [__dmesgEnd__].time = getGmt ();
    __dmesgCircularQueue__ [__dmesgEnd__].message = message;
    if ((__dmesgEnd__ = (__dmesgEnd__ + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__) == __dmesgBeginning__) __dmesgBeginning__ = (__dmesgBeginning__ + 1) % __DMESG_CIRCULAR_QUEUE_LENGTH__;
    portEXIT_CRITICAL (&__csDmesg__);
    Serial.printf ("[%10lu] %s\n", millis (), message.c_str ());
  }

  // redirect other moduls' dmesg here before setup () begins
  bool __redirectDmesg__ () {
    #ifdef __TCP_SERVER__
      TcpDmesg = dmesg;
    #endif  
    #ifdef __FILE_SYSTEM__
      fileSystemDmesg = dmesg;
    #endif  
    #ifdef __NETWORK__
      networkDmesg = dmesg;
    #endif
    #ifdef __FTP_SERVER__
      ftpDmesg = dmesg;
    #endif    
    #ifdef __WEB_SERVER__
      webDmesg = dmesg;
    #endif  
    #ifdef __TIME_FUNCTIONS__
      timeDmesg = dmesg;
    #endif      
    return true;
  }
  bool __redirectedDmesg__ = __redirectDmesg__ ();


            //----- for IW: TO DO: try to put this variables and initializer inside class definition  -----
            static SemaphoreHandle_t __WiFiSnifferSemaphore__ = xSemaphoreCreateMutex (); // to prevent two threads to start sniffing simultaneously
            static String __macToSniffRssiFor__;  // input to __sniffWiFiForRssi__ function
            static int __rssiSniffedForMac__;     // output of __sniffWiFiForRssi__ function


  // TELNET SERVER CLASS

  class telnetServer: public TcpServer {                      
  
    public:

      // define some IAC telnet commands
      #define IAC               "\xff" // 255
      #define DONT              "\xfe" // 254
      #define DO                "\xfd" // 253
      #define WONT              "\xfc" // 252
      #define WILL              "\xfb" // 251
      #define SB                "\xfa" // 250
      #define SE                "\xf0" // 240
      #define LINEMODE          "\x22" //  34
      #define NAWS              "\x1f" //  31
      #define SUPPRESS_GO_AHEAD "\x03"
      #define ECHO              "\x01" //   1 
      
      // keep telnet session parameters in a structure
      typedef struct {
        String userName;
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
                    unsigned int stackSize,                                                                                 // stack size of httpRequestHandler thread, usually 4 KB will do 
                    char *serverIP,                                                                                         // telnet server IP address, 0.0.0.0 for all available IP addresses - 15 characters at most!
                    int serverPort,                                                                                         // telnet server port
                    bool (*firewallCallback) (char *)                                                                       // a reference to callback function that will be celled when new connection arrives 
                   ): TcpServer (__telnetConnectionHandler__, (void *) this, stackSize, 300000, serverIP, serverPort, firewallCallback)
                        {
                          this->__externalTelnetCommandHandler__ = telnetCommandHandler;

                          if (started ()) dmesg ("[telnetServer] started on " + String (serverIP) + ":" + String (serverPort) + (firewallCallback ? " with firewall." : "."));
                          else            dmesg ("[telnetServer] couldn't start.");
                        }

      ~telnetServer ()  { if (started ()) dmesg ("[telnetServer] stopped."); }
      
    private:

      String (* __externalTelnetCommandHandler__) (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) = NULL; // telnet command handler supplied by calling program 

      static void __telnetConnectionHandler__ (TcpConnection *connection, void *thisTelnetServer) { // connectionHandler callback function

        // this is where telnet session begins
        
        telnetServer *ths = (telnetServer *) thisTelnetServer; // we've got this pointer into static member function
        telnetSessionParameters tsp = {"", "", (char *) "", NULL, 0, 0, 0, 0};
        String password;
        
        String cmdLine;

        #if USER_MANAGEMENT == NO_USER_MANAGEMENT

          tsp = {"root", "", "", (char *) "\r\n# ", connection, 0, 0, 0, 0};
          tsp.workingDir = tsp.homeDir = getUserHomeDirectory (tsp.userName);     
          // tell the client to go into character mode, not to echo an send its window size, then say hello 
          connection->sendData (String (IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS) + "Hello " + String (connection->getOtherSideIP ()) + "!\r\nuser: "); 
          dmesg ("[telnetServer] " + tsp.userName + " logged in.");
          connection->sendData ("\r\n\nWelcome " + tsp.userName + ",\r\nyour home directory is " + tsp.homeDir + ",\r\nuse \"help\" to display available commands.\r\n" + tsp.prompt);
        
        #else

          tsp = {"", "", "", (char *) "", connection, 0, 0, 0, 0};
          // tell the client to go into character mode, not to echo an send its window size, then say hello 
          connection->sendData (String (IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC DO NAWS) + "Hello " + String (connection->getOtherSideIP ()) + "!\r\nuser: "); 
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
            
            int argc = 0; String argv [16] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""}; 
            argv [0] = String (cmdLine); argv [0].trim ();
            if (argv [0] != "") {
              argc = 1;
              while (argc < 16) {
                int l;
                
                // try to parse against '\"' first to support long file names
                if (argv [argc - 1].charAt (0) == '\"') {
                  l = argv [argc - 1].indexOf ('\"', 1);
                  if (l > 0) {
                    argv [argc] = argv [argc - 1].substring (l + 1);
                    argv [argc - 1] = argv [argc - 1].substring (0, l + 1); // no need to trim
                    argv [argc].trim ();
                    if (argv [argc] == "") break;
                    argc ++;
                    continue;
                  }
                }
                // parse argv [argc - 1] against ' '
                l = argv [argc - 1].indexOf (" ");
                if (l > 0) {
                  argv [argc] = argv [argc - 1].substring (l + 1);
                  argv [argc - 1] = argv [argc - 1].substring (0, l); // no need to trim
                  argv [argc].trim ();                  
                  if (argv [argc] == "") break;
                  argc ++;
                  continue;
                }
                break;
              }
            }

            for (int i = 0; i < argc; i++) if (argv [i].charAt (0) == '\"' && argv [i].charAt (argv [i].length () - 1) == '\"') argv [i] = argv [i].substring (1, argv [i].length () - 1);

            // debug: for (int i = 0; i < argc; i++) Serial.print ("|" + argv [i] + "|     "); Serial.println ();

            // ----- ask telnetCommandHandler (if it is provided by the calling program) if it is going to handle this command, otherwise try to handle it internally -----
    
            String r;
            // unsigned long timeOutMillis = connection->getTimeOut (); connection->setTimeOut (TcpConnection::INFINITE); // disable time-out checking while proessing telnetCommandHandler to allow longer processing times
            if (ths->__externalTelnetCommandHandler__ && (r = ths->__externalTelnetCommandHandler__ (argc, argv, &tsp)) != "") 
              connection->sendData (r); // send reply to telnet client
            else connection->sendData (ths->__internalTelnetCommandHandler__ (argc, argv, &tsp)); // send reply to telnet client

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
          
          if (argc == 1) return this->__help__ (tsp);
                         return "Wrong syntax. Use help";

        } else if (argv [0] == "quit") { //------------------------------------ QUIT
          
          if (argc == 1) return this->__quit__ (tsp);
                         return "Wrong syntax. Use quit";

        } else if (argv [0] == "clear" || argv [0] == "cls") { //-------------- CLEAR, CLS
          
          if (argc == 1) return this->__clear__ (tsp);
                         return "Wrong syntax. Use clear or cls";
                         
        } else if (argv [0] == "uname") { //----------------------------------- UNAME
    
          if (argc == 1 || (argc == 2 && argv [1] == "-a")) return this->__uname__ ();
                                                            return "Wrong syntax. Use uname or uname -a";

        } else if (argv [0] == "uptime") { //---------------------------------- UPTIME
    
          if (argc == 1) return this->__uptime__ ();
                         return "Wrong syntax. Use uptime";

        } else if (argv [0] == "reboot") { //---------------------------------- REBOOT
    
          if (argc == 1) return this->__reboot__ (tsp);
                         return "Wrong syntax. Use reboot";
          
        } else if (argv [0] == "reset") { //----------------------------------- RESET
    
          if (argc == 1) return this->__reset__ (tsp);
                         return "Wrong syntax. Use reset";
    
        } else if (argc >= 1 && argv [0] == "date") { //----------------------- DATE
          
          if (argc == 1)                     return this->__getDateTime__ ();
          if (argc == 4 && argv [1] == "-s") return this->__setDateTime__ (argv [2], argv [3]);
                                             return "Wrong syntax. Use date or date -s YYYY/MM/DD hh:mm:ss (use hh in 24 hours time format).";

        } else if (argc >= 1 && argv [0] == "ntpdate") { //-------------------- NTPDATE
          
          if (argc == 1 || ((argc == 2 || argc == 3) && argv [1] == "-u")) return this->__ntpdate__ (argv [2]);
                                                                           return "Wrong syntax. Use ntpdate or ntpdate -u or ntpdate -u ntpServer.";

        } else if (argc >= 1 && argv [0] == "crontab") { //-------------------- CRONTAB
          
          if (argc == 1 || (argc == 2 && argv [1] == "-l")) return cronTab ();
                                                            return "Wrong syntax. Use crontab or crontab -l.";

        } else if (argv [0] == "free") { //------------------------------------ FREE
    
          long n;
          if (argc == 1)                                                               return this->__free__ (0, tsp);
          if (argc == 3 && argv [1] == "-s" && (n = argv [2].toInt ()) > 0 && n < 300) return this->__free__ (n, tsp);
                                                                                       return "Wrong syntax. Use free or free -s n (where 0 < n < 300).";
          
        } else if (argv [0] == "dmesg") { //----------------------------------- DMESG

          bool f = false;
          bool t = false;
          for (int i = 1; i < argc; i++) {
            if (argv [i] == "--follow") f = true;
            else if (argv [i] == "-T") t = true;
            else return "Wrong syntax. Use dmesg or dmesg --follow or dmesg -T or both";
          }
          return this->__dmesg__ (f, t, tsp);

        } else if (argv [0] == "mkfs.fat") { //-------------------------------- MKFS.FAT
          
          if (argc == 1) {
            if (tsp->userName == "root") return this->__mkfs__ (tsp);
            else                         return "Only root user may format disk.";
          }
                                         return "Wrong syntax. Use mkfs.fat";
          
        } else if (argv [0] == "fs_info") { // -------------------------------- FS_INFO
          
          if (argc <= 2) return this->__fs_info__ ();
                         return "Wrong syntax. Use fs_info";          

        } else if (argv [0] == "ls" || argv [0] == "dir") { //----------------- LS, DIR

          if (argc == 1) return this->__ls__ (tsp->workingDir, tsp);
          if (argc == 2) return this->__ls__ (argv [1], tsp);
                         return "Wrong syntax. Use ls or ls directoryName";

        } else if (argv [0] == "tree") { //------------------------------------ TREE

          if (argc == 1) return this->__tree__ (tsp->workingDir, tsp);
          if (argc == 2) return this->__tree__ (argv [1], tsp);
                         return "Wrong syntax. Use tree or tree directoryName";
        
        } else if (argv [0] == "cat" || argv [0] == "type") { //--------------- CAT, TYPE
          
          if (argc == 2) return this->__catFileToClient__ (argv [1], tsp);
          if (argc == 3 && argv [1] == ">") return this->__catClientToFile__ (argv [2], tsp);
                         return "Wrong syntax. Use cat fileName or cat > fileName";

        } else if (argv [0] == "rm" || argv [0] == "del") { //----------------- RM
          
          if (argc == 2) return this->__rm__ (argv [1], tsp);
                         return "Wrong syntax. Use rm fileName";

        } else if (argv [0] == "mkdir") { //----------------------------------- MKDIR
          
          if (argc == 2) return this->__mkdir__ (argv [1], tsp);
                         return "Wrong syntax. Use mkdir directoryName";

        } else if (argv [0] == "rmdir") { //----------------------------------- RMDIR
          
          if (argc == 2) return this->__rmdir__ (argv [1], tsp);
                         return "Wrong syntax. Use rmdir directoryName";

        } else if (argv [0] == "cd") { //-------------------------------------- CD
          
          if (argc == 2) return this->__cd__ (argv [1], tsp);
                         return "Wrong syntax. Use cd directoryName";

        } else if (argv [0] == "pwd") { //------------------------------------- PWD
          
          if (argc == 1) return this->__pwd__ (tsp);
                         return "Wrong syntax. Use pwd";

        } else if (argv [0] == "mv" || argv [0] == "ren") { //----------------- MV
          
          if (argc == 3) return this->__mv__ (argv [1], argv [2], tsp);
                         return "Wrong syntax. Use mv srcFileName dstFileName or mv srcDirectoryName dstDirectoryName";

        } else if (argv [0] == "cp" || argv [0] == "copy") { //---------------- CP
          
          if (argc == 3) return this->__cp__ (argv [1], argv [2], tsp);
                         return "Wrong syntax. Use cp srcFileName dstFileName";

        } else if (argv [0] == "vi") { //-------------------------------------- VI

          if (argc == 2)  {
                            unsigned long to = tsp->connection->getTimeOut (); tsp->connection->setTimeOut (TcpConnection::INFINITE); // we don't want TcpConnection to break during editing
                            String s = this->__vi__ (argv [1], tsp);
                            tsp->connection->setTimeOut (to); // restore time-out
                            return s;
                          }
                          return "Wrong syntax. Use vi fileName";                         

        #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT

          } else if (argv [0] == "passwd") { //-------------------------------- PASSWD
            
            if (argc == 1)                                              return this->__passwd__ (tsp->userName, tsp);
            if (argc == 2) {
              if (tsp->userName == "root" || argv [1] == tsp->userName) return this->__passwd__ (argv [1], tsp); 
              else                                                      return "You may not change password for " + argv [1] + ".";
            }

          } else if (argv [0] == "useradd") { //------------------------------- USERADD

            if (tsp->userName != "root")                           return "Only root may add users.";
            if (argc == 6 && argv [1] == "-u" && argv [3] == "-d") return this->__userAdd__ (argv [5], argv [2], argv [4]);
                                                                   return "Wrong syntax. Use useradd -u userId -d userHomeDirectory userName (where userId > 1000).";

          } else if (argv [0] == "userdel") { //------------------------------- USERDEL

            if (tsp->userName != "root") return "Only root may delete users.";
            if (argc != 2)               return "Wrong syntax. Use userdel userName";
            if (argv [1] == "root")      return "You don't really want to to this.";
                                         return this->__userDel__ (argv [1]);

        #endif

        } else if (argv [0] == "ifconfig" || argv [0] == "ipconfig") { //------ IFCONFIG, IPCONFIG
          
          if (argc == 1) return this->__ifconfig__ ();
                         return "Unknown option.";

        } else if (argv [0] == "iw") { //-------------------------------------- IW
    
          if (argc == 1) return this->__iw__ (tsp);
                         return "Wrong syntax. Use ifconfig";

        } else if (argv [0] == "arp") { //------------------------------------- ARP
    
          if (argc == 1)                       return arp_a (); // from network.h
          if ((argc == 2 && argv [1] == "-a")) return arp_a (); // from network.h
                                               return "Wrong syntax. Use arp or arp -a";
    
        } else if (argv [0] == "ping") { //------------------------------------ PING
    
          if (argc == 2) return this->__ping__ (tsp->connection, (char *) argv [1].c_str ()); 
                         return "Wrong syntax. Use ping ipAddres";
   
        } else if (argv [0] == "telnet") { //---------------------------------- TELENT

          long port;
          if (argc == 2)                                                                 return this->__telnet__ (argv [1], 23, tsp);
          if (argc == 3 && (port = argv [2].toInt ()) && port > 0 && (int) port == port) return this->__telnet__ (argv [1], (int) port, tsp);
                                                                                         return "Wrong syntax. Use telnet ipAddress or telnet ipAddress portNumber";
        } else if (argv [0] == "curl") { //------------------------------------ CURL
          
          if (argc == 2) return this->__curl__ ("GET", argv [1]);
          if (argc == 3) return this->__curl__ (argv [1], argv [2]);
                         return "Wrong syntax. Use curl http://url or curl method http://url (where method is GET, PUT, ...):";

        }
        
        // -------------------------------------------------------------------- INVALID COMMAND
                      
        return "Invalid command, use help to display available commands.";
      }

      inline String __help__ (telnetSessionParameters *tsp) __attribute__((always_inline)) {
        String e = this->__catFileToClient__ (getUserHomeDirectory ("telnetserver") + "help.txt", tsp);
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
            return this->__getDateTime__ ();          
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
        portENTER_CRITICAL (&__csDmesg__);  
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
        portEXIT_CRITICAL (&__csDmesg__);
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
          portENTER_CRITICAL (&__csDmesg__);
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
          portEXIT_CRITICAL (&__csDmesg__);
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
            char *buff = (char *) malloc (2048); // get 2 KB of memory from heap (not from the stack)
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
                if (i >= 2048 - 2) { tsp->connection->sendData ((char *) buff, i); i = 0; }
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
        if (!userMayAccess (fp, tsp->homeDir))          return "Access tp " + fp + " denyed.";

        if (makeDir (fp))                               return fp + " made.";
                                                        return "Can't make " + fp;

      }

      inline String __rmdir__ (String& directory, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "" || !isDirectory (fp))              return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))          return "Access tp " + fp + " denyed.";
        if (fp == tsp->homeDir)                         return "You may not remove your home directory.";
        if (fp == tsp->workingDir)                      return "You can't remove your working directory.";

        if (removeDir (fp))                             return fp + " removed.";
                                                        return "Can't remove " + fp;

      }      

      inline String __cd__ (String& directory, telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";

        String fp = fullDirectoryPath (directory, tsp->workingDir);
        if (fp == "" || !isDirectory (fp))              return "Invalid directory name " + directory;
        if (!userMayAccess (fp, tsp->homeDir))          return "Access tp " + fp + " denyed.";

        tsp->workingDir = fp;                           
                                                        return "Your working directory is " + tsp->workingDir;
      }

      inline String __pwd__ (telnetSessionParameters *tsp) { 
        if (!__fileSystemMounted__) return "File system not mounted. You may have to use mkfs.fat to format flash disk first.";
        
        return "Your working directory is " + tsp->workingDir;
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

      inline String __ifconfig__ () __attribute__((always_inline)) {
        String s = "";
        struct netif *netif;
        for (netif = netif_list; netif; netif = netif->next) {
          if (netif_is_up (netif)) {
            if (s != "") s += "\r\n";
            s += String (netif->name [0]) + String (netif->name [1]) + "      hostname: " + (netif->hostname ? String (netif->hostname) : "") + "\r\n" + 
                     "        hwaddr: " + MacAddressAsString (netif->hwaddr, netif->hwaddr_len) + "\r\n" +
                     "        inet addr: " + inet_ntos (netif->ip_addr) + "\r\n" + 
                     "        mtu: " + String (netif->mtu) + "\r\n";
          }
        }
        return s;    
      }

      // ----- IW -----
            
            typedef struct {
              unsigned frame_ctrl:16;
              unsigned duration_id:16;
              uint8_t addr1 [6]; // receiver address 
              uint8_t addr2 [6]; // sender address 
              uint8_t addr3 [6]; // filtering address 
              unsigned sequence_ctrl:16;
              uint8_t addr4 [6]; // optional 
            } wifi_ieee80211_mac_hdr_t;
            
            typedef struct {
              wifi_ieee80211_mac_hdr_t hdr;
              uint8_t payload [0]; // network data ended with 4 bytes csum (CRC32)
            } wifi_ieee80211_packet_t;      

            int __sniffWiFiForRssi__ (String stationMac) { // sniff WiFi trafic for station RSSI - since we are sniffing connected stations we can stay on AP WiFi channel
                                                           // sniffing WiFi is not well documented, there are some working examples on internet however:
                                                           // https://www.hackster.io/p99will/esp32-wifi-mac-scanner-sniffer-promiscuous-4c12f4
                                                           // https://esp32.com/viewtopic.php?t=1314
                                                           // https://blog.podkalicki.com/esp32-wifi-sniffer/
              int rssi;                                          
              xSemaphoreTake (__WiFiSnifferSemaphore__, portMAX_DELAY);
                __macToSniffRssiFor__ = stationMac;
                __rssiSniffedForMac__ = 0;
                esp_wifi_set_promiscuous (true);
                const wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};      
                esp_wifi_set_promiscuous_filter (&filter);
                // esp_wifi_set_promiscuous_rx_cb (&__WiFiSniffer__);
                esp_wifi_set_promiscuous_rx_cb ([] (void* buf, wifi_promiscuous_pkt_type_t type) {
                                                                                                    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *) buf;
                                                                                                    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *) ppkt->payload;
                                                                                                    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
                                                                                                    // TO DO: I'm not 100% sure that this works in all cases since source mac address may not be
                                                                                                    //        always in the same place for all types and subtypes of frame
                                                                                                    if (__macToSniffRssiFor__ == MacAddressAsString ((byte *) hdr->addr2, 6)) __rssiSniffedForMac__ = ppkt->rx_ctrl.rssi;
                                                                                                    return;
                                                                                                  });
                  unsigned long startTime = millis ();
                  while (__rssiSniffedForMac__ == 0 && millis () - startTime < 5000) delay (1); // sniff max 5 second, it should be enough
                  // Serial.printf ("RSSI obtained in %lu milliseconds\n", millis () - startTime);
                  rssi = __rssiSniffedForMac__;
                esp_wifi_set_promiscuous (false);
          
              xSemaphoreGive (__WiFiSnifferSemaphore__);
              return rssi;
            }

      String __iw__ (telnetSessionParameters *tsp) {
        String s = "";
        struct netif *netif;
        for (netif = netif_list; netif; netif = netif->next) {
          if (netif_is_up (netif)) {
            if (s != "") s += "\r\n";
            // display the following information for STA and AP interface (similar to ifconfig)
            s += String (netif->name [0]) + String (netif->name [1]) + "      hostname: " + (netif->hostname ? String (netif->hostname) : "") + "\r\n" +
                 "        hwaddr: " + MacAddressAsString (netif->hwaddr, netif->hwaddr_len) + "\r\n" +
                 "        inet addr: " + inet_ntos (netif->ip_addr) + "\r\n";
 
                    // display the following information for STA interface
                    if (inet_ntos (netif->ip_addr) == WiFi.localIP ().toString ()) {
                      if (WiFi.status () == WL_CONNECTED) {
                        int rssi = WiFi.RSSI ();
                        String rssiDescription = ""; if (rssi == 0) rssiDescription = "not available"; else if (rssi >= -30) rssiDescription = "excelent"; else if (rssi >= -67) rssiDescription = "very good"; else if (rssi >= -70) rssiDescription = "okay"; else if (rssi >= -80) rssiDescription = "not good"; else if (rssi >= -90) rssiDescription = "bad"; else /* if (rssi >= -90) */ rssiDescription = "unusable";
                        s += String ("           STAtion is connected to router:\r\n\r\n") + 
                                     "              inet addr: " + WiFi.gatewayIP ().toString () + "\r\n" +
                                     "              RSSI: " + String (rssi) + " dBm (" + rssiDescription + ")\r\n";
                      } else {
                        s += "           STAtion is disconnected from router\r\n";
                      }
                    // display the following information for local loopback interface
                    } else if (inet_ntos (netif->ip_addr) == "127.0.0.1") {
                        s += "           local loopback\r\n";
                    // display the following information for AP interface
                    } else {
                      wifi_sta_list_t wifi_sta_list = {};
                      tcpip_adapter_sta_list_t adapter_sta_list = {};
                      esp_wifi_ap_get_sta_list (&wifi_sta_list);
                      tcpip_adapter_get_sta_list (&wifi_sta_list, &adapter_sta_list);
                      if (adapter_sta_list.num) {
                        s += "           stations connected to Access Point (" + String (adapter_sta_list.num) + "):\r\n";
                        for (int i = 0; i < adapter_sta_list.num; i++) {
                          tcpip_adapter_sta_info_t station = adapter_sta_list.sta [i];
                          s += String ("\r\n") + 
                                       "              hwaddr: " + MacAddressAsString ((byte *) &station.mac, 6) + "\r\n" + 
                                       "              inet addr: " + inet_ntos (station.ip) + "\r\n";
                                       tsp->connection->sendData (s);
                                       s = "";
                                       int rssi = __sniffWiFiForRssi__ (MacAddressAsString ((byte *) &station.mac, 6));
                                       String rssiDescription = ""; if (rssi == 0) rssiDescription = "not available"; else if (rssi >= -30) rssiDescription = "excelent"; else if (rssi >= -67) rssiDescription = "very good"; else if (rssi >= -70) rssiDescription = "okay"; else if (rssi >= -80) rssiDescription = "not good"; else if (rssi >= -90) rssiDescription = "bad"; else /* if (rssi >= -90) */ rssiDescription = "unusable";
                                       s = "              RSSI: " + String (rssi) + " dBm (" + rssiDescription + ")\r\n";
                        }
                      } else {
                        s += "           there are no stations connected to Access Point\r\n";
                      }
                    }

          }
        }
        tsp->connection->sendData (s);
        return "";
      }

      // ----- PING -----      

          // according to: https://github.com/pbecchi/ESP32_ping
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
            to.sin_addr = *(in_addr *) addr; // inet_addr_from_ipaddr (&to.sin_addr, addr);
            
            if ((err = sendto (s, iecho, ping_size, 0, (struct sockaddr*) &to, sizeof (to)))) pds->transmitted ++;
          
            return (err ? ERR_OK : ERR_VAL);
          }
        
          static bool __pingRecv__ (__pingDataStructure__ *pds, TcpConnection *telnetConnection, int s) {
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
                fromaddr = *(ip4_addr_t *) &from.sin_addr; // inet_addr_to_ipaddr (&fromaddr, &from.sin_addr);
                
                strcpy (ipa, inet_ntos (fromaddr).c_str ()); 
          
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
            return false;
          }  
        
      String __ping__ (TcpConnection *telnetConnection, char *targetIP, int pingCount = PING_DEFAULT_COUNT, int pingInterval = PING_DEFAULT_INTERVAL, int pingSize = PING_DEFAULT_SIZE, int timeOut = PING_DEFAULT_TIMEOUT) {
        // struct sockaddr_in address;
        ip4_addr_t pingTarget;
        int s;
        char cstr [256];
      
        // Create socket
        if ((s = socket (AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
          return "Error creating socket.";
        }
      
        pingTarget.addr = inet_addr (targetIP); 
      
        // Setup socket
        struct timeval tOut;
      
        // Timeout
        tOut.tv_sec = timeOut;
        tOut.tv_usec = 0;
      
        if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tOut, sizeof (tOut)) < 0) {
          closesocket (s);
          return "Error setting socket options";
        }
    
        __pingDataStructure__ pds = {};
        pds.ID = random (0, 0xFFFF); // each consequently running ping command should have its own unique ID otherwise we won't be able to distinguish packets 
        pds.minTime = 1.E+9; // FLT_MAX;
      
        // Begin ping ...
      
        sprintf (cstr, "ping %s: %d data bytes\r\n",  targetIP, pingSize);
        if (!telnetConnection->sendData (cstr)) return "";
        
        while ((pds.pingSeqNum < pingCount) && (!pds.stopped)) {
          if (__pingSend__ (&pds, s, &pingTarget, pingSize) == ERR_OK) if (!__pingRecv__ (&pds, telnetConnection, s)) return "";
          delay (pingInterval * 1000L);
        }
      
        closesocket (s);
      
        sprintf (cstr, "%u packets transmitted, %u packets received, %.1f%% packet loss\r\n", pds.transmitted, pds.received, ((((float) pds.transmitted - (float) pds.received) / (float) pds.transmitted) * 100.0));
        if (!telnetConnection->sendData (cstr)) return "";
      
        if (pds.received) {
          sprintf (cstr, "round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms", pds.minTime, pds.meanTime, pds.maxTime, sqrt (pds.varTime / pds.received));
          if (!telnetConnection->sendData (cstr)) return "";
          return ""; // ok
        }
        return ""; // errd
      }

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
        TcpClient *otherServer = new TcpClient ((char *) otherServerName.c_str (), otherServerPort, 300000); // close also this connection if inactive for more than 5 minutes
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
            String r = webClient ((char *) server.c_str (), port, 15000, method + " " + url);
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
        if (tsp->clientWindowCol2) { // we know that client reports its window size, ask for latest information (the user might have resized the window since beginning of telnet session)
          tsp->connection->sendData (String (IAC DO NAWS));
          // client reply that we are expecting from IAC DO NAWS will be in the form of: IAC (255) SB (250) NAWS (31) col1 col2 row1 row1 IAC (255) SE (240)

          // There is a difference between telnet clients. Windows telent.exe for example will only report client window size as a result of
          // IAC DO NAWS command from telnet server. Putty on the other hand will report client windows size as a reply and will
          // continue sending its window size each time client window is resized. But won't respond to IAC DO NAWS if client
          // window size remains the same. Let's wait 0.25 second, it the reply doesn't arrive it porbably never will.

          unsigned long m = millis (); while (tsp->connection->available () != TcpConnection::AVAILABLE && millis () - m < 250) delay (1);
          if (tsp->connection->available () == TcpConnection::AVAILABLE) {
            // debug: Serial.printf ("[telnet vi debug] IAC DO NAWS response available in %lu ms\n", (unsigned long) (millis () - m));
            do {
              if (!tsp->connection->recvData (&c, 1)) return "";
            } while (c != 255 /* IAC */); // ignore everything before IAC
            tsp->connection->recvData (&c, 1); if (c != 250 /* SB */) return "Client send invalid reply to IAC DO NAWS."; // error in telnet protocol or connection closed
            tsp->connection->recvData (&c, 1); if (c != 31 /* NAWS */) return "Client send invalid reply to IAC DO NAWS."; // error in telnet protocol or connection closed
            tsp->connection->recvData (&c, 1); if (!tsp->connection->recvData ((char *) &tsp->clientWindowCol2, 1)) return ""; 
            tsp->connection->recvData (&c, 1); if (!tsp->connection->recvData ((char *) &tsp->clientWindowRow2, 1)) return "";
            tsp->connection->recvData (&c, 1); // should be IAC, won't check if it is OK
            tsp->connection->recvData (&c, 1); // should be SB, won't check if it is OK
            // debug: Serial.println ("[telnet vi debug]  clinet reported its window size: " + String (tsp->clientWindowCol2) + " x " + String (tsp->clientWindowRow2)); // return "";
          } else {
            // debug: Serial.printf ("[telnet vi debug] IAC DO NAWS response not available in %lu ms, taking previous information\n", (unsigned long) (millis () - m));
          }
        } else { // just assume the defaults and hope that the result will be OK
          tsp->clientWindowCol2 = 80; 
          tsp->clientWindowRow2 = 24;   
          // debug: Serial.println ("[telnet vi debug] Going with default client window size: " + String (tsp->clientWindowCol2) + " x " + String (tsp->clientWindowRow2)); // return "";                      
        }
        if (tsp->clientWindowCol2 < 30 || tsp->clientWindowRow2 < 5) return "Clinent telnet windows is too small for vi.";

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
          // debug: Serial.printf ("[telnet vi debug] %c (%i)\n", c, c);
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
            case 27:  // ESC [ 65 = up arrow, ESC [ 66 = down arrow, ESC[C = right arrow, ESC[D = left arrow, 
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
