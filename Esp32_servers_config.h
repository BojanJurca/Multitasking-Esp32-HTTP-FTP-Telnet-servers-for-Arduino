// Esp32_web_ftp_telnet_server_template configuration
// you can skip some files #included if you don't need the whole functionality


        // Lightweight STL memory settings
        // #define LIST_MEMORY_TYPE          PSRAM_MEM
        // #define VECTOR_QUEUE_MEMORY_TYPE  PSRAM_MEM // please note that this also affect keyValueDatabase
        // #define MAP_MEMORY_TYPE           PSRAM_MEM // please note that this also affect keyValueDatabase
        // bool psramInitialized = psramInit ();

        // locale settings for Cstrings
        // #include "servers/std/locale.hpp"
        // bool localeSet = setlocale (lc_all, "sl_SI.UTF-8");


// uncomment the following line to get additional compile-time information about how the project gets compiled
// #define SHOW_COMPILE_TIME_INFORMATION


// 1. TIME:    #define which time settings, wil be used with time_functions.h - will be included later
    // define which 3 NTP servers will be called to get current GMT (time) from
    // this information will be written into /etc/ntp.conf file if file_system.h will be included
    #define DEFAULT_NTP_SERVER_1                      "1.si.pool.ntp.org"   // <- replace with your information
    #define DEFAULT_NTP_SERVER_2                      "2.si.pool.ntp.org"   // <- replace with your information
    #define DEFAULT_NTP_SERVER_3                      "3.si.pool.ntp.org"   // <- replace with your information
    // define time zone to calculate local time from GMT
    #define TZ                                        "CET-1CEST,M3.5.0,M10.5.0/3" // default: Europe/Ljubljana, or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv


// 2. FILE SYSTEM:     #define which file system you want to use 
    // the file system must correspond to Tools | Partition scheme setting: FILE_SYSTEM_FAT (for FAT partition scheme), FILE_SYSTEM_LITTLEFS (for SPIFFS partition scheme) or FILE_SYSTEM_SD_CARD (if SD card is attached)
    // FAT file system can be bitwise combined with FILE_SYSTEM_SD_CARD, like #define FILE_SYSTEM (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD), LITTLEFS uses less memory

    #define FILE_SYSTEM   FILE_SYSTEM_LITTLEFS // FILE_SYSTEM_LITTLEFS or FILE_SYSTEM_FAT or FILE_SYSTEM_SD_CARD or FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD


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

    // power saving
    #define POVER_SAVING_MODE   WIFI_PS_NONE // WIFI_PS_NONE or WIFI_PD_MIN_MODEM or WIFI_PS_MAX_MODEM // please check: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html

    // use mDNS so the servers can be accessed in local network by HOSTNAME, but uses some additional memory
    #define USE_mDNS // comment this line out to save some memory and code space

    // define the name Esp32 will use as its host name, if USE_mDNS is #dfined this is also the name by wich you can address your ESP32 on your local network
    #define HOSTNAME                                   "MyESP32Server"      // <- replace with your information,  max 32 bytes (ESP_NETIF_HOSTNAME_MAX_SIZE) - if you are using mDNS make sure host name complies also with mDNS requirements
    // replace MACHINETYPE with your information if you want, it is only used in uname telnet command
    #if CONFIG_IDF_TARGET_ESP32
        #define MACHINETYPE "ESP32"
    #elif CONFIG_IDF_TARGET_ESP32S2
        #define MACHINETYPE "ESP32-S2"
    #elif CONFIG_IDF_TARGET_ESP32S3
        #define MACHINETYPE "ESP32-S3"
    #elif CONFIG_IDF_TARGET_ESP32C3
        #define MACHINETYPE "ESP32-C3"
    #elif CONFIG_IDF_TARGET_ESP32C6
        #define MACHINETYPE "ESP32-C6"
    #elif CONFIG_IDF_TARGET_ESP32H2
        #define MACHINETYPE "ESP32-H2"
    #else
        #define MACHINETYPE "ESP32 (other)"
    #endif


// 4. USERS:     #define what kind of user management you want before #including user_management.h
    #define USER_MANAGEMENT                           UNIX_LIKE_USER_MANAGEMENT // NO_USER_MANAGEMENT or HARDCODED_USER_MANAGEMENT or UNIX_LIKE_USER_MANAGEMENT
    // if UNIX_LIKE_USER_MANAGEMENT is selected you must also include file_system.h to be able to use /etc/passwd and /etc/shadow files
    #define DEFAULT_ROOT_PASSWORD                     "rootpassword"        // <- replace with your information if UNIX_LIKE_USER_MANAGEMENT or HARDCODED_USER_MANAGEMENT are used
    #define DEFAULT_WEBADMIN_PASSWORD                 "webadminpassword"    // <- replace with your information if UNIX_LIKE_USER_MANAGEMENT is used
    #define DEFAULT_USER_PASSWORD                     "changeimmediatelly"  // <- replace with your information if UNIX_LIKE_USER_MANAGEMENT is used


// 5. #include (or comment-out) the functionalities you want (or don't want) to use
    #include "./servers/dmesg.hpp"                      // include dmesg_functions.h which is useful for run-time debugging - for dmesg telnet command
    #include "./servers/fileSystem.hpp"                 // most functionalities can run even without a file system if everything is stored in RAM (smaller web pages, ...)   
    #include "./servers/time_functions.h"               // fileSystem.hpp is needed prior to #including time_functions.h if you want to store the default parameters
    #include "./servers/netwk.h"                        // fileSystem.hpp is needed prior to #including network.h if you want to store the default parameters
    #include "./servers/httpClient.h"                   // support to access web pages from other servers and curl telnet command
    #include "./servers/smtpClient.h"                   // fileSystem.hpp is needed prior to #including smtpClient.h if you want to store the default parameters
    #include "./servers/userManagement.hpp"             // fileSystem.hpp is needed prior to #including userManagement.hpp in case of UNIX_LIKE_USER_MANAGEMENT
    #include "./servers/esp32_ping.hpp"                 // include esp32_ping.hpp to occasioanly ping the router to check if ESP32 is still connected to WiFi
    #include "./servers/version_of_servers.h"           // include version_of_servers.h to include version information
    #include "./servers/telnetServer.hpp"               // needs almost all the above files for whole functionality, but can also work without them
    #include "./servers/ftpServer.hpp"                  // fileSystem.hpp is also necessary to use ftpServer.h
    #ifdef FILE_SYSTEM
        #define USE_WEB_SESSIONS // comment this line out to save some memory if you won't use web sessions
    #endif
    #ifdef FILE_SYSTEM
        #define USE_I2S_INTERFACE             // I2S interface improves web based oscilloscope analog sampling (of a single signal) if ESP32 board has one
        // check INVERT_ADC1_GET_RAW and INVERT_I2S_READ #definitions in oscilloscope.h if the signals are inverted
        #include "./servers/oscilloscope.h"   // web based oscilloscope: you must #include httpServer.hpp as well to use it
    #endif
    #include "./servers/httpServer.hpp"       // fileSystem.hpp is needed prior to #including httpServer.h if you want server also to serve .html and other files from built-in flash disk