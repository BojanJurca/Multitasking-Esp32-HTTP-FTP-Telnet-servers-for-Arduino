/*
    Calculate and then execute daily plan - calculate Sun rise and Sun set at the beginning of each day and do something about it.
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

  // correct timing is needed to execute daily plan - define your time zone here
  #define TIMEZONE  CET_TIMEZONE               
  // tell time_functions.h where to read current time from
  #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will syncronize its time with
  #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
  #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
  #include "./servers/time_functions.h"     

  // Sun rise, Sun set information
  #include "sunriset.c"
  #define longitude 14.508850546798792  // put our location here
  #define latitude 46.04903113525957    // put your location here

// execute plan for today
void cronHandler (String& cronCommand) {

  if (cronCommand == "gotTime") { // the first time ESP32 got time from NTP server(s)

    Serial.printf ("[%10lu] [gotTime] at %s\n", millis (), timeToString (getLocalTime ()).c_str ());
    goto calculateDailyPlan;

  } else if (cronCommand == "calculateDailyPlan") { // at the beginning of each day
calculateDailyPlan:

    Serial.printf ("[%10lu] [calculateDailyPlan] at %s\n", millis (), timeToString (getLocalTime ()).c_str ());

    // calculate Sun rise and Sun set on this day (local time)
    time_t l = getLocalTime ();           // raw local time
    struct tm slt = timeToStructTime (l); // structured local time
    
    // Sun rise and Sun set will be calculated as floating point numbers representing hours
    double sunRise, sunSet; sun_rise_set (slt.tm_year + 1900, slt.tm_mon + 1, slt.tm_mday, longitude, latitude, &sunRise, &sunSet); 
    // add these floating point numbers to the beginning of the day then transform them to local time
    time_t d = l / 86400 * 86400; // beginning of the day - the day is defined by local time but its beginning is calculated in GMT
    time_t r = timeToLocalTime (d + sunRise * 3600);  // raw Sun rise local time
    struct tm srt = timeToStructTime (r);             // structured Sun rise local time
    Serial.printf ("[%10lu] [calculateDailyPlan] Sun rises at %s\n", millis (), timeToString (r).c_str ());
    time_t s = timeToLocalTime (d + sunSet * 3600);   // raw Sun set local time
    struct tm sst = timeToStructTime (s);             // structured Sun set local time
    Serial.printf ("[%10lu] [calculateDailyPlan] Sun sets at %s\n", millis (), timeToString (s).c_str ());

    // update cron table with newly calculated information
    cronTabDel ("SunRise"); 
    cronTabAdd (srt.tm_sec, srt.tm_min, srt.tm_hour, ANY, ANY, ANY, "SunRise");
    cronTabDel ("SunSet"); 
    cronTabAdd (sst.tm_sec, sst.tm_min, sst.tm_hour, ANY, ANY, ANY, "SunSet");
                   
  } else if (cronCommand == "SunRise") { // at Sun rise each day

    Serial.printf ("[%10lu] [SunRise] at %s, if you want to do something about it then put your code here\n", millis (), timeToString (getLocalTime ()).c_str ());
    
  } else if (cronCommand == "SunSet") {

    Serial.printf ("[%10lu] [SunSet] at %s, if you want to do something about it then put your code here\n", millis (), timeToString (getLocalTime ()).c_str ());

  }
}


void setup () {
  Serial.begin (115200);

  startWiFi ();                                   // connect to to WiFi router
  
  startCronDaemon (cronHandler, 4 * 1024);        // start cronDaemon which will sinchronize ESP32 internal clock with NTP server(s) and execute cron table commands

  // put inital cron commands into cron table
  cronTabAdd ("* * * * * * gotTime");             // gotTime will trigger only once - the first time ESP32 will synchronize its internal clock with NTP server(s)
  cronTabAdd ("0 0 0 * * * calculateDailyPlan");  // calculateDailyPlan will trigger at 0:0:0 (local time) each day 
}

void loop () {

}
