// #include "./servers/fileSystem.hpp"  // all file name and file info related functions are there, by default FILE_SYSTEM is #defined as FILE_SYSTEM_LITTLEFS, other options are: FILE_SYSTEM_FAT, FILE_SYSTEM_SD_CARD and (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)

#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"
#include "./servers/netwk.h"            // if #included after fileSystem.hpp these files will be created: /network/interfaces, /etc/wpa_supplicant/wpa_supplicant.conf, /etc/dhcpcd.conf, /etc/hostapd/hostapd.conf

#include "./servers/smtpClient.h"       // if #included after fileSystem.hpp SMTP client can also use /etc/mail/sendmail.cf file with default parameters


void setup () {
    Serial.begin (115200);

    // fileSystem.mountLittleFs (true); // or just call    LittleFS.begin (true);

    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP from the router
        delay (1000); 
        Serial.printf ("   .\n"); 
    } 
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());


    String message = "<html>\n"
                     " <p style='font-family: verdana;'>\n"
                     "  Current RSSI = <b>" + String (WiFi.RSSI ()) +  "</b> dBm\n"
                     " </p>\n"
                     "</html>";    

    // send graph to SMTP server, sendMail returns error or success text, fills empty parameters with the ones from configuration file /etc/mail/sendmail.cf
    cstring smtpReply = sendMail (message.c_str (), "HTML email", "\"You\"<***.***@***.***>", "\"Me\"<***.***@***.***>", "*****", "*****", 25, "****.****.****");

    Serial.println (smtpReply);
}

void loop () {

}
