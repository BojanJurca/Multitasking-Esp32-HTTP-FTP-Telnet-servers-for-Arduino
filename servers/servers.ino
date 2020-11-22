// this file is here just to make development of server files easier, it is not part of any project

#include <WiFi.h>

#include "TcpServer.hpp"
#include "network.h"
#include "file_system.h"

// #define USER_MANAGEMENT NO_USER_MANAGEMENT          
// #define USER_MANAGEMENT HARDCODED_USER_MANAGEMENT 
//   #define ROOT_PASSWORD "password" 
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
  startCronDaemonAndInitializeItAtFirstCall (NULL, 3 * 1024);
  
}

void loop () {  

}
