// this file is here just to make development of server files easier, it is not part of any project

#include <WiFi.h>

#include "TcpServer.hpp"
#include "network.h"
#include "file_system.h"

// #define USER_MANAGEMENT NO_USER_MANAGEMENT          
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT 
//   #define ROOT_PASSWORD "rootpassword" 
// (default) #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT   
#include "user_management.h"
#include "ftpServer.hpp"
#include "webServer.hpp"
#include "telnetServer.hpp"
#include "time_functions.h"
#include "oscilloscope.h"

void setup () {  
  Serial.begin (115200);

  // __TEST_DST_TIME_CHANGE__ ();

  mountFileSystem (false);
 // startCronDaemonAndInitializeItAtFirstCall (NULL, 3 * 1024);

  // "start %7s sampling on GPIO %i every %i %2s refresh screen of width %i %2s every %i %2s set %8s slope trigger to %i set %8s slope trigger to %i"
  // char *test = "start digital sampling on GPIO 5, 6, 7 every 10 ms screen width = 400 ms set positive slope trigger to 1 set negative slope trigger to 0";
  char test [200];
  strcpy (test, "start digital sampling on GPIO 5, 6, 7 every 10 ms screen width = 400 ms set positive slope trigger to 1 set negative slope trigger to 0");


  char readType [10] = "";
  int gpio1 = -1;
  int gpio2 = -1;
  int gpio3 = -1;
  int gpio4 = -1;
  int samplingTime = 0;
  char samplingUnit [10] = "";
  int screenWidth = 0;
  char screenWidthUnit [10] = "";

  char *part1 = test;
  char *part2 = strstr (part1, " every"); if (part2) { *part2 = 0; part2++; Serial.printf ("\n%s\n", part2); }
  char *part3 = strstr (part2, " set"); if (part3) *(part3++) = 0;
  
  // int fields = sscanf (test, "start %7s sampling on GPIO %i, %i, %i, %i every %i %2s screen width = %i %2s ", readType, &gpio1, &gpio2, &gpio3, &gpio4, &samplingTime, samplingUnit, &screenWidth, screenWidthUnit);
  // Serial.printf ("parsed %i fields: %s  %i %i %i %i    %i %s    %i %s\n", fields, readType, gpio1, gpio2, gpio3, gpio4, samplingTime, samplingUnit, screenWidth, screenWidthUnit);
  
  int fields = sscanf (part1, "start %7s sampling on GPIO %i, %i, %i, %i ", readType, &gpio1, &gpio2, &gpio3, &gpio4);
  Serial.printf ("parsed %i fields: %s  %i, %i, %i, %i\n", fields, readType, gpio1, gpio2, gpio3, gpio4);
  if (fields && part2) fields += sscanf (part2, "every %i %2s screen width = %i %2s ", &samplingTime, samplingUnit, &screenWidth, screenWidthUnit);

  Serial.printf ("parsed %i fields: %s  %i %i %i %i    %i %s    %i %s\n", fields, readType, gpio1, gpio2, gpio3, gpio4, samplingTime, samplingUnit, screenWidth, screenWidthUnit);

}

void loop () {  

}
