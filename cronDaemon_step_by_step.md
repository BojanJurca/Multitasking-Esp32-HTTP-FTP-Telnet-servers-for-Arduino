# Step-by-step guide to do something at specific time in ESP32_web_ftp_telnet_server_template

ESP32_web_ftp_telnet_server_template uses two things that enables you run specific routines at specific time. The first is cronDaemon and the second is cron table.
CronDaemon is a special task or process that runs inside ESP32 server and is responsible for two things:
-	The first is to update internal ESP32 clock with NTP server(s) once a day. ESP32 server must be aware of current time first if it wants to execute something at specific time.
-	The second is to constantly check the content of cron table inside ESP32 server memory and calls cornHandler function provided by your program when the time is right. 

Cron table resides in ESP32 server memory and can be filled in two ways:
-	ESP32 server reads /etc/crontab file at start up and fills cron table in memory with its content.
-	You can also manage cron table through your code by calling cronTabAdd and cronTabDel functions from your code.

 
## 1. A minimal cron example (a working example)
In this example we are only going to start cronDaemon. Since we are not going to provide cronHandler function the only task of cronDaemno would be to synchronise ESP32 server’s time with NTP server(s) once a day.

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
CronDaemon doesn’t handle cron table commands by itself. We’ll have to provide cronHandler function that would do this task. This example will only handle cron commands in /etc/crontab file so we will also start telnet server that will enable us to edit /etc/crontab file. 

```C++


```

Output examlpe:
```
Current local time is 2021/07/25 20:52:31
```

