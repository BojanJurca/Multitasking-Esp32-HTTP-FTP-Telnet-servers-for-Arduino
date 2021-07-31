# Step-by-step guide to web server of ESP32_web_ftp_telnet_server_template

The initial idea was to develop a tiny web server for ESP32 projects that allows fast development of web user interface. Two things were needed:

  -	A  httpRequestHandler function that would handle (small) programmable responses. httpRequestHandler function takes HTTP request as an argument and returns HTTP response (just the content part to be precise, web server will add HTTP header itself before sending HTTP reply to browser, so you don’t have to worry about this). 
  -	Ability to send (large) .html files as responses to HTTP requests. As it turned out this also required FTP server to be able to upload the .html files to ESP32 in the first place.

Web server can handle both or just one of them. If you need only programmable responses than you can get rid of file system and FTP server. If you only need to serve .html files than you can get rid of httpRequestHandler function. All you have to do in this case is to upload .html files to /var/www/html directory. But you have to set up FTP server
and file system, of course. Don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme |  ...) befor uploading the code to ESP32.

Everything is already prepared in the template. Whatever you are doing it would be a good idea to start with the template and then just start deleting everything you don’t need.

## 1. A minimal web server for short, programmable responses (a working example)


```C++
#include <WiFi.h>


// define how your ESP32 server will present itself to the network and what the output of some telnet comands (like uname) would be

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here


// define how your ESP32 server is going to connect to WiFi network - these are just default settings, you can edit configuration files later

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
#include "./servers/network.h"


// include code for web server

#include "./servers/webServer.hpp"    // if you want to learn more about web server please read: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template/blob/master/Web_server_step_by_step.md

// httpRequestHandler callback function

String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  // debug: Serial.print (httpRequest);

  if (httpRequest.substring (0, 6) == "GET / ") { 
    
    // tell web server that the request has already been handeled by returning something else than ""
    return String ("<HTML><BODY>") + (digitalRead (2) ? "Led is on." : "Led is off.") + String ("</BODY></HTML>");

  }

  return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning ""
}

void setup () {
  Serial.begin (115200);
 
  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist

  // start web server 
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // a callback function that will handle HTTP requests that are not handled by webServer itself
                                        NULL,                         // no wsRequestHandler
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // no firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) webDmesg ("[httpServer] did not start.");

  // ----- add your own code here -----
  
}

void loop () {
              
}
```


## 2. A minimal web server to server .html (and other) files

```C++
#include <WiFi.h>


// define how your ESP32 server will present itself to the network and what the output of some telnet comands (like uname) would be

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here


// FAT file system is needed for full functionality: all configuration files are stored here as well and .html and other files

#include "./servers/file_system.h"


// define how your ESP32 server is going to connect to WiFi network - these are just default settings, you can edit configuration files later

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
#include "./servers/network.h"


// include time function for FTP server is going to need them 
  // define how your ESP32 server is going to handle time: NTP servers where it will get GMT from and how local time will be calculated from GMT
  
  // define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
  // ...
  // #define TIMEZONE  EASTERN_TIMEZONE
  // (default) #define TIMEZONE  CET_TIMEZONE               
  #include "./servers/time_functions.h"     

// include user management for FTP server is going to need it
  // define how your ESP32 server is going to handle users
  // if you want to learn more about user management please read: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template/blob/master/User_management_step_by_step.md
  
  // #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
  // #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
  // (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
  #include "./servers/user_management.h"


// include code FTP and web servers

#include "./servers/webServer.hpp"    
#include "./servers/ftpServer.hpp"    


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist

  // start web server 
  httpServer *httpSrv = new httpServer (NULL,                         // no httpRequestHandler
                                        NULL,                         // no wsRequestHandler 
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // no firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) webDmesg ("[httpServer] did not start.");

  // start FTP server
  ftpServer *ftpSrv = new ftpServer ("0.0.0.0",                       // start FTP server on all available ip addresses
                                     21,                              // controll connection FTP port
                                     NULL);                           // no firewall
  if (!ftpSrv || (ftpSrv && !ftpSrv->started ())) ftpDmesg ("[ftpServer] did not start.");

  // add your own code here

}

void loop () {
                
}
```

Now it is the time to upload .html files:

```
C:\>ftp <YOUR ESP32 IP>
Connected to 10.18.1.114.
220-MyESP32Server FTP server - please login
220
200 UTF8 enabled
User (10.18.1.114:(none)): webadmin
331 enter password
Password: webadminpassword
230 logged on, your home directory is "/var/www/html/"
ftp> put index.html
200 port ok
150 starting transfer
226 transfer complete
ftp: 38 bytes sent in 0.10Seconds 0.37Kbytes/sec.
ftp>
```


## 3. Combining (large) .html files with (short) programmable responses
The .html file will include most of the content and structure whereas programmable responses will provide dynamic or calculated content. Since programmable responses will be called from .html file we will also need a javascript mechanism (inside .html file) capable of this. Javascript already has built-in parser for JSON. All programmable responses will therefor use this format.

```C++
#include <WiFi.h>


// define how your ESP32 server will present itself to the network and what the output of some telnet comands (like uname) would be

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here


// FAT file system is needed for full functionality: all configuration files are stored here as well and .html and other files

#include "./servers/file_system.h"


// define how your ESP32 server is going to connect to WiFi network - these are just default settings, you can edit configuration files later

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
#include "./servers/network.h"


// include time function for FTP server is going to need them 
  // define how your ESP32 server is going to handle time: NTP servers where it will get GMT from and how local time will be calculated from GMT
  
  // define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
  // ...
  // #define TIMEZONE  EASTERN_TIMEZONE
  // (default) #define TIMEZONE  CET_TIMEZONE               
  #include "./servers/time_functions.h"     

// include user management for FTP server is going to need it
  // define how your ESP32 server is going to handle users
  // if you want to learn more about user management please read: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template/blob/master/User_management_step_by_step.md
  
  // #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
  // #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
  // (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
  #include "./servers/user_management.h"


// include code FTP and web servers

#include "./servers/webServer.hpp"    
#include "./servers/ftpServer.hpp"    


// httpRequestHandler for short HTTP replies – instead of HTML replies will be in JSON format 

String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  // debug:
  Serial.print (httpRequest);

  if (httpRequest.substring (0, 16) == "GET /builtInLed ") {
getBuiltInLed:
      String httpReplyInJsonFormat = "{\"id\":\"" + String (HOSTNAME) + "\",\"builtInLed\":\"" + (digitalRead (2) == HIGH ? String ("on") : String ("off")) + "\"}";
      // debug:
      Serial.println (httpReplyInJsonFormat);
      return httpReplyInJsonFormat;
  } else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ") {
      digitalWrite (2, HIGH);
      goto getBuiltInLed;
  } else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ") {
      digitalWrite (2, LOW);
      goto getBuiltInLed;
  } 

  return "";  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist

  // start web server 
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // our httpRequestHandler
                                        NULL,                         // no wsRequestHandler 
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // no firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start.");

  // start FTP server
  ftpServer *ftpSrv = new ftpServer ("0.0.0.0",                       // start FTP server on all available ip addresses
                                     21,                              // controll connection FTP port
                                     NULL);                           // no firewall
  if (!ftpSrv || (ftpSrv && !ftpSrv->started ())) dmesg ("[ftpServer] did not start.");

  // add your own code here
  pinMode (2, OUTPUT); 

}

void loop () {
                
}
```

And the HTML file would look something like this:

```HTML
<html>
  Combining (large) .html files with (short) programmable responses<br><br>
  <hr />
  Led: <input type='checkbox' disabled id='ledSwitch' onClick='turnLed (this.checked)'>
  <hr />
  <script type='text/javascript'>

    // mechanism that makes HTTP requests
    var httpClient = function () { 
      this.request = function (url, method, callback) {
        var httpRequest = new XMLHttpRequest ();
        httpRequest.onreadystatechange = function () {
          if (httpRequest.readyState == 4 && httpRequest.status == 200) callback (httpRequest.responseText);
        }
        httpRequest.open (method, url, true);
        httpRequest.send (null);
      }
    }

    // make a HTTP requests and initialize/populate this page
    var client = new httpClient ();
    client.request ('/builtInLed', 'GET', function (json) {
                                                            // json reply will look like: {"id":"MyESP32Server","builtInLed":"on"}
                                                            var obj=document.getElementById ('ledSwitch'); 
                                                            obj.disabled = false; 
                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');
                                                          });

  function turnLed (switchIsOn) { // sends desired led state to ESP32 server and refreshes ledSwitch state
    client.request (switchIsOn ? '/builtInLed/on' : '/builtInLed/off' , 'PUT', function (json) {
                                                            var obj = document.getElementById ('ledSwitch'); 
                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');                                                           
                                                          });
  }

  </script>
</html>
```


## 4. Handling cookies


... to be continued ...


## 5. Websockets


... to be continued ...

