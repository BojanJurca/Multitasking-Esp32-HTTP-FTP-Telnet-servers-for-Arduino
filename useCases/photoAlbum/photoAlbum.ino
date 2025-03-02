/*
 
    photoAlbum.ino - web portal use case of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
    
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
     
    February 6, 2025, Bojan Jurca

*/

#include <WiFi.h>
// --- PLEASE MODIFY THIS FILE FIRST! --- This is where you can configure your network credentials, which servers will be included, etc ...
#include "Esp32_servers_config.h"

#include "servers/std/Cstring.hpp"
#include "servers/std/console.hpp"
#include "servers/keyValueDatabase.hpp"



                    // ----- photoAlbum -----
                    keyValueDatabase<String, unsigned long> imgViewCountDatabase;
                    keyValueDatabase<String, String> imgCaptionDatabase;

                    #include "photoAlbum.h"
                    // ----- photoAlbum -----

 
// httpRequestHandlerCallback function returns the content part of HTTP reply in HTML, json, ... format (header fields will be added by 
// httpServer before sending the HTTP reply back to the client) or empty string "". In this case, httpServer will try to handle the request
// completely by itself (like serving the right .html file, ...) or return an error code.  
// PLEASE NOTE: since httpServer is implemented as a multitasking server, httpRequestHandlerCallback function can run in different tasks at the same time
//              therefore httpRequestHandlerCallback function must be reentrant.

String httpRequestHandlerCallback (char *httpRequest, httpServer_t::httpConnection_t *httpConnection) { 
    // httpServer will add HTTP header to the String that this callback function returns and then send everithing to the Web browser (this callback function is suppose to return only the content part of HTTP reply)

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

                    // -----web session login/logout-----
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

                    // -----photoAlbum-----
                    if (httpRequestStartsWith ("PUT /photoAlbum/changeImgCaption/")) {
                        if (userName == "webadmin") {
                            char id [FILE_PATH_MAX_LENGTH + 1]; // picture id is its file name without .jpg
                            char encodedImgCaption [256] = {};
                            if (sscanf (httpRequest, "%*[^/]/%*[^/]/%*[^/]/%255[^/]/%255s", (char *) id, (char *) encodedImgCaption) == 2) { 
                                char imgCaption [255]; // it will surely be shorter than encodedImgCaption
                                size_t imgCaptionLen;
                                mbedtls_base64_decode ((unsigned char *) imgCaption, 255, &imgCaptionLen, (const unsigned char *) encodedImgCaption, strlen (encodedImgCaption));
                                imgCaption [imgCaptionLen] = 0;
                                if ((strlen (encodedImgCaption) == 0) == (imgCaptionLen == 0)) {
                                      signed char e;
                                      e = imgCaptionDatabase.Update (id, imgCaption);
                                      if (!e) return "imgCaptionChanged";
                                      else    return "server-side error";
                                } else {
                                    httpConnection->setHttpReplyStatus ("400 bad request");
                                    return "bad request";
                                }
                            } else { // wrong REST call syntax
                                httpConnection->setHttpReplyStatus ("400 bad request");
                                return "bad request";
                            }
                        } else { // user not logged in or some other user than webadmin
                          httpConnection->setHttpReplyStatus ("403 forbidden");
                          return "forbidden";
                        }
                    }

                    if (httpRequestStartsWith ("GET /photoAlbum/")) {
                        if (strstr (httpRequest, ".jpg ") && !strstr (httpRequest, "-small.jpg ")) { // count only ful size picture hits
                            char id [FILE_PATH_MAX_LENGTH + 1];
                            if (sscanf (httpRequest, "%*[^/]/%*[^/]/%255[^.]", id) == 1) { 
                                imgViewCountDatabase [id] ++;

                                if (imgViewCountDatabase.errorFlags ()) { 
                                    cout << "error " << imgViewCountDatabase.errorFlags () << ": imgViewCountDatabase.Upsert (++)\n";
                                    imgViewCountDatabase.clearErrorFlags ();                                    
                                } else {
                                    cout << "OK: imgViewCountDatabase.Upsert (++)\n";
                                }
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
                                  char id [FILE_PATH_MAX_LENGTH + 1];
                                  unsigned long count = 0;
                                  String strCount = "";
                                  String caption = "";

                                  if (sscanf (f.name (), "%255[^-]", id) == 1) {
                                      if (imgCount == 0) {
                                          // add the beginning of image container
                                          retValConstructionError += !retVal.concat ("      <div class='image-container'>\n" \
                                                                                    "\n");
                                      }

                                      count = imgViewCountDatabase [id];
                                      if (count == 0) {
                                          retValConstructionError += !(strCount = "0");
                                          if (imgViewCountDatabase.errorFlags () & ~err_not_found)
                                              cout << "[photoAlbum] imgViewCountDatabase error: " << imgViewCountDatabase.errorFlags () << endl;
                                      } else {
                                          retValConstructionError += !(strCount = String (count));
                                      }

                                      imgCaptionDatabase.Insert (id, id); // insert initial value in to database if id is not already there

                                      signed char e2 = imgCaptionDatabase.FindValue (id, &caption);
                                      switch (e2) {
                                          case err_not_found: caption = id;
                                          case err_ok:        retValConstructionError += !caption;
                                                              break;
                                          default:            cout << "[photoAlbum] imgCaptionDatabase error: " << e2 << endl;
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
                            cout << "Can't open /var/www/html/photoAlbum directory. Does it exist?\n";
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
                            httpConnection->setHttpReplyStatus ("503 service unavailable");
                            return "Service is unavailable right now.";
                        }
                        return retVal;
                    }
                    // -----photoAlbum-----

    return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}


void setup () {
    cinit (false); // Serial.begin (115200);

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
        httpServer_t *httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback); // a callback function that will handle HTTP requests that are not handled by webServer itself
        if (httpServer && *httpServer)
            ;
        else 
            cout << "[httpServer] did not start\n";
    #endif
    #ifdef __FTP_SERVER__
        ftpServer_t *ftpServer = new (std::nothrow) ftpServer_t ();
        if (ftpServer && *ftpServer) 
            ;
        else
            cout << "[ftpServer] did not start\n";
    #endif

                    // -----photoAlbum-----
                    int e;
                    e = imgCaptionDatabase.Open ("/var/www/imgCaptions.db");
                    if (e) 
                        cout << "Error loading data into imgCaptionDatabase: " << e << endl;
                    else 
                        cout << "imgCaptionDatabase: " << imgCaptionDatabase.size () << " captions loaded at startup\n";
                    e = imgViewCountDatabase.Open ("/var/www/imgViews.db");
                    if (e)
                        cout << "Error loading data into imgviewCount: " << e << endl;
                    else
                        cout << "imgviewCount: " << imgViewCountDatabase.size () << " counters loaded at startup\n";
                    // -----photoAlbum-----

}

void loop () {

}
