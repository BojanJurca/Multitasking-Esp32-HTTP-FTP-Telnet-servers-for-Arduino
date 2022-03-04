// this file is here just to make development of server files easier, it is not part of any project

#include <WiFi.h>

#define HOSTNAME                  "MyESP32Server"
#define MACHINETYPE               "ESP32 Dev Modue"
#define DEFAULT_STA_SSID          "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME 
#define DEFAULT_AP_PASSWORD       "" // "YOUR_AP_PASSWORD"





// #define TEST_FTP
#ifdef TEST_FTP // TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP

    #include "dmesg_functions.h"
    #include "perfMon.h"
    
    #include "network.h"
    
    #include "file_system.h"

    #include "time_functions.h"
      #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT // NO_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT
    #include "user_management.h"
  #include "ftpServer.hpp"

  #include "ftpClient.h"

  ftpServer *myFtpServer = NULL;

  void setup () {
    Serial.begin (115200);

    mountFileSystem (true); 
    startWiFi ();
    startCronDaemon (NULL);

    myFtpServer = new ftpServer ((char *) "0.0.0.0", 21, NULL);
    if (myFtpServer->state () != ftpServer::RUNNING) {
      Serial.printf (":(\n");
    } else {
      Serial.printf (":)\n");
    }    

    while (WiFi.localIP ().toString () == "0.0.0.0") { delay (1000); Serial.printf ("   .\n"); } // wait until we get IP from router
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
    delay (100);

    Serial.println (__ftpClient__ ("PUT", "/b", "/a", (char *) "root", (char *) "root", 21, (char *) "10.18.1.200"));  
  }

  void loop () {
    
  }

#endif // TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP TEST_FTP 





#define TEST_TELNET 
#ifdef TEST_TELNET  // TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET 

  // include all .h files telnet server is using to test the whole functionality
      #include "dmesg_functions.h"
      #include "perfMon.h"
      #include "file_system.h"
      #include "network.h"
      #include "time_functions.h"
      #include "httpClient.h"
      #include "ftpClient.h"
      #include "smtpClient.h"

        #define USER_MANAGEMENT NO_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT
      #include "user_management.h"

  #include "telnetServer.hpp"

  String telnetCommandHandlerCallback (int argc, char *argv [], telnetConnection *tcn) {

    #define argv0is(X) (argc > 0 && !strcmp (argv[0], X))  
    #define argv1is(X) (argc > 1 && !strcmp (argv[1], X))
    #define argv2is(X) (argc > 2 && !strcmp (argv[2], X))    
    #define LED_BUILTIN 2
    
            if (argv0is ("led") && argv1is ("state")) {                    // led state telnet command
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

  bool firewall (char *connectingIP) { return true; }

  telnetServer *myTelnetServer; 

  void setup () {
    Serial.begin (115200); Serial.printf ("\r\n\r\n\r\n\r\n");

    mountFileSystem (true); 
    startWiFi ();
    startCronDaemon (NULL);

    myTelnetServer = new telnetServer (telnetCommandHandlerCallback, (char *) "0.0.0.0", 23, firewall);
    if (myTelnetServer->state () != telnetServer::RUNNING) {
      Serial.printf (":(\n");
    } else {
      Serial.printf (":)\n");
    }
    
  }

  void loop () {

  }

#endif // TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET TEST_TELNET 






// #define TEST_SMTP_CLIENT
#ifdef TEST_SMTP_CLIENT // TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT 

  #include "network.h"
  #include "file_system.h"
  #include "smtpClient.h"
  
  void setup () {
    Serial.begin (115200);

    mountFileSystem (false);
    startWiFi (); // for some strange reason you can't set time if WiFi is not initialized

    while (WiFi.localIP ().toString () == "0.0.0.0") { delay (1000); Serial.printf ("   .\n"); } // wait until we get IP from router
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
    delay (100);
    
    Serial.printf ("sendMail (...) = %s\n", (char *) sendMail ((char *) "<html><p style='color:green;'><b>Success!</b></p></html>", (char *) "smtpClient test 1", (char *) "bojan.***@***.com,sabina.***@***.com", (char *) "\"smtpClient\"<bojan.***@***.net>", (char *) "***", (char *) "***", 25, (char *) "smtp.siol.net").c_str ());

    Serial.printf ("sendMail (...) = %s\n", (char *) sendMail ((char *) "<html><p style='color:blue;'><b>Success!</b></p></html>", (char *) "smtpClient test 2", (char *) "bojan.jurca@gmail.com").c_str ());
  }

  void loop () {

  }

#endif // TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT TEST_SMTP_CLIENT





// #define TEST_USER_MANAGEMENT
#ifdef TEST_USER_MANAGEMENT // TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT 

  #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT // HARDCODED_USER_MANAGEMENT // NO_USER_MANAGEMENT  

  #include "user_management.h"
  
  void setup () {
    Serial.begin (115200);

    mountFileSystem (false);
    initializeUsers ();

    Serial.printf ("checkUserNameAndPassword (root, rootpassword) = %i\n", checkUserNameAndPassword ((char *) "root", (char *) "rootpassword"));
    Serial.println ("getUserHomeDirectory (root) = " + getUserHomeDirectory ((char *) "root"));  

    Serial.printf ("userAdd (bojan, 1123, /home/bojan) = %s\n", userAdd ((char *) "bojan", (char *) "1123", (char *) "/home/bojan"));
    Serial.printf ("userDel (bojan) = %s\n", userDel ((char *) "bojan"));
  }

  void loop () {

  }

#endif // TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENT TEST_USER_MANAGEMENTTEST_USER_MANAGEMENT





// #define TEST_TIME_FUNCTIONS
#ifdef TEST_TIME_FUNCTIONS // TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS

  #include "network.h"
  #include "file_system.h" // include file_system.h if you want cronDaemon to access /etc/ntp.conf and /etc/crontab

  void cronHandler (char *cronCommand) {
  
    #define cronCommandIs(X) (!strcmp (cronCommand, X))  

    if (cronCommandIs ("gotTime")) { // triggers only once - when ESP32 reads time from NTP servers for the first time
      Serial.println ("Got time, do whatever needs to be done the first time the time is known.");
      struct tm afterOneHour = timeToStructTime (timeToLocalTime (getGmt () + 60)); // 1 hour = 3600
      char s [100];
      sprintf (s, "%i %i %i * * * afterOneHour", afterOneHour.tm_sec, afterOneHour.tm_min, afterOneHour.tm_hour);
      if (cronTabAdd (s)) Serial.printf ("--- afterOneHour --- added\n"); else Serial.printf ("--- afterOneHour --- NOT added\n"); // change cronTab from within cronHandler
      return;
    } 

    if (cronCommandIs ("afterOneHour")) { 
      Serial.println ("Do whatever needs to be done one hour after the time is known.");
      if (cronTabDel ("afterOneHour")) Serial.printf ("--- afterOneHour --- deleted\n"); else Serial.printf ("--- afterOneHour --- NOT deleted\n"); // change cronTab from within cronHandler
      return;
    }
    
    if (cronCommandIs ("onMinute")) { // triggers each minute
            Serial.printf ("|  %6u  ", ESP.getFreeHeap ());
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_8BIT), heap_caps_get_largest_free_block (MALLOC_CAP_8BIT));
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_32BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_32BIT), heap_caps_get_largest_free_block (MALLOC_CAP_32BIT));
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_minimum_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
            Serial.printf ("|  %6u  %6u  %6u|\n", heap_caps_get_free_size (MALLOC_CAP_EXEC), heap_caps_get_minimum_free_size (MALLOC_CAP_EXEC), heap_caps_get_largest_free_block (MALLOC_CAP_EXEC));      

      Serial.println (cronTab ());
      return;
    }

    return;
  }
  

  void setup () {
    Serial.begin (115200);

    // for (default) CET_TIMEZONE
    Serial.printf ("\n      Testing summer local time change\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1616893200 - 2; testTime < 1616893200 + 2; testTime ++) {
      setGmt (testTime);
      time_t localTestTime = getLocalTime ();
      Serial.printf ("      %llu | ", (unsigned long long) testTime);
      char s [50];
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", (int) (localTestTime - testTime));
    }
    Serial.printf ("\n      Testing autumn local time change\n");
    Serial.printf ("      ----------------------------------------------------------------\n");
    for (time_t testTime = 1635642000 - 2; testTime < 1635642000 + 2; testTime ++) {
      setGmt (testTime);
      time_t localTestTime = getLocalTime ();
      Serial.printf ("      %llu | ", (unsigned long long) testTime);
      char s [50];
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&testTime));
      Serial.printf ("%s | ", s);
      strftime (s, 50, "%Y/%m/%d %H:%M:%S", gmtime (&localTestTime));
      Serial.printf ("%s | ", s);
      Serial.printf ("%i\n", (int) (localTestTime - testTime));
    }    

    mountFileSystem (false);
    startWiFi (); // for some strange reason you can't set time if WiFi is not initialized

    startCronDaemon (cronHandler);
    cronTabAdd ("* * * * * * gotTime"); 
    cronTabAdd ("0 * * * * * onMinute"); 

            Serial.printf ("|free heap| 8 bit free  min  max   |32 bit free  min  max   |default free  min  max  |exec free   min   max   |\n");
            Serial.printf ("---------------------------------------------------------------------------------------------------------------\n");    
  }

  void loop () {

  }

#endif // TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS TEST_TIME_FUNCTIONS





// #define TEST_HTTP_SERVER_AND_CLIENT
#ifdef TEST_HTTP_SERVER_AND_CLIENT // TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT 

  #include "dmesg_functions.h"
  #include "network.h"
  #include "file_system.h" // include file_system.h if you want httpServer to server .html and other files as well
  #include "syslog.h"
  #include "perfMon.h"  // include perfMon.h if you want to monitor performance
  #include "httpServer.hpp"
  #include "httpClient.h"
  #include "oscilloscope.h"

  httpServer *myHttpServer;

  String httpRequestHandler (char *httpRequest, httpConnection *hcn) { 
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

    syslog (httpRequest);

    if (httpRequestStartsWith ("GET / ")) {
                                            // play with cookies
                                            String s = hcn->getHttpRequestCookie ((char *) "refreshCount");
                                            Serial.println ("This page has been refreshed " + s + " times");
                                            hcn->setHttpReplyCookie ("refreshCount", String (s.toInt () + 1));
                                            // return large HTML reply from memory (9 KB in this case)
                                            return F("<html>\n"
                                                     "  <head>\n"
                                                     "    <link rel='shortcut icon' type='image/x-icon' sizes='64x64' href='data:image/png;base64,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'>\n"
                                                     "    <meta http-equiv='content-type' content='text/html;charset=utf-8' />\n"
                                                     "\n" 
                                                     "    <title>Test</title>\n"
                                                     "\n" 
                                                     "    <style>\n"
                                                     "      /* nice page framework */\n"
                                                     "      hr {border: 0; border-top: 1px solid lightgray; border-bottom: 1px solid lightgray}\n"
                                                     "\n" 
                                                     "      h1 {font-family: verdana; font-size: 40px; text-align: center}\n"
                                                     "\n" 
                                                     "      /* define grid for controls */\n"
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

  void wsRequestHandler (char *wsRequest, WebSocket *webSocket) { 
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

    #define wsRequestStartsWith(X) (strstr(wsRequest,X)==wsRequest)
    
    if (wsRequestStartsWith ("GET /rssiReader")) { // data streaming 
                                                    char c;
                                                    do {
                                                      delay (100);
                                                      int i = WiFi.RSSI ();
                                                      c = (char) i;
                                                    } while (webSocket->sendBinary ((byte *) &c,  sizeof (c))); // send RSSI information as long as web browser is willing tzo receive it
                                                  }     
  }

  bool firewall (char *connectingIP) { return true; }
    
  void setup () {
    Serial.begin (115200);
    
    mountFileSystem (true); 
    startWiFi ();

    myHttpServer = new httpServer (httpRequestHandler, wsRequestHandler, (char *) "0.0.0.0", 80, firewall);
    if (myHttpServer->state () != httpServer::RUNNING) {
      Serial.printf (":(\n");
    } else {
      Serial.printf (":)\n");
      if (ESP_IDF_VERSION_MAJOR < 4) {
        // generate 10 HTTP requests so all sockets are going to be used at least once, this way we are going to bypass LwIP bug 
        // that causes a small memory loss but could cause memory fragmentation in multi threaded environment, see: 
        // https://github.com/espressif/esp-idf/issues/8168 (the following helps a lot but not completelly)
        // while (WiFi.localIP ().toString () == "0.0.0.0") { delay (1000); Serial.printf ("   .\n"); } // wait until we get IP from router
        // Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
        // delay (100);
        for (int i = 0; i < CONFIG_LWIP_MAX_SOCKETS; i++) {
          WiFiClient client; 
          if (!client.connect ("127.0.0.1", 80)) {
            Serial.printf ("[httpServer] not accessible on local loopback\n");
          } else {
            client.print ("GET /upTime HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
            unsigned long lastActive = millis ();
            while (true) {
              while (!client.available ()) if (millis () - lastActive >= HTTP_CONNECTION_TIME_OUT) { Serial.printf ("[httpServer] time-out\n"); goto nextHttpRequest; }
              char c = client.read ();
              Serial.print (c);
              if (c == '}') goto nextHttpRequest;
            }
          }
  nextHttpRequest:
          Serial.println ();
        }
      }      
    }

            Serial.printf ("|free heap| 8 bit free  min  max   |32 bit free  min  max   |default free  min  max  |exec free   min   max   |\n");
            Serial.printf ("---------------------------------------------------------------------------------------------------------------\n");
  }

  void loop () {
    delay (121000); // wait slightly more then 2 minutes so that LwIP memory could recover

            Serial.printf ("|  %6u  ", ESP.getFreeHeap ());
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_8BIT), heap_caps_get_largest_free_block (MALLOC_CAP_8BIT));
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_32BIT), heap_caps_get_minimum_free_size (MALLOC_CAP_32BIT), heap_caps_get_largest_free_block (MALLOC_CAP_32BIT));
            Serial.printf ("|  %6u  %6u  %6u", heap_caps_get_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_minimum_free_size (MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT));
            Serial.printf ("|  %6u  %6u  %6u|\n", heap_caps_get_free_size (MALLOC_CAP_EXEC), heap_caps_get_minimum_free_size (MALLOC_CAP_EXEC), heap_caps_get_largest_free_block (MALLOC_CAP_EXEC));

    {
      String r1 = httpClient ((char *) "127.0.0.1", 80, "/upTime");
      if (r1.length ()) Serial.printf ("Got a reply from local loopback: %i bytes\n", r1.length ());
      String r2 = httpClient ((char *) "Yoga13", 80, "/");
      if (r2.length ()) Serial.printf ("Got a reply from my computer, %i bytes\n", r2.length ());
    }

  }

#endif // TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT TEST_HTTP_SERVER_AND_CLIENT
