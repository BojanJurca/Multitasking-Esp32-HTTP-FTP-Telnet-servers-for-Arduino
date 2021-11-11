/*
    Web login that can be improved in many ways
*/

#include <WiFi.h>

  #define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
  #define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

  // tell ESP32 how to connect to your WiFi router
  #define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h) 
  #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
  // tell ESP32 not to set up AP, we do not need it in this example
  #define DEFAULT_AP_SSID           "" // HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
  #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long
  #include "./servers/network.h"

  // file system holds all configuration and .html files 
  // make sure you selected one of FAT partition schemas first (Tools | Partition scheme | ...)
  #include "./servers/file_system.h"

  // correct timing is needed to set cookie expiration time - define your time zone here
  #define TIMEZONE  CET_TIMEZONE               
  // tell time_functions.h where to read current time from
  #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will syncronize its time with
  #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"     
  
  // we are going to rely on alredy built-in user management
  #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT
  #include "./servers/user_management.h"

  // include webServer.hpp
  #include "./servers/webServer.hpp"


String sessionId = String (random (10000, 99000)); // generated at ESP32 startup

// we are going to need httpRequestHandler to handle cookies ant to prevent unauthorized access
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  // login.html page is always accessible, you can also put this response into /var/www/html/login.html file instead of returning it from String literal
  if (httpRequest.substring (0, 16) == "GET /login.html ") { 
    return  "<html>\n"
            "  <head>\n"
            "    <link rel='shortcut icon' type='image/x-icon' sizes='192x192' href='/android-192.png'>\n"
            "    <link rel='icon' type='image/png' sizes='192x192' href='/android-192.png'>\n"
            "    <link rel='apple-touch-icon' sizes='180x180' href='/apple-180.png'>\n"
            "    <meta http-equiv='content-type' content='text/html;charset=utf-8' />\n"
            "\n"
            "    <title>Web login</title>\n"
            "\n"
            "    <style>\n"
            "      hr {border:0; border-top:1px solid lightgray; border-bottom:1px solid lightgray;}\n"
            "      h1 {font-family:verdana; font-size:40px; text-align:center;}\n"
            "\n"      
            "      /* button */\n"
            "      .button {padding: 10px 15px; font-size: 22px;text-align: center; cursor: pointer; outline: none; color: white; border: none; border-radius: 12px; box-shadow: 1px 1px #ccc; position: relative; top: 0px; height: 42px}\n"
            "      button:disabled {background-color: #aaa}\n"
            "      button:disabled:hover {background-color: #aaa}\n"
            "      /* blue button */\n"
            "      .button1 {background-color: #2196F3}\n"
            "      .button1:hover {background-color: #0961aa}\n"
            "      .button1:active {background-color: #0961aa; transform: translateY(3px)}\n"
            "\n"
            "      /* div grid */\n"
            "      div.d1 {position:relative; overflow:hidden; width:100%; font-family:verdana; font-size:22px; color:gray;}\n"
            "      div.d2 {position:relative; float:left; width:25%; font-family:verdana; font-size:30px; color:gray;}\n"
            "      div.d3 {position:relative; float:left; width:30%; font-family:verdana; font-size:30px; color:black;}\n"
            "      div.d4 {position:relative; float:left; width:40%; font-family:verdana; font-size:22px; color:black;}\n"
            "\n"
            "      /* input text */\n"
            "      input[type=text] {padding: 12px 20px; font-size: 22px; height: 38px; width: 90px; margin: 8px 0; box-sizing: border-box; border: 2px solid #2196F3; border-radius: 8px;}\n"
            "      input[type=text]:focus {outline: none; padding: 12px 20px; margin: 8px 0; box-sizing: border-box; border: 2px solid #00ccff; border-radius: 8px;}\n"
            "\n"
            "      /* input password */\n"
            "      input[type=password] {padding: 12px 20px; font-size: 22px; height: 38px; width: 90px; margin: 8px 0; box-sizing: border-box; border: 2px solid #2196F3; border-radius: 8px;}\n"
            "      input[type=password]:focus {outline: none; padding: 12px 20px; margin: 8px 0; box-sizing: border-box; border: 2px solid #00ccff; border-radius: 8px;}\n"
            "    </style>\n"
            "\n"
            "    <script type='text/javascript'>\n"
            "      var HttpClient=function(){\n"
            "        this.request=function(aUrl,aMethod,aCallback){\n"
            "          var anHttpRequest=new XMLHttpRequest();\n"
            "          anHttpRequest.onreadystatechange=function(){\n"
            "            if (anHttpRequest.readyState==4 && anHttpRequest.status==200) aCallback(anHttpRequest.responseText);\n"
            "          }\n"
            "          anHttpRequest.open(aMethod,aUrl,true);\n"
            "          anHttpRequest.send(null);\n"
            "        }\n"
            "      }\n"
            "      // var client=new HttpClient();\n"
            "    </script>\n"
            "\n"
            "  </head>\n"
            "<body>\n"
            "  <h1>Web login</h1><br>\n"
            "\n"
            "  <hr />\n"
            "  <div class='d1'>\n"
            "    <div class='d2'>&nbsp;User name</div>\n"
            "    <div class='d3'>\n"
            "      <input type='text' id='userName' style='width:280px;' required>\n"
            "    </div>\n"
            "    <div class='d4'>\n"
            "      Your user name.\n"
            "    </div>\n"
            "  </div>\n"
            "  <div class='d1'>\n"
            "    <div class='d2'>&nbsp;Password</div>\n"
            "    <div class='d3'> \n"
            "      <input type='password' id='userPassword' style='width:280px;' required>\n"
            "    </div>\n"
            "    <div class='d4'>\n"
            "      Your password.\n"
            "    </div>\n"
            "  </div>\n"
            "  <hr />\n"
            "  <br>\n"
            "  <div class='d1'>\n"
            "    <div class='d2'>&nbsp;</div>\n"
            "    <div class='d3'>\n" 
            "      <button class='button button1' id='loginButton' onclick='tryToLogin ()'>&nbsp;Login&nbsp;</button>\n"
            "    </div>\n"
            "    <div class='d4'>\n"
            "      We use 2 neccessary cookies to make web login work.\n"
            "    </div>\n"
            "  </div>\n"
            "</body>\n"
            "\n"
            "  <script type='text/javascript'>\n"
            "\n"
            "    function tryToLogin () {\n"
            "\n"
            "      if (!document.getElementById ('userName').reportValidity ()) return;\n"      
            "      if (!document.getElementById ('userPassword').reportValidity ()) return;\n"
            "\n"
            "      // try to get sessionId into a cookie\n"
            "      var client = new HttpClient ();\n"
            "      var clientRequest = 'login' + '/' + document.getElementById ('userName').value + '%20' + document.getElementById ('userPassword').value;\n"
            "      client.request  (clientRequest, 'GET', function (serverReply)   {\n"
            "                        if (serverReply == 'loggedIn') {\n"
            "                          // alert (document.cookie); // success, got sessionId cookie\n"
            "                          window.location.href = '/';\n"
            "                        } else {\n"
            "                          alert (serverReply); // report error\n"
            "                        }\n"
            "                        });\n"
            "\n"
            "    }\n"
            "\n"
            "  </script>\n"
            "</html>\n";
  }

  // login - logout funtionality is always accessible
  if (httpRequest.substring (0, 11) == "GET /login/") {               // GET /login/userName%20password - called from login.html when "Login" button is pressed 
    String userName = between (httpRequest, "/login/", "%20");        // get user name from URL
    String password = between (httpRequest, "%20", " ");              // get password from URL
    if (checkUserNameAndPassword (userName, password)) {              // check if they are OK
      wsp->setHttpResponseCookie ("sessionId", sessionId);            // create (simple, demonstration) sessionId cookie, path and expiration time (in GMT) can also be set
      wsp->setHttpResponseCookie ("userName", userName);              // save user name in a cookie for later use
      dmesg ("[httpServer] " + userName + " logged in.");
      return "loggedIn";                                              // notify login.html about success  
    } else {
      wsp->setHttpResponseCookie ("sessionId", "");                   // delete sessionId cookie if it exists
      wsp->setHttpResponseCookie ("userName", "");                    // delete userName cookie if it exists
      dmesg ("[httpServer] " + userName + " entered wrong password.");      
      return "Wrong user name or password.";                          // notify login.html about failure
    }
  }  
  if (httpRequest.substring (0, 12) == "PUT /logout ") {              // called from logout.html when "Logout" button is pressed 
    if (wsp->getHttpRequestCookie ("sessionId") == sessionId) {       // if logged in
      wsp->setHttpResponseCookie ("sessionId", "");                   // delete sessionId cookie if it exists
      wsp->setHttpResponseCookie ("userName", "");                    // delete userName cookie if it exists
      dmesg ("[httpServer] " + wsp->getHttpRequestCookie ("userName") + " logged out.");
    }
    return "LoggedOut.";                                              // notify logout.html
  }

  // protected pages from now on - only for logged in users
  if (wsp->getHttpRequestCookie ("sessionId") != sessionId) {         // check if browser has a valid sessionToken cookie
    wsp->httpResponseStatus = "307 temporary redirect";               // if no, redirect browser to login.html
    wsp->setHttpResponseHeaderField ("Location", "http://" + wsp->getHttpRequestHeaderField ("Host") + "/login.html");
    return "Not logged in.";
  }  

  // you can also put this response into /var/www/html/logout.html file instead of returning it from String literal
  if (httpRequest.substring (0, 17) == "GET /logout.html ") { 
    return  "<html>\n"
            "  <head>\n"
            "    <link rel='shortcut icon' type='image/x-icon' sizes='192x192' href='/android-192.png'>\n"
            "    <link rel='icon' type='image/png' sizes='192x192' href='/android-192.png'>\n"
            "    <link rel='apple-touch-icon' sizes='180x180' href='/apple-180.png'>\n"
            "    <meta http-equiv='content-type' content='text/html;charset=utf-8' />\n"
            "\n"
            "    <title>Web logout</title>\n"
            "\n"
            "    <style>\n"
            "\n"
            "      h1 {font-family:verdana; font-size:40px; text-align:center;}\n"
            "\n"      
            "      /* button */\n"
            "      .button {padding: 10px 15px; font-size: 22px;text-align: center; cursor: pointer; outline: none; color: white; border: none; border-radius: 12px; box-shadow: 1px 1px #ccc; position: relative; top: 0px; height: 42px}\n"
            "      button:disabled {background-color: #aaa}\n"
            "      button:disabled:hover {background-color: #aaa}\n"
            "      /* blue button */\n"
            "      .button1 {background-color: #2196F3}\n"
            "      .button1:hover {background-color: #0961aa}\n"
            "      .button1:active {background-color: #0961aa; transform: translateY(3px)}\n"
            "\n"
            "      /* div grid */\n"
            "      div.d1 {position:relative;overflow:hidden;width:100%;font-family:verdana;font-size:30px;color:gray;}\n"
            "      div.d2 {position:relative;float:left;width:25%;font-family:verdana;font-size:30px;color:gray;}\n"
            "      div.d3 {position:relative;float:left;width:75%;font-family:verdana;font-size:30px;color:black;}\n"
            "\n"
            "    </style>\n"
            "\n"
            "    <script type='text/javascript'>\n"
            "      var HttpClient=function(){\n"
            "        this.request=function(aUrl,aMethod,aCallback){\n"
            "          var anHttpRequest=new XMLHttpRequest();\n"
            "          anHttpRequest.onreadystatechange=function(){\n"
            "            if (anHttpRequest.readyState==4 && anHttpRequest.status==200) aCallback(anHttpRequest.responseText);\n"
            "          }\n"
            "          anHttpRequest.open(aMethod,aUrl,true);\n"
            "          anHttpRequest.send(null);\n"
            "        }\n"
            "      }\n"
            "      // var client=new HttpClient();\n"
            "    </script>\n"
            "\n"
            "  </head>\n"
            "<body>\n"
            "\n"
            "  <h1>Web logout</h1><br>\n"
            "\n"
            "  <div class='d1'>&nbsp;This page is only accessible to the users who have successfully logged in first.</div>\n"
            "\n"
            "  <br><br>\n"
            "\n"
            "  <div class='d1'>\n"
            "    <div class='d2'>&nbsp;</div>\n"
            "    <div class='d3'> \n"
            "      <button class='button button1' id='logoutButton' onclick='doLogout ()'>&nbsp;Logout&nbsp;</button>\n"
            "    </div>\n"
            "  </div>\n"
            "\n"
            "</body>\n"
            "\n"
            "  <script type='text/javascript'>\n"
            "\n"
            "    function doLogout () {\n"
            "      var client = new HttpClient ();\n"
            "      var clientRequest = 'logout';\n"
            "      client.request  (clientRequest, 'PUT', function (serverReply) {\n"
            "                      window.location.href = '/login.html';\n"
            "                          });\n"
            "    }\n"
            "\n"
            "  </script>\n"
            "</html>\n";
  }

  return "";  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


void setup () {
  Serial.begin (115200);
 
  mountFileSystem (true);                                             // get access to configuration files

  startCronDaemon (NULL, 4 * 1024);                                   // start cronDaemon which will sinchronize ESP32 internal clock with NTP server(s)

  initializeUsers ();                                                 // creates user management files (if they don't exist yet) with root, webadmin, webserver and telnetserver users

  startWiFi ();                                                       // connect to to WiFi router    

  // start web server
  httpServer *httpSrv = new httpServer (httpRequestHandler,           // <- pass address of your HTTP request handler function to web server 
                                        NULL,                         // no wsRequestHandler
                                        8 * 1024,                     // 8 KB stack size is usually enough, if httpRequestHandler or wsRequestHandler use more stack increase this value until server is stable
                                        "0.0.0.0",                    // start HTTP server on all available ip addresses
                                        80,                           // HTTP port
                                        NULL);                        // we are not going to use firewall
  if (!httpSrv || (httpSrv && !httpSrv->started ())) dmesg ("[httpServer] did not start."); // insert information into dmesg queue - it can be displayed with dmesg built-in telnet command

  Serial.println ("Web server will use " + sessionId + " as sessionId.");  
}

void loop () {
                
}