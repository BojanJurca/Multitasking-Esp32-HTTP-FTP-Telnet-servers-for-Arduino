# Step-by-step guide to FTP user interface to ESP32_web_ftp_telnet_server_template
There is just one step. FTP server is weather included or not. There is nothing else you can do in your code than run or stop FTP server. Don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme | ...).

## FTP server (a working example)

```C++
#include <WiFi.h>

// configure your ESP32 server

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here - this is how your ESP32 server will present itself to network
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

// file system holds all the files 

#include "./servers/file_system.h"

// configure your network connection - we are only going to connect to WiFi router in STAtion mode and not going to set up an AP mode in this example

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"      // must have at least 8 characters 
#include "./servers/network.h"

// configure the way ESP32 server is going to deal with users - FTP sever may require loging in first

// #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"

// configure the way ESP32 server is going to handle time

  // configure local time zone - this affects the way FTP displays file time
  
  // define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
  // ... or some time zone in between - see time_functions.h ...
  // #define TIMEZONE  EASTERN_TIMEZONE
  // (default) #define TIMEZONE  CET_TIMEZONE               
  
  #include "./servers/time_functions.h"     

// include ftpServer.hpp

#include "./servers/ftpServer.hpp"                           // include FTP server


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist
  
  // start FTP server
  ftpServer *ftpSrv = new ftpServer ((char *) "0.0.0.0",              // start FTP server on all available ip addresses
                                     21,                              // controll connection FTP port
                                     NULL);                           // no firewall
  if (!ftpSrv || (ftpSrv && !ftpSrv->started ())) ftpDmesg ("[ftpServer] did not start.");
  
  // add your own code as needed ...
  
}

void loop () {
                
}
```

Output examlpe:
```
C:\>ftp <your ESP32 IP>
Connected to 10.18.1.114.
220-MyESP32Server FTP server - please login
220
200 UTF8 enabled
User (10.18.1.114:(none)): root
331 enter password
Password: rootpassword
230 logged on, your home directory is "/"
ftp> ls
200 port ok
150 starting transfer
drw-rw-rw-   1 root     root                0  Jan 01 01:00      etc
drw-rw-rw-   1 root     root                0  Jan 01 01:00      var
drw-rw-rw-   1 root     root                0  Jan 01 01:00      network
226 transfer complete
ftp: 217 bytes received in 0.04Seconds 4.93Kbytes/sec.
ftp>
```
