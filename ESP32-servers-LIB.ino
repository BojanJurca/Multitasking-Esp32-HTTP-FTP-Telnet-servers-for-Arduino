/*

    Esp32_web_ftp_telnet_server_template.ino

    This file is part of the Esp32_web_ftp_telnet_server_template project:
    https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino

    Compile this code with Arduino for:
        - one of the ESP32 boards (Tools | Board), and
        - one of the SPIFFS partition schemes (Tools | Partition Scheme).

    This template uses the following libraries (can be included in the Arduino IDE):
        - ESP32_Multitasking_Network_Suite: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library
        - ThreadSafePing: https://github.com/BojanJurca/Thread-safe-ping-Arduino-library-for-ESP32
        - ThreadSafeFS: https://github.com/BojanJurca/Thread-safe-file-sytem-wrapper-Arduino-library-for-ESP32
        - LightweightSTL: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
        - cronDaemon: https://github.com/BojanJurca/Cron-Daemon-for-Arduino

    Please note that this file is only a template. You can include or exclude
    the functionalities you don't need. Some parts of the code are provided
    merely for demonstration purposes.

    April 27, 2026, Bojan Jurca

*/

#include <WiFi.h>


// Configure the servers you want to use in server_config.h before including their header files.
#include "server_config.h" // first inclusion - configuration part


#ifdef LOCALE
    // Arduino library: LightweightSTL
    // https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
    #include <locale.hpp>
#endif
#include <ostream.hpp>          // convenience wrapper for using cout instead of Serial.print
#include <Cstring.hpp>          // C-style strings with C++ operators that reside on the stack (no heap allocation)

// Arduino library: ThreadSafePing
// https://github.com/BojanJurca/Thread-safe-ping-Arduino-library-for-ESP32
#include <ThreadSafePing.h>     // used in this template to periodically verify WiFi connectivity

// Arduino library: ThreadSafeFS
// https://github.com/BojanJurca/Thread-safe-file-sytem-wrapper-Arduino-library-for-ESP32
#include <LittleFS.h>               // or SPIFFS or FFat, ...
#include <threadSafeFS.h>           // thread-safe wrapper file system wrapper
threadSafeFS::FS TSFS (LittleFS);   // use thread-safe wrapper arround selected LittleFS
using File = threadSafeFS::File;    // use thread-safe wrapper for all file operations in your code from now on

// Arduino library: cronDaemon
// https://github.com/BojanJurca/Cron-Daemon-for-Arduino
// if cronDaemon.h is included after threadSafeFS.h it will create /etc/crontab file
#include <cronDaemon.h>         // used in this template to periodically synchronize the internal clock with NTP servers or check WiFi connectivity
// Create a cronDaemon process that reads the crontab and calls
// cronHandlerCallback whenever scheduled events occur.
//
// The third argument (bool runCronDaemonInItsOwnTask = true) tells cronDaemon
// whether it should run in its own FreeRTOS task or be hosted inside the
// main setup/loop task.
//
// If you set this argument to false (as in this example), cronDaemon will NOT
// create its own task. This saves approximately 8 KB of RAM, but requires you
// to manually call cronDaemon.nextRun() from the loop() function.
void cronHandlerCallback (const char *cronCommand);
cronDaemon_t cronDaemon (TSFS, cronHandlerCallback, false);

// Arduino library: ESP32_Multitasking_Network_Suite
// https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library
#include <dmesg.hpp>


// Create the default configuration files and read from them
#include "server_config.h" // second inclusion - implementation part

// include the servers
#include <ntpClient.h>
#include <ftpServer.h>
    ftpServer_t *ftpServer = NULL;
#include <httpServer.h>
    httpServer_t *httpServer = NULL;
// Include oscilloscope
// #define USE_I2S_INTERFACE             // I2S interface improves web based oscilloscope analog sampling (of a single signal) if ESP32 board has one
// check INVERT_ADC1_GET_RAW and INVERT_I2S_READ #definitions in oscilloscope.h if the signals are inverted
#include "oscilloscope.h"
// Provide help text for Telnet user-defined commands
#ifdef POWER_SAVING
    #define USER_DEFINED_TELNET_HELP_TEXT   "\r\n  user management:" \
                                            "\r\n       useradd -d <userHomeDirectory> <userName>" \
                                            "\r\n       userdel <userName>" \
                                            "\r\n       passwd [<userName>]" \
                                            "\r\n  power saving:" \
                                            "\r\n       power saving" \
                                            "\r\n       power saving on" \
                                            "\r\n       power saving off"
#else
    #define USER_DEFINED_TELNET_HELP_TEXT   "\r\n  user management:" \
                                            "\r\n       useradd -d <userHomeDirectory> <userName>" \
                                            "\r\n       userdel <userName>" \
                                            "\r\n       passwd [<userName>]"
#endif
#include <telnetServer.h>
    telnetServer_t *telnetServer = NULL;

#ifdef USE_OTA
    #include <ESPmDNS.h>
    #include <NetworkUdp.h>
    #include <ArduinoOTA.h>
    uint32_t last_ota_time = 0;
#endif

// UNIX-like user management
#include "userManagement.h"
userManagement_t *userManagement;


// ----- handle user-defined Telnet commands -----

String telnetCommandHandlerCallback (int argc, char *argv [], telnetServer_t::telnetConnection_t *tcn) {

    // Must be reentrant !!!


    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))

    // ----- UNIX-like user management -----
    if (argv0is ("useradd"))        { 
                                        if (strcmp (tcn->getUserName (), "root"))   return "Only root may add users";
                                        if (argc == 4 && argv1is ("-d"))            return userManagement->userAdd (argv [3], argv [2]);
                                                                                    return "Wrong syntax, use useradd -d userHomeDirectory userName";
                                    }

    else if (argv0is ("userdel"))   {
                                            if (strcmp (tcn->getUserName (), "root"))   return "Only root may delete users";
                                            if (argc != 2)                              return "Wrong syntax. Use userdel userName";
                                            if (!strcmp (argv [1], "root"))             return "You don't really want to to this";
                                                                                        return userManagement->userDel (argv [1]);
                                    }

    else if (argv0is ("passwd"))    {
                                            const char *forUser = tcn->getUserName (); // usually, but may be changed later
                                            if (argc == 1) { // user changing password for himself
                                                // read current password
                                                tcn->dontEcho ();
                                                if (tcn->sendString ("Enter current password: ") <= 0) 
                                                    return "\r"; 
                                                char password [USER_PASSWORD_MAX_LENGTH + 1];
                                                if (tcn->recvLine (password, sizeof (password), true) != 13) { // the line is not ended with Enter
                                                    tcn->doEcho ();                  
                                                    return "\r\nPassword not changed"; 
                                                }
                                                tcn->doEcho (); 
                                                // check if password is valid for user
                                                if (!userManagement->checkUserNameAndPassword (tcn->getUserName (), password))
                                                    return "Wrong password";                                                     
                                            }
                                            if (argc == 2) {
                                                forUser = argv [1];
                                                if (!strcmp (tcn->getUserName (), argv [1])) { // user changing password for himself
                                                    // read current password
                                                    tcn->dontEcho ();
                                                    if (tcn->sendString ("Enter current password: ") <= 0) 
                                                        return "\r";
                                                    char password [USER_PASSWORD_MAX_LENGTH + 1];
                                                    if (tcn->recvLine (password, sizeof (password), true) != 13) { // the line is not ended with Enter
                                                        tcn->doEcho ();                  
                                                        return "\r\nPassword not changed"; 
                                                    }
                                                    tcn->doEcho (); 
                                                    // check if password is valid for user
                                                    if (!userManagement->checkUserNameAndPassword (argv [1], password))
                                                        return "Wrong password";                                                     
                                                } else if (!strcmp (tcn->getUserName (), "root")) { // root is changing password for another user
                                                    // check if user exists with getUserHomeDirectory
                                                    Cstring<255> homeDirectory = userManagement->getHomeDirectory (argv [1]);
                                                    if (homeDirectory == "")       
                                                        return "User does not exist";
                                                } else {
                                                    return "You may change only your own password";
                                                }
                                            }

                                            // it is OK to change the password now, read it twice
                                            char password1 [USER_PASSWORD_MAX_LENGTH + 1];
                                            char password2 [USER_PASSWORD_MAX_LENGTH + 1];
                                            tcn->dontEcho ();
                                            if (tcn->sendString ("\r\nEnter new password: ") <= 0)
                                                return "\r"; 
                                            if (tcn->recvLine (password1, sizeof (password1), true) != 13) { // the line is not ended with Enter
                                                tcn->doEcho ();
                                                return "\r\nPassword not changed"; 
                                            }
                                            if (tcn->sendString ("\r\nRe-enter new password: ") <= 0)
                                                return "\r"; 
                                            if (tcn->recvLine (password2, sizeof (password2), true) != 13) { // the line is not ended with Enter
                                                tcn->doEcho ();
                                                return "\r\nPassword not changed"; 
                                            }
                                            tcn->doEcho ();
                                            // check passwords
                                            if (strcmp (password1, password2))
                                                return "\r\nPasswords do not match";
                                            // change password
                                            if (userManagement->passwd (forUser, password1))
                                                return "\r\nPassword changed";
                                            else
                                                return "\r\nError changing password";  
                                    }
    #ifdef POWER_SAVING

        // ----- WiFi POWER SAVING -----

        if (argv0is ("power"))      {
                                        if (argv1is ("saving")) {
                                            if (argc == 2) {
                                                wifi_ps_type_t ps_type;
                                                esp_err_t err = esp_wifi_get_ps (&ps_type);
                                                if (err == ESP_OK)
                                                    if (ps_type == WIFI_PS_NONE)
                                                        return "power saving is off";
                                                    else
                                                        return "power saving is on";
                                                else
                                                    return "Couldn't get power saving mode";
                                            } else if (argc == 3) {
                                                if (argv2is ("on")) {
                                                    esp_err_t err = esp_wifi_set_ps (POWER_SAVING);
                                                    if (err == ESP_OK)
                                                        return "power saving is on";
                                                    else
                                                        return "Couldn't set power saving mode";
                                                } else if (argv2is ("off")) {
                                                    esp_err_t err = esp_wifi_set_ps (WIFI_PS_NONE);
                                                    if (err == ESP_OK)
                                                        return "power saving is off";
                                                    else
                                                        return "Couldn't set power saving mode";
                                                }                                                
                                            }
                                        }
                                        return "Wrong syntax, use power saving [on | off]";
                                    }
    #endif

    return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning ""
}


// ----- for demonstration only - you may freely delete the following definitions -----

#include "measurements.hpp"
measurements<60> freeHeap60;        // measure free heap each minute for possible memory leaks
measurements<24> freeHeap24;        // measure free heap each hour for possible memory leaks
measurements<24> freeBlock24;       // measure max free block of memory each hour
measurements<60> httpRequestCount;  // measure how many web connections arrive each minute
#undef LED_BUILTIN
#define LED_BUILTIN 2               // built-in led


// ----- handle HTTP requests -----

String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) { 

    // Must be reentrant !!!


    #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)

    httpRequestCount.increase_valueCounter (); // gether statistics

    // REST API
    if (httpRequestIs ("GET /builtInLed "))             { 
                                                        getBuiltInLed:
                                                            return "{\"id\":\"" HOSTNAME "\",\"builtInLed\":\"" + String (digitalRead (LED_BUILTIN) ? "on" : "off") + "\"}\r\n";
                                                        }                                                                    
    else if (httpRequestIs ("PUT /builtInLed/on "))     {
                                                            digitalWrite (LED_BUILTIN, HIGH);
                                                            goto getBuiltInLed;
                                                        }
    else if (httpRequestIs ("PUT /builtInLed/off "))    {
                                                            digitalWrite (LED_BUILTIN, LOW);
                                                            goto getBuiltInLed;
                                                        }
    else if (httpRequestIs ("GET /state "))             {
                                                            time_t t = ntpClient_t ().getUpTime (); // t holds seconds
                                                            int seconds = t % 60; t /= 60;          // t now holds minutes
                                                            int minutes = t % 60; t /= 60;          // t now holds hours
                                                            int hours = t % 24;   t /= 24;          // t now holds days
                                                            char upTime [25]; 
                                                            *upTime = 0;
                                                            if (t) 
                                                            sprintf (upTime, "%lu days, ", (unsigned long) t);
                                                            sprintf (upTime + strlen (upTime), "%02i:%02i:%02i", hours, minutes, seconds);

                                                            return "{\"id\":\"" HOSTNAME "\","
                                                                    "\"upTime\":\"" + String (upTime) + "\","
                                                                    "\"httpRequestCount\": " + httpRequestCount.toJson () + ","
                                                                    "\"freeHeap60\": " + freeHeap60.toJson () + ","
                                                                    "\"freeHeap24\": " + freeHeap24.toJson () + ","
                                                                    "\"freeBlock24\": " + freeBlock24.toJson () + ""
                                                                    "}";
                                                        }

    return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning ""
}

void wsRequestHandlerCallback (const char *httpRequest, httpServer_t::webSocket_t *webSck) {

    // Must be reentrant !!!
    

    #define httpRequestIs(X) (strstr(httpRequest,X)==httpRequest)
    
    #ifdef __OSCILLOSCOPE__
          if (httpRequestIs ("GET /runOscilloscope"))      runOscilloscope (webSck);      // used by oscilloscope.html
    #endif

    if (httpRequestIs ("GET /rssiReader"))  { 
                                                char c;
                                                do {
                                                    delay (100);
                                                    int i = WiFi.RSSI ();
                                                    c = (char) i;
                                                } while (webSck->sendBlock ((byte *) &c, sizeof (c))); // keep sending RSSI information as long as web browser is receiving it
                                            }
}


// ----- firewalll -----

bool firewallCallback (char *clientIP, char *serverIP) {

    // Must be reentrant !!!
    

    return true; // return false to refuse the connection for certain clientIP or serverIP 
}


// ----- cron handler -----

static bool timeAlreadySynchronized = false;

void cronHandlerCallback (const char *cronCommand) {

    // Does not have to reentrant but keep in mind that cronHandlerCallback may be running in a different task than setup-loop


    #define cronCommandIs(X) (!strcmp (cronCommand, X))

    // There are three special cronCommands: ONCE A MINUTE, ONCE AN HOUR, and ONCE A DAY.
    // These run at fixed intervals regardless of whether the ESP32 has already
    // obtained the correct time from NTP servers.
    // All other cronCommands run only after the correct time has been set.

        if (cronCommandIs ("ONCE A MINUTE"))    {   
                                                    // As long as the ESP32 has not yet obtained the correct time from
                                                    // NTP servers, try synchronizing once per minute.
                                                    if (!timeAlreadySynchronized) // 1600000000 ~2020
                                                        timeAlreadySynchronized = *(ntpClient_t ().syncTime ()) == 0; // syncTime did not return error message
                                                }
    else if (cronCommandIs ("ONCE A DAY"))      {
                                                    // Once the time is set, synchronize the internal clock with NTP servers daily.
                                                    // if (time (NULL) > 1600000000) // 1600000000 ~2020
                                                    ntpClient_t ().syncTime ();                                                                                    
                                                } 
    else if (cronCommandIs ("ONCE AN HOUR"))    {   
                                                    // Check once per hour whether the router is reachable.
                                                    wifi_mode_t wifiMode = WIFI_OFF;
                                                    if (esp_wifi_get_mode (&wifiMode) != ESP_OK) {
                                                        cout << ( dmesgQueue << "[cronHandlerCallback] " "couldn't get WiFi mode" );
                                                    } else {
                                                        if (wifiMode & WIFI_STA) { // WiFi works in STAtion mode  
                                                            ThreadSafePing_t routerPing (WiFi.gatewayIP ());
                                                            routerPing.ping (4);
                                                            if (!routerPing.received ()) {
                                                                cout << ( dmesgQueue << "[cronHandlerCallback] " "ping of router failed, reconnecting WiFi STAtion" );
                                                                WiFi.disconnect ();
                                                                WiFi.reconnect ();
                                                            }
                                                        }
                                                    }
                                                }

    // The following cronCommands can be provided either programmatically
    // (via cronTab.insert) or through the /etc/crontab file.

    else if (cronCommandIs ("gotTime"))         {
                                                    // "* * * * * * gotTime" is triggered only once — the moment the ESP32
                                                    // obtains the correct time from NTP servers for the first time.
                                                    time_t t = time (NULL);
                                                    struct tm st;
                                                    localtime_r (&t, &st);
                                                    cout << "Got time at " << st << " (local time), do whatever needs to be done the first time the time is known." << endl;
                                                }
    else if (cronCommandIs ("onMinute"))        {
                                                    // "0 * * * * * onMinute" occurs at the beginning of every minute.
                                                    // This is a good moment to update our demonstration measurement queues.
                                                    time_t t = time (NULL);
                                                    struct tm st;
                                                    localtime_r (&t, &st);
                                                    freeHeap60.push_back ( { (unsigned char) st.tm_min, (int16_t) (ESP.getFreeHeap () / 1024) } );                                                    
                                                    // Number of HTTP requests received during the last minute.
                                                    httpRequestCount.push_back_and_reset_valueCounter ((int16_t) st.tm_min);
                                                }
    else if (cronCommandIs ("onHour"))          {
                                                    // "0 0 * * * * onHour" occurs at the beginning of every hour.
                                                    // Another good moment to update hourly measurement queues.
                                                    time_t t = time (NULL);
                                                    struct tm st;
                                                    localtime_r (&t, &st);
                                                    freeHeap24.push_back ( { (unsigned char) st.tm_hour, (int16_t) (ESP.getFreeHeap () / 1024) } );
                                                    freeBlock24.push_back ( { (unsigned char) st.tm_hour, (int16_t) (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) / 1024) } );
                                                }
}


// returns        "/" for full access
// something like "/home/name" for limited access
//                "" for no access
Cstring<255> getUserHomeDirectoryCallback (const Cstring<64>& userName, const Cstring<64>& password) {

    // Must be reentrant !!!


    Cstring<255> retVal;
    // check if userName and password are correct
    if (userManagement && userManagement->checkUserNameAndPassword (userName, password))
        return userManagement->getHomeDirectory (userName);
    // else
    return "";  // access (login) denyed
}


void setup () {

    pinMode (LED_BUILTIN, OUTPUT | INPUT);
    digitalWrite (LED_BUILTIN, LOW);


    // Initialize console output.
    // Optional arguments: waitForSerial=false, waitAfterSerial=100 ms, serialSpeed=115200.
    cinit ();
    cout << showpoint;


    #ifdef LOCALE
        if (setlocale (lc_all, LOCALE))
            cout << ( dmesgQueue << "[locale] " "set to " << LOCALE );
        else
            cout << ( dmesgQueue << "[locale] " "could not set " << LOCALE );
    #endif


    // If a file system is used, mount it now.
    LittleFS.begin (true);
    // LittleFS.format ();


    // Create the userManagement instance.
    userManagement = new (std::nothrow) userManagement_t (TSFS);


    // Configure time zone prior to inserting events into cronTab for cronDaemon works with local time.
    ntpClient_t (TSFS, DEFAULT_NTP_SERVER_1, DEFAULT_NTP_SERVER_2, DEFAULT_NTP_SERVER_3);
    setenv ("TZ", zoneinfo (TSFS, DEFAULT_TZ), 1);
    tzset ();
    const char* tz = getenv ("TZ");
    if (tz)
        cout << ( dmesgQueue << "[time] TZ set to " << tz );
    else
        cout << ( dmesgQueue << "[time] TZ not set" );

    // ----- Demonstration entries — feel free to remove them -----
    cronTab.insert ("* * * * * * gotTime");   // triggers once — when ESP32 obtains time from NTP for the first time
    cronTab.insert ("0 * * * * * onMinute");  // triggers every minute at second 0
    cronTab.insert ("0 0 * * * * onHour");    // triggers every hour at 00:00
    //               | | | | | | |
    //               | | | | | | |___ cron command, this information will be passed to cronHandlerCallback when the time comes
    //               | | | | | |___ day of week (0 - 7 or *; Sunday=0 and also 7)
    //               | | | | |___ month (1 - 12 or *)
    //               | | | |___ day (1 - 31 or *)
    //               | | |___ hour (0 - 23 or *)
    //               | |___ minute (0 - 59 or *)
    //               |___ second (0 - 59 or *)


    // Connect to the WiFi router.
    WiFi.onEvent ([] (WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
            cout << ( dmesgQueue << "[WiFi][STA] " "connected" );
            #ifdef POWER_SAVING
                esp_err_t err = esp_wifi_set_ps (POWER_SAVING);
                if (err == ESP_OK)
                    cout << ( dmesgQueue << "[power saving] " "is on" );
                else
                    cout << ( dmesgQueue << "[power saving] " "couldn't set power saving" );
            #endif
        } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            cout << ( dmesgQueue << "[WiFi][STA] " "got IP address: " << WiFi.localIP () );
            timeAlreadySynchronized = *(ntpClient_t ().syncTime ()) == 0; // syncTime did not return error message
        }
    });
    // read WiFi settings from configuration files which are stored on TSFS
    WiFi_start (TSFS); // if not file system is used skip TSFS argument
    // if no file system is used call WiFi_start () with no arguments which uses DEFAULT_ ... definitions instead,
    // or simply call WiFi.begin ("YOUR STA SSID", "YOUR STA PASSWORD");


    // Start HTTP server. All the arguments are optional.
    httpServer = new (std::nothrow) httpServer_t (TSFS,                         // threadSafeFS::FS& fileSystem,
                                                  httpRequestHandlerCallback,   // String httpRequestHandlerCallback (const char *httpRequest, httpServer_t::httpConnection_t *hcn) = NULL,
                                                  wsRequestHandlerCallback);    // void (*wsRequestHandlerCallback) (const char *httpRequest, httpServer_t::webSocket_t *webSck) = NULL,
                                                                                // int serverPort = 80,
                                                                                // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,
                                                                                // bool runListenerInItsOwnTask = true

    if (httpServer && *httpServer)
        cout << ( dmesgQueue << "[httpServer] " "started" );
    else
        cout << ( dmesgQueue << "[httpServer] " "did not start" );

    // Start the FTP server. To save ~3 KB of RAM, the listener can run inside the
    // setup/loop task instead of its own task.
    // In this mode you must call ftpServer->accept() manually from loop().
    ftpServer = new (std::nothrow) ftpServer_t (TSFS,                           // threadSafeFS::FS& fileSystem,
                                                                                // Optional arguments:
                                                getUserHomeDirectoryCallback,   // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                21,                             // int serverPort = 21
                                                firewallCallback,               // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                false);                         // bool runListenerInItsOwnTask = true

    if (ftpServer && *ftpServer)
        cout << ( dmesgQueue << "[ftpServer] " "started" );
    else
        cout << ( dmesgQueue << "[ftpServer] " "did not start" );

    // Start the Telnet server. To save ~3 KB of RAM, the listener can run inside the
    // setup/loop task instead of its own task.
    // In this mode you must call telnetServer->accept() manually from loop().
    telnetServer = new (std::nothrow) telnetServer_t (TSFS,                         // threadSafeFS::FS& fileSystem,
                                                      getUserHomeDirectoryCallback, // Cstring<255> (*getUserHomeDirectory) (const Cstring<64>& userName, const Cstring<64>& password) = NULL
                                                      telnetCommandHandlerCallback, // String (*telnetCommandHandlerCallback) (int argc, char *argv [], telnetConnection_t *tcn) = NULL
                                                      23,                           // int serverPort = 23
                                                      firewallCallback,             // bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL
                                                      false);                       // bool runListenerInItsOwnTask = true

    if (telnetServer && *telnetServer)
        cout << (dmesgQueue << "[telnetServer] " "started");
    else
        cout << (dmesgQueue << "[telnetServer] " "did not start");


    #ifdef USE_OTA
        ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH) {
                    type = "sketch";
                } else {  // U_SPIFFS
                    type = "filesystem";
                }

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                if (millis() - last_ota_time > 500) {
                    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
                    last_ota_time = millis();
                }
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) {
                    Serial.println("Auth Failed");
                } else if (error == OTA_BEGIN_ERROR) {
                    Serial.println("Begin Failed");
                } else if (error == OTA_CONNECT_ERROR) {
                    Serial.println("Connect Failed");
                } else if (error == OTA_RECEIVE_ERROR) {
                    Serial.println("Receive Failed");
                } else if (error == OTA_END_ERROR) {
                    Serial.println("End Failed");
                }
            });

        ArduinoOTA.begin();

        Serial.println("Ready");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    #endif
}

void loop () {
    if (ftpServer) ftpServer->accept();      // Running without its own listening task, so call accept() here.
    if (telnetServer) telnetServer->accept(); // Same: no dedicated listening task, accept() must be called manually.
    // httpServer has its own listening task, so accept() is not called here.

    cronDaemon.nextRun(); // cronDaemon runs without its own task, so call nextRun() here.

    // Do not block loop() for too long; otherwise accept() may miss incoming connections.


    #ifdef USE_OTA
        ArduinoOTA.handle ();
    #endif
}
