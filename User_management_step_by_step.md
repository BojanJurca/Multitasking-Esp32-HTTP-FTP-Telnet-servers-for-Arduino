# Step-by-step guide to user management in ESP32_web_ftp_telnet_server_template
User management is composed of functions in user_management.h, built-in telnet commands (useradd, userdel and passwd) and two Unix/Linux like configuration files:

  - /etc/passwd   - this is where user information is stored
  - /etc/shadow   - this is where hashed passwords are stored

Although it is not meant for these files to be edited manually you can easily view their content if you have set up telnet server:

```
C:\>telnet <YOUR ESP32 IP>
Hello 10.18.1.88!
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# cat /etc/passwd
root:x:0:::/:
webserver::100:::/var/www/html/:
telnetserver::101:::/var/telnet/:
webadmin:x:1000:::/var/www/html/:

# cat /etc/shadow
root:$5$de362bbdf11f2df30d12f318eeed87b8ee9e0c69c8ba61ed9a57695bbd91e481:::::::
webadmin:$5$40c6af3d1540ca2af132e1e93e7f5a5f624280b9d4d552a0bb103afe17c75c53:::::::

#
```

You can not set access rights for different objects, files for example, so everything a logged in user can or can't do must be checked by your code. A rough idea is that the level of access rights is determined by user’s home directory (as stored in /etc/passwd). This principle is fully respected by built-in telnet and FTP commands, but you must take care of everything else you write yourself, meaning if you don't prevent something based on user's name or user's home directory then the code will be executed. You can use all the functions in user_management.h for his purpose like: checkUserNameAndPassword, getUserHomeDirectory, userAdd, userDel, …

What we can see from the files above is that by default ESP32_web_ftp_telnet_server_template already creates two real users with their passwords: root/rootpassword and webadmin/webadminpassword. Webserver and telnetserver doesn’t have their passwords since they are not real users. The records in /etc/passwd only hold their home directories. This is where web server is going to look for .html, .png and other files and where telnet server is going to look for help.txt file.  

## 1. Checking user rights inside telnet session (a working example)
Telnet session is just a TCP connection through which characters are being send in both directions. At the beginning, the user introduces himself by logging in and this stays the same until telnet session or TCP connection ends. It is easy to remember which user has logged in since one TCP connection is handled by one thread/task/process on the server side. This is not something you would do from your code, but user's name and user’s home directory are passed to telnetCommandHandler by telnet server each time the user sends a command to execution by pressing enter. You can access this information through telnetSessionParameters and do with it whatever you consider necessary.

```C++
#include <WiFi.h>

// define how your ESP32 server will present itself to the network and what the output of some telnet comands (like uname) would be

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

// FAT file system is needed for configuration files and some built-in telnet commands

#include "./servers/file_system.h"

// define how your ESP32 server is going to connect to WiFi network - these are just default settings, you can edit configuration files later

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
#include "./servers/network.h"

// define how your ESP32 server is going to handle time: NTP servers where it will get GMT from and how local time will be calculated from GMT

#define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
#define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"

// define TIMEZONE  KAL_TIMEZONE                                // define time zone you are in (see time_functions.h)
// ...
// #define TIMEZONE  EASTERN_TIMEZONE
// (default) #define TIMEZONE  CET_TIMEZONE               
#include "./servers/time_functions.h"     

// define how your ESP32 server is going to handle users
// if you want to learn more about user management please read: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template/blob/master/User_management_step_by_step.md

// #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"

// include code for telnet  servers
#include "./servers/telnetServer.hpp" // if you want to learn more about telnet server please read: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template/blob/master/Telnet_server_step_by_step.md

// ----- telnet command handler example - if you don't want to handle telnet commands yourself just delete this function and pass NULL to telnetSrv instead of its address -----

String telnetCommandHandler (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) { // - must be reentrant!

  #define LED_BUILTIN 2                                 
         if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "off") { // turn led off telnet command
            digitalWrite (LED_BUILTIN, LOW);
            return "This was easy, everyone could have done it.";

  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {  // turn led on telnet  command
            if (tsp->userName == "root") {
              digitalWrite (LED_BUILTIN, HIGH);
              return "Led is on.";
            } else {
              return "I'm sorry " + tsp->userName + ", but only root can turn the LED on.";
            }
  }

  return ""; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist

  // start telnet server
  telnetServer *telnetSrv = new telnetServer (telnetCommandHandler,   // a callback function that will handle telnet commands that are not handled by telnet server itself
                                              16 * 1024,              // 16 KB stack size is usually enough, if telnetCommandHanlder uses more stack increase this value until server is stable
                                              "0.0.0.0",              // start telnt server on all available ip addresses
                                              23,                     // telnet port
                                              NULL);                  // no firewall
  if (!telnetSrv || (telnetSrv && !telnetSrv->started ())) dmesg ("[telnetServer] did not start.");
}

void loop () {
                
}
```

Output example:
```
C:\>telnet <YOUR ESP32 IP>
Hello 10.18.1.88!
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# useradd -u 2000 -d /home/bojan bojan
User created.
# passwd bojan
Enter new password for bojan: bojanpassword
Re-enter new password for bojan: bojanpassword
Password changed for bojan.
# quit

C:\>telnet <YOUR ESP32 IP>
Hello 10.18.1.88!
user: bojan
password: bojanpassword

Welcome bojan,
your home directory is /home/bojan/,
use "help" to display available commands.

$ turn led off
This was easy, everyon could have done it.
$ turn led on
I'm sorry bojan, but only root can turn the LED on.
$ pwd
Your working directory is /home/bojan
$ cd ..
Access to /home denyed.
$
```


## 2. Checking user rights inside FTP session
Since there is nothing you can do from your code to interfere with FTP sessions you can’t do anything about user access rights as well. 


## 3. Checking user rights inside web session (a working example)
Normally everybody can use web server but if you would like to add login/logout functionality things are more complicated than with telnet sessions. The main difference comes from the fact that one telnet session consists of one TCP connection whereas one web session consists of many TCP connections. Normally there is one TCP connection for each web page that browser requests. When handling one TCP connection you cannot automatically know what happened in another. For example: if the user successfully passes user name and password to web server through one web page (one TCP connection handled by one server thread/task/process) there is no secure way you can get this information when the user tries to open another web page (in another TCP connection handled by another server thread/task/process). To overcome this difficulty web server and browser exchange pieces of information stored in cookies here and there, each time the browser requests a new page from web server.

The following example is just a very basic login - logout mechanism, that could be improved in many ways. Its purpose is merely to demonstrate basic web session principles. If a random user tries to open session.html page which is allowed only for logged in users, he will be automatically redirected to login.html page. 

```C++
#include <WiFi.h>

// define how your ESP32 server will present itself to the network and what the output of some telnet comands (like uname) would be

#define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
#define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

// FAT file system is needed for configuration files and some built-in telnet commands

#include "./servers/file_system.h"

// define how your ESP32 server is going to connect to WiFi network - these are just default settings, you can edit configuration files later

#define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
#define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
#define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
#define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
#include "./servers/network.h"


// define how your ESP32 server is going to handle users
// if you want to learn more about user management please read: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template/blob/master/User_management_step_by_step.md

// #define USER_MANAGEMENT NO_USER_MANAGEMENT                   // define the kind of user management project is going to use (see user_management.h)
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT            
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
#include "./servers/user_management.h"

// include code for web server
#include "./servers/webServer.hpp"    


// httpRequestHandler

String sessionId = String (random (10000, 20000));  // this variable is initialized when ESP32 starts up
                                                    // it remains the same for all sessions while ESP32 is running
                                                    // this is one of weak points of this example

String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  // first check if user is logged in 
  bool userIsLoggedIn = false;
  
  // - if he has just logged in then username and password would be in URL (// http://...?username=...&password=...)
  // - if he has logged in once before then username would bi in username cookie and sessionId would have a value of variable sessionId

  String userName = between (httpRequest, "username=", "&");
  String password = between (httpRequest, "password=", " ");
  if (userName > "") {
    // the user is just trying to login - check userName and password against valid ESP32 users 
    if (checkUserNameAndPassword (userName, password)) {    // OK
      wsp->setHttpResponseCookie ("sessionId", sessionId);  // store sessionId in a cookie for later requests 
      wsp->setHttpResponseCookie ("userName", userName);    // store userName in a cookie for later requests
      userIsLoggedIn = true;
      Serial.println (userName + " logged in.");
    } else {                                                // FAILED
      wsp->setHttpResponseCookie ("sessionId", "");         // delete sessionId cookie if it exists
      wsp->setHttpResponseCookie ("userName", "");          // delete userName cookie if it exists
      Serial.println (userName + " failed to log in.");
    }    
  } else {
    // perhaps the user has logged in once before and sessionId is already stored in a cookie
    if (wsp->getHttpRequestCookie ("sessionId") == sessionId) { // OK
      userName = wsp->getHttpRequestCookie ("userName");
      userIsLoggedIn = true;
    } else {                                                    // wrong sessionId
      wsp->setHttpResponseCookie ("sessionId", "");             // delete sessionId cookie if it exists
      wsp->setHttpResponseCookie ("userName", "");              // delete userName cookie if it exists      
    }
  }


  // then process HTTP request

  if (httpRequest.substring (0, 15) == "GET /login.html") { // user requested login.html

    // if already logged in user landed on this page then logg him out
    wsp->setHttpResponseCookie ("sessionId", "");             // delete sessionId cookie if it exists
    wsp->setHttpResponseCookie ("userName", "");              // delete userName cookie if it exists      
    
    // return HTML response
    return "<html>\n"
           "  <body>\n"
           "    <h2>Please login</h2>\n"
           "    <form action='/session.html'>\n"
           "      <label for='username'>username:</label><br>\n"
           "      <input type='text' id='username' name='username'><br>\n"
           "      <label for='password'>password:</label><br>\n"
           "      <input type='text' id='password' name='password'><br><br>\n"
           "      <input type='submit' value='login'>\n"
           "    </form>\n" 
           "    <p>When you click the 'login' button, the form-data will be sent to a page 'session.html'.</p>\n"
           "  </body>\n"
           "</html>\n";

  } else if (httpRequest.substring (0, 17) == "GET /session.html") { // user requested session.html page which is protected only for logged in users

    // if the user is not logged in then reirect him to login.html
    if (!userIsLoggedIn) {
      wsp->httpResponseStatus = "307 temporary redirect";           // redirect browser ...
      wsp->setHttpResponseHeaderField ("Location", "/login.html");  // ... to login.html
      return "Not logged in.";                                      // whatever - browser will ignore this
    }

    // return HTML response 
    return "<html>\n"
           "  <body>\n"
           "    <h2>You have successfully logged in.</h2>\n"
           "    <form action='/login.html'>\n"
           "      <input type='submit' value='logout'>\n"
           "    </form>\n" 
           "    <p>When you click the 'logout' button, you will be redirected to a page 'login.html'.</p>\n"
           "  </body>\n"
           "</html>\n";
    
  }

  return "";
}
  
void setup () {
  Serial.begin (115200);
 
  // FFat.format ();
  mountFileSystem (true);                                             // this is the first thing to do - all configuration files are on file system

  initializeUsersAtFirstCall ();                                      // creates user management files with root, webadmin, webserver and telnetserver users, if they don't exist

  startNetworkAndInitializeItAtFirstCall ();                          // starts WiFi according to configuration files, creates configuration files if they don't exist

  // start web server 
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // a callback function that will handle HTTP requests that are not handled by webServer itself
                                        NULL,                         // no wsRequestHandler
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // no firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) webDmesg ("[httpServer] did not start.");
}

void loop () {
                
}
```


## 4. Simplified user management
There are three levels of user management. You can choose which one to use by #define-ing USER_MANAGEMENT inside your code before #include-ing user_management.h. By default, USER_MANAGEMENT is #define-d as UNIX_LIKE_USER_MANAGEMENT. This is the case that is described above.

If you want to simplify thing a little bit you can #define USER_MANAGEMENT to be HARDCODED_USER_MANAGEMENT. In this case you will only have one user: root. Its ROOT_PASSWORD is #defined as “rootpassword” which is stored in ESP32’s RAM during start up. You can change this definition and keep it secret if you wish.

Another simplification is to #define USER_MANAGEMENT as NO_USER_MANAGEMENT. In this case user password and access rights are not going to be checked at all so everyone can do everithing.
