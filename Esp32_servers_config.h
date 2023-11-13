// Esp32_web_ftp_telnet_server_template configuration
// you can skip some files #included if you don't need the whole functionality


// uncomment the following line to get additional compile-time information about how the project gets compiled
// #define SHOW_COMPILE_TIME_INFORMATION


// include version_of_servers.h to include version information
#include "./servers/version_of_servers.h"
// include dmesg_functions.h which is useful for run-time debugging - for dmesg telnet command
#include "./servers/dmesg_functions.h"


// 1. TIME:    #define which time settings, wil be used with time_functions.h - will be included later
    // define which 3 NTP servers will be called to get current GMT (time) from
    // this information will be written into /etc/ntp.conf file if file_system.h will be included
    #define DEFAULT_NTP_SERVER_1                      "1.si.pool.ntp.org"   // <- replace with your information
    #define DEFAULT_NTP_SERVER_2                      "2.si.pool.ntp.org"   // <- replace with your information
    #define DEFAULT_NTP_SERVER_3                      "3.si.pool.ntp.org"   // <- replace with your information
    // define time zone to calculate local time from GMT
    #define TZ                                        "CET-1CEST,M3.5.0,M10.5.0/3" // default: Europe/Ljubljana, or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv


// 2. FILE SYSTEM:     #define which file system you want to use 
    // the file system must correspond to Tools | Partition scheme setting: FILE_SYSTEM_FAT (for FAT partition scheme), FILE_SYSTEM_LITTLEFS (for SPIFFS partition scheme) or FILE_SYSTEM_SD_CARD (if SC card is attached)
    // FAT file system can be bitwise combined with FILE_SYSTEM_SD_CARD, like #define FILE_SYSTEM (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)

    #define FILE_SYSTEM   FILE_SYSTEM_LITTLEFS // LittleFS uses the least of memory, which may be needed for all the functionalities to run


// 3. NETWORK:     #define how ESP32 will use the network
    // STA(tion)
    // #define how ESP32 will connecto to WiFi router
    // this information will be written into /etc/wpa_supplicant/wpa_supplicant.conf file if file_system.h will be included
    // if these #definitions are missing STAtion will not be set up
    #define DEFAULT_STA_SSID                          "YOUR_STA_SSID"       // <- replace with your information
    #define DEFAULT_STA_PASSWORD                      "YOUR_STA_PASSWORD"   // <- replace with your information
    // the use of DHCP or static IP address wil be set in /network/interfaces if file_system.h will be included, the following is information needed for static IP configuration
    // if these #definitions are missing DHCP will be assumed
    // #define DEFAULT_STA_IP                            "10.18.1.200"      // <- replace with your information
    // #define DEFAULT_STA_SUBNET_MASK                   "255.255.255.0"    // <- replace with your information
    // #define DEFAULT_STA_GATEWAY                       "10.18.1.1"        // <- replace with your information
    // #define DEFAULT_STA_DNS_1                         "193.189.160.13"   // <- replace with your information
    // #define DEFAULT_STA_DNS_2                         "193.189.160.23"   // <- replace with your information

    // A(ccess) P(oint)
    // #define how ESP32 will set up its access point
    // this information will be writte into /etc/dhcpcd.conf and /etc/hostapd/hostapd.conf files if file_system.h will be included
    // if these #definitions are missing Access Point will not be set up
    // #define DEFAULT_AP_SSID                           HOSTNAME           // <- replace with your information,
    // #define DEFAULT_AP_PASSWORD                       "YOUR_AP_PASSWORD" // <- replace with your information, at least 8 characters
    // #define DEFAULT_AP_IP                             "192.168.0.1"      // <- replace with your information
    // #define DEFAULT_AP_SUBNET_MASK                    "255.255.255.0"    // <- replace with your information

    // define the name Esp32 will use as its host name 
    #define HOSTNAME                                   "MyEsp32Server"      // <- replace with your information,  max 32 bytes
    #define MACHINETYPE                                "ESP32 Dev Module"   // <- replace with your information, machine type, it is only used in uname telnet command


// 4. USERS:     #define what kind of user management you want before #including user_management.h
    #define USER_MANAGEMENT                           UNIX_LIKE_USER_MANAGEMENT // or HARDCODED_USER_MANAGEMENT or NO_USER_MANAGEMENT
    // if UNIX_LIKE_USER_MANAGEMENT is selected you must also include file_system.h to be able to use /etc/passwd and /etc/shadow files
    #define DEFAULT_ROOT_PASSWORD                     "rootpassword"        // <- replace with your information if UNIX_LIKE_USER_MANAGEMENT or HARDCODED_USER_MANAGEMENT are used
    #define DEFAULT_WEBADMIN_PASSWORD                 "webadminpassword"    // <- replace with your information if UNIX_LIKE_USER_MANAGEMENT is used
    #define DEFAULT_USER_PASSWORD                     "changeimmediatelly"  // <- replace with your information if UNIX_LIKE_USER_MANAGEMENT is used


// 5. #include (or comment-out) the functionalities you want (or don't want) to use
#include "./servers/fileSystem.hpp"                 // most functionalities can run even without a file system if everything is stored in RAM (smaller web pages, ...)   
#include "./servers/time_functions.h"               // fileSystem.hpp is needed prior to #including time_functions.h if you want to store the default parameters
#include "./servers/network.h"                      // file>ystem.hpp is needed prior to #including network.h if you want to store the default parameters
#include "./servers/httpClient.h"                   // support to access web pages from other servers and curl telnet command
#include "./servers/ftpClient.h"                    // fileSystem.hpp is needed prior to #including ftpClient.h if you want to store the default parameters
#include "./servers/smtpClient.h"                   // fileSystem.hpp is needed prior to #including smtpClient.h if you want to store the default parameters
#include "./servers/userManagement.hpp"             // fileSystem.hpp is needed prior to #including userManagement.hpp in case of UNIX_LIKE_USER_MANAGEMENT
#include "./servers/telnetServer.hpp"               // needs almost all the above files for whole functionality, but can also work without them
#include "./servers/ftpServer.hpp"                  // fileSystem.hpp is also necessary to use ftpServer.h
#include "./servers/oscilloscope.h"                 // web based oscilloscope: you must #include httpServer.hpp as well to use it
#define WEB_SESSIONS                                // comment this line out if you won't use web sessions
#include "./servers/httpServer.hpp"                 // fileSystem.hpp is needed prior to #including httpServer.h if you want server also to serve .html and other files from built-in flash disk


// uncomment the following line to get additional compile-time information about how the project gets compiled
// #define SHOW_COMPILE_TIME_INFORMATION


// backward compatibility
#if (FILE_SYSTEM & FILE_SYSTEM_FAT) == FILE_SYSTEM_FAT || (FILE_SYSTEM & FILE_SYSTEM_LITTLEFS) == FILE_SYSTEM_LITTLEFS
    [[deprecated("Replaced by fileSystem.mount(bool)")]]
    bool mountFileSystem (bool formatIfUnformatted) { return fileSystem.mount (formatIfUnformatted); }
#endif

[[deprecated("Replaced by userManagement.initialize()")]]
void initializeUsers () { userManagement.initialize (); }

