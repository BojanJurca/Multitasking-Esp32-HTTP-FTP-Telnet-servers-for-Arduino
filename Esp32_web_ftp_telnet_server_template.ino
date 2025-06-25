/*

    Esp32_web_ftp_telnet_server_template.ino

        Compile this code with Arduino for:
            - one of ESP32 boards (Tools | Board) and 
            - one of SPIFFS partition schemes (Tools | Partition scheme) for LittleFS file system or FAT partition schemes for FAT file systm 

    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
     
    May 22, 2025, Bojan Jurca


    PLEASE NOTE THAT THIS FILE IS JUST A TEMPLATE. YOU CAN INCLUDE OR EXCLUDE FUNCTIONALITIES YOU NEED OR DON'T NEED IN Esp32_servers_config.h.
    YOU CAN FREELY DELETE OR MODIFY ALL THE CODE YOU DON'T NEED. SOME OF THE CODE IS PUT HERE MERELY FOR DEMONSTRATION PURPOSES AND IT IS 
    MARKED AS SUCH. JUST DELETE OR MODIFY IT ACCORDING TO YOUR NEEDS.

*/


#include <WiFi.h>
#include "./servers/std/console.hpp"

// --- PLEASE MODIFY THIS FILE FIRST! --- This is where you can configure your network credentials, which servers will be included, etc ...
#include "Esp32_servers_config.h"

#include "./servers/std/Cstring.hpp"

#ifdef USE_OTA
    #include <ESPmDNS.h>
    #include <NetworkUdp.h>
    #include <ArduinoOTA.h>
#endif


                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING DEFINITIONS -----
                    #include "measurements.hpp"
                    measurements<60> freeHeap;                  // measure free heap each minute for possible memory leaks
                    measurements<60> freeBlock;                 // measure max free heap block each minute for possible heap-related problems
                    measurements<60> httpRequestCount;          // measure how many web connections arrive each minute
                    #ifndef LED_BUILTIN
                        #define LED_BUILTIN 2                   // replace with your LED GPIO
                    #endif

 
// HTTP request handler example - if you don't need to handle HTTP requests yourself just delete this function and pass NULL to httpServer instead
// later in the code. httpRequestHandlerCallback function returns the content part of HTTP reply in HTML, json, ... format (header fields will be added by 
// httpServer before sending the HTTP reply back to the client) or empty string "". In this case, httpServer will try to handle the request
// completely by itself (like serving the right .html file, ...) or return an error code.
//
// PLEASE NOTE: since httpServer is implemented as a multitasking server, httpRequestHandlerCallback function can run in different tasks at the same time
//              therefore httpRequestHandlerCallback function must be reentrant.

String httpRequestHandlerCallback (char *httpRequest, httpServer_t::httpConnection_t *httpConnection) { 
    // httpServer will add HTTP header to the String that this callback function returns and then send everithing to the Web browser (the callback function is suppose to return only the content part of HTTP reply)

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)


                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE -----
                    httpRequestCount.increase_valueCounter ();                      // gether some statistics

                    // variables used by example 05
                    char niceSwitch1 [6] = "false";
                    static int niceSlider3 = 3;
                    char niceRadio5 [3] = "fm";

                    // ----- handle HTTP protocol requests -----
                        if (httpRequestStartsWith ("GET /example01.html "))         { // used by example 01: Dynamically generated HTML page
                                                                                        return "<HTML>Example 01 - dynamic HTML page<br><br><hr />" + String (digitalRead (LED_BUILTIN) ? "Led is on." : "Led is off.") + "<hr /></HTML>";
                                                                                    }
                    else if (httpRequestStartsWith ("GET /example07.html "))        { // used by example 07
                                                                                        cstring refreshCounter = httpConnection->getHttpRequestCookie ("refreshCounter");
                                                                                        if (refreshCounter == "") refreshCounter = "0";
                                                                                        refreshCounter = cstring (atoi (refreshCounter) + 1);
                                                                                        httpConnection->setHttpReplyCookie ("refreshCounter", refreshCounter, time () + 60);  // set 1 minute valid cookie that will be send to browser in HTTP reply
                                                                                        return "<HTML>Web cookies<br><br>This page has been refreshed " + String (refreshCounter) + " times. Click refresh to see more.</HTML>";
                                                                                    }                                                                                  
                    else if (httpRequestStartsWith ("GET /builtInLed "))            { // used by example 02, example 03, example 04, index.html: REST function for static HTML page
                                                                                        getBuiltInLed:
                                                                                            return "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + String (digitalRead (LED_BUILTIN) ? "on" : "off") + "\"}\r\n";
                                                                                    }                                                                    
                    else if (httpRequestStartsWith ("PUT /builtInLed/on "))         { // used by example 03, example 04: REST function for static HTML page
                                                                                        digitalWrite (LED_BUILTIN, HIGH);
                                                                                        goto getBuiltInLed;
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /builtInLed/off "))        { // used by example 03, example 04, index.html: REST function for static HTML page
                                                                                        digitalWrite (LED_BUILTIN, LOW);
                                                                                        goto getBuiltInLed;
                                                                                    }
                    else if (httpRequestStartsWith ("GET /state "))                 { // used by index.html: REST function for static HTML page
                                                                                        time_t t = getUptime ();       // t holds seconds
                                                                                        int seconds = t % 60; t /= 60; // t now holds minutes
                                                                                        int minutes = t % 60; t /= 60; // t now holds hours
                                                                                        int hours = t % 24;   t /= 24; // t now holds days
                                                                                        char upTime [25]; 
                                                                                        *upTime = 0;
                                                                                        if (t) 
                                                                                        sprintf (upTime, "%lu days, ", t);
                                                                                        sprintf (upTime + strlen (upTime), "%02i:%02i:%02i", hours, minutes, seconds);

                                                                                        return  "{\"id\":\"" HOSTNAME "\","
                                                                                                "\"upTime\":\"" + String (upTime) + "\","
                                                                                                "\"httpRequestCount\": " + httpRequestCount.toJson () + ","
                                                                                                "\"freeHeap\": " + freeHeap.toJson () + ","
                                                                                                "\"freeBlock\": " + freeBlock.toJson () + ""
                                                                                                "}";
                                                                                    }
                    else if (httpRequestStartsWith ("GET /niceSwitch1 "))           { // used by example 05.html: REST function for static HTML page
                                                                                        returnNiceSwitch1State:
                                                                                            return "{\"id\":\"niceSwitch1\",\"value\":\"" + String (niceSwitch1) + "\"}";
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /niceSwitch1/"))           { // used by example 05.html: REST function for static HTML page
                                                                                        *(httpRequest + 21) = 0;
                                                                                        strcpy (niceSwitch1, strstr (httpRequest + 17, "true") ? "true": "false");
                                                                                        goto returnNiceSwitch1State; // return success (or possible failure) back to the client
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /niceButton2/pressed "))   { // used by example 05.html: REST function for static HTML page
                                                                                        return "{\"id\":\"niceButton2\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                                    }
                    else if (httpRequestStartsWith ("GET /niceSlider3 "))           { // used by example 05.html: REST function for static HTML page
                                                                                        returnNiceSlider3Value:
                                                                                            return "{\"id\":\"niceSlider3\",\"value\":\"" + String (niceSlider3) + "\"}";
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /niceSlider3/"))           { // used by example 05.html: REST function for static HTML page
                                                                                        niceSlider3 = atoi (httpRequest + 17);
                                                                                        cout << "[Got request from web browser for niceSlider3]: " << niceSlider3 << endl;
                                                                                        goto returnNiceSlider3Value; // return success (or possible failure) back to the client
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /niceButton4/pressed "))   { // used by example 05.html: REST function for static HTML page
                                                                                        cout << "[Got request from web browser for niceButton4]: pressed\n";
                                                                                        return "{\"id\":\"niceButton4\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                                    }
                    else if (httpRequestStartsWith ("GET /niceRadio5 "))            { // used by example 05.html: REST function for static HTML page
                                                                                        returnNiceRadio5Value:
                                                                                            return "{\"id\":\"niceRadio5\",\"modulation\":\"" + String (niceRadio5) + "\"}";
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /niceRadio5/"))            { // used by example 05.html: REST function for static HTML page
                                                                                        httpRequest [18] = 0;
                                                                                        cout << httpRequest + 16 << endl;
                                                                                        strcpy (niceRadio5, strstr (httpRequest + 16, "am") ? "am" : "fm");
                                                                                        goto returnNiceRadio5Value; // return success (or possible failure) back to the client
                                                                                    }
                    else if (httpRequestStartsWith ("PUT /niceButton6/pressed "))   { // used by example 05.html: REST function for static HTML page
                                                                                        cout << "[Got request from web browser for niceButton6]: pressed\n";
                                                                                        return "{\"id\":\"niceButton6\",\"value\":\"pressed\"}"; // the client will actually not use this return value at all but we must return something
                                                                                    }

                    // ----- USED FOR DEMONSTRATION OF WEB SESSION HANDLING, YOU MAY FREELY DELETE THE FOLLOWING CODE -----
                    #ifdef USE_WEB_SESSIONS
                        // ----- web sessions - this part is still public, the users do not need to login -----

                        Cstring<64> sessionToken = httpConnection->getHttpRequestCookie ("sessionToken").c_str ();
                        Cstring<64>userName;
                        if (sessionToken > "") userName = httpConnection->getUserNameFromToken (sessionToken); // if userName > "" the user is loggedin

                        // REST call to login       
                        if (httpRequestStartsWith ("GET /login/")) { // GET /login/userName%20password - called (for example) from login.html when "Login" button is pressed
                            // delete sessionToken from cookie and database if it already exists
                            if (sessionToken > "") {
                                httpConnection->deleteWebSessionToken (sessionToken);
                                httpConnection->setHttpReplyCookie ("sessionToken", "");
                                httpConnection->setHttpReplyCookie ("sessionUser", "");
                            }
                            // check new login credentials and login
                            Cstring<64>password;
                            if (sscanf (httpRequest, "%*[^/]/login/%64[^%%]%%20%64s", (char *) userName, (char *) password) == 2) {
                                if (userManagement.checkUserNameAndPassword (userName, password)) { // check if they are OK against system users or find another way of authenticating the user
                                    sessionToken = httpConnection->newWebSessionToken (userName, WEB_SESSION_TIME_OUT == 0 ? 0 : time () + WEB_SESSION_TIME_OUT).c_str (); // use 0 for infinite
                                    httpConnection->setHttpReplyCookie ("sessionToken", (char *) sessionToken, WEB_SESSION_TIME_OUT == 0 ? 0 : time () + WEB_SESSION_TIME_OUT); // TIME_OUT is in sec, use 0 for infinite
                                    httpConnection->setHttpReplyCookie ("sessionUser", (char *) userName, WEB_SESSION_TIME_OUT == 0 ? 0 : time () + WEB_SESSION_TIME_OUT); // TIME_OUT is in sec, use 0 for infinite
                                    return "loggedIn"; // notify the client about success
                                } else {
                                    return "Not logged in. Wrong username and/or password."; // notify the client login.html about failure
                                }
                            }
                        }

                        // REST call to logout
                        if (httpRequestStartsWith ("PUT /logout ")) {
                            // delete token from the cookie and the database
                            httpConnection->deleteWebSessionToken (sessionToken);
                            httpConnection->setHttpReplyCookie ("sessionToken", "");
                            httpConnection->setHttpReplyCookie ("sessionUser", "");
                            return "Logged out.";
                        }

                        // ----- web sessions - this part is portected for loggedin users only -----
                        if (httpRequestStartsWith ("GET /protectedPage.html ") || httpRequestStartsWith ("GET /logout.html ")) { 
                            // check if the user is logged in
                            if (userName > "") {
                                // prolong token validity, both in the cookie and in the database
                                if (WEB_SESSION_TIME_OUT > 0) {
                                    httpConnection->updateWebSessionToken (sessionToken, userName, time () + WEB_SESSION_TIME_OUT);
                                    httpConnection->setHttpReplyCookie ("sessionToken", (char *) sessionToken, time () + WEB_SESSION_TIME_OUT);
                                    httpConnection->setHttpReplyCookie ("sessionUser", (char *) userName, time () + WEB_SESSION_TIME_OUT);
                                }
                                return ""; // httpServer will search for protectedPage.html after this function returns
                            } else {
                                // redirect client to login.html
                                httpConnection->setHttpReplyHeaderField ("Location", "/login.html");
                                httpConnection->setHttpReplyStatus ("307 temporary redirect");
                                return "Not logged in.";
                            }
                        }

                    #endif      

    return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


// WebSocket request handler example - if you don't web sockets just delete this function and pass NULL to httpServer instead later in the code. 
//
// PLEASE NOTE: since httpServer is implemented as a multitasking server, wsRequestHandlerCallback function can run in different tasks at the same time.
//              Therefore wsRequestHandlerCallback function must be reentrant.


            // Example 10 - basic WebSockets demonstration
            
            void example10_webSockets (httpServer_t::webSocket_t *webSocket) {
                while (true) {
                    switch (webSocket->peek ()) {
                        case 0:                           // not ready
                                                          delay (1);
                                                          break;
                        case STRING_FRAME_TYPE:           { // 1 = text received
                                                              cstring s;
                                                              webSocket->recvString ((char *) s, s.max_size ());
                                                              cout << "[example 10] got text from browser over webSocket: " << s << endl;
                                                              break;
                                                          }
                        case BINARY_FRAME_TYPE:           { // 2 = binary data received
                                                              byte buffer [256];
                                                              int bytesRead = webSocket->recvBlock (buffer, sizeof (buffer));
                                                              cout << "[example 10] got " << bytesRead << " bytes of binary data from browser over webSocket\n";
                                                              // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                                              // (I know they are 16 bit integers because I have written javascript client example myself but this information can not be obtained from webSocket)
                                                              int16_t *i = (int16_t *) buffer;
                                                              while ((byte *) (i + 1) <= buffer + bytesRead) 
                                                                  cout << *i ++;
                                                              cout << "[example 10] if the sequence is -21 13 -8 5 -3 2 -1 1 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 6765 10946 17711 28657\n"
                                                                      "             it means that both, endianness and complements are compatible with javascript client.\n";
                                                              // send text data
                                                              if (!webSocket->sendString ("Thank you webSocket client, I'm sending back 8 32 bit binary floats.")) goto errorInCommunication;
                        
                                                              // send binary data
                                                              float geometricSequence [8] = {1.0}; for (int i = 1; i < 8; i++) geometricSequence [i] = geometricSequence [i - 1] / 2;
                                                              if (!webSocket->sendBlock ((byte *) geometricSequence, sizeof (geometricSequence))) goto errorInCommunication;
                                                                              
                                                              break;  // this is where webSocket connection ends - in our simple "protocol" browser closes the connection but it could be the server as well ...
                                                                      // ... just "return;" in this case (instead of break;)
                                                          }
                        default: // -1 = error, cosed or time-out
                    errorInCommunication:     
                                                          cout << "[example 10] closing WebSocket\n";
                                                          return; // close this connection
                    }
                }
            }

void wsRequestHandlerCallback (char *httpRequest, httpServer_t::webSocket_t *webSocket) {
    // this callback function is supposed to handle WebSocket communication - once it returns WebSocket will be closed
  
    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)
    
    #ifdef __OSCILLOSCOPE__
          if (httpRequestStartsWith ("GET /runOscilloscope"))       runOscilloscope (webSocket);      // used by oscilloscope.html
    #endif

                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE -----              
                    if (httpRequestStartsWith ("GET /rssiReader"))  { 

                        unsigned long startMillis = millis ();
                        char c;
                        do {
                            delay (100);
                            int i = WiFi.RSSI ();
                            c = (char) i;
                        } while (webSocket->sendBlock ((byte *) &c, sizeof (c))); // keep sending RSSI information as long as web browser is receiving it

                    } else if (httpRequestStartsWith ("GET /example10_WebSockets"))  {

                        example10_webSockets (webSocket); // used by example10.html
                    
                    }
}


// Telnet command handler example - if you don't need to telnet commands yourself just delete this function and pass NULL to telnetSrv instead
// later in the code. telnetCommandHandlerCallback function returns the command's output (reply) or empty string "". In this case, telnetServer 
// will try to handle the command completely by itself (serving already built-in commands, like ls, vi, ...) or return an error message.  
// PLEASE NOTE: since telnetServer is implemented as a multitasking server, telnetCommandHandlerCallback function can run in different tasks at the same time.
//              Therefore telnetCommandHandlerCallback function must be reentrant.

String telnetCommandHandlerCallback (int argc, char *argv [], telnetServer_t::telnetConnection_t *telnetConnection) {
    // the String that this callback function returns will be written to Telnet client console as a response to Telnet command (already parsed to argv)

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))
    // add more #definitions if neeed 

                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE -----     
                            if (argv0is ("led") && argv1is ("state"))                 { // led state telnet command
                                                                                          return "Led is " + String (digitalRead (LED_BUILTIN) ? "on." : "off.");
                                                                                      } 
                    else if (argv0is ("turn") && argv1is ("led") && argv2is ("on"))   { // turn led on telnet command
                                                                                          digitalWrite (LED_BUILTIN, HIGH);
                                                                                          return "Led is on.";
                                                                                      }
                    else if (argv0is ("turn") && argv1is ("led") && argv2is ("off"))  { // turn led off telnet command
                                                                                          digitalWrite (LED_BUILTIN, LOW);
                                                                                          return "Led is off.";
                                                                                      }

                    // ----- USED FOR DEMONSTRATION OF RETRIEVING INFORMATION FROM WEB SESSION TOKEN DATABASE, YOU MAY FREELY DELETE THE FOLLOWING CODE -----
                    #ifdef USE_WEB_SESSIONS
                        if (argv0is ("web") && argv1is ("sessions") && argv2is ("truncate")) {
                            webSessionTokenDatabase.Truncate ();
                            return "\r";
                        } else if (argv0is ("web") && argv1is ("sessions")) {  
                            signed char e; // or just int
                            webSessionTokenInformation_t webSessionTokenInformation;
                            telnetConnection->sendString ((char *) "web session token                                                valid for  user name\n\r-------------------------------------------------------------------------------------");
                            for (auto p: webSessionTokenDatabase) {
                                e = webSessionTokenDatabase.FindValue (p.key, &webSessionTokenInformation, p.blockOffset);
                                if (e == err_ok && time () && webSessionTokenInformation.expires > time ()) {
                                    char c [180];
                                    sprintf (c, "\n\r%s %5li s    %s", p.key.c_str (), webSessionTokenInformation.expires - time (), webSessionTokenInformation.userName.c_str ());
                                    telnetConnection->sendString (c);
                                }
                            }
                            return "\r"; // in fact nothing, but if the handler returns "", the Telnet server will try to handle the command internally, so just return a harmless \r
                        }
                    #endif

                    // used to turn power saving mode off before OTA upgrade
                    else if (argv0is ("power") && argv1is ("saving"))                   { 
                                                                                            esp_wifi_set_ps (POVER_SAVING_MODE);
                                                                                            if (POVER_SAVING_MODE == WIFI_PS_NONE)
                                                                                                return "WiFi is set to no power saving mode";
                                                                                            else
                                                                                                return "WiFi is set to power saving mode";
                                                                                        }
                    else if (argv0is ("no") && argv1is ("power") && argv2is ("saving")) { 
                                                                                            esp_wifi_set_ps (WIFI_PS_NONE);
                                                                                            return "WiFi is set to no power saving mode";
                                                                                        }


    return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning "" reply
}


                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE AND PASS NULL TO THE SERVERS LATER IN THE CODE INSTEAD -----               
                    bool firewallCallback (char *clientIP, char *serverIP) {  // firewall callback function, return true if IP is accepted or false if not - must be reentrant!
                        return true;                                          // let everithing through in this example
                    }


// Cron command handler example - if you don't need it just delete this function and pass NULL to startCoronDaemon function instead
// later in the code. 

void cronHandlerCallback (const char *cronCommand) {
    // this callback function is supposed to handle cron commands/events that occur at time specified in crontab

    #define cronCommandIs(X) (!strcmp (cronCommand, X))  

                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE -----
                    if (cronCommandIs ("ONCE_AN_HOUR"))                         {   // built-in cron event, triggers once an hour even if the time is not (yet) known
                                                                                    // check if ESP32 works in WIFI_STA mode
                                                                                    wifi_mode_t wifiMode = WIFI_OFF;
                                                                                    if (esp_wifi_get_mode (&wifiMode) != ESP_OK) {
                                                                                        #ifdef __DMESG__
                                                                                            dmesgQueue << "[cronHandlerCallback] couldn't get WiFi mode";
                                                                                        #endif
                                                                                        cout << "[cronHandlerCallback] couldn't get WiFi mode\n";
                                                                                    } else {
                                                                                        if (wifiMode & WIFI_STA) { // WiFi works in STAtion mode  
                                                                                            // is  STAtion mode is diconnected?
                                                                                            if (!WiFi.isConnected ()) { 
                                                                                                #ifdef __DMESG__
                                                                                                    dmesgQueue << "[cronHandlerCallback] STAtion disconnected, reconnecting to WiFi";
                                                                                                #endif
                                                                                                cout << "[cronHandlerCallback] STAtion disconnected, reconnecting to WiFi\n";
                                                                                                WiFi.reconnect (); 
                                                                                            } else { // check if it really works anyway 
                                                                                                esp32_ping routerPing (WiFi.gatewayIP ().toString ().c_str ());
                                                                                                for (int i = 0; i < 4; i++) {
                                                                                                    routerPing.ping (1);
                                                                                                    if (routerPing.received ())
                                                                                                        break;
                                                                                                    delay (1000);
                                                                                                }
                                                                                                if (!routerPing.received ()) {
                                                                                                    #ifdef __DMESG__
                                                                                                        dmesgQueue << "[cronHandlerCallback] ping of router failed, reconnecting WiFi STAtion";
                                                                                                    #endif
                                                                                                    cout << "[cronHandlerCallback] ping of router failed, reconnecting WiFi STAtion\n";
                                                                                                    WiFi.reconnect (); 
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                    else if (cronCommandIs ("gotTime"))                         {   // triggers only once - the first time ESP32 sets its clock (when it gets time from NTP server for example)
                                                                                    // char buf [26]; // 26 bytes are needed
                                                                                    // ascTime (localTime (time ()), buf, sizeof (buf));
                                                                                    // Serial.println ("Got time at " + String (buf) + " (local time), do whatever needs to be done the first time the time is known.");
                                                                                    cout << "Got time at " << time () << " (local time), do whatever needs to be done the first time the time is known.";
                                                                                }           
                    else if (cronCommandIs ("newYear'sGreetingsToProgrammer"))  {   // triggers at the beginning of each year
                                                                                    cout << "[cronHandlerCallback] *** HAPPY NEW YEAR ***!\n";
                                                                                }
                    else if (cronCommandIs ("onMinute"))                        {   // triggers each minute - collect some measurements for the purpose of this demonstration
                                                                                    struct tm st = localTime (time ()); 
                                                                                    freeHeap.push_back ( { (unsigned char) st.tm_min, (int) (ESP.getFreeHeap () / 1024) } ); // take s sample of free heap in KB 
                                                                                    freeBlock.push_back ( { (unsigned char) st.tm_min, (int) (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) / 1024) } ); // take s sample of free heap in KB 
                                                                                    httpRequestCount.push_back_and_reset_valueCounter (st.tm_min);          // take sample of number of web connections that arrived last minute 
                                                                                }
}


void setup () {
    cinit (); // Serial.begin (115200);
    cout << "[server] " MACHINETYPE "(" << (int) ESP.getCpuFreqMHz () << " MHz) " HOSTNAME " SDK: " << ESP.getSdkVersion () << " " VERSION_OF_SERVERS " compiled at: " __DATE__ " " __TIME__ << endl; 


    #ifdef FILE_SYSTEM
        // 1. Mount file system - this is the first thing to do since all the configuration files reside on the file system
        #if (FILE_SYSTEM & FILE_SYSTEM_LITTLEFS) == FILE_SYSTEM_LITTLEFS
            fileSystem.mountLittleFs (true);                                            
            // fileSystem.formatLittleFs (); Serial.printf ("\nFormatting file system with LittleFS ...\n\n"); // format flash disk to start everithing from the scratch
        #endif
        #if (FILE_SYSTEM & FILE_SYSTEM_FAT) == FILE_SYSTEM_FAT
            fileSystem.mountFAT (true);
            // fileSystem.formatFAT (); Serial.printf ("\nFormatting file system with FAT ...\n\n"); // format flash disk to start everithing from the scratch            
        #endif
        #if (FILE_SYSTEM & FILE_SYSTEM_SD_CARD) == FILE_SYSTEM_SD_CARD
            fileSystem.mountSD ("/SD", 5); // SD Card if attached, provide the mount point and CS pin
        #endif
    #endif


    // 2. Start the WiFi (STAtion and/or A(ccess) P(oint), DHCP or static IP, depending on the configuration files.
    // fileSystem.deleteFile ("/network/interfaces");                         // contation STA(tion) configuration       - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf");     // contation STA(tion) credentials         - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/dhcpcd.conf");                            // contains A(ccess) P(oint) configuration - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/hostapd/hostapd.conf");                   // contains A(ccess) P(oint) credentials   - deleting this file would cause creating default one
    startWiFi ();   


    // 3. Start cron daemon - it will synchronize internal ESP32's clock with NTP servers once a day and execute cron commands.
    // fileSystem.deleteFile ("/usr/share/zoneinfo");                         // contains timezone information           - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/ntp.conf");                               // contains ntp server names for time sync - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/crontab");                                // scontains cheduled tasks                - deleting this file would cause creating empty one

                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE -----
                    cronTab.insert ("* * * * * * gotTime");  // triggers only once - when ESP32 reads time from NTP servers for the first time
                    cronTab.insert ("0 0 0 1 1 * newYear'sGreetingsToProgrammer");  // triggers at the beginning of each year
                    cronTab.insert ("0 * * * * * onMinute");                        // triggers each minute at 0 seconds
                    cronTab.insert ("0 0 * * * * onHour");                          // triggers each hour at 0:0
                    //               | | | | | | |
                    //               | | | | | | |___ cron command, this information will be passed to cronHandlerCallback when the time comes
                    //               | | | | | |___ day of week (0 - 7 or *; Sunday=0 and also 7)
                    //               | | | | |___ month (1 - 12 or *)
                    //               | | | |___ day (1 - 31 or *)
                    //               | | |___ hour (0 - 23 or *)
                    //               | |___ minute (0 - 59 or *)
                    //               |___ second (0 - 59 or *)

    cronDaemon.start (cronHandlerCallback);


    // 4. Write the default user management files /etc/passwd and /etc/passwd it they don't exist yet (it only makes sense with UNIX_LIKE_USER_MANAGEMENT).
    // fileSystem.deleteFile ("/etc/passwd");                                 // contains users' accounts information    - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/passwd");                                 // contains users' passwords               - deleting this file would cause creating default one
    userManagement.initialize ();


    // 5. Start the servers
    #ifdef __HTTP_SERVER__                                         // all the arguments are optional
        httpServer_t *httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback, // (optional) a callback function that will handle HTTP requests that are not handled by webServer itself
                                                                    wsRequestHandlerCallback,   // (optional) a callback function that will handle WS requests, NULL to ignore WS requests
                                                                    80,                         // (optional) default HTTP port
                                                                    NULL                        // (optional) we won't use firewallCallback function for HTTP server FOR DEMONSTRATION PURPOSES
                                                                   );
        if (httpServer && *httpServer) {
            cout << "[httpServer] started\n";
        } else {
            #ifdef __DMESG__
                dmesgQueue << "[httpServer] did not start";
            #endif
            cout << "[httpServer] did not start" << endl;          
        }
    #endif
    #ifdef __FTP_SERVER__                                       // all the arguments are optional
        ftpServer_t *ftpServer = new (std::nothrow) ftpServer_t (21,              // (optional) default FTP port
                                                                 firewallCallback // (optional) let's use firewallCallback function for FTP server FOR DEMONSTRATION PURPOSES ONLY
                                                                ); 
        if (ftpServer && *ftpServer) {
            cout << "[ftpServer] started\n";
        } else {
            #ifdef __DMESG__
                dmesgQueue << "[ftpServer] did not start";
            #endif
            cout << "[ftpServer] did not start" << endl;                    
        }
    #endif
    #ifdef __TELNET_SERVER__                                             // all the arguments are optional
        telnetServer_t *telnetServer = new (std::nothrow) telnetServer_t (telnetCommandHandlerCallback, // (optional) a callback function that will handle Telnet commands that are not handled by telnetServer itself
                                                                          23,                           // (optional) default Telnet port
                                                                          firewallCallback              // (optional) let's use firewallCallback function for Telnet server FOR DEMONSTRATION PURPOSES ONLY
                                                                         );            
        if (telnetServer && *telnetServer) {
            cout << "[telnetServer] started\n";
        } else {
            #ifdef __DMESG__
                dmesgQueue << "[telnetServer] did not start";
            #endif
            cout << "[telnetServer] did not start" << endl;
        }
    #endif


                    // ----- USED FOR DEMONSTRATION ONLY, YOU MAY FREELY DELETE THE FOLLOWING CODE -----
                    pinMode (LED_BUILTIN, OUTPUT | INPUT);
                    digitalWrite (LED_BUILTIN, LOW);


    #ifdef USE_OTA
        ArduinoOTA
            .onStart ([] () {
                String type;
                if (ArduinoOTA.getCommand () == U_FLASH) {
                    type = "sketch";
                } else {  // U_SPIFFS
                    type = "filesystem";
                }

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println ("Start updating " + type);
            })
            .onEnd ([] () {
                Serial.println ("\nEnd");
            })
            .onProgress ([] (unsigned int progress, unsigned int total) {
                Serial.printf ("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError ([] (ota_error_t error) {
                Serial.printf ("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) {
                    Serial.println ("Auth Failed");
                } else if (error == OTA_BEGIN_ERROR) {
                    Serial.println ("Begin Failed");
                } else if (error == OTA_CONNECT_ERROR) {
                    Serial.println ("Connect Failed");
                } else if (error == OTA_RECEIVE_ERROR) {
                    Serial.println ("Receive Failed");
                } else if (error == OTA_END_ERROR) {
                    Serial.println ("End Failed");
                }
            });

        ArduinoOTA.begin ();

        Serial.println ("Ready");
        Serial.print ("IP address: ");
        Serial.println (WiFi.localIP ());
    #endif
}

void loop () {
    #ifdef USE_OTA
        ArduinoOTA.handle ();
    #endif
}
