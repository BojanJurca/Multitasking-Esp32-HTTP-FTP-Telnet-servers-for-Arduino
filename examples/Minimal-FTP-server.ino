#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"

#include "./servers/fileSystem.hpp"       // all file name and file info related functions are there, by default FILE_SYSTEM is #defined as FILE_SYSTEM_LITTLEFS, other options are: FILE_SYSTEM_FAT, FILE_SYSTEM_SD_CARD and (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)
#include "./servers/netwk.h"              // if #included after fileSystem.hpp these files will be created: /network/interfaces, /etc/wpa_supplicant/wpa_supplicant.conf, /etc/dhcpcd.conf, /etc/hostapd/hostapd.conf
#include "./servers/userManagement.hpp"   // checkUserNameAndPassword and getHomeDirectory functions are declared there, by default USER_MANAGEMENT is #defined as NO_USER_MANAGEMENT, other options are HARDCODED_USER_MANAGEMENT and UNIX_LIKE_USER_MANAGEMENT
#include "./servers/ftpServer.hpp"

ftpServer_t *ftpServer = NULL;

void setup () {
    Serial.begin (115200);

    fileSystem.mountLittleFs (true); // or just call    LittleFS.begin (true);

    startWiFi (); // or just call    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    ftpServer = new (std::nothrow) ftpServer_t (); // there are arguments available to handle firewall, ...
    if (ftpServer && *ftpServer)
        Serial.printf ("FTP server started\n");
    else
        Serial.printf ("FTP server did not start\n");

    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP address from the router
        delay (1000); 
        Serial.printf ("   .\n"); 
    } 
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
}

void loop () {
    
}
