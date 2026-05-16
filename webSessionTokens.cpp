/*
  
    webSessionTokens.cpp
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
    
    May 22, 2026, Bojan Jurca

*/


#include "webSessionTokens.h"

#include <vector.hpp>


webSessionTokens_t::webSessionTokens_t (threadSafeFS::FS& fileSystem) : __fileSystem__ (fileSystem) {
    if (!__fileSystem__.isDirectory ("/var/www/tokens")) {
        __fileSystem__.mkdir ("/var"); 
        __fileSystem__.mkdir ("/var/www");
        __fileSystem__.mkdir ("/var/www/tokens");
    }

    deleteExpiredTokens ();
};

Cstring<TOKEN_MAX_LENGTH>webSessionTokens_t::newToken (const char *userName, time_t expires = 0) {
    // check arguments
    if (!userName || *userName == 0 || (expires > 0 && expires < 1600000000)) // 1600000000 ~2020
        return "";

    // generate 16 characters randomlong token
    static const char base64url[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";
    char token [17];                        // 16 chars + null terminator
    for (int i = 0; i < 16; i++) {
        uint32_t r = esp_random ();         // 32-bit hardware RNG
        token [i] = base64url [r & 0x3F];   // take lower 6 bits → 0–63
    }
    token [16] = '\0';

    // create token file
    Cstring<255> fileName = "/var/www/tokens/"; fileName += token;

    threadSafeFS::File f = __fileSystem__.open (fileName, "w");
    if (!f)
        return "";

    if (fprintf (f, "%llu\r\n%s", (unsigned long long) expires, userName) <= 0)
        return "";

    return token;
}

Cstring<64> webSessionTokens_t::getUserNameFromToken (const char *token) {
    // open token file
    Cstring<255> fileName = "/var/www/tokens/"; fileName += token;
    threadSafeFS::File f = __fileSystem__.open (fileName, "r");
    if (!f || f.isDirectory ())
        return "";

    char buf [120] = {};
    if (f.read ((uint8_t *) buf, sizeof (buf) - 1) <= 0)
        return "";

    time_t expires;
    Cstring<64> userName;

    if (sscanf (buf, "%llu\r\n%64s", (unsigned long long *) &expires, (char *) &userName) < 2)
        return "";

    time_t t = time (NULL);
    if (expires > 0 && (t < 1600000000 || expires <= t)) // 1600000000 ~2020
        return "";

    return userName;
}

bool webSessionTokens_t::deleteToken (const char *token) {
    // get token file name
    Cstring<255> fileName = "/var/www/tokens/"; fileName += token;
    return __fileSystem__.remove (fileName);
}

int webSessionTokens_t::deleteExpiredTokens () {
    int count = 0;
    vector<Cstring<255>> files;

    for (auto f1 : __fileSystem__.open ("/var/www/tokens")) {
        Cstring<255> fileName = "/var/www/tokens/"; fileName += f1.name ();
        
        // save file names in vector, we can not delete the file within the loop since the file is beeing opened

        if (files.push_back (fileName)) // if error pusshing back
            break;
    }

    for (auto fileName : files) {
        threadSafeFS::File f2 = __fileSystem__.open (fileName, "r");
        if (f2 && !f2.isDirectory ()) { 
            char buf [120] = {};
            if (f2.read ((uint8_t *) buf, sizeof (buf) - 1) > 0) {
                time_t expires;
                Cstring<64> userName;
                if (sscanf (buf, "%llu\r\n%64s", (unsigned long long *) &expires, (char *) &userName) >= 1) {
                    time_t t = time (NULL);
                    if (expires > 0 && (t < 1600000000 || expires <= t)) { // 1600000000 ~2020
                        f2.close ();
                        if (__fileSystem__.remove (fileName)) {
                            count ++;
                        }
                    }
                }
            }
        }
    }

    return count;
}