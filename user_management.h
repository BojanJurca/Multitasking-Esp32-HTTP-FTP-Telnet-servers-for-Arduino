/*
 * 
 * User_management.h 
 * 
 *  This file is part of A_kind_of_esp32_OS_template.ino project: https://github.com/BojanJurca/A_kind_of_esp32_OS_template
 * 
 *  User_management initially creates user management files:
 * 
 *    /etc/passwd        - modify the code below with your users
 *    /etc/shadow        - modify the code below with your passwords
 * 
 * INSTRUCTIONS
 * 
 *  Run A_kind_of_esp32_OS_template the first time with INITIALIZE_USERS == DO_INITIALIZE_USERS 
 *  to generate user management files. Once user management files are generated there's no need 
 *  of generating them again, you can switch INITIALIZE_USERS to DONT_INITIALIZE_USERS
 * 
 *  Functions needed to manage users can be found in this module.
 * 
 * History:
 *          - first release, November 18, 2018, Bojan Jurca
 *  
 */
 

#ifndef __USER_MANAGEMENT__
  #define __USER_MANAGEMENT__


  // change this definitions according to your needs

  #define NO_USER_MANAGEMENT                    1   // everyone is allowed to use FTP anf Telnet
  #define HARDCODED_USER_MANAGEMENT             2   // define user name nad password that will be hard coded into program
  #define UNIX_LIKE_USER_MANAGEMENT             3   // user name and password will be checked by user_management.h
  // select one of the above login methods
  #define USER_MANAGEMENT  UNIX_LIKE_USER_MANAGEMENT

  #define DO_INITIALIZE_USERS              1             // this option is normally used ony the first time you run A_kind_of_est32_OS_template on ESP32
                                                         // change firstUserInitialization function below according to your needs
  #define DONT_INITIALIZE_USERS            2             // once the users are initialized, this option is preferable
  // select one of the above user initialization methods
  #define INITIALIZE_USERS  DO_INITIALIZE_USERS
  

  #if USER_MANAGEMENT == NO_USER_MANAGEMENT           // ----- NO_USER_MANAGEMENT -----
  
    #define USER_HOME_DIRECTORY "/"                                                 // (home direcotry for FTP and Telnet user) change according to your needs
    #define WEBSERVER_HOME_DIRECTORY "/var/www/html/"                               // (where .html files are located) change according to your needs
    #define TELNETSERVER_HOME_DIRECTORY "/var/telnet/"                              // (where telnet help file is located) change according to your needs
    
    void usersInitialization () {;}                                                 // don't need to initialize users in this mode, we are not going to use user name and password at all
    bool checkUserNameAndPassword (char *userName, char *password) { return true; } // everyone can logg in
    char *getUserHomeDirectory (char *userName) { 
                                                  if (!strcmp (userName, "webserver"))         return WEBSERVER_HOME_DIRECTORY;
                                                  else if (!strcmp (userName, "telnetserver")) return TELNETSERVER_HOME_DIRECTORY;
                                                  else                                         return USER_HOME_DIRECTORY;
                                                }
  #endif  

  #if USER_MANAGEMENT == HARDCODED_USER_MANAGEMENT    // ----- HARDCODED_USER_MANAGEMENT -----
  
    #define USERNAME "root"                           // change according to your needs
    #define PASSWORD "rootpassword"                   // change according to your needs
    #define USER_HOME_DIRECTORY "/"                                                 // (home direcotry for FTP and Telnet user) change according to your needs
    #define WEBSERVER_HOME_DIRECTORY "/var/www/html/"                               // (where .html files are located) change according to your needs
    #define TELNETSERVER_HOME_DIRECTORY "/var/telnet/"                              // (where telnet help file is located) change according to your needs
    
    void usersInitialization () {;}                                                 // don't need to initialize users in this mode, we are not going to use user name and password at all
    bool checkUserNameAndPassword (char *userName, char *password) { return (!strcmp (userName, USERNAME) && !strcmp (password, PASSWORD)); }
    char *getUserHomeDirectory (char *userName) { 
                                                  if (!strcmp (userName, "webserver"))         return WEBSERVER_HOME_DIRECTORY;
                                                  else if (!strcmp (userName, "telnetserver")) return TELNETSERVER_HOME_DIRECTORY;
                                                  else                                         return USER_HOME_DIRECTORY;
                                                }
  #endif

  #if USER_MANAGEMENT == UNIX_LIKE_USER_MANAGEMENT    // ----- UNIX_LIKE_USER_MANAGEMENT -----
  
    #include "file_system.h"  // user_management.h needs file_system.h
    #include <mbedtls/md.h>   // needed to calculate hashed passwords
  
    static char *__sha256__ (char *clearText) { // converts clearText into 265 bit SHA, returns character representation in hexadecimal format of hash value
      byte shaByteResult [32];
      static char shaCharResult [65];
      mbedtls_md_context_t ctx;
      mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
      mbedtls_md_init (&ctx);
      mbedtls_md_setup (&ctx, mbedtls_md_info_from_type (md_type), 0);
      mbedtls_md_starts (&ctx);
      mbedtls_md_update (&ctx, (const unsigned char *) clearText, strlen (clearText));
      mbedtls_md_finish (&ctx, shaByteResult);
      mbedtls_md_free (&ctx);
      for (int i = 0; i < 32; i++) sprintf (shaCharResult + 2 * i, "%02x", (int) shaByteResult [i]);
      return shaCharResult;  
    }
  
    void usersInitialization () {                                            // creates user management files with "root", "webadmin" and "webserver" users
                                                                             // only 3 fields are used: user name, hashed password and home directory
      if (!SPIFFSmounted) { Serial.printf ("[user_management] can't manage users configuration files since file system is not mounted"); return; }
      
      #if INITIALIZE_USERS == DO_INITIALIZE_USERS
  
        if (File file = SPIFFS.open ("/etc/passwd", FILE_WRITE)) { 
          if (file.printf ("root:x:0:::/:\r\n") != 15)                     {file.close (); goto userInitializationError;}; // create "root" user with ID = 0 and home direcory = / (ID is not used, you can also skip it)
          if (file.printf ("webserver::100:::/var/www/html/:\r\n") != 34)  {file.close (); goto userInitializationError;}; // create "webserver" user with ID = 100 and home directory /var/www/html/ (ID is not used, you can also skip it)
          if (file.printf ("telnetserver::101:::/var/telnet/:\r\n") != 35)  {file.close (); goto userInitializationError;}; // create "telnetserver" user with ID = 100 and home directory /var/tlnet/ (ID is not used, you can also skip it)
          if (file.printf ("webadmin:x:1000:::/var/www/html/:\r\n") != 35) {file.close (); goto userInitializationError;}; // create "web-admin" user with ID = 1000 and home directory /var/www/html/
          // add entries for each additional user (with ID >= 1000 to comply with UNIX but it doesn't really matter here)
          file.close ();
          Serial.printf ("[user_management] /etc/passwd created.\n");
          if (File file = SPIFFS.open ("/etc/shadow", FILE_WRITE)) {
            if (file.printf ("root:$5$%s:::::::\r\n", __sha256__ ("rootpassword")) != 81)         {file.close (); goto userInitializationError;}; // create "root" password_ <user>:$5$<sha_256 ("rootpassword")>
            // webserver is a system account that doesn't need password
            if (file.printf ("webadmin:$5$%s:::::::\r\n", __sha256__ ("webadminpassword")) != 85) {file.close (); goto userInitializationError;}; // create "webadmin" password_ <user>:$5$<sha_256 ("webadminpassword")> 
            // TO DO: change "root" and "webadmin" password in the line above, add entries for additional users
            file.close ();
            Serial.printf ("[user_management] /etc/shadow created.\n");
          }
        } else {
    userInitializationError:
          Serial.printf ("[user_management] user initialization failed\n");
        }
      #endif
    }
    
    bool checkUserNameAndPassword (char *userName, char *password) { // scan through /etc/shadow file for (user name, pasword) pair and return true if found
      File file;
      if ((bool) (file = SPIFFS.open ("/etc/shadow", FILE_READ)) && !file.isDirectory ()) { // read /etc/shadow file: https://www.cyberciti.biz/faq/understanding-etcshadow-file/
        do {
          char line [256]; int i = 0; while (i < sizeof (line) - 1 && file.available () && (line [i] = (char) file.read ()) >= ' ') {if (line [i] == ':') line [i] = ' '; i++;} line [i] = 0;
          if (*line < ' ') continue;
          char name [33]; char sha256Password [68];
          if (2 != sscanf (line, "%32s %67s", name, &sha256Password)) {Serial.printf ("[user_management] bad format of /etc/shadow file\n"); file.close (); return false;} // failure
          if (!strcmp (userName, name)) { // user name matches
            // if (getUserHomeDirectory (userName)) { // home directory is set so user must be valid
              if (!strcmp (sha256Password + 3, __sha256__ (password))) { // password matches
                file.close (); return true; // success
              }
            // }
          }
        } while (file.available ());
        file.close (); return false; // failure
      } else {Serial.printf ("[user_management] can't read /etc/shadow file\n"); return false;} // failure
    } 
    
    static char *getUserHomeDirectory (char *userName) { // scans through /etc/passwd file for userName and returns home direcory of the user or NULL if not found
      static char homeDir [33]; *homeDir = 0; // SPIFFS_OBJ_NAME_LEN + 1
      File file;
      if ((bool) (file = SPIFFS.open ("/etc/passwd", FILE_READ)) && !file.isDirectory ()) { // read /etc/passwd file: https://www.cyberciti.biz/faq/understanding-etcpasswd-file-format/
        do {
          char line [256]; int i = 0; while (i < sizeof (line) - 1 && file.available () && (line [i] = (char) file.read ()) >= ' ') i++; line [i] = 0;
          if (*line < ' ') continue;
          if (char *q = strchr (line, ':')) {
            *q = 0;
            if (!strcmp (userName, line)) if (char *p = strchr (q + 1, ':')) if (q = strchr (p + 1, ':')) if (p = strchr (q + 1, ':')) if (q = strchr (p + 1, ':')) if (p = strchr (q + 1, ':')) {
              *p = 0;
              if (strlen (q + 1) < sizeof (homeDir)) strcpy (homeDir, q + 1);
            }
          }
          if (*homeDir) {file.close (); if (*(homeDir + strlen (homeDir) - 1) != '/') strcat (homeDir, "/"); return homeDir; } // success
        } while (file.available ());
        file.close (); return NULL; // failure
      } else {Serial.printf ("[user_management] can't read /etc/passwd\n"); return NULL;} // failure
    }

    bool changeUserPassword (char *userName, char *newPassword) {
      String s = "";
      char c;
      File file;
      if ((bool) (file = SPIFFS.open ("/etc/shadow", "r+")) && !file.isDirectory ()) {
        while (file.available ()) if ((c = file.read ()) != '\r') s += String (c);
        int i = s.indexOf (String (userName) + ":$5$");
        if (i >= 0) { 
          s = s.substring (0, i + strlen (userName) + 4) + String (__sha256__ (newPassword)) + s.substring (i + strlen (userName) + 68);
          file.seek (0, SeekSet);
          if (file.printf (s.c_str ()) != s.length ()) {
            file.close ();
            return false;  
          } else {
            file.close ();
            return true;           
          }          
        } else {
          file.close ();
          return false;
        }
      } else {
        Serial.printf ("[user_management] can't read /etc/shadow\n");
        return false;
      }
    }
    
  #endif
  
#endif
