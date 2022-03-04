/*
    Control and monitor ESP32 pin values with telnet
*/

#include <WiFi.h>

  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command

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

  // telnetServer is using user_management.h to handle login/logout
    #define USER_MANAGEMENT NO_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // finally include telnetServer.hpp
  #include "./servers/telnetServer.hpp"


  String telnetCommandHandlerCallback (int argc, char *argv [], telnetConnection *tcn) { // note that each Telnet connection runs in its own thread/task, more of them may run simultaneously so this function should be reentrant

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))    
    #define LED_BUILTIN 2
    
    // process 'turn led on' and 'turn led off' commands - these commands return immediately with a response to the user
    if (argv0is ("turn") && argv1is ("led") && argv2is ("on")) {
      digitalWrite (LED_BUILTIN, HIGH);
      return "LED is on.";  // response for the user, a response different from "" will also tell telnet server not to try to handle the command itself
    } 
    if (argv0is ("turn") && argv1is ("led") && argv2is ("off")) {
      digitalWrite (LED_BUILTIN, LOW);
      return "LED is on.";  // response for the user, a response different from "" will also tell telnet server not to try to handle the command itself
    } 
  
    // process 'monitor led' command - this command runs for a longer time, it should display intermediate resuts and the user should be able to stop execution anytime
    if (argv0is ("monitor") && argv1is ("led")) {
      if (sendAll (tcn->getSocket (), (char *) "Press Ctrl-C to stop monitoring.\r\n", strlen ("Press Ctrl-C to stop monitoring.\r\n"), TELNET_CONNECTION_TIME_OUT) <= 0) // display text while executing the command
        return "Connection is closed.";                             // stop execution since telnet connection is already closed
      while (true) {
        char *s = digitalRead (LED_BUILTIN) ? (char *) "Led is on.\r\n" : (char *) "Led is off.\r\n";
        if (sendAll (tcn->getSocket (), s, strlen (s), TELNET_CONNECTION_TIME_OUT) <= 0) return "Connection is closed."; // display intermediate result, stop execution if telnet connection is already closed
        unsigned long startMillis = millis ();
        while (millis () - startMillis < 1000) { // display period = 1 s = 1000 ms
          delay (100);
          if (tcn->readTelnetChar (true)) { // just peek if a character is waiting to be read
            char c = tcn->readTelnetChar (false); // actually read the character
            if (c == 3) return "\r\nCtrl-C"; // return if user pressed Ctrl-C
          } 
          if (tcn->connectionTimeOut ()) { 
            sendAll (tcn->getSocket (), (char *) "\r\nTime-out", strlen ("\r\nTime-out"), TELNET_CONNECTION_TIME_OUT); 
            tcn->closeConnection (); 
            return "Connection is closed."; 
          }
        }
      }
    } 

    // let the Telnet server handle command itself by returning ""
    return "";
  }


  telnetServer *myTelnetServer; 

void setup () {
  Serial.begin (115200);

  startWiFi ();

  myTelnetServer = new telnetServer ( telnetCommandHandlerCallback, // use your own telnet commands
                                      (char *) "0.0.0.0",           // telnetServer IP or 0.0.0.0 for all available IPs
                                      23,                           // default telnet port
                                      NULL
                                    );
  if (myTelnetServer->state () != telnetServer::RUNNING) Serial.printf ("[%10lu] Could not start telnetServer\n", millis ());

  pinMode (LED_BUILTIN, OUTPUT);
}

void loop () {

}
