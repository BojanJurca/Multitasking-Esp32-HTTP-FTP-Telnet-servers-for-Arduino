// this file is here just to make development of server files easier, it is not part of any project

#include <WiFi.h>

// #define SHOW_COMPILE_TIME_INFORMATION
#define HOSTNAME                  "MyESP32Server"
#define MACHINETYPE               "ESP32 Dev Modue"
#define DEFAULT_STA_SSID          "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
// #define DEFAULT_AP_SSID           HOSTNAME, leave undefined if you don't want to use AP
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"
#define TZ "CET-1CEST,M3.5.0,M10.5.0/3" // default: Europe/Ljubljana or choose another (POSIX) time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

// uncomment one of the tests at a time
// #define DBG
// #define TEST_DMESG
// #define TEST_FS
// #define TEST_FTP
 #define TEST_TELNET
// #define TEST_SMTP_CLIENT
// #define TEST_USER_MANAGEMENT
// #define TEST_TIME_FUNCTIONS
// #define TEST_HTTP_SERVER_AND_CLIENT


#ifdef DBG

    #include "tcpConnection.hpp"
    #include "tcpServer.hpp"

    void setup () {
        Serial.begin (115200);
        delay (3000);
        Serial.println ("DBG ...");
    }

    void loop () {

    }

#endif 




#ifdef TEST_DMESG // TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG 

    #include "dmesg.hpp"
    

    void setup () {
        Serial.begin (115200);
        delay (3000);
        Serial.println ("testing ...");

            Serial.printf ("v dmesgQueue je %i elementov\n", dmesgQueue.size ());
            for (int i = 0; i < dmesgQueue.size (); i++) {
                Serial.printf ("   %s\n", dmesgQueue [i].message.c_str ());
            }

        dmesgQueue << "[Test]";
        dmesgQueue << "[Test]" << " there are ";
        dmesgQueue << "[Test] there are " << 0;
        dmesgQueue << "[Test] there are " << 0 << " errors in dmesgQueue";

        dmesgQueue << String ("abc") << String ("def");
        dmesgQueue << Cstring<15> ("ABC") << Cstring<15> ("DEF");
        dmesgQueue << "[the last entry]";


        setlocale (lc_all, "sl_SI.UTF-8");
        dmesgQueue << (float) 1.2;
        dmesgQueue << (double) 3.14;
        dmesgQueue << time (NULL);


        for (auto e: dmesgQueue) { // use Iterator for scanning the dmesg queue because locking is already implemented here
            Serial.print (e.milliseconds);
            Serial.print (" ");
            Serial.println (e.message);
        }

        for (auto i = dmesgQueue.begin (); i != dmesgQueue.end (); ++ i) {
            Serial.print ((*i).milliseconds);
            Serial.print (" ");
            Serial.println ((*i).message);
        }

    }

    void loop () {
        delay (10000);

        static dmesgQueueEntry_t *lastBack = &dmesgQueue.back ();

        dmesgQueue << "next message";

        if (lastBack != &dmesgQueue.back ()) {
            Serial.println ("-------- dmesgQueue changed --------\n");
            lastBack = &dmesgQueue.back ();

            for (auto e: dmesgQueue) { // use Iterator for scanning the dmesg queue because locking is already implemented here
                Serial.print (e.milliseconds);
                Serial.print (" ");
                Serial.println (e.message);
            }

        }
    }

#endif // TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG TEST_DMESG 





#ifdef TEST_FS // TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS 

    #define SHOW_COMPILE_TIME_INFORMATION 

    // #define FILE_SYSTEM (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD) // FILE_SYSTEM_FAT or FILE_SYSTEM_LITTLEFS optionally bitwise combined with FILE_SYSTEM_SD_CARD
    #define FILE_SYSTEM FILE_SYSTEM_LITTLEFS
    #include "fileSystem.hpp"
    #include "netwk.h"
    #include "telnetServer.hpp"
    #include "ftpServer.hpp"
    #include "httpServer.hpp"

    void setup () {
        Serial.begin (115200);
        while (!Serial)
            delay (100);
        delay (1000);

        // fileSystem.format ();
        // fileSystem.formatFAT ();

        fileSystem.mountLittleFs (true);
        // fileSystem.formatLittleFs ();

        // fileSystem.mountFAT (true); // built-in falsh disk
        // fileSystem.mountSD ("/", 5); // SD Card if attached
    }

    void loop () {
      
    }

#endif // TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS TEST_FS 





#ifdef TEST_FTP // TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP

    // #define SHOW_COMPILE_TIME_INFORMATION

    #define FILE_SYSTEM             FILE_SYSTEM_LITTLEFS
    #define USER_MANAGEMENT         UNIX_LIKE_USER_MANAGEMENT // NO_USER_MANAGEMENT
    #include "dmesg.hpp"
    #include "fileSystem.hpp"
    #include "netwk.h"
    #include "userManagement.hpp"
    #include "ftpServer.hpp"

    ftpServer_t *myFtpServer = NULL;

void setup () {
    Serial.begin (115200);
    delay (3000);

    fileSystem.mountLittleFs (true);
    // fileSystem.formatLittleFs ();

    userManagement.initialize ();

    startWiFi (); // or just call:    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    myFtpServer = new ftpServer_t ();
    if (myFtpServer &&  *myFtpServer) {
        Serial.printf ("FTP server started\n");
    } else {
        Serial.printf ("FTP server did not start\n");
    }

    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP from router
        delay (1000); 
        Serial.printf ("   .\n"); 
    } 
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
    WiFi.enableIPv6 ();
    delay (100);
}

void loop () {

}

#endif // TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP 





#ifdef TEST_TELNET  // TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET 

  // include all .h files telnet server is using to test the whole functionality
  
//      #define FILE_SYSTEM FILE_SYSTEM_LITTLEFS // FILE_SYSTEM_FAT or FILE_SYSTEM_LITTLEFS optionally bitwise combined with FILE_SYSTEM_SD_CARD: (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD) 
 

      #define USE_mDNS

      // #include "std/locale.hpp"
      #include "std/console.hpp"
      #include "dmesg.hpp"
      #include "fileSystem.hpp"
      #include "netwk.h"
      #include "time_functions.h"
      // #include "httpClient.h"
      // #include "smtpClient.h"
      #define USER_MANAGEMENT   UNIX_LIKE_USER_MANAGEMENT   // NO_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT
      #include "userManagement.hpp"
      #include "telnetServer.hpp"


  String telnetCommandHandlerCallback (int argc, char *argv [], telnetServer_t::telnetConnection_t *tcn) {

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))    
    #ifndef LED_BUILTIN
        #define LED_BUILTIN 2
    #endif
    
            if (argv0is ("led") && argv1is ("state")) {                  // led state telnet command
              return "Led is " + String (digitalRead (LED_BUILTIN) ? "on." : "off.");
    } else if (argv0is ("turn") && argv1is ("led") && argv2is ("on")) {  // turn led on telnet command
              digitalWrite (LED_BUILTIN, HIGH);
              return "Led is on.";
    } else if (argv0is ("turn") && argv1is ("led") && argv2is ("off")) { // turn led off telnet command
              digitalWrite (LED_BUILTIN, LOW);
              return "Led is off.";
    }

    // let the Telnet server handle command itself
    return "";
  }

  // bool firewall (char *connectingclientIP, char*serverIP) { return true; }

  telnetServer_t *myTelnetServer; 

  void setup () {
    cinit ();

    fileSystem.mountLittleFs (true);
    // fileSystem.formatLittleFs ();
    // fileSystem.mountSD ("/SD", 5);
    // fileSystem.mountSD ("/", 5);

    userManagement.initialize ();

    // fileSystem.deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf"); // STA-tic credentials
    startWiFi ();

    do {
        delay (1000);
        Serial.print (".");
    } while (WiFi.status () != WL_CONNECTED);
    Serial.println ();

    startCronDaemon (NULL);

    myTelnetServer = new telnetServer_t (telnetCommandHandlerCallback);
    if (myTelnetServer && *myTelnetServer) // check if server instance creation succeded && the server is running as it should
        cout << "Telnet server is running :)\n";
    else
        cout << "Telnet server is not running :(\n";

    // setlocale (lc_all, "sl_SI.UTF-8");
  }

  void loop () {

  }

#endif // TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET 





#ifdef TEST_SMTP_CLIENT // TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT 

  #include "fileSystem.hpp"
  #include "netwk.h"
  #include "smtpClient.h"
  
  void setup () {
    Serial.begin (115200);

    fileSystem.mountLittleFs (false);
    startWiFi (); // for some strange reason you can't set time if WiFi is not initialized

    while (WiFi.localIP ().toString () == "0.0.0.0") { delay (1000); Serial.printf ("   .\n"); } // wait until we get IP from router
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
    delay (100);
    
    Serial.printf ("sendMail (...) = %s\n", sendMail ("<html><p style='color:green;'><b>Success!</b></p></html>", "smtpClient test 1", "bojan.***@***.com,sabina.***@***.com", "\"smtpClient\"<bojan.***@***.net>", "***", "***", 25, "smtp.siol.net"));

    Serial.printf ("sendMail (...) = %s\n", sendMail ("<html><p style='color:blue;'><b>Success!</b></p></html>", "smtpClient test 2", "bojan.***@***.com"));
  }

  void loop () {

  }

#endif // TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT





#ifdef TEST_USER_MANAGEMENT // TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT 

  #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT // NO_USER_MANAGEMENT  

  #include "fileSystem.hpp"
  #include "userManagement.hpp"
  
  void setup () {
    Serial.begin (115200);

    fileSystem.mountLittleFs (false);
    userManagement.initialize ();

    Serial.printf ("checkUserNameAndPassword (root, rootpassword) = %i\n", userManagement.checkUserNameAndPassword ((char *) "root", (char *) "rootpassword"));
    char s [FILE_PATH_MAX_LENGTH + 1];
    userManagement.getHomeDirectory (s, sizeof (s), (char *) "root");
    Serial.print ("getUserHomeDirectory (root) = ");  Serial.println (s);  

    Serial.printf ("userAdd (bojan, 1123, /home/bojan) = %s\n", userManagement.userAdd ((char *) "bojan", (char *) "1123", (char *) "/home/bojan"));
    Serial.printf ("userDel (bojan) = %s\n", userManagement.userDel ((char *) "bojan"));
  }

  void loop () {

  }

#endif // TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENTTEST_USER_MANAGEMENT





#ifdef TEST_TIME_FUNCTIONS // TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS

    #define FILE_SYSTEM FILE_SYSTEM_LITTLEFS 

    #define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3" // Europe/Ljubljana, all time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

    #include "fileSystem.hpp" // include fileSystem.hpp if you want cronDaemon to access /etc/ntp.conf and /etc/crontab
    #include "netwk.h"
    #include "time_functions.h"


    void cronHandler (char *cronCommand) {
      
        #define cronCommandIs(X) (!strcmp (cronCommand, X))  

        if (cronCommandIs ("gotTime")) { // triggers only once - when ESP32 reads time from NTP servers for the first time

            char buf [27]; // 26 bytes are needed
            ascTime (localTime (time ()), buf, sizeof (buf));

            Serial.printf ("%s (%s) Got time at %s (local time), ESP32 has been running %lu seconds. Do whatever needs to be done the first time the time is known.\r\n", __func__, cronCommand, buf, getUptime ());

            struct tm afterOneMinute = localTime (time () + 60); // 1 min = 60 s
            char s [100];
            sprintf (s, "%i %i %i * * * afterOneMinute", afterOneMinute.tm_sec, afterOneMinute.tm_min, afterOneMinute.tm_hour);
            if (cronTabAdd (s)) Serial.printf ("--- afterOneMinute --- added\n"); else Serial.printf ("--- afterOneMinute --- NOT added\n"); // change cronTab from within cronHandler
            return;
        } 

        if (cronCommandIs ("afterOneMinute")) { 
            Serial.printf ("ESP32 has been running %lu seconds. Do whatever needs to be done one minute after the time is known.\r\n", getUptime ());
            if (cronTabDel ((char *) "afterOneMinute")) Serial.printf ("--- afterOneMinute --- deleted\n"); else Serial.printf ("--- afterOneMinute --- NOT deleted\n"); // change cronTab from within cronHandler
            return;
        }

        if (cronCommandIs ("onMinute")) { // triggers each minute
            Serial.printf ("|  %6u  ", ESP.getFreeHeap ());
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_8BIT), heap_caps_get_largest_free_block (MALLOC_CAP_8BIT));
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_32BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_32BIT), heap_caps_get_largest_free_block (MALLOC_CAP_32BIT));
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_minimum_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
            Serial.printf ("|  %6u  %6u  %6u|\n", heap_caps_get_free_size (MALLOC_CAP_EXEC), heap_caps_get_minimum_free_size (MALLOC_CAP_EXEC), heap_caps_get_largest_free_block (MALLOC_CAP_EXEC));
            return;
        }

        return;
    }
    

    void setup () {
        Serial.begin (115200);
        // while (!Serial) delay (100);
        delay (3000);

        // set time zone
        setenv ("TZ", TIMEZONE, 1); 
        tzset ();

        
        // fileSystem.mount (false);
        // startWiFi (); 
        WiFi.mode (WIFI_STA);
        // WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);
        // while (WiFi.localIP ().toString () == "0.0.0.0") { Serial.println ("."); delay (1000); }

            Serial.println ("--- Time set to 1708387200 = 2024/20/02 00:00:00 - GMT");
            // setTimeOfDay (1708387200); // 2024/20/02 00:00:00 - GMT
                  ntpDate (  "1.si.pool.ntp.org"  );

            char buf [27]; 
            Serial.println ("--- GMT");
              Serial.println (time ());
              ascTime (gmTime (time ()), buf, sizeof (buf)); 
              Serial.println (buf);
            Serial.println ("--- Local");
            struct tm slt = localTime (time ());
            ascTime (slt, buf, sizeof (buf)); 
            Serial.println (buf);

        // startCronDaemon (cronHandler);
        // cronTabAdd ((char *) "* * * * * * gotTime"); 
        // cronTabAdd ((char *) "0 * * * * * onMinute"); 

        // Serial.printf ("|free heap| 8 bit free  min  max   |32 bit free  min  max   |default free  min  max  |exec free   min   max   |\n");
        // Serial.printf ("---------------------------------------------------------------------------------------------------------------\n");    

    }

    void loop () {

    }

#endif // TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS



#ifdef TEST_HTTP_SERVER_AND_CLIENT // TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT 

  // #define SHOW_COMPILE_TIME_INFORMATION

  // #define FILE_SYSTEM (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD) // FILE_SYSTEM_FAT or FILE_SYSTEM_LITTLEFS optionally bitwise combined with FILE_SYSTEM_SD_CARD
  #define FILE_SYSTEM FILE_SYSTEM_LITTLEFS 

  // #define HTTP_SERVER_CORE 0 

  #define WEB_SESSIONS
  #define USE_mDNS

  //#include "dmesg.hpp"
  #include "fileSystem.hpp" // include fileSystem.hpp if you want httpServer to server .html and other files as well
  #include "netwk.h"
  #define USE_WEB_SESSIONS
  #include "httpServer.hpp"
  #include "httpClient.h"
  #include "oscilloscope.h"

  httpServer_t *myHttpServer;

  String httpRequestHandlerCallback (char *httpRequest, httpServer_t::httpConnection_t *hcn) { 
    // input: char *httpRequest: the whole HTTP request but without ending \r\n\r\n
    // input example: GET / HTTP/1.1
    //                Host: 192.168.0.168
    //                User-Agent: curl/7.55.1
    //                Accept: */*
    // output: String: content part of HTTP reply, the header will be added by HTTP server before sending it to the client (browser)
    // output example: <html>
    //                   Congratulations, this is working.
    //                 </html>
    // must be reentrant, many httpRequestHandler may run simultaneously in different threads (tasks)
    // don't put anything on the heap (malloc, new, String, ...) for longer time, this would fragment ESP32's memmory

    // DEBUG: Serial.printf ("socket: %i    %s\n", hcn->socket (), httpRequest);

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

    if (httpRequestStartsWith ("GET / ")) {
                                            // play with cookies
                                            cstring s = hcn->getHttpRequestCookie ((char *) "refreshCount");
                                            Serial.printf ("This page has been refreshed %s times\n", s);
                                            hcn->setHttpReplyCookie ("refreshCount", cstring (atoi (s) + 1));
                                            // return large HTML reply from memory (9 KB in this case)
                                            return F("<html>\n"
                                                     "  <head>\n"
                                                     "    <meta http-equiv='content-type' content='text/html;charset=utf-8' />\n"
                                                     "\n" 
                                                     "    <title>Test</title>\n"
                                                     "\n" 
                                                     "    <style>\n"
                                                     "\n"
                                                     "      hr {border: 0; border-top: 1px solid lightgray; border-bottom: 1px solid lightgray}\n"
                                                     "\n" 
                                                     "      h1 {font-family: verdana; font-size: 40px; text-align: center}\n"
                                                     "\n" 
                                                     "\n"
                                                     "      div.d1 {position: relative; width: 100%; height: 40px; font-family: verdana; font-size: 30px; color: #2597f4;}\n"
                                                     "      div.d2 {position: relative; float: left; width: 50%; font-family: verdana; font-size: 30px; color: gray;}\n"
                                                     "      div.d3 {position: relative; float: left; width: 50%; font-family: verdana; font-size: 30px; color: black;}\n"
                                                     "\n" 
                                                     "    </style>\n"
                                                     "  </head>\n"
                                                     "   <body>\n"
                                                     "\n"     
                                                     "     <br><h1><small>Test of capabilities</small></h1>\n"
                                                     "     <div class='d1'>\n"
                                                     "       <b>HTML/REST/JSON <small>bi-directional communication with ESP32</small></b>\n"
                                                     "     </div>\n"
                                                     "     <hr />\n"
                                                     "     <div class='d1'>\n"
                                                     "       <div class='d2'>&nbspUp time <small></small></div>\n"
                                                     "       <div class='d3' id='upTime'>...</div>\n"
                                                     "     </div>\n"
                                                     "     <hr />\n"
                                                     "\n" 
                                                     "     <br><br>\n"
                                                     "     <div class='d1'>\n"
                                                     "       <b>HTML/WebSocket <small>data streaming</small></b>\n"
                                                     "     </div>\n"
                                                     "     <hr />\n"
                                                     "     <div class='d1'>\n"
                                                     "       <div class='d2'>&nbspRSSI <small></small></div>\n"
                                                     "       <div class='d3' id='rssi'>...</div>\n"
                                                     "     </div>\n"
                                                     "     <hr />\n"
                                                     "\n" 
                                                     "     <script type='text/javascript'>\n"
                                                     "       var httpClient=function(){\n"
                                                     "         this.request=function(url,method,callback){\n"
                                                     "          var httpRequest=new XMLHttpRequest();\n"
                                                     "          httpRequest.onreadystatechange=function(){\n"
                                                     "               if (httpRequest.readyState==4 && httpRequest.status==200) callback(httpRequest.responseText);\n"
                                                     "          }\n"
                                                     "          httpRequest.open(method,url,true);\n"
                                                     "          httpRequest.send(null);\n"
                                                     "         }\n"
                                                     "       }\n"
                                                     "       var client=new httpClient();\n"
                                                     "\n" 
                                                     "       function refreshUpTime(json){ // refresh up time with latest information from ESP\n"
                                                     "         document.getElementById('upTime').innerText=(JSON.parse(json).upTime) + ' s';\n"
                                                     "       }\n"
                                                     "       setInterval(function(){\n"
                                                     "         client.request('/upTime','GET',function(json){refreshUpTime(json);});\n"
                                                     "       }, 1000);\n"
                                                     "\n" 
                                                     "       // WebSocket streaming:\n"
                                                     "\n" 
                                                     "       if ('WebSocket' in window) {\n"
                                                     "         var ws = new WebSocket ('ws://' + self.location.host + '/rssiReader'); // open webSocket connection\n"
                                                     "         ws.onopen = function () {;};\n"
                                                     "         ws.onmessage = function (evt) {\n" 
                                                     "           if (evt.data instanceof Blob) { // RSSI readings as 1 byte binary data\n"
                                                     "             // receive binary data as blob and then convert it into array\n"
                                                     "             var myByteArray = null;\n"
                                                     "             var myArrayBuffer = null;\n"
                                                     "             var myFileReader = new FileReader ();\n"
                                                     "             myFileReader.onload = function (event) {\n"
                                                     "               myArrayBuffer = event.target.result;\n"
                                                     "               myByteArray = new Int8Array (myArrayBuffer); // <= RSSI information is now in the array of size 1\n"
                                                     "               document.getElementById('rssi').innerText = myByteArray [0].toString () + ' dBm';\n"
                                                     "             };\n"
                                                     "             myFileReader.readAsArrayBuffer (evt.data);\n"
                                                     "           }\n"
                                                     "         };\n"
                                                     "       } else {\n"
                                                     "         alert ('WebSockets are not supported by your browser.');\n"
                                                     "       }\n"
                                                     "\n" 
                                                     "    </script>\n"
                                                     "  </body>\n"
                                                     "</html>\n");
                                          }

    // return small calculated JSON replyes to REST requests
    if (httpRequestStartsWith ("GET /upTime ")) return "{\"upTime\":\"" + String (millis () / 1000) + "\"}";

    // let the HTTP server try to find a file and send it as a reply
    return "";
  }

            void example10_webSockets (httpServer_t::webSocket_t *webSck) {
              while (true) {
                switch (webSck->peek ())        {
                  case 0:                         delay (1); // not ready
                                                  break;
                  case 1:       { // text received STRING_FRAME_TYPE
                                                  cstring s;
                                                  webSck->recvString ((char *) s, s.max_size ()); //// bbb if
                                                  cout << "[example 10] got text from browser over webSocket: " << s << endl;
                                                  break;
                                                }
                  case 2:       { // binary data received BINARY_FRAME_TYPE
                                                  byte buffer [256];
                                                  int bytesRead = webSck->recvBlock (buffer, sizeof (buffer)); 
                                                  // if ... /// bbb

                                                  cout << "[example 10] got " << bytesRead << " bytes of binary data from browser over webSocket\n";
                                                  // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                                  // (I know they are 16 bit integers because I have written javascript client example myself but this information can not be obtained from webSocket)
                                                  int16_t *i = (int16_t *) buffer;
                                                  while ((byte *) (i + 1) <= buffer + bytesRead) 
                                                      cout << *i ++;
                                                  cout << "[example 10] if the sequence is -21 13 -8 5 -3 2 -1 1 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 6765 10946 17711 28657\n"
                                                          "             it means that both, endianness and complements are compatible with javascript client.\n";
                                                  // send text data
                                                  if (!webSck->sendString ("Thank you webSocket client, I'm sending back 8 32 bit binary floats.")) goto errorInCommunication;
            
                                                  // send binary data
                                                  float geometricSequence [8] = {1.0}; for (int i = 1; i < 8; i++) geometricSequence [i] = geometricSequence [i - 1] / 2;
                                                  if (!webSck->sendBlock ((byte *) geometricSequence, sizeof (geometricSequence))) goto errorInCommunication;
                                                                   
                                                  break; // this is where webSocket connection ends - in our simple "protocol" browser closes the connection but it could be the server as well ...
                                                         // ... just "return;" in this case (instead of break;)
                                                }
                  default: // WebSocket::ERROR, WebSocket::CLOSE, WebSocket::TIME_OUT
            errorInCommunication:     
                                                cout << "[example 10] closing WebSocket\n";
                                                return; // close this connection
                }
              }
            }

  void wsRequestHandlerCallback (char *httpRequest, httpServer_t::webSocket_t *webSck) { 
    // input: char *wsRequest: the whole HTTP request but without ending \r\n\r\n
    // input example: GET /rssiReader HTTP/1.1
    //                Host: 10.18.1.200
    //                Connection: Upgrade
    //                Pragma: no-cache
    //                Cache-Control: no-cache
    //                User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36
    //                Upgrade: websocket
    //                Origin: http://10.18.1.200
    //                Sec-WebSocket-Version: 13
    //                Accept-Encoding: gzip, deflate
    //                Accept-Language: sl-SI,sl;q=0.9,en-GB;q=0.8,en;q=0.7
    //                Sec-WebSocket-Key: mvODNT/nBotKV+3U6LGVIw==
    //                Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
    // must be reentrant, many wsRequestHandler may run simultaneously in different threads (tasks)
    // don't put anything on the heap (malloc, new, String, ...) for longer time, this would fragment ESP32's memmory

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

    if (httpRequestStartsWith ("GET /rssiReader"))  { // data streaming 
                                                        char c;
                                                        do {
                                                            delay (100);
                                                            int i = WiFi.RSSI ();
                                                            c = (char) i;
                                                        } while (webSck->sendBlock (&c,  sizeof (c))); // send RSSI information as long as web browser is willing to receive it
                                                    }
    else if (httpRequestStartsWith ("GET /example10_WebSockets")) example10_webSockets (webSck); // used by example10.html
  }


  bool firewall (char *clientIP, char *serverIP) { return true; }
    
  void setup () {
      Serial.begin (115200);

      fileSystem.mountLittleFs (true);
      startWiFi ();

      myHttpServer = new httpServer_t (httpRequestHandlerCallback, wsRequestHandlerCallback, 80, firewall);
      if (myHttpServer && *myHttpServer) {
          Serial.printf (":(\n");
      } else {
          Serial.printf (":)\n");
      }

      // Serial.printf ("|free heap| 8 bit free  min  max   |32 bit free  min  max   |default free  min  max  |exec free   min   max   |\n");
      // Serial.printf ("---------------------------------------------------------------------------------------------------------------\n");
  }

  void loop () {
      /*
      delay (121000); // wait slightly more then 2 minutes so that LwIP memory could recover

              Serial.printf ("|  %6u  ", ESP.getFreeHeap ());
              Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_8BIT), heap_caps_get_largest_free_block (MALLOC_CAP_8BIT));
              Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_32BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_32BIT), heap_caps_get_largest_free_block (MALLOC_CAP_32BIT));
              Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_minimum_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
              Serial.printf ("|  %6u  %6u  %6u|\n", heap_caps_get_free_size (MALLOC_CAP_EXEC), heap_caps_get_minimum_free_size (MALLOC_CAP_EXEC), heap_caps_get_largest_free_block (MALLOC_CAP_EXEC));

      {
          String r1 = httpRequest ((char *) "127.0.0.1", 80, (char *) "/upTime");
          if (r1.length ()) Serial.printf ("Got a reply from local loopback: %i bytes, %s\n", r1.length (), r1.c_str ());
      }
      */
  }

#endif // TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT
