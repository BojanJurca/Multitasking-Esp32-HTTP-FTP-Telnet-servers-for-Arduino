/*
    SMTP client example
*/

#include <WiFi.h>

  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command

  // #include "./servers/file_system.h"       // include file system if you want to use /etc/mail/sendmail.cf configuration file with defaults

  // define how network.h will connect to the router
    #define HOSTNAME                  "MyESP32Server"       // define the name of your ESP32 here
    #define MACHINETYPE               "ESP32 NodeMCU"       // describe your hardware here
    // tell ESP32 how to connect to your WiFi router if this is needed
    #define DEFAULT_STA_SSID          "YOUR_STA_SSID"       // define default WiFi settings  
    #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
    // tell ESP32 not to set up AP if it is not needed
    #define DEFAULT_AP_SSID           ""  // HOSTNAME       // set it to "" if you don't want ESP32 to act as AP 
    #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"    // must have at leas 8 characters
  #include "./servers/network.h"   

  // finally include smtpClient.h
  #include "./servers/smtpClient.h"

 
void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                  // include file system if you want to use /etc/mail/sendmail.cf configuration file with defaults
  startWiFi ();

  // wait until IP is obtained from router
  while (WiFi.localIP ().toString () == "0.0.0.0") { delay (1000); Serial.printf ("   .\n"); }

  // get remote web page
  String smtpReply = sendMail (
                               (char *) "<html><p style='color:green;'><b>Success!</b></p></html>", // message can be HTML formatted
                               (char *) "Test 1",                                                   // subject
                               (char *) "bojan.***@***.com,sabina.***@***.com",                     // to addresses (= to SMTP field)
                               (char *) "\"smtpClient\"<bojan.***@***.net>",                        // from address (= from SMTP field)
                               (char *) "***",                                                      // password for SMTP server
                               (char *) "***",                                                      // username for SMTP server
                               25,                                                                  // default SMTP port
                               (char *) "smtp.siol.net"                                             // SMTP server
                              );
  Serial.println (smtpReply);

  // sendMail ((char *) "<html><p style='color:green;'><b>Success!</b></p></html>", (char *) "Test 2"); // if defaults from /etc/mail/sendmail.cf are used you can skip some or all of the arguments
}

void loop () {

}
