/*
    A minimal Telnet server
*/

#include <WiFi.h>

  // include all .h and .hpp files for full functionality of telnetServer
  
  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command
  // #include "./servers/perfMon.h"           // contains performence countrs that can be displayed by using "perfmon" telnet command
  // #include "./servers/file_system.h"       // telnetServer can handle some basic file commands like "ls", "cp", ..., there is also a simple text editor "vi"

  // define how network.h will connect to the router
    #define HOSTNAME                  "MyESP32Server"       // define the name of your ESP32 here
    #define MACHINETYPE               "ESP32 NodeMCU"       // describe your hardware here
    // tell ESP32 how to connect to your WiFi router if this is needed
    #define DEFAULT_STA_SSID          "YOUR_STA_SSID"       // define default WiFi settings  
    #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
    // tell ESP32 not to set up AP if it is not needed
    #define DEFAULT_AP_SSID           ""  // HOSTNAME       // set it to "" if you don't want ESP32 to act as AP 
    #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"    // must have at leas 8 characters
  #include "./servers/network.h"              // some network telnet commands like "ping", "ifconfig", ... are also supported here

  // #include "./servers/time_functions.h"    // telnetServer can handle some time commands like "date", "ntpdate", ...
  // #include "./servers/httpClient.h"        // to include "curl" telnet command
  // #include "./servers/smtpClient.h"        // to include "sendmail" telnet command

  // telnetServer is using user_management.h to handle login/logout
    #define USER_MANAGEMENT NO_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // finally include telnetServer.hpp
  #include "./servers/telnetServer.hpp"

  /*
  String telnetCommandHandlerCallback (int argc, char *argv [], telnetConnection *tcn) { // note that each Telnet connection runs in its own thread/task, more of them may run simultaneously so this function should be reentrant

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))    
    #define LED_BUILTIN 2
    
            if (argv0is ("led") && argv1is ("state")) {                    // led state telnet command
              return "Led is " + String (digitalRead (LED_BUILTIN) ? "on." : "off.");
    } else if (argv0is ("turn") && argv1is ("led") && argv2is ("on")) {  // turn led on telnet command
              digitalWrite (LED_BUILTIN, HIGH);
              return "Led is on.";
    } else if (argv0is ("turn") && argv1is ("led") && argv2is ("off")) { // turn led off telnet command
              digitalWrite (LED_BUILTIN, LOW);
              return "Led is off.";
    }

    // let the Telnet server handle command itself by returning ""
    return "";
  }
  */

  /*
  bool firewallCallback (char *connectingIP) { return true; } // decide based on connectingIP to accept or not to accept the connection
  */

  telnetServer *myTelnetServer; 

void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                              // use configuration files and file telnet commands
  // initializeUsers ();                                  // creates defult /etc/passwd and /etc/shadow but only if they don't exist yet
  startWiFi ();
  // startCronDaemon (NULL);                              // sichronize ESP32 time with NTP servers

  myTelnetServer = new telnetServer ( NULL,               // or telnetCommandHandlerCallback if you want to use your own telnet commands
                                      (char *) "0.0.0.0", // telnetServer IP or 0.0.0.0 for all available IPs
                                      23,                 // default telnet port
                                      NULL                // or firewallCallback if you want to use firewall
                                    );
  if (myTelnetServer->state () != telnetServer::RUNNING) Serial.printf ("[%10lu] Could not start telnetServer\n", millis ());
}

void loop () {

}
