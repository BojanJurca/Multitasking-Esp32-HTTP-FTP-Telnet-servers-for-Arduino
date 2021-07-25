# Step-by-step guide to telnet user interface to ESP32_web_ftp_telnet_server_template

Telnet user interface to ESP32 Arduino project is easy to implement. It can be done in almost not time. It is especially useful while testing certain functionalities during development for example.
Don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme |  ...).
 
## 1. A minimal telnet server (a working example)
This is the minimum you must do to make telnet server running. It won't create telnet user interface to your project, but it will enable most of the telnet built-in commands to work (quit, clear, help, uname, uptime, reboot, reset, date, ntpdate, free, dmesg, mkfs.fat, fs_info, ls, tree, mkdir, rmdir, cd, pwd, cat, vi, cp, mv, rm, passwd, useradd, userdel, ifconfig, iw, arp, ping, telnet, curl).

```C++
#include <WiFi.h>

// configure your ESP32 server

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here - this is how your ESP32 server will present itself to network, this text is also used by uname telnet command
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here - this text is only used by uname telnet command

// file system holds all configuration files and also some telnet commands (cp, rm ...) deal with files so this is necessary (don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme |  ... first)

#include "./servers/file_system.h"

// configure your network connection - we are only going to connect to WiFi router in STAtion mode and not going to set up an AP mode in this example

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"      // must have at least 8 characters 
#include "./servers/network.h"

// configure the way ESP32 server is going to deal with users - telnet sever may require loging in first, this module also defines the location of telnet help.txt file

// #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"

// configure the way ESP32 server is going to handle time

  // configure local time zone - this affects the way telnet commands display your local time
  
  // define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
  // ... or some time zone in between - see time_functions.h ...
  // #define TIMEZONE  EASTERN_TIMEZONE
  // (default) #define TIMEZONE  CET_TIMEZONE               
  
  // define NTP servers ESP32 server is going to use for time sinchronization
  
  #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
  #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"     

// include telnetServer.hpp

#include "./servers/telnetServer.hpp"                           // include Telnet server


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  // uncomment the following line if you want your ESP32 server to synchronize its time with NTP server authomatically
  // startCronDaemonAndInitializeItAtFirstCall (cronHandler, 8 * 1024);  // creates /etc/ntp.conf with default NTP server names and synchronize ESP32 time with them once a day
                                                                      // creates empty /etc/crontab, reads it at start up and executes cronHandler when the time is right
                                                                      // 3 KB stack size is minimal requirement for NTP time synchronization, add more if your cronHandler requires more

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist
  
  // start telnet server
  telnetServer *telnetSrv = new telnetServer (NULL, // no telnetCommandHandler 
                                              16 * 1024,              // 16 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stable
                                              "0.0.0.0",              // start telnet server on all available ip addresses
                                              23,                     // telnet port
                                              NULL);                  // use firewall callback function for telnet server (replace with NULL if not needed)
  if (!telnetSrv || (telnetSrv && !telnetSrv->started ())) dmesg ("[telnetServer] did not start.");

  // add your own code as needed ...
  
}

void loop () {
                
}
```

A stack of 16 KB seems a lot at first glance but there are only two telnet commands that require this amount of stack: tree and vi. If you decide not to use these commands in your project stack may be much smaller.

Output examlpe:
```
c:\>telnet <YOUR ESP32 IP>
Hello 10.18.1.88!
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# tree
drw-rw-rw-   1 root     root                0  Jan 01 00:00      /
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /etc
-rw-rw-rw-   1 root     root              119  Jan 01 01:00      /etc/passwd
-rw-rw-rw-   1 root     root              166  Jan 01 01:00      /etc/shadow
-rw-rw-rw-   1 root     root              151  Jan 01 01:00      /etc/dhcpcd.conf
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /etc/wpa_supplicant
-rw-rw-rw-   1 root     root              126  Jan 01 01:00      /etc/wpa_supplicant/wpa_supplicant.conf
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /etc/hostapd
-rw-rw-rw-   1 root     root              152  Jan 01 01:00      /etc/hostapd/hostapd.conf
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /var
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /var/www
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /var/www/html
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /var/telnet
drw-rw-rw-   1 root     root                0  Jan 01 01:00      /network
-rw-rw-rw-   1 root     root              294  Jan 01 01:00      /network/interfaces
#
```

## 2. Handle (some of the) telnet commands inside your project (a working example)
This example shows how you can handle some telnet commands from your code by adding telnetCommandHandler function. This method is suitable for commands that return quickly.

```C++
#include <WiFi.h>

// configure your ESP32 server

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here - this is how your ESP32 server will present itself to network, this text is also used by uname telnet command
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here - this text is only used by uname telnet command

// file system holds all configuration files and also some telnet commands (cp, rm ...) deal with files so this is necessary (don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme |  ... first)

#include "./servers/file_system.h"

// configure your network connection - we are only going to connect to WiFi router in STAtion mode and not going to set up an AP mode in this example

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"      // must have at least 8 characters 
#include "./servers/network.h"

// configure the way ESP32 server is going to deal with users - telnet sever may require loging in first, this module also defines the location of telnet help.txt file

// #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"

// configure the way ESP32 server is going to handle time

  // configure local time zone - this affects the way telnet commands display your local time
  
  // define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
  // ... or some time zone in between - see time_functions.h ...
  // #define TIMEZONE  EASTERN_TIMEZONE
  // (default) #define TIMEZONE  CET_TIMEZONE               
  
  // define NTP servers ESP32 server is going to use for time sinchronization
  
  #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
  #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"     

// include telnetServer.hpp

#include "./servers/telnetServer.hpp"                           // include Telnet server


String telnetCommandHandler (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) { // - must be reentrant!

  // handle "turn led on" command:
  
  if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {
    digitalWrite (2, HIGH);
                          // tell telnetServer that the command has already been handeled by returning something else than "" 
    return "LED is on.";  // this text will be returned to telnet client as a response to "turn led on" command 
    
  } 

  return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  // uncomment the following line if you want your ESP32 server to synchronize its time with NTP server authomatically
  // startCronDaemonAndInitializeItAtFirstCall (cronHandler, 8 * 1024);  // creates /etc/ntp.conf with default NTP server names and synchronize ESP32 time with them once a day
                                                                      // creates empty /etc/crontab, reads it at start up and executes cronHandler when the time is right
                                                                      // 3 KB stack size is minimal requirement for NTP time synchronization, add more if your cronHandler requires more

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist
  
  // start telnet server
  telnetServer *telnetSrv = new telnetServer (telnetCommandHandler, // <- pass address of your telnet command handler function to telnet server 
                                              16 * 1024,              // 16 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stable
                                              "0.0.0.0",              // start telnet server on all available ip addresses
                                              23,                     // telnet port
                                              NULL);                  // use firewall callback function for telnet server (replace with NULL if not needed)
  if (!telnetSrv || (telnetSrv && !telnetSrv->started ())) dmesg ("[telnetServer] did not start.");

  // add your own code as needed ...
  
}

void loop () {
                
}
```

Output examlpe:
```
c:\>telnet <YOUR ESP32 IP>
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# turn led on
LED is on.
#
```


# ... to be continued …



## 3. Handle long running telnet commands inside your project (a working example)
If you intend to run telnet commands that run longer but want to display intermediate results before they end, you will have to use telnetSessionParameters.

