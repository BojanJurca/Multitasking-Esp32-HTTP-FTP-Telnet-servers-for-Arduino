/*

    netwk.h

    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino

    network.h reads network configuration from file system and sets both ESP32 network interfaces accordingly
 
    It is a little awkward why Unix, Linux are using so many network configuration files and how they are used):

      /network/interfaces                       - modify the code below with your IP addresses
      /etc/wpa_supplicant/wpa_supplicant.conf   - modify the code below with your WiFi SSID and password
      /etc/dhcpcd.conf                          - modify the code below with your access point IP addresses
      /etc/hostapd/hostapd.conf                 - modify the code below with your access point SSID and password

    May 22, 2025, Bojan Jurca

*/


#include <WiFi.h>
// #include <ETH.h> // uncomment if ethernet is needed
#ifdef USE_mDNS
    #include <ESPmDNS.h>
#endif
#include <lwip/netif.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#define EAGAIN 11
#define ENAVAIL 119
#include <lwip/netdb.h>
#include "std/Cstring.hpp"
#include "std/console.hpp"


#ifndef __NETWK__
    #define __NETWK__

        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #ifndef _ETH_H_
                #pragma message "__NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__ __NETWK__"
            #else
                #pragma message "__NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH __NETWK__ with ETH"
            #endif
        #endif


        #ifndef __FILE_SYSTEM__
          #ifdef SHOW_COMPILE_TIME_INFORMATION
              #pragma message "Compiling netwk.h without file system (fileSystem.hpp), netwk.h will not use configuration files"
          #endif
        #endif


        // ----- TUNNING PARAMETERS -----
        #ifndef POVER_SAVING_MODE
            #define POVER_SAVING_MODE   WIFI_PS_NONE // WIFI_PS_NONE or WIFI_PD_MIN_MODEM or WIFI_PS_MAX_MODEM // please check: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html
        #endif


        // ----- missing functions and definitions -----

        #define EAGAIN 11
        #define ENAVAIL 119

        // missing gai_strerror function
        #include <lwip/netdb.h>
        const char *gai_strerror (int err) {
            switch (err) {
                case EAI_AGAIN:     return (const char *) "temporary failure in name resolution";
                case EAI_BADFLAGS:  return (const char *) "invalid value for ai_flags field";
                case EAI_FAIL:      return (const char *) "non-recoverable failure in name resolution";
                case EAI_FAMILY:    return (const char *) "ai_family not supported";
                case EAI_MEMORY:    return (const char *) "memory allocation failure";
                case EAI_NONAME:    return (const char *) "name or service not known";
                // not in lwip: case EAI_OVERFLOW:  return (const char *) "argument buffer overflow";
                case EAI_SERVICE:   return (const char *) "service not supported for ai_socktype";
                case EAI_SOCKTYPE:  return (const char *) "ai_socktype not supported";
                // not in lwip: case EAI_SYSTEM:    return (const char *) "system error returned in errno";
                default:            return (const char *) "invalid gai_errno code";
            }
        }


        // ----- functions and variables in this modul -----

        void startWiFi ();


        // ----- TUNNING PARAMETERS -----

        #ifndef HOSTNAME
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "HOSTNAME was not defined previously, #defining the default MyESP32Server in network.h"
            #endif
          #define HOSTNAME                  "YourESP32Server"  // use default if not defined previously, max 32 bytes (ESP_NETIF_HOSTNAME_MAX_SIZE) - if you are using mDNS make sure host name complies also with mDNS requirements
        #endif
        // define default STAtion mode parameters to be written to /etc/wpa_supplicant/wpa_supplicant.conf if you want to use ESP as WiFi station
        #ifndef DEFAULT_STA_SSID
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_STA_SSID was not defined previously, #defining the default YOUR_STA_SSID which most likely will not work"
            #endif
        #endif
        #ifndef DEFAULT_STA_PASSWORD
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_STA_PASSWORD was not defined previously, #defining the default YOUR_STA_PASSWORD which most likely will not work"
            #endif
          #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
        #endif

        // define default static IPv4, subnet mask and gateway to be written to /network/interfaces if you want ESP to connect to WiFi with static IP instead of using DHCP
        #ifndef DEFAULT_STA_IP
          #ifdef SHOW_COMPILE_TIME_INFORMATION
              #pragma message "DEFAULT_STA_IP was left undefined, DEFAULT_STA_IP is only needed when static IP address is used"
          #endif
          // #define DEFAULT_STA_IP            "10.18.1.200"       // IP that ESP32 is going to use if static IP is configured
        #endif
        #ifndef DEFAULT_STA_SUBNET_MASK
          #ifdef SHOW_COMPILE_TIME_INFORMATION
              #pragma message "DEFAULT_STA_SUBNET_MASK was left undefined, DEFAULT_STA_SUBNET_MASK is only needed when static IP address is used"
          #endif
          // #define DEFAULT_STA_SUBNET_MASK   "255.255.255.0"
        #endif
        #ifndef DEFAULT_STA_GATEWAY
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_STA_GATEWAY was left undefined, DEFAULT_STA_GATEWAY is only needed when static IP address is used"
            #endif
          // #define DEFAULT_STA_GATEWAY       "10.18.1.1"       // your router's IP
        #endif
        #ifndef DEFAULT_STA_DNS_1
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_STA_DNS_1 was left undefined, DEFAULT_STA_DNS_1 is only needed when static IP address is used"
            #endif
          // #define DEFAULT_STA_DNS_1         "193.189.160.13"    // or whatever your internet provider's DNS is
        #endif
        #ifndef DEFAULT_STA_DNS_2
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_STA_DNS_2 was left undefined, DEFAULT_STA_DNS_2 is only needed when static IP address is used"
            #endif
            // #define DEFAULT_STA_DNS_2         "193.189.160.23"    // or whatever your internet provider's DNS is
        #endif  
        // define default static IPv6 to be written to /network/interfaces if you want ESP to connect to WiFi with static IP instead of using DHCP
        #ifndef DEFAULT_STA_IPv6
          #ifdef SHOW_COMPILE_TIME_INFORMATION
              #pragma message "DEFAULT_STA_IPv6 was left undefined, DEFAULT_STA_IPv6 is only needed when static IP address is used"
          #endif
          // #define DEFAULT_STA_IPv6            "fe80::caf0:9eff:fe2d:f740"       // IP that ESP32 is going to use if static IP is configured
        #endif

        // define default Access Point parameters to be written to /etc/hostapd/hostapd.conf if you want ESP to serve as an access point  
        #ifndef DEFAULT_AP_SSID
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_AP_SSID was left undefined, DEFAULT_AP_SSID is only needed when A(ccess) P(point) is used"
            #endif
        #endif
        #ifndef DEFAULT_AP_PASSWORD
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_AP_PASSWORD was not defined previously, #defining the default YOUR_AP_PASSWORD in case A(ccess) P(point) is used"
            #endif
            #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"  // at leas 8 characters if AP is used
        #endif
        // define default access point IP and subnet mask to be written to /etc/dhcpcd.conf if you want to define them yourself
        #ifndef DEFAULT_AP_IP
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_AP_IP was not defined previously, #defining the default 192.168.0.1 in case A(ccess) P(point) is used"
            #endif
            #define DEFAULT_AP_IP               "192.168.0.1"
        #endif
        #ifndef DEFAULT_AP_SUBNET_MASK
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "DEFAULT_AP_SUBNET_MASK was not defined previously, #defining the default 255.255.255.0 in case A(ccess) P(point) is used"
            #endif
            #define DEFAULT_AP_SUBNET_MASK      "255.255.255.0"
        #endif

        #define MAX_INTERFACES_SIZE         512 // how much memory is needed to temporary store /network/interfaces
        #define MAX_WPA_SUPPLICANT_SIZE     512 // how much memory is needed to temporary store /etc/wpa_supplicant/wpa_supplicant.conf
        #define MAX_DHCPCD_SIZE             512 // how much memory is needed to temporary store /etc/dhcpcd.conf
        #define MAX_HOSTAPD_SIZE            512 // how much memory is needed to temporary store /etc/hostapd/hostapd.conf
    
        // ----- CODE -----


        bool assignStaticIPv6AddressToNetif (char *staticIPv6address, esp_netif_t *esp_netif) {
            // find lwip netif structure for a given esp_netif object
            netif *lwip_netif = NULL;
            struct netif *netif = netif_list;
            while (netif != NULL) {
                if (netif->state == esp_netif)
                    lwip_netif = netif;
                netif = netif->next;
            }

            // assign it a static IPv6 address
            if (lwip_netif) {
                ip6_addr_t ip6;
                inet6_aton (staticIPv6address, &ip6);
                netif_ip6_addr_set (lwip_netif, 0, &ip6);
                netif_ip6_addr_set_state (lwip_netif, 0, IP6_ADDR_PREFERRED);
                // DEBUG: cout << "Configured static IPv6 address: " << ip6addr_ntoa (&ip6) << endl;
                return true;
            } else {
                // DEBUG: cout << "Failed to get lwIP netif implementation" << endl;
            }
            return false;
        }

        // static IPv6 address cn be assigned to STA only after the connection is made so we'll define staIPv6 globaly to be accessible to WiFi.onEvent function as well
        #ifdef DEFAULT_STA_IPv6
            char staIPv6 [INET6_ADDRSTRLEN] = DEFAULT_STA_IPv6; 
        #else
            char staIPv6 [INET6_ADDRSTRLEN] = ""; 
        #endif

        #ifdef USE_mDNS
            // remember servers' ports
            int __httpPort__ = 0;
            int __telnetPort__ = 0;
            int __ftpPort__ = 0;

            void registerPortsWithMDNS () {

                #ifdef USE_mDNS
                    // set up mDNS
                    if (MDNS.begin (HOSTNAME)) { 
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[netwk][mDNS] started for " << HOSTNAME << ", free heap left: " << esp_get_free_heap_size ();
                        #endif
                        cout << (const char *) "[netwk][mDNS] started for " << HOSTNAME << endl;
                    } else {
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[netwk][mDNS] could not start mDNS for " << HOSTNAME;
                        #endif
                        cout << (const char *) "[netwk][mDNS] could not start mDNS for " << HOSTNAME << endl;
                    }
                #endif

                if (__telnetPort__)
                    MDNS.addService ("telnet", "tcp", __telnetPort__);
                if (__httpPort__)
                    MDNS.addService ("http", "tcp", __httpPort__);
                if (__ftpPort__)
                    MDNS.addService ("ftp", "tcp", __ftpPort__);
            }
        #endif


    void startWiFi () {                  // starts WiFi according to configuration files, creates configuration files if they don't exist
        // WiFi.disconnect (true);
        WiFi.mode (WIFI_OFF);
    
        // these parameters are needed to start ESP32 WiFi in different modes
        #ifdef DEFAULT_STA_SSID
            char staSSID [34] = DEFAULT_STA_SSID; // enough for max 32 characters for SSID
        #else
            char staSSID [34] = ""; // enough for max 32 characters for SSID
        #endif
        #ifdef DEFAULT_STA_PASSWORD
            char staPassword [65] = DEFAULT_STA_PASSWORD; // enough for max 63 characters for password
        #else
            char staPassword [65] = ""; // enough for max 63 characters for password
        #endif

        #ifdef DEFAULT_STA_IP
            char staIPv4 [INET_ADDRSTRLEN] = DEFAULT_STA_IP; // enough for IPv4 (max 15 characters) 
        #else
            char staIPv4 [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters) 
        #endif
        #ifdef DEFAULT_STA_SUBNET_MASK
            char staSubnetMask [INET_ADDRSTRLEN] = DEFAULT_STA_SUBNET_MASK; // enough for IPv4 (max 15 characters) 
        #else
            char staSubnetMask [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters) 
        #endif
        #ifdef DEFAULT_STA_GATEWAY
            char staGateway [INET_ADDRSTRLEN] = DEFAULT_STA_GATEWAY; // enough for IPv4 (max 15 characters) 
        #else
            char staGateway [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters) 
        #endif
        #ifdef DEFAULT_STA_DNS_1
            char staDns1 [INET_ADDRSTRLEN] = DEFAULT_STA_DNS_1; // enough for IPv4 (max 15 characters) 
        #else
            char staDns1 [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters) 
        #endif
        #ifdef DEFAULT_STA_DNS_2
            char staDns2 [INET_ADDRSTRLEN] = DEFAULT_STA_DNS_2; // enough for IPv4 (max 15 characters) 
        #else
            char staDns2 [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters) 
        #endif

        #ifdef DEFAULT_AP_SSID
            char apSSID [34] = DEFAULT_AP_SSID; // enough for max 32 characters for SSID
        #else
            char apSSID [34] = ""; // enough for max 32 characters for SSID
        #endif
        #ifdef DEFAULT_AP_PASSWORD
            char apPassword [65] = DEFAULT_AP_PASSWORD; // enough for max 63 characters for password
        #else
            char apPassword [65] = ""; // enough for max 63 characters for password
        #endif

        #ifdef DEFAULT_AP_IP
            char apIP [INET_ADDRSTRLEN] = DEFAULT_AP_IP; // enough for IPv4 (max 15 characters)
        #else
            char apIP [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters)
        #endif
        #ifdef DEFAULT_AP_SUBNET_MASK
            char apSubnetMask [INET_ADDRSTRLEN] = DEFAULT_AP_SUBNET_MASK; // enough for IPv4 (max 15 characters)
        #else
            char apSubnetMask [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters)
        #endif
        #ifdef DEFAULT_AP_IP
            char apGateway [INET_ADDRSTRLEN] = DEFAULT_AP_IP; // enough for IPv4 (max 15 characters)
        #else
            char apGateway [INET_ADDRSTRLEN] = ""; // enough for IPv4 (max 15 characters)
        #endif
        #ifdef DEFAULT_AP_IPv6
            char apIPv6 [INET6_ADDRSTRLEN] = DEFAULT_AP_IPv6; 
        #else
            char apIPv6 [INET6_ADDRSTRLEN] = ""; 
        #endif

    
        #ifdef __FILE_SYSTEM__
            if (fileSystem.mounted ()) { 
                // read interfaces configuration from /network/interfaces, create a new one if it doesn't exist
                if (!fileSystem.isFile ("/network/interfaces")) {
                    // create directory structure
                    if (!fileSystem.isDirectory ("/network")) { fileSystem.makeDirectory ("/network"); }
                    cout << F ("[netwk] /network/interfaces does not exist, creating default one ... ");
                    bool created = false;
                    File f = fileSystem.open ("/network/interfaces", "w");          
                    if (f) {
                        #if defined DEFAULT_STA_IP && defined DEFAULT_STA_SUBNET_MASK && defined DEFAULT_STA_GATEWAY && defined DEFAULT_STA_DNS_1 && defined DEFAULT_STA_DNS_2
                            String defaultContent = F ("# WiFi STA(tion) configuration - reboot for changes to take effect\r\n\r\n"
                                                        "# get IPv4 address from DHCP\r\n"
                                                        "#  iface STA inet dhcp\r\n"         
                                                        "\r\n"
                                                        "# use static IPv4 address\r\n"                   
                                                        "   iface STA inet static\r\n"
                                                        "      address " DEFAULT_STA_IP "\r\n" 
                                                        "      netmask " DEFAULT_STA_SUBNET_MASK "\r\n" 
                                                        "      gateway " DEFAULT_STA_GATEWAY "\r\n"
                                                        "      dns1 " DEFAULT_STA_DNS_1 "\r\n"
                                                        "      dns2 " DEFAULT_STA_DNS_2 "\r\n"
                                                        "\r\n"
                                                        "# get IPv6 address from DHCP\r\n"
                                                        "#   iface STA inet6 dhcp\r\n"                  
                                                        "\r\n"
                                                        #ifdef DEFAULT_STA_IPv6
                                                            "# use static IPv6 address\r\n"
                                                            "   iface STA inet6 static\r\n"
                                                            "      address " DEFAULT_STA_IPv6 "\r\n"
                                                        #else
                                                            "# use static IPv6 address (example below)\r\n"
                                                            "#   iface STA inet6 static\r\n"
                                                            "#      address \r\n"
                                                        #endif
                                                        );
                        #else
                            String defaultContent = F ("# WiFi STA(tion) configuration - reboot for changes to take effect\r\n\r\n"
                                                        "# get IPv4 address from DHCP\r\n"
                                                        "   iface STA inet dhcp\r\n"
                                                        "\r\n"
                                                        "# use static IPv4 address (example below)\r\n"   
                                                        "#  iface STA inet static\r\n"
                                                        #ifdef DEFAULT_STA_IP
                                                        "#     address " DEFAULT_STA_IP "\r\n"
                                                        #else
                                                        "#     address \r\n"
                                                        #endif
                                                        #ifdef DEFAULT_STA_SUBNET_MASK
                                                        "#     netmask " DEFAULT_STA_SUBNET_MASK "\r\n"
                                                        #else
                                                        "#     netmask 255.255.255.0\r\n"
                                                        #endif
                                                        #ifdef DEFAULT_STA_GATEWAY
                                                        "#     gateway " DEFAULT_STA_GATEWAY "\r\n"
                                                        #else
                                                        "#     gateway <your router's IPv4 address>\r\n"
                                                        #endif
                                                        #ifdef DEFAULT_STA_DNS_1
                                                        "#     dns1 " DEFAULT_STA_DNS_1 "\r\n"
                                                        #else
                                                        "#     dns1 \r\n"
                                                        #endif
                                                        #ifdef DEFAULT_STA_DNS_2
                                                        "#     dns2 " DEFAULT_STA_DNS_2 "\r\n"
                                                        #else
                                                        "#     dns2 \r\n"
                                                        #endif
                                                        "\r\n"
                                                        "# get IPv6 address from DHCP\r\n"
                                                        "#   iface STA inet6 dhcp\r\n"                  
                                                        "\r\n"
                                                        #ifdef DEFAULT_STA_IPv6
                                                            "# use static IPv6 address\r\n"
                                                            "   iface STA inet6 static\r\n"
                                                            "      address " DEFAULT_STA_IPv6 "\r\n"
                                                        #else
                                                            "# use static IPv6 address (example below)\r\n"
                                                            "#   iface STA inet6 static\r\n"
                                                            "#      address \r\n"
                                                        #endif
                                                        );
                        #endif
                        created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());                                
                        f.close ();
                        fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                    }
                    cout <<  (created ? (const char *) "created" : (const char *) "error") << endl;
                }
                {
                    cout << F ("[netwk] reading /network/interfaces ... ");
                    // /network/interfaces STA(tion) configuration - parse configuration file if it exists
                    char buffer [MAX_INTERFACES_SIZE] = "\n";
                    if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/network/interfaces")) {
                        strcat (buffer, "\n");
                        cout << endl;

                        char *p4 = stristr (buffer, "\niface STA inet static");
                        char *p6 = stristr (buffer, "\niface STA inet6 static");

                        if (p4) {

                            // static IPv4 configuration
                            *staIPv4 = *staSubnetMask = *staGateway = *staDns1 = *staDns2 = 0;
                            char *p;                    

                            if ((p = stristr (p4, "\naddress")) && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 8, "%*[ =]%16[0-9.]", staIPv4);
                            if ((p = stristr (p4, "\nnetmask")) && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 8, "%*[ =]%16[0-9.]", staSubnetMask);
                            if ((p = stristr (p4, "\ngateway")) && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 8, "%*[ =]%16[0-9.]", staGateway);
                            if ((p = stristr (p4, "\ndns1"))    && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 5, "%*[ =]%16[0-9.]", staDns1);
                            if ((p = stristr (p4, "\ndns2"))    && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 5, "%*[ =]%16[0-9.]", staDns2);
                            // DEBUG: cout << F ("[netwk] using static IPv4") << endl;

                        } else {

                            // IPv4 configuration with DHCP
                            // DEBUG: cout << F ("[netwk] using IPv4 DHCP") << endl;

                        }

                        if (stristr (buffer, "iface STA inet6 dhcp")) {
                            
                            // IPv6 configuration with DHCP - let's just flag the address with space - this will tell us later to enable IPv6
                            staIPv6 [0] = ' ';
                            // DEBUG: cout << F ("[netwk] using IPv6 DHCP") << endl;

                        }

                        if (p6) {
                            // static IPv6 configuration
                            *staIPv6 = 0;
                            char *p;
                            if ((p = stristr (p6, "\naddress")) && (!p4 || p6 < p4 && p < p4 || p6 > p4 && p > p4)) sscanf (p + 8, "%*[ =]%39[0-9A-Fa-f:]", staIPv6);
                            // DEBUG: cout << F ("[netwk] using static IPv6") << endl;

                        } 

                    } else {
                        cout << ((const char *) "error") << endl;
                    } 
                }


                // read STAtion credentials from /etc/wpa_supplicant/wpa_supplicant.conf, create a new one if it doesn't exist
                if (!fileSystem.isFile ("/etc/wpa_supplicant/wpa_supplicant.conf")) {
                    // create directory structure
                    if (!fileSystem.isDirectory ("/etc/wpa_supplicant")) { fileSystem.makeDirectory ("/etc"); fileSystem.makeDirectory ("/etc/wpa_supplicant"); }
                    cout << F ("[netwk] /etc/wpa_supplicant/wpa_supplicant.conf does not exist, creating default one ... ");
                    bool created = false;
                    File f = fileSystem.open ("/etc/wpa_supplicant/wpa_supplicant.conf", "w");          
                    if (f) {
                        String defaultContent = F ("# WiFi STA (station) credentials - reboot for changes to take effect\r\n\r\n"
                                                    #ifdef DEFAULT_STA_SSID
                                                        "   ssid " DEFAULT_STA_SSID "\r\n"
                                                    #else
                                                        "   ssid \r\n"
                                                    #endif
                                                    #ifdef DEFAULT_STA_PASSWORD
                                                        "   psk " DEFAULT_STA_PASSWORD "\r\n"
                                                    #else
                                                        "   psk \r\n"
                                                    #endif           
                                                    );
                        created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                        f.close ();
                        fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                    }
                    cout << (created ? (const char *) "created" : (const char *) "error") << endl;
                }
                {              
                    cout << F ("[netwk] reading /etc/wpa_supplicant/wpa_supplicant.conf ... ");
                    // /etc/wpa_supplicant/wpa_supplicant.conf STA(tion) credentials - parse configuration file if it exists
                    char buffer [MAX_WPA_SUPPLICANT_SIZE] = "\n";
                    if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/wpa_supplicant/wpa_supplicant.conf")) {
                        strcat (buffer, "\n");                
                        cout << endl;

                        *staSSID = *staPassword = 0;
                        char *p;                    
                        if ((p = stristr (buffer, "\nssid"))) sscanf (p + 5, "%*[ =]%33[^\n]", staSSID); // sscanf (p + 5, "%*[ =]%33[!-~]", staSSID);
                        for (int i = strlen (staSSID) - 1; i && staSSID [i] <= ' '; i--) staSSID [i] = 0; // right-trim
                        if ((p = stristr (buffer, "\npsk"))) sscanf (p + 4, "%*[ =]%63[^\n]", staPassword); // sscanf (p + 4, "%*[ =]%63[!-~]", staPassword);
                        for (int i = strlen (staPassword) - 1; i && staPassword [i] <= ' '; i--) staPassword [i] = 0; // right-trim

                    } else {
                        cout << (const char *) "error" << endl;
                    }
                }


                // read A(ccess) P(oint) configuration from /etc/dhcpcd.conf, create a new one if it doesn't exist
                if (!fileSystem.isFile ("/etc/dhcpcd.conf")) {
                    // create directory structure
                    if (!fileSystem.isDirectory ("/etc")) fileSystem.makeDirectory ("/etc");
                    cout << F ("[netwk] /etc/dhcpcd.conf does not exist, creating default one ... ");
                    bool created = false;
                    File f = fileSystem.open ("/etc/dhcpcd.conf", "w");          
                    if (f) {
                        String defaultContent = F ("# WiFi AP configuration - reboot for changes to take effect\r\n\r\n"
                                                    "\r\n"
                                                    "# use static IPv4 address (example below)\r\n"
                                                    #ifdef DEFAULT_AP_IP
                                                        "   static ip_address " DEFAULT_AP_IP "\r\n"
                                                    #else
                                                        "   static ip_address \r\n"
                                                    #endif
                                                    #ifdef DEFAULT_AP_SUBNET_MASK
                                                        "   netmask " DEFAULT_AP_SUBNET_MASK "\r\n"
                                                    #else
                                                        "   netmask \r\n"
                                                    #endif
                                                    #ifdef DEFAULT_AP_IP
                                                        "   gateway " DEFAULT_AP_IP "\r\n"
                                                    #else
                                                        "   gateway \r\n"
                                                    #endif
                                                    "\r\n"
                                                    "# use static IPv6 address (example below)\r\n"
                                                    #ifdef DEFAULT_AP_IPv6
                                                        "   static ip6_address " DEFAULT_AP_IPv6 "\r\n"
                                                    #else
                                                        "#   static ip6_address \r\n"
                                                    #endif
                                                    );
                        created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                        f.close ();
                        fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                    }
                    cout << (created ? (const char *) "created" : (const char *) "error") << endl;
                }
                {              
                    cout << F ("[netwk] reading /etc/dhcpcd.conf ... ");
                    // /etc/dhcpcd.conf contains A(ccess) P(oint) configuration - parse configuration file if it exists
                    char buffer [MAX_DHCPCD_SIZE] = "\n";
                    if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/dhcpcd.conf")) {
                        strcat (buffer, "\n");
                        cout << endl;

                        char *p4 = stristr (buffer, "\nstatic ip_address");
                        char *p6 = stristr (buffer, "\nstatic ip6_address");

                        if (p4) {

                            // static IPv4 configuration
                            *apIP = *apSubnetMask = *apGateway = 0;
                            char *p;                    
                                                                                                                    sscanf (p4 + 18, "%*[ =]%16[0-9.]", apIP);
                            if ((p = stristr (p4, "\nnetmask")) && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 8, "%*[ =]%16[0-9.]", apSubnetMask);
                            if ((p = stristr (p4, "\ngateway")) && (!p6 || p4 < p6 && p < p6 || p4 > p6 && p > p6)) sscanf (p + 8, "%*[ =]%16[0-9.]", apGateway);

                        }

                        if (p6) {

                            // static IPv6 configuration
                            sscanf (p6 + 19, "%*[ =]%39[0-9A-Fa-f:]", apIPv6);

                        }

                    } else {
                        cout << (const char *) "error" << endl;
                    }
                }


                // read A(ccess) P(oint) credentials from /etc/hostapd/hostapd.conf, create a new one if it doesn't exist
                if (!fileSystem.isFile ("/etc/hostapd/hostapd.conf")) {
                  // create directory structure
                  if (!fileSystem.isDirectory ("/etc/hostapd")) { fileSystem.makeDirectory ("/etc"); fileSystem.makeDirectory ("/etc/hostapd"); }
                  cout << F ("[netwk] /etc/hostapd/hostapd.conf does not exist, creating default one ... ");
                  bool created = false;
                  File f = fileSystem.open ("/etc/hostapd/hostapd.conf", "w");
                  if (f) {
                      String defaultContent = F ("# WiFi AP credentials - reboot for changes to take effect\r\n\r\n"
                                                 #ifdef DEFAULT_AP_SSID
                                                   "   ssid " DEFAULT_AP_SSID "\r\n"
                                                 #else
                                                   "   ssid \r\n"
                                                 #endif
                                                 "   # use at least 8 characters for wpa_passphrase\r\n"
                                                 #ifdef DEFAULT_AP_PASSWORD
                                                   "   wpa_passphrase " DEFAULT_AP_PASSWORD "\r\n"
                                                 #else
                                                 "   wpa_passphrase \r\n"
                                                 #endif                
                                                );
                        created = (f.printf (defaultContent.c_str()) == defaultContent.length ());
                        f.close ();

                        fileSystem.diskTraffic.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                    }
                    cout << (created ? (const char *) "created" : (const char *) "error") << endl; 
                }
                {              
                    cout << F ("[netwk] reading /etc/hostapd/hostapd.conf ... ");
                    // /etc/hostapd/hostapd.conf contains A(ccess) P(oint) credentials - parse configuration file if it exists
                    char buffer [MAX_HOSTAPD_SIZE] = "\n";
                    if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/hostapd/hostapd.conf")) {
                        strcat (buffer, "\n");
                        cout << endl;
                        
                        *apSSID = *apPassword = 0;
                        char *p;
                        if ((p = stristr (buffer, "\nssid"))) sscanf (p + 5, "%*[ =]%33[^\n]", apSSID);
                        for (int i = strlen (apSSID) - 1; i && apSSID [i] <= ' '; i--) apSSID [i] = 0; // right-trim
                        if ((p = stristr (buffer, "\nwpa_passphrase"))) sscanf (p + 15, "%*[ =]%63[^\n]", apPassword);
                        for (int i = strlen (apPassword) - 1; i && apPassword [i] <= ' '; i--) apPassword [i] = 0; // right-trim

                    } else {
                        cout << ((const char *) "error") << endl;
                    }
                }

            } else 
        #endif
            {
                cout << F ("[netwk] file system not mounted, can't read or write configuration files") << endl;
            }

    
        // https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiIPv6/WiFiIPv6.ino
        WiFi.onEvent ([] (WiFiEvent_t event, WiFiEventInfo_t info) {

            switch (event) {
                case ARDUINO_EVENT_WIFI_READY: 
                    cout << F ("[netwk][STA] interface ready") << endl;
                    break;
                case ARDUINO_EVENT_WIFI_SCAN_DONE:
                    cout << F ("[netwk][STA] completed scan for access points") << endl;
                    break;
                case ARDUINO_EVENT_WIFI_STA_START:
                    cout << F ("[netwk][STA] client started") << endl;
                    break;
                case ARDUINO_EVENT_WIFI_STA_STOP:
                    cout << F ("[netwk][STA] clients stopped") << endl;
                    break;
                case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][STA] connected to " << WiFi.SSID ().c_str () << ", free heap left: " << esp_get_free_heap_size ();
                    #endif
                    cout << (const char *) "[netwk][STA] connected to " << WiFi.SSID ().c_str () << endl;

                    // set up static STA IPv6 only when connected to router
                    if (staIPv6 [0] > ' ') { // if static IPv6
                        if (assignStaticIPv6AddressToNetif (staIPv6, esp_netif_get_handle_from_ifkey ("WIFI_STA_DEF"))) {
                            #ifdef __DMESG__
                                dmesgQueue << (const char *) "[netwk][STA] is using static IPv6 address : " << staIPv6;
                            #endif
                            cout << (const char *) "[netwk][STA] is using static IPv6 address: " << staIPv6 << endl;
                        } else {
                            #ifdef __DMESG__
                                dmesgQueue << (const char *) "[netwk][STA] could not assign static IPv6 address: " << staIPv6;
                            #endif
                            cout << (const char *) "[netwk][STA] could not assign static IPv6 address: " << staIPv6 << endl;
                        }
                    }
                    break;
                case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][STA] disconnected";
                    #endif
                    cout << (const char *) "[netwk][STA] disconnected" << endl;
                    break;
                case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
                    cout << F ("[netwk][STA] authentication mode has changed") << endl;
                    break;
                case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][STA] got IPv4 address: " << WiFi.localIP ().toString ().c_str () << (const char *) " gateway address: " << WiFi.gatewayIP ().toString ().c_str ();
                    #endif
                    cout << (const char *) "[netwk][STA] got IPv4 address: " << WiFi.localIP ().toString ().c_str () << (const char *) " gateway address: " << WiFi.gatewayIP ().toString ().c_str ();

                    // register all the ports used
                    #ifdef USE_mDNS
                        registerPortsWithMDNS ();
                    #endif

                    break;
                case ARDUINO_EVENT_WIFI_STA_LOST_IP:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][STA] lost IP address. IP address reset to 0";
                    #endif
                    cout << (const char *) "[netwk][STA] lost IP address. IP address reset to 0" << endl;
                    break;
                case ARDUINO_EVENT_WPS_ER_SUCCESS:
                    cout << F ("[netwk][STA] WiFi Protected Setup (WPS) succeeded in enrollee mode") << endl;
                    break;
                case ARDUINO_EVENT_WPS_ER_FAILED:
                    cout << F ("[netwk][STA] WiFi Protected Setup (WPS) failed in enrollee mode") << endl;
                    break;
                case ARDUINO_EVENT_WPS_ER_TIMEOUT:
                    cout << F ("[netwk][STA] WiFi Protected Setup (WPS) timeout in enrollee mode") << endl;
                    break;
                case ARDUINO_EVENT_WPS_ER_PIN:
                    cout << F ("[netwk][STA] WiFi Protected Setup (WPS) pin code in enrollee mode") << endl;
                    break;

                case ARDUINO_EVENT_WIFI_AP_START:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][AP] access point started";
                    #endif
                    cout << (const char *) "[netwk][AP] access point started" << endl;

                    // register all the ports used
                    #ifdef USE_mDNS
                        registerPortsWithMDNS ();
                    #endif

                    break;
                case ARDUINO_EVENT_WIFI_AP_STOP:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][AP] access point stopped";
                    #endif
                    cout << (const char *) "[netwk][AP] access point stopped" << endl;
                    break;
                case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
                    // cout << "Client connected") << endl;
                    break;
                case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
                    // cout << "Client disconnected" << endl;
                    break;
                case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
                    // cout << "Assigned IP address to client" << endl;
                    break;
                case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
                    // cout << "Received probe request" << endl;
                    break;

                case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][AP] got IPv6 address: " << WiFi.softAPlinkLocalIPv6 ().toString ().c_str ();
                    #endif
                    cout << (const char *) "[netwk][AP] got IPv6 address: " << WiFi.softAPlinkLocalIPv6 ().toString ().c_str () << endl;

                    // register all the ports used
                    #ifdef USE_mDNS
                        registerPortsWithMDNS ();
                    #endif

                    break;
                case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[netwk][STA] got IPv6 address(es): " << WiFi.linkLocalIPv6 ().toString ().c_str () << ", " << WiFi.globalIPv6 ().toString ().c_str ();
                    #endif
                    cout << (const char *) "[netwk][STA] got IPv6 address(es): " << WiFi.linkLocalIPv6 ().toString ().c_str () << ", " << WiFi.globalIPv6 ().toString ().c_str () << endl;

                    // register all the ports used
                    #ifdef USE_mDNS
                        registerPortsWithMDNS ();
                    #endif

                    break;

                #ifdef _ETH_H_

                    case ARDUINO_EVENT_ETH_GOT_IP6:
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[netwk][ETH] got IPv6 address: " << ETH.linkLocalIPv6 ().toString ().c_str ();
                        #endif
                        cout << (const char *) "[netwk][ETH] got IPv6 address: ", ETH.linkLocalIPv6 ().toString ().c_str () << endl;

                        // register all the ports used
                        #ifdef USE_mDNS
                            registerPortsWithMDNS ();
                        #endif

                        break;

                    case ARDUINO_EVENT_ETH_START:
                        cout << F ("[netwk][ETH] started") << endl;
                        break;
                    case ARDUINO_EVENT_ETH_STOP:
                        cout << F ("[netwk][ETH] clients stopped") << endl;
                        break;
                    case ARDUINO_EVENT_ETH_CONNECTED:
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[netwk][ETH] connected";
                        #endif
                        cout << (const char *) "[netwk][ETH] connected" << endl; 
                        break;
                    case ARDUINO_EVENT_ETH_DISCONNECTED:
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[netwk][ETH] disconnected";
                        #endif
                        cout << (const char *) "[netwk][ETH] disconnected" << endl;
                        break;
                    case ARDUINO_EVENT_ETH_GOT_IP:
                        #ifdef __DMESG__
                            dmesgQueue << (const char *) "[netwk][ETH] got IPv4 address: " << ETH.localIP ().toString ().c_str ();
                        #endif
                        cout << (const char *) "[netwk][ETH] got IPv4 address: " << ETH.localIP ().toString ().c_str () << endl;

                        // register all the ports used
                        #ifdef USE_mDNS
                            registerPortsWithMDNS ();
                        #endif

                        break;
                
                #endif

                default: break;
            }

        });    
    
        // set up STA
        if (*staSSID) { 
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[netwk][STA] connecting to: " << staSSID;
            #endif
            cout << (const char *) "[netwk][STA] connecting to: " << staSSID << endl;

            // set up STA IPv4
            if (*staIPv4) { 
                // dmesg ("[netwk][STA] connecting STAtion to router with static IP: ", (char *) (String (staIPv4) + " GW: " + String (staGateway) + " MSK: " + String (staSubnetMask) + " DNS: " + String (staDns1) + ", " + String (staDns2)).c_str ());
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[netwk][STA] static IPv4 address: " << staIPv4;
                #endif
                cout << (const char *) "[netwk][STA] static IPv4 address: " << staIPv4 << endl;
                WiFi.config (IPAddress (staIPv4), IPAddress (staGateway), IPAddress (staSubnetMask), *staDns1 ? IPAddress (staDns1) : IPAddress (255, 255, 255, 255), *staDns2 ? IPAddress (staDns2) : IPAddress (255, 255, 255, 255)); // INADDR_NONE == 255.255.255.255
            } else { 
                #ifdef __DMESG__
                    dmesgQueue << F ("[netwk][STA] is using DHCP for IPv4");
                #endif
                cout << F ("[netwk][STA] is using DHCP for IPv4") << endl;
            }  
            // get IPv6 from DHCP
            if (staIPv6 [0] == ' ') { // if flagged earlier for IPv6 DHCP
                WiFi.enableIPv6 ();
                #ifdef __DMESG__
                    dmesgQueue << F ("[netwk][STA] is using DHCP for IPv6");
                #endif
                cout << F ("[netwk][STA] is using DHCP for IPv6") << endl;
            }

            WiFi.begin (staSSID, staPassword);
        }

        // set up AP
        if (*apSSID) {
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[netwk][AP] setting up access point: " << apSSID;
            #endif
            cout << (const char *) "[netwk][AP] setting up access point: " << apSSID << endl;

            if (WiFi.softAP (apSSID, apPassword)) { 
                // dmesg ("[netwk] [AP] initializing access point: ", (char *) (String (apSSID) + "/" + String (apPassword) + ", " + String (apIP) + ", " + String (apGateway) + ", " + String (apSubnetMask)).c_str ()); 
                WiFi.softAPConfig (IPAddress (apIP), IPAddress (apGateway), IPAddress (apSubnetMask));
                WiFi.begin ();
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[netwk][AP] static IPv4 address: " << WiFi.softAPIP ().toString ().c_str ();
                #endif
                cout << (const char *) "[netwk][AP] static IPv4 address: " << WiFi.softAPIP ().toString ().c_str () << endl;
            } else {
                // ESP.restart ();
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[netwk][AP] failed to initialize access point"; 
                #endif
                cout << (const char *) "[netwk][AP] failed to initialize access point" << endl;
            }
        } 

        // set WiFi mode
        if (*staSSID) {
            if (*apSSID) {
                WiFi.mode (WIFI_AP_STA); // both, AP and STA modes
            } else {
                WiFi.mode (WIFI_STA); // only STA mode
            }
        } else {
            if (*apSSID)
                WiFi.mode (WIFI_AP); // only AP mode
        }

        /* moved to onEvent function
        // set up static STA IPv6
        if (*staSSID && staIPv6 [0] > ' ') { // if static IPv6
            if (assignStaticIPv6AddressToNetif (staIPv6, esp_netif_get_handle_from_ifkey ("WIFI_STA_DEF"))) {
                #ifdef __DMESG__
                    dmesgQueue << F ("[netwk][STA] is using static IPv6 address : ") << staIPv6;
                #endif
                cout << F ("[netwk][STA] is using static IPv6 address: ") << staIPv6);                
            } else {
                #ifdef __DMESG__
                    dmesgQueue << F ("[netwk][STA] could not assign static IPv6 address: ")  << staIPv6 << endl;
                #endif
                cout << F ("[netwk][STA] could not assign static IPv6 address: ") << staIPv6 << endl;
            }
        }
        */

        // set up static AP IPv6
        if (*apSSID && apIPv6 [0] > ' ') { // if static IPv6
            if (assignStaticIPv6AddressToNetif (apIPv6, esp_netif_get_handle_from_ifkey ("WIFI_AP_DEF"))) {
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[netwk][AP] is using static IPv6 address : " << apIPv6;
                #endif
                cout << (const char *) "[netwk][AP] is using static IPv6 address: " << apIPv6 << endl;
            } else {
                #ifdef __DMESG__
                    dmesgQueue << (const char *) "[netwk][AP] could not assign static IPv6 address: "  << apIPv6;
                #endif
                cout << (const char *) "[netwk][AP] could not assign static IPv6 address: " << apIPv6 << endl;
            }
        }

        // power saving?
        esp_wifi_set_ps (POVER_SAVING_MODE); // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html

        // rename hostname for all adapters
        esp_netif_t *netif = esp_netif_next_unsafe (NULL);
        while (netif) {
            if (esp_netif_set_hostname (netif, HOSTNAME) != ESP_OK) {
                #ifdef __DMESG__
                    dmesgQueue << F ("[netwk] couldn't change adapter's hostname");
                #endif
                cout << F ("[netwk] couldn't change adapter's hostname") << endl;
            }
            netif = esp_netif_next_unsafe (netif);
        }

    }

#endif
