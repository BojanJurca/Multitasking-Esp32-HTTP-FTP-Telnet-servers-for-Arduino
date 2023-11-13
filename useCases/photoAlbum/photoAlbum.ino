/*
 
    photoAlbum.ino - an use case of Esp32_web_ftp_telnet_server_template project 
    
        Compile this code with Arduino for:
            - one of ESP32 boards (Tools | Board) and 
            - one of FAT partition schemes (Tools | Partition scheme) for FAT file system or one of SPIFFS partition schemes for LittleFS file system

    This example of web portal demonstrates:
    - dynamically generated web portal
    - web sessions with login/logout options
    - local database data storage

    Quick start guide
    - Upload by using FTP your .jpg photo files to /var/www/html/photoAlbum directory.
    - Create smaller versions of your .jpg photo files (approximate 240px x 140px) and give them \"-small\" postfix. For example:
      if your photo file is 20230314_095846.jpg then its smaller version should have name 20230314_095846-small.jpg.
    - Upload these files to /var/www/html/photoAlbum directory as well.
    - Since built-in flash disk is small compared to picture sizes, it is recommended to use a SC card

    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
     
    October 23, 2023, Bojan Jurca

*/

#include <WiFi.h>
// --- PLEASE MODIFY THIS FILE FIRST! --- This is where you can configure your network credentials, which servers will be included, etc ...
#include "Esp32_servers_config.h"


                    // -----photoALbum-----
                    persistentKeyValuePairs<String, unsigned long> imgViewCountDatabase;
                    persistentKeyValuePairs<String, String> imgCaptionDatabase;

                    #include "photoAlbum.h"
                    // -----photoALbum-----

 
// httpRequestHandlerCallback function returns the content part of HTTP reply in HTML, json, ... format (header fields will be added by 
// httpServer before sending the HTTP reply back to the client) or empty string "". In this case, httpServer will try to handle the request
// completely by itself (like serving the right .html file, ...) or return an error code.  
// PLEASE NOTE: since httpServer is implemented as a multitasking server, httpRequestHandlerCallback function can run in different tasks at the same time
//              therefore httpRequestHandlerCallback function must be reentrant.

String httpRequestHandlerCallback (char *httpRequest, httpConnection *hcn) { 
    // httpServer will add HTTP header to the String that this callback function returns and then send everithing to the Web browser (this callback function is suppose to return only the content part of HTTP reply)

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

                    // -----web session login/logout-----
                    fsString<64> sessionToken = hcn->getHttpRequestCookie ("sessionToken").c_str ();
                    fsString<64>userName;
                    if (sessionToken > "") userName = hcn->getUserNameFromToken (sessionToken); // if userName > "" the user is loggedin

                    // REST call to login       
                    if (httpRequestStartsWith ("GET /login/")) { // GET /login/userName%20password - called (for example) from login.html when "Login" button is pressed
                        // delete sessionToken from cookie and database if it already exists
                        if (sessionToken > "") {
                            hcn->deleteWebSessionToken (sessionToken);
                            hcn->setHttpReplyCookie ("sessionToken", "");
                            hcn->setHttpReplyCookie ("sessionUser", "");
                        }
                        // check new login credentials and login
                        fsString<64>password;
                        if (sscanf (httpRequest, "%*[^/]/login/%64[^%%]%%20%64s", (char *) userName, (char *) password) == 2) {
                            if (userManagement.checkUserNameAndPassword (userName, password)) { // check if they are OK against system users or find another way of authenticating the user
                                sessionToken = hcn->newWebSessionToken (userName, WEB_SESSION_TIME_OUT == 0 ? 0 : time () + WEB_SESSION_TIME_OUT).c_str (); // use 0 for infinite
                                hcn->setHttpReplyCookie ("sessionToken", (char *) sessionToken, WEB_SESSION_TIME_OUT == 0 ? 0 : time () + WEB_SESSION_TIME_OUT); // TIME_OUT is in sec, use 0 for infinite
                                hcn->setHttpReplyCookie ("sessionUser", (char *) userName, WEB_SESSION_TIME_OUT == 0 ? 0 : time () + WEB_SESSION_TIME_OUT); // TIME_OUT is in sec, use 0 for infinite
                                return "loggedIn"; // notify the client about success
                            } else {
                                return "Not logged in. Wrong username and/or password."; // notify the client login.html about failure
                            }
                        }
                    }

                    // REST call to logout
                    if (httpRequestStartsWith ("PUT /logout ")) {
                        // delete token from the cookie and the database
                        hcn->deleteWebSessionToken (sessionToken);
                        hcn->setHttpReplyCookie ("sessionToken", "");
                        hcn->setHttpReplyCookie ("sessionUser", "");
                        return "Logged out.";
                    }

                    // -----photoAlbum-----
                    if (httpRequestStartsWith ("PUT /photoAlbum/changeImgCaption/")) {
                        if (userName == "webadmin") {
                            char id [256]; // picture id is its file name without .jpg
                            char encodedImgCaption [256] = {};
                            if (sscanf (httpRequest, "%*[^/]/%*[^/]/%*[^/]/%255[^/]/%255s", (char *) id, (char *) encodedImgCaption) == 2) { 
                                char imgCaption [255]; // it will surely be shorter than encodedImgCaption
                                size_t imgCaptionLen;
                                mbedtls_base64_decode ((unsigned char *) imgCaption, 255, &imgCaptionLen, (const unsigned char *) encodedImgCaption, strlen (encodedImgCaption));
                                imgCaption [imgCaptionLen] = 0;
                                if ((strlen (encodedImgCaption) == 0) == (imgCaptionLen == 0)) {
                                      persistentKeyValuePairs<String, String>::errorCode e;
                                      e = imgCaptionDatabase.Upsert (id, imgCaption);
                                      if (!e) return "imgCaptionChanged";
                                      else    return imgCaptionDatabase.errorCodeText (e);
                                } else {
                                    hcn->setHttpReplyStatus ("400 bad request");
                                    return "bad request";
                                }
                            } else { // wrong REST call syntax
                                hcn->setHttpReplyStatus ("400 bad request");
                                return "bad request";
                            }
                        } else { // user not logged in or some other user than webadmin
                          hcn->setHttpReplyStatus ("403 forbidden");
                          return "forbidden";
                        }
                    }

                    if (httpRequestStartsWith ("GET /photoAlbum/")) {
                        if (strstr (httpRequest, ".jpg ") && !strstr (httpRequest, "-small.jpg ")) { // count only ful size picture hits
                            char id [256];
                            if (sscanf (httpRequest, "%*[^/]/%*[^/]/%255[^.]", id) == 1) { 
                                persistentKeyValuePairs<String, unsigned long>::errorCode e; 
                                e = imgViewCountDatabase.Upsert (id, [] (unsigned long& value) { value ++; }, 1);
                                if (e != imgViewCountDatabase.OK) Serial.printf ("[photoAlbum] imgViewCountDatabase ++ error: %s\n", imgViewCountDatabase.errorCodeText (e));
                            }
                        }                  
                    }

                    if (httpRequestStartsWith ("GET /photoAlbum/photoAlbum.html ")) {
                        String retVal;
                        int retValConstructionError = 0;

                        // first part of /photoAlbum/photoAlbum.html"
                        retVal = F (photoAlbumBeginning);
                        retValConstructionError += (!retVal);

                        // scan /var/www/html/photoAlbum directory for *-short.jpg images and include them into image container 
                        int imgCount = 0;

                        File d = fileSystem.open ("/var/www/html/photoAlbum"); 
                        if (d) {
                          for (File f = d.openNextFile (); f; f = d.openNextFile ()) {
                              if (strstr (f.name (), "-small.jpg")) {
                                  // get id (original picture .jpg file name) from f.name (), image's hit count and its caption
                                  char id [256];
                                  unsigned long count = 0;
                                  String strCount = "";
                                  String caption = "";

                                  if (sscanf (f.name (), "%255[^-]", id) == 1) {
                                      if (imgCount == 0) {
                                          // add the beginning of image container
                                          retValConstructionError += !retVal.concat ("      <div class='image-container'>\n" \
                                                                                    "\n");
                                      }

                                      persistentKeyValuePairs<String, unsigned long>::errorCode e1 = imgViewCountDatabase.FindValue (id, &count);
                                      switch (e1) {
                                          case imgViewCountDatabase.NOT_FOUND:  count = 0;
                                          case imgViewCountDatabase.OK:         retValConstructionError += !(strCount = String (count));
                                                                                break;
                                          default:                              Serial.printf ("[photoAlbum] imgViewCountDatabase error: %s\n", imgViewCountDatabase.errorCodeText (e1));
                                                                                break;
                                      }
                                      persistentKeyValuePairs<String, String>::errorCode e2 = imgCaptionDatabase.FindValue (id, &caption);
                                      switch (e2) {
                                          case imgCaptionDatabase.NOT_FOUND:  caption = id;
                                          case imgCaptionDatabase.OK:         retValConstructionError += !caption;
                                                                              break;
                                          default:                            Serial.printf ("[photoAlbum] imgCaptionDatabase error: %s\n", imgCaptionDatabase.errorCodeText (e2));
                                                                              break;
                                      }

                                      retValConstructionError += !retVal.concat ("    <div class='image'><a href='");
                                      retValConstructionError += !retVal.concat (id);
                                      retValConstructionError += !retVal.concat (".jpg'><img src='");
                                      retValConstructionError += !retVal.concat (f.name ());
                                      retValConstructionError += !retVal.concat ("' onload='onImgLoad (this)' onerror='onImgError (this)' onclick='sessionStorage.setItem(\"");
                                      retValConstructionError += !retVal.concat (id);
                                      retValConstructionError += !retVal.concat ("-views\", (parseInt (document.getElementById(\"");
                                      retValConstructionError += !retVal.concat (id);
                                      retValConstructionError += !retVal.concat ("-views\").innerText) + 1).toString () + \" views\");' /></a>\n" \
                                                                                "        <div class='d2' id='");
                                      retValConstructionError += !retVal.concat (id);
                                      retValConstructionError += !retVal.concat ("-views'>");
                                      retValConstructionError += !retVal.concat (strCount);
                                      retValConstructionError += !retVal.concat (" views</div>\n" \
                                                                                "        <div class='d1'>\n" \
                                                                                "          <textarea name='imgCaption' id='");                                          
                                      retValConstructionError += !retVal.concat (id);
                                      retValConstructionError += !retVal.concat ("' disabled onfocusout='changeImgCaption (id);'>");
                                      retValConstructionError += !retVal.concat (caption);
                                      retValConstructionError += !retVal.concat ("</textarea>\n" \
                                                                                "        </div>\n" \
                                                                                "      </div>\n" \
                                                                                "\n");
                                  }

                                  imgCount ++;
                              }
                          }
                          d.close ();
                        } else {
                            Serial.printf ("Can't open /var/www/html/photoAlbum directory. Does it already exist?\n");
                        }


                        if (imgCount == 0) { // let the user know what to do - include quick start guide instructions
                            retValConstructionError += !retVal.concat (photoAlbumEmpty);
                        } else {
                            // add the ending of image container
                            retValConstructionError += !retVal.concat ("    </div>\n");
                        }

                        // last part of /photoAlbum/photoAlbum.html"
                        retValConstructionError += !retVal.concat (photoAlbumEnd);

                        if (retValConstructionError) {
                            hcn->setHttpReplyStatus ("503 service unavailable");
                            return "Service is unavailable right now.";
                        }
                        return retVal;
                    }
                    // -----photoAlbum-----

    return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


void setup () {
    Serial.begin (115200);
    Serial.println (string (MACHINETYPE " (") + string ((int) ESP.getCpuFreqMHz ()) + (char *) " MHz) " HOSTNAME " SDK: " + ESP.getSdkVersion () + (char *) " " VERSION_OF_SERVERS " compiled at: " __DATE__ " " __TIME__); 

    // 1. Mount file system - this is the first thing to do since all the configuration files reside on the file system.
    #if (FILE_SYSTEM & FILE_SYSTEM_LITTLEFS) == FILE_SYSTEM_LITTLEFS
        // fileSystem.formatLittleFs (); Serial.printf ("\nFormatting file system with LittleFS ...\n\n"); // format flash disk to start everithing from the scratch
        fileSystem.mountLittleFs (true);                                            
    #endif
    #if (FILE_SYSTEM & FILE_SYSTEM_FAT) == FILE_SYSTEM_FAT
        // fileSystem.formatFAT (); Serial.printf ("\nFormatting file system with FAT ...\n\n"); // format flash disk to start everithing from the scratch
        fileSystem.mountFAT (true);
    #endif  
    #if (FILE_SYSTEM & FILE_SYSTEM_SD_CARD) == FILE_SYSTEM_SD_CARD
        fileSystem.mountSD ("/SD", 5); // SD Card if attached, provide the mount poind and CS pin
    #endif


    // 2. Start cron daemon - it will synchronize internal ESP32's clock with NTP servers once a day and execute cron commands.
    // fileSystem.deleteFile ("/usr/share/zoneinfo");                         // contains timezone information           - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/ntp.conf");                               // contains ntp server names for time sync - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/crontab");                                // contains scheduled tasks                - deleting this file would cause creating empty one
    startCronDaemon (NULL);


    // 3. Write the default user management files /etc/passwd and /etc/passwd it they don't exist yet (it only makes sense with UNIX_LIKE_USER_MANAGEMENT).
    // fileSystem.deleteFile ("/etc/passwd");                                 // contains users' accounts information    - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/passwd");                                 // contains users' passwords               - deleting this file would cause creating default one
    userManagement.initialize ();


    // 4. Start the WiFi (STAtion and/or A(ccess) P(oint), DHCP or static IP, depending on the configuration files.
    // fileSystem.deleteFile ("/network/interfaces");                         // contation STA(tion) configuration       - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/wpa_supplicant/wpa_supplicant.conf");     // contation STA(tion) credentials         - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/dhcpcd.conf");                            // contains A(ccess) P(oint) configuration - deleting this file would cause creating default one
    // fileSystem.deleteFile ("/etc/hostapd/hostapd.conf");                   // contains A(ccess) P(oint) credentials   - deleting this file would cause creating default one
    startWiFi ();   


    // 5. Start the servers.
    #ifdef __HTTP_SERVER__
        httpServer *httpSrv = new (std::nothrow) httpServer ( httpRequestHandlerCallback, // a callback function that will handle HTTP requests that are not handled by webServer itself
                                                              NULL,                       // a callback function that will handle WS requests, NULL to ignore WS requests
                                                              "0.0.0.0",                  // start HTTP server on all available IP addresses
                                                              80,                         // default HTTP port
                                                              NULL);                      // we won't use firewallCallback function for HTTP server
        if (!httpSrv && httpSrv->state () != httpServer::RUNNING) dmesg ("[httpServer] did not start.");
    #endif
    #ifdef __FTP_SERVER__
        ftpServer *ftpSrv = new (std::nothrow) ftpServer ("0.0.0.0",          // start FTP server on all available ip addresses
                                                          21,                 // default FTP port
                                                          NULL);              // we won't use firewallCallback function for HTTP server
        if (ftpSrv && ftpSrv->state () != ftpServer::RUNNING) dmesg ("[ftpServer] did not start.");
    #endif

                    // -----photoAlbum-----
                    int e;
                    e = imgCaptionDatabase.loadData ("/var/www/imgCaptions.kvp");                    
                    if (e) Serial.printf ("Error loading data into imgCaptionDatabase: %i\n", e);
                    else Serial.printf ("imgCaptionDatabase: %i captions loaded at startup\n", imgCaptionDatabase.size ());
                    e = imgViewCountDatabase.loadData ("/var/www/imgViews.kvp");
                    if (e) Serial.printf ("Error loading data into imgviewCount: %i\n", e);
                    else Serial.printf ("imgviewCount: %i counters loaded at startup\n", imgViewCountDatabase.size ());
                    // -----photoAlbum-----

}

void loop () {

}
