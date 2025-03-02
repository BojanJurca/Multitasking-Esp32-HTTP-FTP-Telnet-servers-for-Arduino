#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"

                                        // the set of built-in Telnet commands varies according to which libraries are included
                                        // the following built-in Telnet commands are always supported:
                                        //          help, clear, uname, free, nohup, reboot, quit, date, ping, ifconfig, arp, iw, netstat, kill, telnet

// #include "./servers/dmesg.hpp"       // run-time message queue useful for debugging
                                        // #including dmesg.hpp prior to #including telnetSerevr.hpp brings support for:   
                                        //          dmesg

// #include "./servers/fileSystem.hpp"  // all file name and file info related functions are there, by default FILE_SYSTEM is #defined as FILE_SYSTEM_LITTLEFS, other options are: FILE_SYSTEM_FAT, FILE_SYSTEM_SD_CARD and (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
                                        // #including fileSystem.hpp prior to #including telnetSerevr.hpp brings support for:   
                                        //          mkfs.fat, mkfs.littlefs, fs_info, ls, tree, mkdir, rmdir, cd, pwd, cat, vi, cp, mv, rm, diskstat


#include "./servers/netwk.h"            // if #included after fileSystem.hpp these files will be created: /network/interfaces, /etc/wpa_supplicant/wpa_supplicant.conf, /etc/dhcpcd.conf, /etc/hostapd/hostapd.conf

#include "./servers/userManagement.hpp" // checkUserNameAndPassword and getHomeDirectory functions are declared there, by default USER_MANAGEMENT is #defined as NO_USER_MANAGEMENT, other options are HARDCODED_USER_MANAGEMENT and UNIX_LIKE_USER_MANAGEMENT
                                        // #defining USER_MANAGEMENT as UNIX_LIKE_USER_MANAGEMENT brings support for:
                                        //          useradd, userdel, passwd


// #include "./servers/time_functions.h"// all NTP and crontab related functions are there, by default TZ is #defined as "CET-1CEST,M3.5.0,M10.5.0/3" (change to your time zone) and DEFAULT_NTP_SERVER_1 as "1.si.pool.ntp.org", ...
                                        // #including time_functions.h prior to #including telnetSerevr.hpp brings support for:   
                                        //          uptime, ntpdate, crontab

// #include "./servers/httpClient.h"    // #including httpClient.h prior to #including telnetSerevr.hpp brings support for:   
                                        //          curl

// #include "./servers/smtpClient.h"    // #including ftpClient.h prior to #including telnetSerevr.hpp brings support for:   
                                        //          sendMail

#include "servers/telnetServer.hpp"


String telnetCommandHandlerCallback (int argc, char *argv [], telnetServer_t::telnetConnection_t *telnetConnection) {  // handle the Telnet command and send a reply back to the Telnet client

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))

    if (argv0is ("turn") && argv1is ("led") && argv2is ("on")) {  // turn led on telnet command

                                                                    digitalWrite (LED_BUILTIN, HIGH);
                                                                    return "Led is on.";

    } else if (argv0is ("turn") && argv1is ("led") && argv2is ("off")) { // turn led off telnet command

                                                                    digitalWrite (LED_BUILTIN, LOW);
                                                                    return "Led is off.";

    }

    return ""; // if not handeled above then let the Telnet server try built-in commands (like ifconfig, ...)
}


telnetServer_t *telnetServer = NULL;

void setup () {
    Serial.begin (115200);

    // fileSystem.mountLittleFs (true); // or just call    LittleFS.begin (true);

    startWiFi (); // or just call    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    telnetServer = new (std::nothrow) telnetServer_t (telnetCommandHandlerCallback); // there are other arguments available to handle WebSockets, firewall, ... if telnetServer will only serve already built-in commands this argument can be omitted
    if (telnetServer && *telnetServer)
        Serial.printf ("Telnet server started\n");
    else
        Serial.printf ("Telnet server did not start\n");

    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP address from the router
        delay (1000); 
        Serial.printf ("   .\n"); 
    } 
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
}

void loop () {
    
}
