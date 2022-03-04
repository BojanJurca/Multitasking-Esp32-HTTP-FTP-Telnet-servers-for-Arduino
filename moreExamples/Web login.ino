/*
    Web login example
*/

#include <WiFi.h>

  #include "./servers/dmesg_functions.h"      // contains message queue that can be displayed by using "dmesg" telnet command

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

  // define where time functions will get current time from
    // #define TIMEZONE                  CET_TIMEZONE       // see time_functions.h for supported time zonnes
    #define DEFAULT_NTP_SERVER_1      "1.si.pool.ntp.org"
    #define DEFAULT_NTP_SERVER_2      "2.si.pool.ntp.org"
    #define DEFAULT_NTP_SERVER_3      "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"       // httpServer need time functions to correctly calculate expiration time for cookies

  // we are going to rely on alredy built-in usermanagement
    #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT // UNIX_LIKE_USER_MANAGEMENT // NO_USER_MANAGEMENT 
  #include "./servers/user_management.h"

  // finally include httpServer.hpp
  #include "./servers/httpServer.hpp"

  String httpRequestHandler (char *httpRequest, httpConnection *hcn) { // note that each HTTP connection runs in its own thread/task, more of them may run simultaneously so this function should be reentrant
    // input: char *httpRequest: the whole HTTP request but without ending \r\n\r\n
    // input example: GET / HTTP/1.1
    //                Host: 192.168.0.168
    //                User-Agent: curl/7.55.1
    //                Accept: */*
    // output: String: content part of HTTP reply, the header will be added by HTTP server before sending it to the client (browser)
    // output example: <html>
    //                   Congratulations, this is working.
    //                 </html>

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

    // DEBUG: Serial.println (httpRequest);

    // UNPROTECTED PART - ALWAYS ACCESSIBLE UNPROTECTED PART - ALWAYS ACCESSIBLE UNPROTECTED PART - ALWAYS ACCESSIBLE UNPROTECTED PART - ALWAYS ACCESSIBLE 

    // login.html page, you can also save it to /var/www/html/login.html
    if (httpRequestStartsWith ("GET /login.html ")) return F ("<html>\n"
                                                              "  <head>\n"
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
                                                              "      div.d1 {position:relative; width:100%; height: 46px; font-family:verdana; font-size:22px; color:gray;}\n"
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
                                                              "\n"
                                                              "      // mechanism that makes REST calls and get their replies\n"
                                                              "      var httpClient = function () {\n"
                                                              "        this.request = function (url, method, callback) {\n"
                                                              "          var httpRequest = new XMLHttpRequest ();\n"
                                                              "          var httpRequestTimeout;\n"
                                                              "          httpRequest.onreadystatechange = function () {\n"
                                                              "            // console.log (httpRequest.readyState);\n"
                                                              "            if (httpRequest.readyState == 1) { // 1 = OPENED, start timing\n"
                                                              "              httpRequestTimeout = setTimeout (function () { alert ('Server did not reply (in time).'); }, 1500);\n"
                                                              "            }\n"
                                                              "            if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText\n"
                                                              "              clearTimeout (httpRequestTimeout);\n"
                                                              "              if (httpRequest.status == 200) callback (httpRequest.responseText); // 200 = OK\n"
                                                              "              else alert ('Server reported error ' + httpRequest.status + ' ' + httpRequest.responseText); // some other reply status, like 404, 503, ...\n"
                                                              "            }\n"
                                                              "          }\n"
                                                              "          httpRequest.open (method, url, true);\n"
                                                              "          httpRequest.send (null);\n"
                                                              "        }\n"
                                                              "      }\n"
                                                              "\n"
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
                                                              "  <br>\n"
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
                                                              "      Uses a cookie.\n"
                                                              "    </div>\n"
                                                              "  </div>\n"
                                                              "</body>\n"
                                                              "\n"
                                                              "  <script type='text/javascript'>\n"
                                                              "    function tryToLogin () {\n"
                                                              "      if (!document.getElementById ('userName').reportValidity ()) return;\n"      
                                                              "      if (!document.getElementById ('userPassword').reportValidity ()) return;\n"
                                                              "\n"
                                                              "      // try to get sessionId into a cookie\n"
                                                              "      var client = new httpClient ();\n" 
                                                              "      var clientRequest = 'login' + '/' + document.getElementById ('userName').value + '%20' + document.getElementById ('userPassword').value;\n"
                                                              "      client.request (clientRequest, 'PUT', function (serverReply) {\n"                                                        
                                                              "                                                                     if (serverReply == 'loggedIn') {\n"
                                                              "                                                                       alert ('Session token = ' + document.cookie); // success, got sessionId cookie\n"
                                                              "                                                                       window.location.href = '/logout.html'; // navigate to the first page after login\n"
                                                              "                                                                     } else {\n"
                                                              "                                                                       alert (serverReply); // report error\n"
                                                              "                                                                     }\n"
                                                              "                                                                   });\n"
                                                              "    }\n"
                                                              "  </script>\n"
                                                              "</html>");

    // REST login call: PUT /login/userName%20password - called from login.html when "Login" button is pressed 
    if (httpRequestStartsWith ("PUT /login/"))  { 
                                                  char userName [USER_PASSWORD_MAX_LENGTH + 1]; strBetween (userName, sizeof (userName), httpRequest, "/login/", "%20");
                                                  char password [USER_PASSWORD_MAX_LENGTH + 1]; strBetween (password, sizeof (password), httpRequest, "%20", " ");
                                                  // DEBUG: Serial.println ("|" + String (userName) + "|" + String (password) + "|");
                                                  
                                                  if (checkUserNameAndPassword (userName, password)) {                  // check if username and password are OK with already built-in user management
                                                    String sessionToken = generateWebSessionToken (hcn);                // generate web session token
                                                    setWebSessionTokenAdditionalInformation (sessionToken, userName);   // link userName with web session token in built-in "database"
                                                    if (sessionToken > "") {
                                                      hcn->setHttpReplyCookie ("sessionToken", sessionToken);           // pass session token to the browser through a cookie, path and expiration time can also be specified
                                                    } else {
                                                      return "Can't login right now. Please try again in few minutes."; // notify login.html in the browser about failure ("database" is full) 
                                                    }
                                                    return "loggedIn";                                                  // notify login.html in the browser about success
                                                  } else {                                                              // wrong user name and/or password
                                                    hcn->setHttpReplyCookie ("sessionToken", "");                       // delete sessionToken cookie if it exists
                                                    return "Wrong user name or password.";                              // notify login.html in the browser about failure
                                                  }
                                                }

    // PROTECTED PART - FOR LOGED IN USERS ONLY PROTECTED PART - FOR LOGED IN USERS ONLY PROTECTED PART - FOR LOGED IN USERS ONLY PROTECTED PART

    // check if the user is logged in - this is if we'have got a cookie with a valid session token in the httpRequest
    String sessionToken = hcn->getHttpRequestCookie ((char *) "sessionToken");
    bool loggedIn = isWebSessionTokenValid (sessionToken, hcn);                   // logged-in users are those with valid session tokens
    // String userName = getWebSessionTokenAdditionalInformation (sessionToken);  // if userNames is needed get it from built-in "database" where it is lnked with session token
    // String homeDirectory = getUserHomeDirectory ((char *) userName.c_str ());  // user rights are defined by his/her home directory, get it from built-in user management
    // Serial.println (userName + " may access " + homeDirectory);

    // REST logout call: PUT /logout - called from logout.html when "Logout" button is pressed 
    if (httpRequestStartsWith ("PUT /logout ")) { 
                                                  if (loggedIn) {
                                                    deleteWebSessionToken (sessionToken);                               // delete session token from local "database"
                                                    hcn->setHttpReplyCookie ("sessionToken", "");                       // empty session token cookie, path and expiration time can also be specified
                                                    return  "LoggedOut."; // notify client logout.html about success    // notify logout.html in the browser about success
                                                  } else {
                                                    return "Not logged in.";                                            // notify logout.html in the browser about failure
                                                  }
                                                }

    // rdirect all other HTTP request that do not have valid session tokens (in cookies) to login.html
    if (!loggedIn) {        
      hcn->setHttpReplyStatus ((char *) "307 temporary redirect");  // change reply status to 307 and  ...
      String host = hcn->getHttpRequestHeaderField ((char *) "Host");
      char location [66]; sprintf (location, "http://%s/login.html", (char *) host.c_str ());
      hcn->setHttpReplyHeaderField ("Location", location);          // ... set Location so client browser will know where to go next
      return "Not logged in.";
    }  

    // logout.html page, you can also save it to /var/www/html/logout.html
    if (httpRequestStartsWith ("GET /logout.html ")) return F ("<html>\n"
                                                              "  <head>\n"
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
                                                              "\n"
                                                              "      // mechanism that makes REST calls and get their replies\n"
                                                              "      var httpClient = function () {\n"
                                                              "        this.request = function (url, method, callback) {\n"
                                                              "          var httpRequest = new XMLHttpRequest ();\n"
                                                              "          var httpRequestTimeout;\n"
                                                              "          httpRequest.onreadystatechange = function () {\n"
                                                              "            // console.log (httpRequest.readyState);\n"
                                                              "            if (httpRequest.readyState == 1) { // 1 = OPENED, start timing\n"
                                                              "              httpRequestTimeout = setTimeout (function () { alert ('Server did not reply (in time).'); }, 1500);\n"
                                                              "            }\n"
                                                              "            if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText\n"
                                                              "              clearTimeout (httpRequestTimeout);\n"
                                                              "              if (httpRequest.status == 200) callback (httpRequest.responseText); // 200 = OK\n"
                                                              "              else alert ('Server reported error ' + httpRequest.status + ' ' + httpRequest.responseText); // some other reply status, like 404, 503, ...\n"
                                                              "            }\n"
                                                              "          }\n"
                                                              "          httpRequest.open (method, url, true);\n"
                                                              "          httpRequest.send (null);\n"
                                                              "        }\n"
                                                              "      }\n"
                                                              "\n"
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
                                                              "  function doLogout () {\n"
                                                              "    var client = new httpClient ();\n"
                                                              "    var clientRequest = 'logout';\n"
                                                              "    client.request (clientRequest, 'PUT', function (serverReply) {\n"
                                                              "                                                                   window.location.href = '/login.html'; // navigate to login page\n"
                                                              "                                                                 });\n"
                                                              "  }\n"
                                                              "  </script>\n"
                                                              "</html>");

    // let the HTTP server try to find a file in /var/www/html and send it as a reply
    return "";
  }


  httpServer *myHttpServer;
 
void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                              // to enable httpServer to server .html and other files from /var/www/html directory and also to enable UNIX_LIKE_USER_MANAGEMENT
  // initializeUsers ();                                  // creates defult /etc/passwd and /etc/shadow but only if they don't exist yet - only for UNIX_LIKE_USER_MANAGEMENT
  startWiFi ();
  // startCronDaemon (NULL);                              // sichronize ESP32 clock with NTP servers if you want to use cookies with expiration time

  myHttpServer = new httpServer (
                                  httpRequestHandler,               // or NULL if httpRequestHandler is not going to be used
                                  NULL,                             // or wsRequestHandler iw webSockets are to be used
                                  (char *) "0.0.0.0",               // httpServer IP or 0.0.0.0 for all available IPs
                                  80,                               // default HTTP port
                                  NULL                              // or firewallCallback if you want to use firewall
                                );
  if (myHttpServer->state () != httpServer::RUNNING) Serial.printf ("[%10lu] Could not start httpServer\n", millis ());
}

void loop () {

}
