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

    #define httpRequestStartsWith(X) (strstr(httpRequest,X))
    // DEBUG: Serial.println (); Serial.println (httpRequest); Serial.println ();

    // UNPROTECTED PART - ALWAYS ACCESSIBLE UNPROTECTED PART - ALWAYS ACCESSIBLE UNPROTECTED PART - ALWAYS ACCESSIBLE UNPROTECTED PART - ALWAYS ACCESSIBLE 

    // loginForm.html page, you can also save it to /var/www/html/loginForm.html - this is the first example of logging in with HTML form
    if (httpRequestStartsWith ("GET /loginForm.html ")) return F ("<html>\n"
                                                                  "  <head>\n"
                                                                  "    <title>HTML form login</title>\n"
                                                                  "    <style>\n"
                                                                         // header
                                                                  "      h1 {\n"
                                                                  "        font-family: verdana;\n"
                                                                  "        font-size: 40px;\n"
                                                                  "        text-align: center;\n"
                                                                  "      }\n"
                                                                         // div 
                                                                  "      div.d1 {\n"
                                                                  "        position: relative;\n"
                                                                  "        float: left;\n"
                                                                  "        width: 200px;\n"
                                                                  "        font-family: verdana;\n"
                                                                  "        font-size: 30px;\n"
                                                                  "        color: gray;\n"
                                                                  "      }\n"
                                                                  "      div.d2 {\n"
                                                                  "        position: relative;\n"
                                                                  "        float: left;\n"
                                                                  "        width: 100%;\n"
                                                                  "        font-family: verdana;\n"
                                                                  "        font-size: 16px;\n"
                                                                  "        color: gray;\n"
                                                                  "      }\n"
                                                                         // input text
                                                                  "      input[type=text] {\n"
                                                                  "        padding: 12px 20px;\n"
                                                                  "        font-size: 22px;\n"
                                                                  "        height: 38px;\n"
                                                                  "        width: 200px;\n"
                                                                  "        margin: 8px 0;\n"
                                                                  "        box-sizing: border-box;\n"
                                                                  "        border: 2px solid #2196F3;\n"
                                                                  "        border-radius: 8px;\n"
                                                                  "      }\n"
                                                                  "      input[type=text]:focus {\n"
                                                                  "        outline: none;\n"
                                                                  "        padding: 12px 20px;\n"
                                                                  "        margin: 8px 0;\n"
                                                                  "        box-sizing: border-box;\n"
                                                                  "        border: 2px solid #00ccff;\n"
                                                                  "        border-radius: 8px;\n"
                                                                  "      }\n"
                                                                         // input password
                                                                  "      input[type=password] {\n"
                                                                  "        padding: 12px 20px;\n"
                                                                  "        font-size: 22px;\n"
                                                                  "        height: 38px;\n"
                                                                  "        width: 200px;\n"
                                                                  "        margin: 8px 0;\n"
                                                                  "        box-sizing: border-box;\n"
                                                                  "        border: 2px solid #2196F3;\n"
                                                                  "        border-radius: 8px;\n"
                                                                  "      }\n"
                                                                  "      input[type=password]:focus {\n"
                                                                  "        outline: none;\n"
                                                                  "        padding: 12px 20px;\n"
                                                                  "        margin: 8px 0;\n"
                                                                  "        box-sizing: border-box;\n"
                                                                  "        border: 2px solid #00ccff;\n"
                                                                  "        border-radius: 8px;\n"
                                                                  "      }\n"
                                                                         // input submit
                                                                  "      input[type=submit] {\n"
                                                                  "         padding: 10px 15px;\n"
                                                                  "         font-size: 22px;\n"
                                                                  "         text-align: center;\n"
                                                                  "         cursor: pointer;\n"
                                                                  "         outline: none;\n"
                                                                  "         color: white;\n"
                                                                  "         border: none;\n"
                                                                  "         border-radius: 12px;\n"
                                                                  "         box-shadow: 1px 1px #ccc;\n"
                                                                  "         position: relative;\n"
                                                                  "         top: 0px;\n"
                                                                  "         height: 42px;\n"
                                                                  "         background-color: #2196F3;\n"
                                                                  "      }\n"
                                                                  "      #sb:hover {\n"
                                                                  "        background-color: #0961aa;\n"
                                                                  "      }\n"                                                              
                                                                  "      #sb:active {\n"
                                                                  "        background-color: #0961aa;\n "
                                                                  "        transform: translateY(3px);\n "
                                                                  "      }\n"                                                              
                                                                  "    </style>\n"
                                                                  "  </head>\n"
                                                                  
                                                                  "  <body>\n"
                                                                  "    <h1>Login with HTML form - example</h1>\n"
                                                                  "    <div class='d2'>&nbsp;Try login with RESTful API: <a href='/login.html'>/login.html</a></div><br><br>\n"
                                                                  
                                                                  "    <form id='loginForm' method='post' action='/login/'>\n" // use POST methof and try to login
                                                                  "      <div class='d1'>User name</div>\n"    
                                                                  "      <input type='text' name='username' id='username' placeholder='Username'>\n"
                                                                  "      <br><br>\n"    
                                                                  "      <div class='d1'>Password</div>\n"
                                                                  "      <input type='Password' name='password' id='password' placeholder='Password'>\n"
                                                                  "      <br><br>\n"    
                                                                  "      <input type='submit' id= 'sb' value='Login'>\n"
                                                                  "      <br><br><div class='d2'>Uses a cookie.</div>\n"
                                                                  "    </form>\n" 
                                                                  "  </body>\n"
                                                                  "</html>");

    // login.html page, you can also save it to /var/www/html/login.html
    if (httpRequestStartsWith ("GET /login.html ")) return F ("<html>\n"
                                                              "  <head>\n"
                                                              "    <title>RESTful login</title>\n"
                                                              "    <style>\n"
                                                                     // horizontal rule
                                                              "      hr {\n"
                                                              "        border: 0;\n"
                                                              "        border-top: 1px solid lightgray;\n"
                                                              "        border-bottom: 1px solid lightgray;\n"
                                                              "      }\n"
                                                                     // header
                                                              "      h1 {\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 40px;\n"
                                                              "        text-align: center;\n"
                                                              "      }\n"
                                                                     // button
                                                              "      .button {\n"
                                                              "        padding: 10px 15px;\n"
                                                              "        font-size: 22px;\n"
                                                              "        text-align: center;\n"
                                                              "        cursor: pointer;\n"
                                                              "        outline: none;\n"
                                                              "        color: white;\n"
                                                              "        border: none;\n"
                                                              "        border-radius: 12px;\n"
                                                              "        box-shadow: 1px 1px #ccc;\n"
                                                              "        position: relative;\n"
                                                              "        top: 0px;\n"
                                                              "        height: 42px;\n"
                                                              "      }\n"
                                                              "      button:disabled {\n"
                                                              "        background-color: #aaa;\n"
                                                              "      }\n"
                                                              "      button:disabled:hover {\n"
                                                              "        background-color: #aaa;\n"
                                                              "      }\n"
                                                                     // blue button
                                                              "      .button1 {\n"
                                                              "        background-color: #2196F3;\n"
                                                              "       }\n"
                                                              "      .button1:hover {\n"
                                                              "        background-color: #0961aa;\n"
                                                              "      }\n"
                                                              "      .button1:active {\n"
                                                              "        background-color: #0961aa;\n"
                                                              "        transform: translateY(3px);\n"
                                                              "      }\n"
                                                                     // div grid
                                                              "      div.d1 {\n"
                                                              "        position: relative;\n"
                                                              "        width: 100%;\n"
                                                              "        height: 46px;\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 22px;\n"
                                                              "        color:gray;\n"
                                                              "      }\n"
                                                              "      div.d2 {\n"
                                                              "        position: relative;\n"
                                                              "        float: left;\n"
                                                              "        width: 25%;\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 30px;\n"
                                                              "        color:gray;\n"
                                                              "      }\n"
                                                              "      div.d3 {\n"
                                                              "        position: relative;\n"
                                                              "        float: left;\n"
                                                              "        width: 30%;\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 30px;\n"
                                                              "        color:black;\n"
                                                              "      }\n"
                                                              "      div.d4 {\n"
                                                              "        position: relative;\n"
                                                              "        float: left;\n"
                                                              "        width: 40%;\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 22px;\n"
                                                              "        color: black;\n"
                                                              "      }\n"
                                                                     // input text
                                                              "      input[type=text] {\n"
                                                              "        padding: 12px 20px;\n"
                                                              "        font-size: 22px;\n"
                                                              "        height: 38px;\n"
                                                              "        width: 90px;\n"
                                                              "        margin: 8px 0;\n"
                                                              "        box-sizing: border-box;\n"
                                                              "        border: 2px solid #2196F3;\n"
                                                              "        border-radius: 8px;\n"
                                                              "      }\n"
                                                              "      input[type=text]:focus {\n"
                                                              "        outline: none;\n"
                                                              "        padding: 12px 20px;\n"
                                                              "        margin: 8px 0;\n"
                                                              "        box-sizing: border-box;\n"
                                                              "        border: 2px solid #00ccff;\n"
                                                              "        border-radius: 8px;\n"
                                                              "      }\n"
                                                                     // password
                                                              "      input[type=password] {\n"
                                                              "        padding: 12px 20px;\n"
                                                              "        font-size: 22px;\n"
                                                              "        height: 38px;\n"
                                                              "        width: 90px;\n"
                                                              "        margin: 8px 0;\n"
                                                              "        box-sizing: border-box;\n"
                                                              "        border: 2px solid #2196F3;\n"
                                                              "        border-radius: 8px;\n"
                                                              "      }\n"
                                                              "      input[type=password]:focus {\n"
                                                              "        outline: none;\n"
                                                              "        padding: 12px 20px;\n"
                                                              "        margin: 8px 0;\n"
                                                              "        box-sizing: border-box;\n"
                                                              "        border: 2px solid #00ccff;\n"
                                                              "        border-radius: 8px;\n"
                                                              "      }\n"
                                                              "    </style>\n"
                                                              
                                                              "    <script type='text/javascript'>\n"
                                                              "      // mechanism that makes REST calls and get their replies\n"
                                                              "      var httpClient = function () {\n"
                                                              "        this.request = function (url, method, callback) {\n"
                                                              "          var httpRequest = new XMLHttpRequest ();\n"
                                                              "          var httpRequestTimeout;\n"
                                                              "          httpRequest.onreadystatechange = function () {\n"
                                                              // "            console.log (httpRequest.readyState);\n"
                                                              "            if (httpRequest.readyState == 1) { // 1 = OPENED, start timing\n"
                                                              "              httpRequestTimeout = setTimeout (function () { alert ('Server did not reply (in 5 seconds).'); }, 5000);\n"
                                                              "            }\n"
                                                              "            if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText\n"
                                                              "              clearTimeout (httpRequestTimeout);\n"
                                                              "              if (httpRequest.status == 200) callback (httpRequest.responseText); // 200 = OK, text contains redirection page address\n"
                                                              "              else alert ('Error ' + httpRequest.status + ' ' + httpRequest.responseText); // some other reply status, like 404, 503, ...\n"
                                                              "            }\n"
                                                              "          }\n"
                                                              "          httpRequest.open (method, url, true);\n"
                                                              "          httpRequest.send (null);\n"
                                                              "        }\n"
                                                              "      }\n"
                                                              "    </script>\n"
                                                              "  </head>\n"
                                                              
                                                              "  <body>\n"
                                                              "    <h1>Login with RESTful API - example</h1><br>\n"
                                                              "    <div class='d1'>&nbsp;<small><small>Try login with HTML form: <a href='/loginForm.html'>/loginForm.html</a></small></small></div><br><br>\n"
                                                              
                                                              "    <hr />\n"
                                                              "    <div class='d1'>\n"
                                                              "      <div class='d2'>&nbsp;User name</div>\n"
                                                              "      <div class='d3'>\n"
                                                              "        <input type='text' id='userName' style='width:280px;' required>\n"
                                                              "      </div>\n"
                                                              "      <div class='d4'>Your user name.</div>\n"
                                                              "    </div>\n"
                                                              "    <br>\n"
                                                              "    <div class='d1'>\n"
                                                              "      <div class='d2'>&nbsp;Password</div>\n"
                                                              "      <div class='d3'> \n"
                                                              "        <input type='password' id='userPassword' style='width:280px;' required>\n"
                                                              "      </div>\n"
                                                              "      <div class='d4'>Your password.</div>\n"
                                                              "    </div>\n"
                                                              
                                                              "    <hr />\n"
                                                              "    <br>\n"
                                                              "    <div class='d1'>\n"
                                                              "      <div class='d2'>&nbsp;</div>\n"
                                                              "      <div class='d3'>\n" 
                                                              "        <button class='button button1' id='loginButton' onclick='tryToLogin ()'>&nbsp;Login&nbsp;</button>\n"
                                                              "      </div>\n"
                                                              "      <div class='d4'>Uses a cookie.</div>\n"
                                                              "    </div>\n"
                                                              "  </body>\n"

                                                              "  <script type='text/javascript'>\n"
                                                              "    function tryToLogin () {\n"
                                                              "      if (!document.getElementById ('userName').reportValidity ()) return;\n"      
                                                              "      if (!document.getElementById ('userPassword').reportValidity ()) return;\n"
                                                              "      // try to get sessionId into a cookie\n"
                                                              "      var client = new httpClient ();\n" 
                                                              "      var clientRequest = 'login' + '/username=' + document.getElementById ('userName').value + '&password=' + document.getElementById ('userPassword').value;\n"
                                                              "      client.request (clientRequest, 'POST', function (serverReply) {\n"                                                        
                                                              "                                                                       alert (document.cookie); // success, got sessionId cookie\n"
                                                              "                                                                       window.location.href = serverReply; // navigate to the first page after login\n"
                                                              "                                                                   });\n"
                                                              "    }\n"
                                                              "  </script>\n"
                                                              
                                                              "</html>");

    // HTML form or RESTful login call: POST /login/ ... username=userName&password=password - called from login.html when "Login" button is pressed 
    if (httpRequestStartsWith ("POST /login/"))  { 
                                                  char userName [USER_PASSWORD_MAX_LENGTH + 1]; 
                                                  strBetween (userName, sizeof (userName), httpRequest, "username=", "&");
                                                  char password [USER_PASSWORD_MAX_LENGTH + 1]; 
                                                  bool restLogin = true;
                                                  strBetween (password, sizeof (password), httpRequest, "password=", " "); // RESTful API call - parameters are in HTTP request URL, ending with " "
                                                  if (!*password) {
                                                    restLogin = false;
                                                    strBetween (password, sizeof (password), httpRequest, "password=", ""); // HTML from call - parameters are in HTTP request content, ending with 0
                                                  }
                                                  // DEBUG: Serial.println ("|" + String (userName) + "|" + String (password) + "|");
                                                  
                                                  if (checkUserNameAndPassword (userName, password)) {                        // check if username and password are OK with already built-in user management
                                                    String sessionToken = generateWebSessionToken (hcn);                      // generate web session token
                                                    if (sessionToken == "") {                                                 // error, "database" is full
                                                      hcn->setHttpReplyStatus ((char *) "503 Service Unavailable");     
                                                      return "Can't login right now. Please try again in few minutes.";       // notify browser about failure 
                                                    }
                                                    if (!setWebSessionTokenAdditionalInformation (sessionToken, userName)) {  // link userName with web session token in built-in "database" so we will be always able to get username from the token when needed
                                                      hcn->setHttpReplyStatus ((char *) "503 Service Unavailable");     
                                                      return "Can't login right now. Please try again in few minutes.";       // notify browser about failure 
                                                    }
                                                    // logged in, pass the necessary information to the browser
                                                    hcn->setHttpReplyCookie ("sessionToken", sessionToken);                   // pass session token to the browser through a cookie, path and expiration time can also be specified
                                                    // tell the browser where to go next
                                                    if (restLogin) { // reply with 200 following with page to redirect, redirection itself will be done by javascript in the browser
                                                      hcn->setHttpReplyStatus ((char *) "200 OK");                            // reply text will contain the redirection information ...
                                                      return "/logout.html";                                                  // ... the first protected page in this example   
                                                    } else { // reply with 303 to tell browser to redirect
                                                      hcn->setHttpReplyStatus ((char *) "303 redirect");                      // redirect to the first (in this example only) protected page (code 303 tells browser to use GET method in next request) ...
                                                      hcn->setHttpReplyHeaderField ("Location", "/logout.html");              // ... wich is logout.html in this example
                                                      return "loggedIn";                                                      // whatever
                                                    }
                                                  } else {                                                                    // wrong user name and/or password
                                                    hcn->setHttpReplyStatus ((char *) "401 Unauthorized");                    // not logged in
                                                    hcn->setHttpReplyCookie ("sessionToken", "");                             // delete sessionToken cookie if it exists
                                                    return "Wrong user name or password.";                                    // notify login.html in the browser about failure
                                                  }
                                                }

    // PROTECTED PART - FOR LOGED IN USERS ONLY PROTECTED PART - FOR LOGED IN USERS ONLY PROTECTED PART - FOR LOGED IN USERS ONLY PROTECTED PART

    // check if the user is logged in - this is if we'have got a cookie with a valid session token in the httpRequest
    String sessionToken = hcn->getHttpRequestCookie ((char *) "sessionToken");
    bool loggedIn = isWebSessionTokenValid (sessionToken, hcn);                   // logged-in users are those with valid session tokens
    // String userName = getWebSessionTokenAdditionalInformation (sessionToken);  // if userNames is needed get it from built-in "database" where it is lnked with session token
    // String homeDirectory = getUserHomeDirectory ((char *) userName.c_str ());  // user rights are defined by his/her home directory, get it from built-in user management
    // Serial.println (userName + " may access " + homeDirectory);

    // rdirect all other HTTP request that do not have valid session tokens (in cookies) to login.html
    if (!loggedIn) {        
      hcn->setHttpReplyStatus ((char *) "303 redirect");  // change reply status to 307 and  ...
      char location [66]; sprintf (location, "/login.html");
      hcn->setHttpReplyHeaderField ("Location", location);          // ... set Location so client browser will know where to go next
      return "Not logged in.";
    }  

    // RESTful logout call: POST /logout - called from logout.html when "Logout" button is pressed 
    if (httpRequestStartsWith ("POST /logout ")) { 
                                                   deleteWebSessionToken (sessionToken);                               // delete session token from local "database"
                                                   hcn->setHttpReplyCookie ("sessionToken", "");                       // empty session token cookie, path and expiration time can also be specified
                                                   return "LoggedOut.";                                                // notify logout.html in the browser about success
                                                }

    // logout.html page, you can also save it to /var/www/html/logout.html
    if (httpRequestStartsWith ("GET /logout.html ")) return F ("<html>\n"
                                                              "  <head>\n"
                                                              "    <title>Web logout</title>\n"
                                                              "    <style>\n"
                                                                     // header
                                                              "      h1 {\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 40px;\n"
                                                              "        text-align: center;\n"
                                                              "      }\n"
                                                                     // button
                                                              "      .button {\n"
                                                              "        padding: 10px 15px;\n"
                                                              "        font-size: 22px;\n"
                                                              "        text-align: center;\n"
                                                              "        cursor: pointer;\n"
                                                              "        outline: none;\n"
                                                              "        color: white;\n"
                                                              "        border: none;\n"
                                                              "        border-radius: 12px;\n"
                                                              "        box-shadow: 1px 1px #ccc;\n"
                                                              "        position: relative;\n"
                                                              "        top: 0px;\n"
                                                              "        height: 42px;\n"
                                                              "      }\n"
                                                              "      button:disabled {\n"
                                                              "        background-color: #aaa;\n"
                                                              "      }\n"
                                                              "      button:disabled:hover {\n"
                                                              "        background-color: #aaa;\n"
                                                              "      }\n"
                                                                     // blue button
                                                              "      .button1 {\n"
                                                              "        background-color: #2196F3;\n"
                                                              "       }\n"
                                                              "      .button1:hover {\n"
                                                              "        background-color: #0961aa;\n"
                                                              "      }\n"
                                                              "      .button1:active {\n"
                                                              "        background-color: #0961aa;\n"
                                                              "        transform: translateY(3px);\n"
                                                              "      }\n"
                                                                     // div grid
                                                              "      div.d1 {\n"
                                                              "        position: relative;\n"
                                                              "        overflow: hidden;\n"
                                                              "        width: 100%;\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 30px;\n"
                                                              "        color:gray;\n"
                                                              "      }\n"
                                                              "      div.d2 {\n"
                                                              "        position: relative;\n"
                                                              "        float: left;\n"
                                                              "        width: 25%;\n"
                                                              "        font-family: verdana;\n"
                                                              "        font-size: 30px;\n"
                                                              "        color:gray;\n"
                                                              "      }\n"
                                                              "      div.d3 {\n"
                                                              "        position: relative;\n"
                                                              "          float: left;\n"
                                                              "          width: 75%;\n"
                                                              "          font-family: verdana;\n"
                                                              "          font-size: 30px;\n"
                                                              "          color: black;\n"
                                                              "        }\n"
                                                              "    </style>\n"

                                                              "    <script type='text/javascript'>\n"
                                                              "      // mechanism that makes REST calls and get their replies\n"
                                                              "      var httpClient = function () {\n"
                                                              "        this.request = function (url, method, callback) {\n"
                                                              "          var httpRequest = new XMLHttpRequest ();\n"
                                                              "          var httpRequestTimeout;\n"
                                                              "          httpRequest.onreadystatechange = function () {\n"
                                                              // "            console.log (httpRequest.readyState);\n"
                                                              "            if (httpRequest.readyState == 1) { // 1 = OPENED, start timing\n"
                                                              "              httpRequestTimeout = setTimeout (function () { alert ('Server did not reply (in 5 seconds).'); }, 5000);\n"
                                                              "            }\n"
                                                              "            if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText\n"
                                                              "              clearTimeout (httpRequestTimeout);\n"
                                                              "              if (httpRequest.status == 200) callback (httpRequest.responseText); // 200 = OK, text contains redirection page address\n"
                                                              "              else alert ('Error ' + httpRequest.status + ' ' + httpRequest.responseText); // some other reply status, like 404, 503, ...\n"
                                                              "            }\n"
                                                              "          }\n"
                                                              "          httpRequest.open (method, url, true);\n"
                                                              "          httpRequest.send (null);\n"
                                                              "        }\n"
                                                              "      }\n"
                                                              "    </script>\n"
                                                              
                                                              "  </head>\n"
                                                              "<body>\n"
                                                              "\n"
                                                              "  <h1>Logout</h1><br>\n"
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
                                                              "    client.request (clientRequest, 'POST', function (serverReply) {\n"
                                                              "                                                                    window.location.href = '/login.html'; // navigate to login page\n"
                                                              "                                                                  });\n"
                                                              "  }\n"
                                                              "  </script>\n"
                                                              "</html>");

    // let the HTTP server try to find a file in /var/www/html and send it as a reply - NOTE THAT THIS PART IS STILL PROTECTED
    return "";
  }


  httpServer *myHttpServer;
 
void setup () {
  Serial.begin (115200);

  // mountFileSystem (true);                              // to enable httpServer to server .html and other files from /var/www/html directory and also to enable UNIX_LIKE_USER_MANAGEMENT
  // initializeUsers ();                                  // creates defult /etc/passwd and /etc/shadow but only if they don't exist yet - only for UNIX_LIKE_USER_MANAGEMENT
  startWiFi ();
  startCronDaemon (NULL);                                 // sichronize ESP32 clock with NTP servers if you want to use (login) cookies with expiration time

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
