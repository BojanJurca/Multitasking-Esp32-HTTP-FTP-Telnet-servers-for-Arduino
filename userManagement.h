/*
  
    userManagement.h 
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    UNIX-like user management.
  
    April 27, 2026, Bojan Jurca

*/


#pragma once
#ifndef __USER_MANAGEMENT_H__
    #define __USER_MANAGEMENT_H__

    //#include <cstddef>
    #include <mbedtls/md.h>
    #include <time.h>
    #include <string.h>
    #include <Cstring.hpp>
    #include <threadSafeFS.h>


    // TUNING PARAMETERS
    #define USER_PASSWORD_MAX_LENGTH 64
    #define MAX_ETC_PASSWD_SIZE (1024 + 512)
    #define MAX_ETC_SHADOW_SIZE (1024 + 512)
    #define DEFAULT_ROOT_PASSWORD "rootpassword"
    #define DEFAULT_ROOT_PASSWORD_SHA "de362bbdf11f2df30d12f318eeed87b8ee9e0c69c8ba61ed9a57695bbd91e481" // = __SHA256__ ("rootpassword")
    #define DEFAULT_WEBADMIN_PASSWORD "webadminpassword"
    #define DEFAULT_WEBADMIN_PASSWORD_SHA "40c6af3d1540ca2af132e1e93e7f5a5f624280b9d4d552a0bb103afe17c75c53" // = __SHA256__ ("webadminpassword")
    #define DEFAULT_USER_PASSWORD "changeimmediatelly"
    #define DEFAULT_USER_PASSWORD_SHA "ef286dbce1c8edcb4db441e0b717942a90e7f26264fc46e0a518d446ef8da48c" // = __SHA256__ ("changeimmediatelly")


    class userManagement_t {

        public:

            // Singleton accessor
            static userManagement_t &getInstance ();

            bool checkUserNameAndPassword (const char *userName, const char *password);

            Cstring<255> getHomeDirectory (const char *userName);

            bool passwd (const char *userName, const char *newPassword);

            const char *userAdd (const char *userName, const char *userHomeDirectory);

            const char *userDel (const char *userName);

            userManagement_t (threadSafeFS::FS& fileSystem);

        private:

            threadSafeFS::FS& __fileSystem__;

            bool __writePasswd__ (const char *buffer); 
            bool __readPasswd__ (char *buffer);
            bool __writeShadow__ (const char *buffer); 
            bool __readShadow__ (char *buffer);

            static bool __sha256__ (char *buffer, size_t bufferSize, const char *clearText);
    };

#endif
