# ESP32 with HTTP server, Telnet server, file system, FTP server FTP client, SMTP client, cron daemon and user management.


**It is more or less about the Web user interface for ESP32 projects. Demo ESP32 server is available at [193.77.159.208](http://193.77.159.208)**


![Screenshot](presentation.gif)


## What is new in version 2.0 beta 1?


Honestly, not much, only a FTP client and HTTP sessions (login/logout). But since API interface has slightly changed there is a version 2.0. I do not find the time to write manuals, please check examples and the code itself. It is pretty straightforward.
Beside this, the performance, efficiency and stability have been largely improved. An IDF version 4.4 is also supported.


## HTTP server


HTTP server can handle HTTP requests in two different ways. As a programmed response to (for example REST) requests or by sending .html files from /var/www/html directory. Cookies and WebSockets are also supported to some extent.

**HTTP server performance** 

![HTTP server performance](performance.gif)


## Telnet server


Telnet server can, similarly to HTTP server, handle commands in two different ways. As a programmed response to some commands or already built-in commands (like ls, ping, ...). There is also a very basic text-editor built in for editing small configuration files.


## FTP server


FTP server is needed for uploading configuration files, .html files, ... to ESP32 file system. Both active and passive modes are supported.


## Time zones


time_functions.h provides GMT to local time conversion from 35 different time zones. #define TIMEZONE to one of the supported time zones or modify timeToLocalTime function yourself. 


## Configuration files


```C++
   - /etc/passwd                                     - Contains users' accounts information.
   - /etc/shadow                                     - Contains hashed users' passwords.
   - /network/interfaces                             - Contains WiFi STA(tion) configuration.
   - /etc/wpa_supplicant/wpa_supplicant.conf         - Contains WiFi STA(tion) credentials.
   - /etc/dhcpcd.conf                                - Contains WiFi A(ccess) P(oint) configuration.
   - /etc/hostapd/hostapd.conf                       - Contains WiFi A(ccess) P(oint) credentials.
   - /etc/ntp.conf                                   - Contains NTP time servers names.
   - /etc/crontab                                    - Contains scheduled tasks.
   - /etc/mail/sendmail.cf                           - contains sendMail default settings.
   - /etc/ftp/ftpclient.cf                           - contains ftpPut and ftpGet default settings.
```


## Setup instructions


1. Copy all files in this package into Esp32_web_ftp_telnet_server_template directory.2. Open Esp32_web_ftp_telnet_server_template.ino with Arduino IDE.3. modify (some or all) the default #definitions (that will be later written to configuration files) **before** the sketch is run for the first time:

```C++
#include "./servers/dmesg_functions.h"
#include "./servers/perfMon.h"         // #include perfMon.h prior to other modules to make sure you're monitoring everything#include "./servers/file_system.h"  
  // #define network parameters before #including network.h  
  #define HOSTNAME                                  "MyESP32Server"
  #define DEFAULT_STA_SSID                          "YOUR_STA_SSID"  
  #define DEFAULT_STA_PASSWORD                      "YOUR_STA_PASSWORD"  
  #define DEFAULT_AP_SSID                           "" // HOSTNAME - leave empty if you don't want to use AP
  #define DEFAULT_AP_PASSWORD                       "" // "YOUR_AP_PASSWORD" - at least 8 characters  
  // ... add other #definitions from network.h
#include "./servers/network.h"                      // file_system.h is needed prior to #including network.h if you want to store the default parameters  
  // #define how you want to calculate local time and which NTP servers GMT will be synchronized with before #including time_functions.h  
  #define DEFAULT_NTP_SERVER_1                      "1.si.pool.ntp.org"  
  #define DEFAULT_NTP_SERVER_2                      "2.si.pool.ntp.org"  
  #define DEFAULT_NTP_SERVER_3                      "3.si.pool.ntp.org"  
  #define TIMEZONE CET_TIMEZONE                     // or another one supported in time_functions.h
#include "./servers/time_functions.h"               // file_system.h is needed prior to #including time_functions.h if you want to store the default parameters#include "./servers/httpClient.h"
#include "./servers/ftpClient.h"                    // file_system.h is needed prior to #including ftpClient.h if you want to store the default parameters#include "./servers/smtpClient.h"                   // file_system.h is needed prior to #including smtpClient.h if you want to store the default parameters  
  // #define what kind of user management you want before #including user_management.h  
  #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT // NO_USER_MANAGEMENT
#include "./servers/user_management.h"              // file_system.h is needed prior to #including user_management.h in case of UNIX_LIKE_USER_MANAGEMENT  
  // #define machint type, it is only used in uname telnet command  
  #define MACHINETYPE                               "ESP32 Dev Modue"#include "./servers/telnetServer.hpp"               // needs almost all the above files for the whole functionality
#include "./servers/ftpServer.hpp"                  // file_system.h is also necessary to use ftpServer.h
#include "./servers/httpServer.hpp"                 // file_system.h is needed prior to #including httpServer.h if you want server also to serve .html and other files
```

4. Select one of FAT partition schemas (Tool | Partition Scheme).
5. Compile the sketch and run it on your ESP32. Doing this the following will happen:

   - ESP32 flash memory will be formatted with the FAT file system. WARNING: every information you have stored into ESP32’s flash memory will be lost.
   - Configuration files will be created with the default settings.
   - Two users will be created: **root** with **rootpassword** and **webadmin** with **webadminpassword**.

At this point, you can already test if everything is going on as planned by http, FTP or telnet to your ESP32. Your ESP32 is already working as a server but there are a few minor things yet left to be done.

6. FTP (demo and example: index.html, ...) files from html directory to ESP32's /var/www/html/ directory.
7. Delete all the examples and functionalities that don't need and all the references to them in the code. They are included just to make the development easier for you.


## Debugging the code 


Telnet server provides Unix/Linux like dmesg circular message queue. You can monitor your ESP32 behaviour even when it is not connected to a computer with a USB cable. In your C++ code use dmesg () function to insert important messages about the state of your code into a circular queue. When you want to view it, connect to your ESP32 with Telnet client and type dmesg command.


![Screenshot](dmesg.png)


## Debugging the signals


ESP32 oscilloscope is a web-based application included in Esp32_web_ftp_telnet_server_template. It is accessible through a web browser once oscilloscope.html is uploaded to ESP32's /var/www/html directory.


![Screenshot](oscilloscope.png)
