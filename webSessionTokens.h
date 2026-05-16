/*
  
    webSessionTokens.h 
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    May 22, 2026, Bojan Jurca

*/


#pragma once
#ifndef __WEB_SESSION_TOKENS_H__
    #define __WEB_SESSION_TOKENS_H__

    #include <LittleFS.h>               // or SPIFFS or FFat, ...
    #include <threadSafeFS.h>           // thread-safe wrapper file system wrapper
    #include <Cstring.hpp>


    #include <mbedtls/md.h>
    #include <time.h>
    #include <string.h>

    #include <threadSafeFS.h>


    // TUNING PARAMETERS
    #define TOKEN_MAX_LENGTH 16         // tokens are stored in /var/www/tokens/... files which leaves only 16 characters (on SPIFFS) to be used for file names


    class webSessionTokens_t {

        public:

            webSessionTokens_t (threadSafeFS::FS& fileSystem);

            Cstring<TOKEN_MAX_LENGTH>newToken (const char *userName, time_t expires);

            Cstring<64> getUserNameFromToken (const char *token);

            bool deleteToken (const char *token);

            int deleteExpiredTokens ();

        private:

            threadSafeFS::FS& __fileSystem__;
    };

#endif
