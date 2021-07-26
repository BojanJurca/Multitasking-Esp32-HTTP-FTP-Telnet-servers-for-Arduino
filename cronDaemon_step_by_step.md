# Step-by-step guide to do something at specific time in ESP32_web_ftp_telnet_server_template

ESP32_web_ftp_telnet_server_template uses two things that enables you to run specific routines at specific time. The first is cronDaemon and the second is cron table.
CronDaemon is a special task or process that runs inside ESP32 server and is responsible for two things:
-	The first is to update internal ESP32 clock with NTP server(s) once a day. ESP32 server must be aware of current time first if it wants to execute something at specific time.
-	The second is to constantly check the content of cron table inside ESP32 server memory and calls cornHandler function provided by your program when the time is right. 

Cron table resides in ESP32 server memory and can be filled in two ways:
-	ESP32 server reads /etc/crontab file at start up and fills cron table in memory with its content.
-	You can also manage cron table through your code by calling cronTabAdd and cronTabDel functions from your code.

 
## 1. A minimal cron example (a working example)
In this example we are only going to start cronDaemon. Since we are not going to provide cronHandler function the only task of cronDaemon would be to synchronise ESP32 server’s time with NTP server(s) once a day.

```C++
#include <WiFi.h>

// configure your ESP32 server

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here - this is how your ESP32 server will present itself to network, this text is also used by uname telnet command
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here - this text is only used by uname telnet command

// file system holds all configuration files like /etc/ntp.conf or /etc/crontab

#include "./servers/file_system.h"

// configure your network connection - we are only going to connect to WiFi router in STAtion mode and not going to set up an AP mode in this example

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"      // must have at least 8 characters 
#include "./servers/network.h"

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

void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  startCronDaemonAndInitializeItAtFirstCall (NULL, 3 * 1024);  // creates /etc/ntp.conf with default NTP server names and synchronize ESP32 time with them once a day
                                                                      // creates empty /etc/crontab, reads it at start up and executes cronHandler when the time is right
                                                                      // 3 KB stack size is minimal requirement for NTP time synchronization, add more if your cronHandler requires more

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist

  // add your own code as needed ...
  
}

void loop () {
  time_t now = getLocalTime ();
  if (!now) Serial.println ("Didn't get time from NTP server(s) yet.");
  else      Serial.println ("Current local time is " + timeToString (now));
  delay (1000);                
}
```

Output examlpe:
```
[        50] [cronDaemon] started.
[        83] [network] starting WiFi
[        83] [network] [STA] connecting STAtion to router using DHCP.
[       199] [network] [STA] WiFi client started.
Didn't get time from NTP server(s) yet.
Didn't get time from NTP server(s) yet.
Didn't get time from NTP server(s) yet.
[      2301] [network] [STA] connected to WiFi.
[      3114] [network] [STA] got IP address: 10.18.1.114.
Didn't get time from NTP server(s) yet.
Didn't get time from NTP server(s) yet.
Didn't get time from NTP server(s) yet.
[     15065] [time] synchronized with 1.si.pool.ntp.org
[     15065] [time] time set to 2021/07/25 20:52:29.
Current local time is 2021/07/25 20:52:29
Current local time is 2021/07/25 20:52:30
Current local time is 2021/07/25 20:52:31
```


## 2. Handling /etc/crontab (a working example)
CronDaemon doesn’t handle cron table commands by itself. We’ll have to provide cronHandler function that would do this task. This example will only handle cron commands in /etc/crontab file so we will also start telnet server that will enable us to edit /etc/crontab file with the help of vi command. 

```C++
#include <WiFi.h>

// configure your ESP32 server

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here - this is how your ESP32 server will present itself to network, this text is also used by uname telnet command
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here - this text is only used by uname telnet command

// file system holds all configuration files like /etc/ntp.conf or /etc/crontab

#include "./servers/file_system.h"

// configure your network connection - we are only going to connect to WiFi router in STAtion mode and not going to set up an AP mode in this example

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"      // must have at least 8 characters 
#include "./servers/network.h"

// provide user management for telnet server

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


// cronHandller function
void cronHandler (String& cronCommand) {

  // handle cron command
  if (cronCommand == "execute15secondsPastEachMinute")
    Serial.println (cronCommand + " executed at " + timeToString (getLocalTime ()));

}


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  startCronDaemonAndInitializeItAtFirstCall (cronHandler, 8 * 1024);  // creates /etc/ntp.conf with default NTP server names and synchronize ESP32 time with them once a day
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

First we have to edit /etc/crontab file. Its structure is pretty much like any Unix/Linux crontab file (with minor exception that also a field for seconds is added):

```
C:\>telnet <your ESP32  IP>
Hello 10.18.1.88!
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# vi /etc/crontab

---+---------------------------------------------------------------------------------------- Save: Ctrl-S, Exit: Ctrl-X
  1|# scheduled tasks (in local time) - reboot for changes to take effect
  2|#
  3|# .------------------- second (0 - 59 or *)
  4|# |  .---------------- minute (0 - 59 or *)
  5|# |  |  .------------- hour (0 - 23 or *)
  6|# |  |  |  .---------- day of month (1 - 31 or *)
  7|# |  |  |  |  .------- month (1 - 12 or *)
  8|# |  |  |  |  |  .---- day of week (0 - 7 or *; Sunday=0 and also 7)
  9|# |  |  |  |  |  |
 10|# *  *  *  *  *  * cronCommand to be executed
 11|# * 15  *  *  *  * exampleThatRunsQuaterPastEachHour
 12| 15  *  *  *  *  * execute15secondsPastEachMinute
   |
   |
---+--------------------------------------------------------------------------------------------------------------------

# reboot
```

Output examlpe:
```
[     15083] [time] time set to 2021/07/25 21:26:24.
execute15secondsPastEachMinute executed at 2021/07/25 21:27:15
execute15secondsPastEachMinute executed at 2021/07/25 21:28:15
execute15secondsPastEachMinute executed at 2021/07/25 21:29:15
```


## 3. Managing cron table content from code (a working example)
You can use cronTabAdd and cronTabDel functions in your code to control the content of the cron table. This comes handy when you have to calculate a plan of work for example.


```C++
#include <WiFi.h>

// configure your ESP32 server

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here - this is how your ESP32 server will present itself to network, this text is also used by uname telnet command
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here - this text is only used by uname telnet command

// file system holds all configuration files like /etc/ntp.conf or /etc/crontab

#include "./servers/file_system.h"

// configure your network connection - we are only going to connect to WiFi router in STAtion mode and not going to set up an AP mode in this example

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"      // must have at least 8 characters 
#include "./servers/network.h"

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


// cronHandller function
void cronHandler (String& cronCommand) {

  // handle cron command
  if (cronCommand == "gotTime" || cronCommand == "calculatePlanOfTheDay") {
    
    // this demo is not about calculating Sun set, so we'll just assume we have
    // calculated that it happens at 17:45 (local time)
    time_t sunSet = 1627321518; // = 2021/07/26 17:45:18 (local time)
    Serial.printf ("sunSet = %lu\n", sunSet);
    // delete yesterday's sunSet cron command from cron table
    cronTabDel ("sunSet");
    // add today's sunSet command to cron table
    struct tm structSunSet = timeToStructTime (sunSet);
    cronTabAdd (structSunSet.tm_sec, structSunSet.tm_min, structSunSet.tm_hour, ANY, ANY, ANY, "sunSet");
    Serial.println ("Plan of the day has been calculated.");
    
  } else if (cronCommand == "sunSet") {

    // do something at Sun set each day
    Serial.println ("Sun set at " + timeToString (getLocalTime ()) +  " in case you want to do something about that.");
  
  }
    
}


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  startCronDaemonAndInitializeItAtFirstCall (cronHandler, 8 * 1024);  // creates /etc/ntp.conf with default NTP server names and synchronize ESP32 time with them once a day
                                                                      // creates empty /etc/crontab, reads it at start up and executes cronHandler when the time is right
                                                                      // 3 KB stack size is minimal requirement for NTP time synchronization, add more if your cronHandler requires more

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist


  // add two cron commands to cron table
  cronTabAdd ("* * * * * * gotTime");               // this command will execute only once - the first time cronDaemon gets current time - see cronHandler
  cronTabAdd ("0 0 0 * * * calculatePlanOfTheDay"); // this command will execute at the beginning of each day - see cronHandler   

  
  // add your own code as needed ...
  
}

void loop () {

}
```

Output examlpe:
```
[      2311] [network] [STA] connected to WiFi.
[      3128] [network] [STA] got IP address: 10.18.1.114.
[     15079] [time] synchronized with 1.si.pool.ntp.org
[     15080] [time] time set to 2021/07/26 17:44:52.
[     16081] [cronDaemon] there are no sunSet commands to delete from cron table.
Plan of the day has been calculated.
Sun set at 2021/07/26 17:45:18 in case you want to do something about that.
```
