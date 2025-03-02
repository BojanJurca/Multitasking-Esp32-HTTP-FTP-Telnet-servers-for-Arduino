// #include "./servers/fileSystem.hpp"  // all file name and file info related functions are there, by default FILE_SYSTEM is #defined as FILE_SYSTEM_LITTLEFS, other options are: FILE_SYSTEM_FAT, FILE_SYSTEM_SD_CARD and (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)


// define local time settings by changing the following defaults
#define DEFAULT_NTP_SERVER_1 "1.si.pool.ntp.org"
#define DEFAULT_NTP_SERVER_2 "2.si.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3 "3.si.pool.ntp.org"
#define TZ "CET-1CEST,M3.5.0,M10.5.0/3" // select POSIX time zone: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#include "./servers/time_functions.h"   // if #included after fileSystem.hpp time functions will use configuration files  /usr/share/zoneinfo, /etc/ntp.conf and /etc/crontab


#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"


void cronHandlerCallback (const char *cronCommand) {
    // this callback function is supposed to handle cron commands/events that occur at time specified in crontab

    #define cronCommandIs(X) (!strcmp (cronCommand, X))

    Serial.println (cronCommand);

         if (cronCommandIs ("gotTime"))                         {   // triggers only once - the first time ESP32 sets its clock (when it gets time from NTP server for example)
                                                                    char buf [27]; // 26 bytes are needed
                                                                    ascTime (localTime (time ()), buf, sizeof (buf));
                                                                    Serial.print ("Got time at "); Serial.print (buf); Serial.println (" (local time), do whatever needs to be done the first time the time is known");
                                                                }           
    else if (cronCommandIs ("newYear'sGreetingsToProgrammer"))  {   // triggers at the beginning of each year
                                                                    Serial.println ("[cronDaemon] *** HAPPY NEW YEAR ***!");
                                                                }
    else if (cronCommandIs ("onMinute"))                        {   // triggers each minute at 0 seconds
                                                                    Serial.println ("[cronDaemon] just in case you need to do something");
                                                                }
}


void setup () {
    Serial.begin (115200);

    // fileSystem.mountLittleFs (true); // or just call    LittleFS.begin (true);

    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    startCronDaemon (cronHandlerCallback); // start cron daemon task, there is another arguments available set cron daemon process stack size

    // add cron commands, if a file system is used they can be also entered into /etc/crontab file
    cronTabAdd ("* * * * * * gotTime");                         // triggers only once - when ESP32 reads time from NTP servers for the first time
    cronTabAdd ("0 0 0 1 1 * newYear'sGreetingsToProgrammer");  // triggers at the beginning of each year
    cronTabAdd ("0 * * * * * onMinute");                        // triggers each minute at 0 seconds
    //           | | | | | | |
    //           | | | | | | |___ cron command, this information will be passed to cronHandlerCallback when the time comes
    //           | | | | | |___ day of week (0 - 7 or *; Sunday=0 and also 7)
    //           | | | | |___ month (1 - 12 or *)
    //           | | | |___ day (1 - 31 or *)
    //           | | |___ hour (0 - 23 or *)
    //           | |___ minute (0 - 59 or *)
    //           |___ second (0 - 59 or *)


    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP address from the router
        delay (1000);
        Serial.printf ("   .\n");
    } 
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
}

void loop () {
    
}
