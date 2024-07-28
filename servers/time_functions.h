  /*

    time_functions.h
 
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino

    Cron daemon synchronizes the time with NTP server accessible from internet once a day.

    Jul 27, 2024, Bojan Jurca

    Nomenclature used in time_functions.h - for easier understaning of the code:

      - "buffer size" is the number of bytes that can be placed in a buffer. 

                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".      

      - "max length" is the number of characters that can be placed in a variable.

                  In case of C 0-terminated strings the terminating 0 character is not included so the buffer should be at least 1 byte larger.    
*/


    // ----- includes, definitions and supporting functions -----

    #include <WiFi.h>
    #include <ctime>
    #include <esp_sntp.h>
    #include "std/Cstring.hpp"
    #include "std/console.hpp"


#ifndef __TIME_FUNCTIONS__
    #define __TIME_FUNCTIONS__

    #ifndef TZ
        #ifdef TIMEZONE    
            #define TZ TIMEZONE
        #else
            #ifdef SHOW_COMPILE_TIME_INFORMATION
                #pragma message "TZ was not defined previously, #defining the default CET-1CEST,M3.5.0,M10.5.0/3 in time_functions.h"
            #endif
            #define TZ "CET-1CEST,M3.5.0,M10.5.0/3" // default: Europe/Ljubljana, or select another (POSIX) time zones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
        #endif
    #endif

    #ifndef __FILE_SYSTEM__
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Compiling time_functions.h without file system (fileSystem.hpp), time_functions.h will not use configuration files"
        #endif
    #endif


    // ----- functions and variables in this modul -----

    time_t time ();
    void setTimeOfDay (time_t);
    struct tm gmTime (time_t);
    struct tm localTime (time_t);
    char *ascTime (const struct tm, char *buf, size_t len);
    time_t getUptime ();
    bool cronTabAdd (uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, const char *, bool);
    bool cronTabAdd (const char *, bool);
    int cronTabDel (const char *);
    void startCronDaemon (void (* cronHandler) (const char *), size_t);
    void __timeAvailable__ (struct timeval *);


    // TUNNING PARAMETERS

    // NTP servers that ESP32 server is going to sinchronize its time with
    #ifndef DEFAULT_NTP_SERVER_1
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "DEFAULT_NTP_SERVER_1 was not defined previously, #defining the default 1.si.pool.ntp.org in time_finctions.h"
        #endif
        #define DEFAULT_NTP_SERVER_1    "1.si.pool.ntp.org"
    #endif
    #ifndef DEFAULT_NTP_SERVER_2
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "DEFAULT_NTP_SERVER_2 was not defined previously, #defining the default 2.si.pool.ntp.org in time_finctions.h"
        #endif
      #define DEFAULT_NTP_SERVER_2      "2.si.pool.ntp.org"
    #endif
    #ifndef DEFAULT_NTP_SERVER_3
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "DEFAULT_NTP_SERVER_3 was not defined previously, #defining the default 3.si.pool.ntp.org in time_finctions.h"
        #endif
        #define DEFAULT_NTP_SERVER_3    "3.si.pool.ntp.org"
    #endif

    #define MAX_ETC_NTP_CONF_SIZE 1 * 1024            // 1 KB will usually do - initialization reads the whole /etc/ntp.conf file in the memory 
                                                      
    // crontab definitions
    #define CRON_DAEMON_STACK_SIZE 8 * 1024
    #define CRONTAB_MAX_ENTRIES 32                    // this defines the size of cron table - increase this number if you going to handle more cron events
    #define CRON_COMMAND_MAX_LENGTH 48                // the number of characters in the longest cron command
    #define ANY 255                                   // byte value that corresponds to * in cron table entries (should be >= 60 so it can be distinguished from seconds, minutes, ...)
    #define MAX_TZ_ETC_NTP_CONF_SIZE CRONTAB_MAX_ENTRIES * (20 + CRON_COMMAND_MAX_LENGTH) // this should do
  

    // ----- CODE -----

    unsigned long __timeHasBeenSet__ = 0;
    RTC_DATA_ATTR time_t __startupTime__ = 0;


    static SemaphoreHandle_t __cronSemaphore__ = xSemaphoreCreateRecursiveMutex (); // controls access to cronDaemon's variables
  
    int __cronTabEntries__ = 0;
  
    struct cronEntryType {
        bool readFromFile;                              // true, if this entry is read from /etc/crontab file, false if it is inserted by program code
        bool markForExecution;                          // falg if the command is to be executed
        bool executed;                                  // flag if the command is beeing executed
        uint8_t second;                                 // 0-59, ANY (255) means *
        uint8_t minute;                                 // 0-59, ANY (255) means *
        uint8_t hour;                                   // 0-23, ANY (255) means *
        uint8_t day;                                    // 1-31, ANY (255) means *
        uint8_t month;                                  // 1-12, ANY (255) means *
        uint8_t day_of_week;                            // 0-6 and 7, Sunday to Saturday, 7 is also Sunday, ANY (255) means *
        char cronCommand [CRON_COMMAND_MAX_LENGTH + 1]; // cronCommand to be passed to cronHandler when time condition is met - it is reponsibility of cronHandler to do with it what is needed
        time_t lastExecuted;                            // the time cronCommand has been executed
    } __cronEntry__ [CRONTAB_MAX_ENTRIES];
  
    // adds new entry into crontab table
    bool cronTabAdd (uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t day_of_week, const char *cronCommand, bool readFromFile = false) {
        bool b = false;    
        xSemaphoreTakeRecursive (__cronSemaphore__, portMAX_DELAY);
            if (__cronTabEntries__ < CRONTAB_MAX_ENTRIES - 1) {
                __cronEntry__ [__cronTabEntries__] = {readFromFile, false, false, second, minute, hour, day, month, day_of_week, {}, 0};
                strncpy (__cronEntry__ [__cronTabEntries__].cronCommand, cronCommand, CRON_COMMAND_MAX_LENGTH);
                __cronEntry__ [__cronTabEntries__].cronCommand [CRON_COMMAND_MAX_LENGTH] = 0;
                __cronTabEntries__ ++;
            }
            b = true;
        xSemaphoreGiveRecursive (__cronSemaphore__);
        if (b) return true;
        #ifdef __DMESG__
            dmesgQueue << "[time][cronDaemon][cronTabAdd] can't add cronCommand, cron table is full: " << cronCommand;
        #endif
        cout << "[time][cronDaemon][cronTabAdd] can't add cronCommand, cron table is full: " << cronCommand << endl;
        return false;
    }
    
    // adds new entry into crontab table
    bool cronTabAdd (const char *cronTabLine, bool readFromFile = false) { // parse cronTabLine and then call the function above
        char second [3]; char minute [3]; char hour [3]; char day [3]; char month [3]; char day_of_week [3]; char cronCommand [65];
        if (sscanf (cronTabLine, "%2s %2s %2s %2s %2s %2s %64s", second, minute, hour, day, month, day_of_week, cronCommand) == 7) {
            int8_t se = strcmp (second, "*")      ? atoi (second)      : ANY; if ((!se && *second != '0')      || se > 59)  { 
                                                                                                                                #ifdef __DMESG__
                                                                                                                                    dmesgQueue << "[time][cronDaemon][cronAdd] invalid second: " << second; 
                                                                                                                                #endif
                                                                                                                                cout << "[time][cronDaemon][cronAdd] invalid second: " << second << endl;  
                                                                                                                                return false; 
                                                                                                                            }
            int8_t mi = strcmp (minute, "*")      ? atoi (minute)      : ANY; if ((!mi && *minute != '0')      || mi > 59)  { 
                                                                                                                                #ifdef __DMESG__
                                                                                                                                    dmesgQueue << "[time][cronDaemon][cronAdd] invalid minute: " << minute; 
                                                                                                                                #endif
                                                                                                                                cout << "[time][cronDaemon][cronAdd] invalid minute: " << minute << endl;  
                                                                                                                                return false; 
                                                                                                                            }
            int8_t hr = strcmp (hour, "*")        ? atoi (hour)        : ANY; if ((!hr && *hour != '0')        || hr > 23)  {
                                                                                                                                #ifdef __DMESG__
                                                                                                                                    dmesgQueue << "[time][cronDaemon][cronAdd] invalid hour: " << hour; 
                                                                                                                                #endif
                                                                                                                                cout << "[time][cronDaemon][cronAdd] invalid hour: " << hour << endl;  
                                                                                                                                return false; 
                                                                                                                            }
            int8_t dm = strcmp (day, "*")         ? atoi (day)         : ANY; if (!dm                          || dm > 31)  { 
                                                                                                                                #ifdef __DMESG__
                                                                                                                                    dmesgQueue << "[time][cronDaemon][cronAdd] invalid day: " << day; 
                                                                                                                                #endif
                                                                                                                                cout << "[time][cronDaemon][cronAdd] invalid day: " << day << endl;
                                                                                                                                return false; 
                                                                                                                            }
            int8_t mn = strcmp (month, "*")       ? atoi (month)       : ANY; if (!mn                          || mn > 12)  { 
                                                                                                                                #ifdef __DMESG__
                                                                                                                                    dmesgQueue << "[time][cronDaemon][cronAdd] invalid month: " << month; 
                                                                                                                                #endif
                                                                                                                                cout << "[time][cronDaemon][cronAdd] invalid month: " << month << endl;  
                                                                                                                                return false; 
                                                                                                                            }
            int8_t dw = strcmp (day_of_week, "*") ? atoi (day_of_week) : ANY; if ((!dw && *day_of_week != '0') || dw > 7)   {
                                                                                                                                #ifdef __DMESG__
                                                                                                                                    dmesgQueue << "[time][cronDaemon][cronAdd] invalid day of week: " << day_of_week; 
                                                                                                                                #endif
                                                                                                                                cout << "[time][cronDaemon][cronAdd] invalid day of week: " << day_of_week << endl;
                                                                                                                                return false; 
                                                                                                                            }
            if (!*cronCommand) { 
                #ifdef __DMESG__
                    dmesgQueue << "[time][cronDaemon][cronAdd] missing cron command";
                #endif
                cout << "[time][cronDaemon][cronAdd] missing cron command\n";  
                return false;
            }
            return cronTabAdd (se, mi, hr, dm, mn, dw, cronCommand, readFromFile);
        } else {
            #ifdef __DMESG__
                dmesgQueue << "[time][cronDaemon][cronAdd] invalid cronTabLine: " << cronTabLine;
            #endif
            cout << "[time][cronDaemon][cronAdd] invalid cronTabLine: " << cronTabLine << endl;
            return false;
        }
    }
    
    // deletes entry(es) from crontam table
    int cronTabDel (const char *cronCommand) { // returns the number of cron commands being deleted
        int cnt = 0;
        xSemaphoreTakeRecursive (__cronSemaphore__, portMAX_DELAY);
            int i = 0;
            while (i < __cronTabEntries__) {
                if (!strcmp (__cronEntry__ [i].cronCommand, cronCommand)) {        
                    for (int j = i; j < __cronTabEntries__ - 1; j ++) { __cronEntry__ [j] = __cronEntry__ [j + 1]; }
                    __cronTabEntries__ --;
                    cnt ++;
                } else {
                    i ++;
                }
            }
        xSemaphoreGiveRecursive (__cronSemaphore__);
        if (!cnt) {
            #ifdef __DMESG__
                dmesgQueue << "[time][cronDaemon][cronTabDel] cronCommand not found: " << cronCommand;
            #endif
            cout << "[time][cronDaemon][cronTabDel] cronCommand not found: " << cronCommand << endl;
        }
        return cnt;
    }

    // startCronDaemon functions runs __cronDaemon__ as a separate task. It does two things: 
    // 1. - it executes cron commands from cron table when the time is right
    // 2. - it synchronizes time with NTP servers once a day
    void __cronDaemon__ (void *ptrCronHandler) { 
        #ifdef __DMESG__
            dmesgQueue << "[time][cronDaemon] is running on core " << xPortGetCoreID ();
        #endif
        cout << "[time][cronDaemon] started\n";

        void (* cronHandler) (const char *) = (void (*) (const char *)) ptrCronHandler;  
        unsigned long lastMinuteCheckMillis = millis ();
        unsigned long lastHourCheckMillis = millis ();
        unsigned long lastDayCheckMillis = millis ();

        while (true) {
            delay (10);

            // trigger "EVERY_MINUTE" built in event? (triggers regardles wether real time is known or not)
            if (millis () - lastMinuteCheckMillis >= 60000) { 
                lastMinuteCheckMillis = millis ();
                if (cronHandler != NULL) cronHandler ("ONCE_A_MINUTE");

                // if the time has note been set yet try doing it now
                if (!time ())
                    sntp_restart ();
            }

            // trigger "EVERY_HOUR" built in event? (triggers regardles wether real time is known or not)
            if (millis () - lastHourCheckMillis >= 3600000) { 
                lastHourCheckMillis = millis ();
                if (cronHandler != NULL) cronHandler ("ONCE_AN_HOUR");
            }

            // trigger "EVERY_DAY" built in event? (triggers regardles wether real time is known or not)
            if (millis () - lastDayCheckMillis >= 86400000) { 
                lastDayCheckMillis = millis ();
                if (cronHandler != NULL) cronHandler ("ONCE_A_DAY");

                // synchronize time with NTP servers once a day
                sntp_restart ();
            } 

            // 3. execute cron commands from cron table
            time_t now = time ();
            if (!now) continue; // if the time is not known cronDaemon can't do anythig
            static time_t previous = now;
            for (time_t l = previous + 1; l <= now; l++) {
                delay (1);
                struct tm slt; localtime_r (&l, &slt); 
                //scan through cron entries and find commands that needs to be executed (at time l)
                xSemaphoreTakeRecursive (__cronSemaphore__, portMAX_DELAY);
                    // mark cron commands for execution
                    for (int i = 0; i < __cronTabEntries__; i ++) {
                        if ((__cronEntry__ [i].second == ANY      || __cronEntry__ [i].second == slt.tm_sec) && 
                            (__cronEntry__ [i].minute == ANY      || __cronEntry__ [i].minute == slt.tm_min) &&
                            (__cronEntry__ [i].hour == ANY        || __cronEntry__ [i].hour == slt.tm_hour) &&
                            (__cronEntry__ [i].day == ANY         || __cronEntry__ [i].day == slt.tm_mday) &&
                            (__cronEntry__ [i].month == ANY       || __cronEntry__ [i].month == slt.tm_mon + 1) &&
                            (__cronEntry__ [i].day_of_week == ANY || __cronEntry__ [i].day_of_week == slt.tm_wday || __cronEntry__ [i].day_of_week == slt.tm_wday + 7) ) {

                                if (!__cronEntry__ [i].executed) {
                                  __cronEntry__ [i].markForExecution = true;
                                }
                                                
                        } else {

                                __cronEntry__ [i].executed = false;
              
                        }
                    }          
                    // execute marked cron commands
                    int execCnt = 1;
                    while (execCnt) { // if a command during execution deletes other commands we have to repeat this step
                        execCnt = 0;
                        for (int i = 0; i < __cronTabEntries__; i ++) {
                            if ( __cronEntry__ [i].markForExecution ) {

                                if (__cronEntry__ [i].markForExecution) {
                                    __cronEntry__ [i].executed = true;
                                    __cronEntry__ [i].lastExecuted = now;
                                    __cronEntry__ [i].markForExecution = false;
                                    if (cronHandler != NULL) cronHandler (__cronEntry__ [i].cronCommand); // this should be called last in case cronHandler calls croTabAdd or cronTabDel itself                        
                                    execCnt ++; 
                                    delay (1);
                                }

                                __cronEntry__ [i].markForExecution = false;
                            }
                        }
                    }
                xSemaphoreGiveRecursive (__cronSemaphore__);
            }
            previous = now;
        }
    }

    // Runs __cronDaemon__ as a separate task. The first time it is called it also creates default configuration files /etc/ntp.conf and /etc/crontab
    void startCronDaemon (void (* cronHandler) (const char *)) { // synchronizes time with NTP servers from /etc/ntp.conf, reads /etc/crontab, returns error message. If /etc/ntp.conf or /etc/crontab don't exist (yet) creates the default one

        #ifdef __FILE_SYSTEM__
            if (fileSystem.mounted ()) {
              // read TZ (timezone) configuration from /usr/share/zoneinfo, create a new one if it doesn't exist
              if (!fileSystem.isFile ("/usr/share/zoneinfo")) {
                  // create directory structure
                  if (!fileSystem.isDirectory ("/usr/share")) { fileSystem.makeDirectory ("/usr"); fileSystem.makeDirectory ("/usr/share"); }
                  cout << "[time][cronDaemon] /usr/share/zoneinfo does not exist, creating default one ... ";
                  bool created = false;
                  File f = fileSystem.open ("/usr/share/zoneinfo", "w");
                  if (f) {
                      String defaultContent = F ( "# timezone configuration - reboot for changes to take effect\r\n"
                                                  "# choose one of available (POSIX) timezones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv\r\n\r\n"
                                                  "TZ " TZ "\r\n");
                    created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                    f.close ();

                    diskTrafficInformation.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way

                  }
                  cout << (created ? "created\n" : "error\n");
              
              }
              char __TZ__ [100] = TZ;
              {
                  cout << "[time][cronDaemon] reading /usr/share/zoneinfo ... ";
                  // parse configuration file if it exists
                  char buffer [MAX_TZ_ETC_NTP_CONF_SIZE] = "\n";
                  if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/usr/share/zoneinfo")) {
                      strcat (buffer, "\n");
                      // cout << buffer << "\n";

                      // char __TZ__ [100] = TZ;
                      char *p;
                      if ((p = stristr (buffer, "\nTZ"))) sscanf (p + 3, "%*[ =]%98[!-~]", __TZ__);                      

                      setenv ("TZ", __TZ__, 1);
                      tzset ();
                      #ifdef __DMESG__
                          dmesgQueue << "[time][cronDaemon] TZ environment variable set to " << __TZ__;
                      #endif
                      cout << "OK, TZ environment variable set to " << __TZ__ << endl;
                  } else {
                      cout << "error\n";
                  }
              }

              // read NTP configuration from /etc/ntp.conf, create a new one if it doesn't exist
              if (!fileSystem.isFile ((char *) "/etc/ntp.conf")) {
                  // create directory structure
                  if (!fileSystem.isDirectory ((char *) "/etc")) { fileSystem.makeDirectory ((char *) "/etc"); }
                  cout << "[time][cronDaemon] /etc/ntp.conf does not exist, creating default one ... ";
                  bool created = false;
                  File f = fileSystem.open ((char *) "/etc/ntp.conf", "w");
                  if (f) {
                      String defaultContent = F ( "# configuration for NTP - reboot for changes to take effect\r\n\r\n"
                                                  "server1 " DEFAULT_NTP_SERVER_1 "\r\n"
                                                  "server2 " DEFAULT_NTP_SERVER_2 "\r\n"
                                                  "server3 " DEFAULT_NTP_SERVER_3 "\r\n");
                      created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                      f.close ();

                      diskTrafficInformation.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way

                  }
                  cout << (created ? "created\n" : "error\n");
              }
              {
                  cout << "[time][cronDaemon] reading /etc/ntp.conf ... ";
                  // parse configuration file if it exists
                  char buffer [MAX_TZ_ETC_NTP_CONF_SIZE] = "\n";
                  if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/ntp.conf")) {
                      strcat (buffer, "\n");
                      // cout << buffer;

                      char ntpServer1 [255] = ""; // DNS host name may have max 253 characters
                      char ntpServer2 [255] = ""; // DNS host name may have max 253 characters
                      char ntpServer3 [255] = ""; // DNS host name may have max 253 characters 

                      char *p;                    
                      if ((p = stristr (buffer, "\nserver1"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", ntpServer1);
                      if ((p = stristr (buffer, "\nserver2"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", ntpServer2);
                      if ((p = stristr (buffer, "\nserver3"))) sscanf (p + 8, "%*[ =]%253[0-9A-Za-z.-]", ntpServer3);

                      // cout << ntpServer1 << ", " << ntpServer1 << ", " << ntpServer1 << endl;

                      // configTime (0, 0, ntpServer1, ntpServer2, ntpServer3);
                      configTzTime (__TZ__, ntpServer1, ntpServer2, ntpServer3);
                      sntp_set_time_sync_notification_cb (__timeAvailable__);
                      #ifdef __DMESG__
                          dmesgQueue << "[time][cronDaemon] NTP servers set to " << ntpServer1 << ", " << ntpServer2 << ", " << ntpServer3;
                      #endif
                      cout << "OK, NTP servers set to " << ntpServer1 << ", " << ntpServer2 << ", " << ntpServer3 << endl; 
                  } else {
                      cout << "error\n";
                  }
              }

              // read scheduled tasks from /etc/crontab
              if (!fileSystem.isFile ((char *) "/etc/crontab")) {
                  // create directory structure
                  if (!fileSystem.isDirectory ((char *) "/etc")) { fileSystem.makeDirectory ((char *) "/etc"); }          
                  cout << "[time][cronDaemon] /etc/crontab does not exist, creating default one ... ";
                  bool created = false;
                  File f = fileSystem.open ((char *) "/etc/crontab", "w");
                  if (f) {
                      String defaultContent = F("# scheduled tasks (in local time) - reboot for changes to take effect\r\n"
                                                "#\r\n"
                                                "# .------------------- second (0 - 59 or *)\r\n"
                                                "# |  .---------------- minute (0 - 59 or *)\r\n"
                                                "# |  |  .------------- hour (0 - 23 or *)\r\n"
                                                "# |  |  |  .---------- day of month (1 - 31 or *)\r\n"
                                                "# |  |  |  |  .------- month (1 - 12 or *)\r\n"
                                                "# |  |  |  |  |  .---- day of week (0 - 7 or *; Sunday=0 and also 7)\r\n"
                                                "# |  |  |  |  |  |\r\n"
                                                "# *  *  *  *  *  * cronCommand to be executed\r\n"
                                                "# * 15  *  *  *  * exampleThatRunsQuaterPastEachHour\r\n");

                    created = (f.printf (defaultContent.c_str ()) == defaultContent.length ());
                    f.close ();

                    diskTrafficInformation.bytesWritten += defaultContent.length (); // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way

                  }
                  cout << (created ? "created\n" : "error\n");
              }
              {
                  cout << "[time][cronDaemon] reading /etc/crontab ... ";
                  // parse scheduled tasks if /etc/crontab exists
                  char buffer [MAX_TZ_ETC_NTP_CONF_SIZE] = "\n";
                  if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 3, "/etc/crontab")) {
                      strcat (buffer, "\n");
                      // cout << buffer;

                      char *p, *q;
                      p = buffer + 1;
                      while (*p) {
                          if ((q = strstr (p, "\n"))) {
                              *q = 0;
                              if (*p) cronTabAdd (p, true);
                              p = q + 1;
                          }
                        }

                        cout << "OK, " << __cronTabEntries__ << " entries\n";
                    } else {
                        cout << "error\n";
                    }
              }
            } else {
                cout << "[time][cronDaemon] file system not mounted, can't read or write configuration files\n";

                // set the default timezone and NTP configs
                configTzTime (TZ, DEFAULT_NTP_SERVER_1, DEFAULT_NTP_SERVER_2, DEFAULT_NTP_SERVER_3);
                sntp_set_time_sync_notification_cb (__timeAvailable__);

            }
        #else
            cout << "[time][cronDaemon] is starting without a file system\n";

            // set the default timezone and NTP configs
            configTzTime (TZ, DEFAULT_NTP_SERVER_1, DEFAULT_NTP_SERVER_2, DEFAULT_NTP_SERVER_3);
            sntp_set_time_sync_notification_cb (__timeAvailable__);

        #endif    
        
        // run periodic synchronization with NTP servers and execute cron commands in a separate thread
        #define tskNORMAL_PRIORITY 1
        if (pdPASS != xTaskCreate (__cronDaemon__, "cronDaemon", CRON_DAEMON_STACK_SIZE, (void *) cronHandler, tskNORMAL_PRIORITY, NULL)) {
            #ifdef __DMESG__
                dmesgQueue << "[time][cronDaemon] xTaskCreate error, could not start cronDaemon";
            #endif
            cout << "[time][cronDaemon] xTaskCreate error, could not start cronDaemon\n";
        }
    }
  
    // a more convinient version of time function: returns GMT or 0 if the time is not set
    time_t time () {
        time_t t = time (NULL);
        if (t < 1687461154) return 0; // 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
        return t;
    }

    // returns the number of seconds ESP32 has been running
    time_t getUptime () {
        time_t t = time (NULL);
        return __timeHasBeenSet__ ? t - __startupTime__ :  millis () / 1000; // if the time has already been set, 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
    }
  
    char *ascTime (const struct tm st, char *buf, size_t len) {
        strftime(buf, len, "%Y/%m/%d %T", &st);
        return buf; 
    }

    void __timeAvailable__ (struct timeval *t) {
        #ifdef __DMESG__
            char result [27];
            ascTime (localTime (t->tv_sec), result, sizeof (result));
            dmesgQueue << "[time][cronDaemon] got time adjustment from NTP: " << result;
        #endif
        if (!__startupTime__)
            __startupTime__ = t->tv_sec - getUptime (); // recalculate
        __timeHasBeenSet__ ++;
    }

    // sets internal clock, also sets/corrects __startupTime__ internal variable
    void setTimeOfDay (time_t t) {         
        time_t oldTime = time (NULL);
        struct timeval newTime = {t, 0};
        settimeofday (&newTime, NULL); 

        char buf [100];
        if (__timeHasBeenSet__) {
            ascTime (localTime (oldTime), buf, sizeof(buf));
            strcat (buf, " to ");
            ascTime (localTime (t), buf + strlen (buf), sizeof(buf)-strlen (buf));
            #ifdef __DMESG__
                dmesgQueue << "[time] time corrected from " << buf;
            #endif
            cout << "[time] time corrected from " << buf << endl;
        } else {
            __startupTime__ = t - getUptime (); // recalculate
            __timeHasBeenSet__ ++;
            ascTime (localTime (t), buf, sizeof(buf));
            #ifdef __DMESG__
                dmesgQueue << "[time] time set to " << buf; 
            #endif
            cout << "[time] time set to " << buf << endl;
        }
    }

    // a more convinient version of thread safe gmtime_r function: converts GMT in time_t to GMT in struct tm
    struct tm gmTime (const time_t t) {
        struct tm st;
        return *gmtime_r (&t, &st);
    }

    // a more convinient version of thread safe localtime_r function: converts GMT in time_t to local time in struct tm
    struct tm localTime (const time_t t) {
        struct tm st;
        return *localtime_r (&t, &st);
    }

#endif
