/*
  
    userManagement.cpp
  
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    UNIX-like user management.
  
    April 27, 2026, Bojan Jurca

*/


#include "userManagement.h"


userManagement_t::userManagement_t (threadSafeFS::FS& fileSystem) : __fileSystem__ (fileSystem) {
    // create /etc/passwd if it doesn't exist
    if (!__fileSystem__.isFile ("/etc/passwd")) {
        // create /etc directory
        if (!__fileSystem__.isDirectory ("/etc"))
            __fileSystem__.mkdir ("/etc"); 

        bool created = __writePasswd__ ("root:::::/:\r\n" 
                                        "webadmin:::::/var/www/html/:");

        cout << (created ? "/etc/passwd created\n" : "error creating /etc/passwd\n");
    }

    // create /etc/shadow if it doesn't exist
    if (!__fileSystem__.isFile ("/etc/shadow")) {
        // /etc directory already exists

        bool created = __writeShadow__ ("root:$5$" DEFAULT_ROOT_PASSWORD_SHA ":::::::\r\n"
                                        "webadmin:$5$" DEFAULT_WEBADMIN_PASSWORD_SHA ":::::::");

        cout << (created ? "/etc/shadow created\n" : "error creating /etc/shadow\n");
    }
};

// scan through /etc/shadow file for (user name, pasword) pair and return true if found
bool userManagement_t::checkUserNameAndPassword (const char *userName, const char *password) {
    // initial checking
    if (strlen (userName) > USER_PASSWORD_MAX_LENGTH || strlen (password) > USER_PASSWORD_MAX_LENGTH) 
        return false;

    // read /etc/shadow
    char buffer [MAX_ETC_SHADOW_SIZE + 2] = {};
    buffer [0] = '\n';
    if (!__readShadow__ (buffer + 1))
        return false;

    // find user's record in the buffer
    char srchStr [USER_PASSWORD_MAX_LENGTH + 65 + 7];
    sprintf (srchStr, "\n%s:$5$", userName);
    __sha256__ (srchStr + strlen (srchStr), 65, password);
    strcat (srchStr, ":"); // we get something like "\nroot:$5$de362bbdf11f2df30d12f318eeed87b8ee9e0c69c8ba61ed9a57695bbd91e481:"

    if (strstr (buffer, srchStr)) // the beginning of the line matches the user name and sha of password
        return true; // success

    return false;
}

// returns        "/" for full access
// something like "/home/name" for limited access
//                "" for no access
Cstring<255> userManagement_t::getHomeDirectory (const char *userName) {
    Cstring<255> homeDirectory;
    if (strlen (userName) > USER_PASSWORD_MAX_LENGTH)
        return "";

    // read /etc/passwd
    char buffer [MAX_ETC_PASSWD_SIZE + 3] = {}; 
    buffer [0] = '\n';
    if (!__readPasswd__ (buffer + 1))
        return "";

    // find user's record in the buffer
    char srchStr [USER_PASSWORD_MAX_LENGTH + 2]; 
    sprintf (srchStr, "\n%s:", userName); // we get something like \nroot:$5$

    char *p = strstr (buffer, srchStr);
    if (!p)
        return ""; // user not found in /etc/passwd

    for (int i = 0; i < 5; i++)
        if (!(p = strchr (p + 1, ':')))
            return "";

    // the following uses too much stack:
     if (sscanf (p + 1, "%255[^:]", (char *) (homeDirectory.c_str ())) <= 0)
        return "";

    return homeDirectory;
}

// bool passwd (userName, newPassword) assignes a new password for the user by writing it's SHA value into /etc/shadow file, return success
bool userManagement_t::passwd (const char *userName, const char *newPassword) {
    // initial checking
    if (strlen (newPassword) > USER_PASSWORD_MAX_LENGTH) 
        return false;

    // read /etc/shadow
    char buffer [MAX_ETC_SHADOW_SIZE + 2] = {};
    buffer [0] = '\n';
    if (!__readShadow__ (buffer + 1))
        return false;

    // find user's record in the buffer
    char srchStr [USER_PASSWORD_MAX_LENGTH + 6];
    sprintf (srchStr, "\n%s:$5$", userName);

    char *p = strstr (buffer, srchStr);
    if (!p || p - buffer > MAX_ETC_SHADOW_SIZE - 65)
        return false; // user not found in /etc/shadow

    // replace SHA of old password with SHA of new password
    char newPasswordSHA [65];
    __sha256__ (newPasswordSHA, 65, newPassword);
    memcpy (p + strlen (srchStr), newPasswordSHA, 64);

    // write /etc/shadow
    if (!__writeShadow__ (buffer + 1))
        return false;

    return true; // success      
}

// char *userAdd (userName, userId, userHomeDirectory) adds userName, userId, userHomeDirectory to /etc/passwd and /etc/shadow, returns success or error message
const char *userManagement_t::userAdd (const char *userName, const char *userHomeDirectory) {

    if (!userName || strlen (userName) < 1)                     return "Missing user name";
    if (strlen (userName) > USER_PASSWORD_MAX_LENGTH)           return "User name too long";
    if (strstr (userName, ":"))                                 return "User name may not contain ':' character";
    if (!userHomeDirectory || strlen (userHomeDirectory) < 1)   return "Missing user's home directory";
    if (strlen (userHomeDirectory) > 255)                       return "User's home directory name too long";

    Cstring<255> homeDirectory = userHomeDirectory;

    {
        // read /etc/passwd
        Cstring<MAX_ETC_PASSWD_SIZE + 1> buffer = "\n";
        if (!__readPasswd__ ((char *) (buffer.c_str () + 1)))
            return "Can't read /etc/passwd";

        // find user's record in the buffer
        char srchStr [USER_PASSWORD_MAX_LENGTH + 2]; 
        sprintf (srchStr, "\n%s:", userName); // we get something like \nroot:$5$

        char *p = strstr (buffer.c_str (), srchStr);
        if (p)
            return "User with this name already exists";

        buffer += "\r\n";
        buffer += userName;
        buffer += ":::::";
        buffer += homeDirectory;
        buffer += ":";

        if (buffer.errorFlags ())
            return "Not enough space in /etc/passwd";

        // write /etc/passwd
        if (!__writePasswd__ ((char *) (buffer.c_str () + 1)))
            return "Can't write /etc/passwd";
    }

    {
        Cstring<MAX_ETC_SHADOW_SIZE + 1> buffer = "\n";
        if (!__readShadow__ ((char *) (buffer.c_str () + 1)))
            return "Can't read /etc/shadow";

        // find user's record in the buffer
        char srchStr [USER_PASSWORD_MAX_LENGTH + 2]; 
        sprintf (srchStr, "\n%s:", userName); // we get something like \nroot:$5$

        char *p = strstr (buffer.c_str (), srchStr);
        if (p) {
            memcpy (p + strlen (srchStr), DEFAULT_USER_PASSWORD_SHA, 65);
        } else {
            buffer += "\r\n";
            buffer += userName;
            buffer += ":$5$";
            buffer += DEFAULT_USER_PASSWORD_SHA;
            buffer += ":::::::";

            if (buffer.errorFlags ())
                return "Not enough space in /etc/shadow";

            // write /etc/passwd
            if (!__writeShadow__ ((char *) (buffer.c_str () + 1)))
                return "Can't write /etc/shadow";
        }

    }
    return "User created with default password '" DEFAULT_USER_PASSWORD "'";
}

// char *userDel (userName) deletes userName from /etc/passwd and /etc/shadow, returns success or error message
const char *userManagement_t::userDel (const char *userName) {

    if (!userName || strlen (userName) < 1)                     return "Missing user name";
    if (strlen (userName) > USER_PASSWORD_MAX_LENGTH)           return "User name too long";

    {
        // read /etc/passwd
        char buffer [MAX_ETC_PASSWD_SIZE + 3] = {}; 
        buffer [0] = '\n';
        if (!__readPasswd__ (buffer + 1))
            return "";

        // find user's record in the buffer
        char srchStr [USER_PASSWORD_MAX_LENGTH + 2]; 
        sprintf (srchStr, "\n%s:", userName); // we get something like \nroot:$5$

        char *p = strstr (buffer, srchStr);
        if (!p)
            return "User with this name doesn't exist";

        p++; // p now points directly to username

        // find the end of the line
        char *q = p;
        while (*q > '\n')
            q++;
        if (*q) {
            // end of line found
            while (*q && *q < ' ')
                q++;
            strcpy (p, q);
        } else {
            // end of line not found - this is the last record, close and rtrim the buffer
            for (*p = 0; p > buffer && *p <= ' '; p--)
                *p = 0;
        }

        // write /etc/passwd
        if (!__writePasswd__ (buffer + 1))
            return "Can't write /etc/passwd";
    }

    {
        // read /etc/shadow
        char buffer [MAX_ETC_SHADOW_SIZE + 2] = {};
        buffer [0] = '\n';
        if (!__readShadow__ (buffer + 1))
            return "Can't read /etc/shadow";

        // find user's record in the buffer
        char srchStr [USER_PASSWORD_MAX_LENGTH + 6];
        sprintf (srchStr, "\n%s:$5$", userName);

        char *p = strstr (buffer, srchStr);
        if (p) {
            p++; // p now points directly to username

            // find the end of the line
            char *q = p;
            while (*q > '\n')
                q++;
            if (*q) {
                // end of line found
                while (*q && *q < ' ')
                    q++;
                strcpy (p, q);
            } else {
                // end of line not found - this is the last record, close and rtrim the buffer
                for (*p = 0; p > buffer && *p <= ' '; p--)
                    *p = 0;
            }

            // write /etc/shadow
            if (!__writeShadow__ (buffer + 1))
                return "Can't write /etc/shadow";
        }
    }
    return "User deleted";
}

// converts clearText to 256 bit SHA, returns character representation in hexadecimal format of hash value
bool userManagement_t::__sha256__ (char *buffer, size_t bufferSize, const char *clearText) {
    *buffer = 0;
    byte shaByteResult [32];
    char shaCharResult [65];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init (&ctx);
    mbedtls_md_setup (&ctx, mbedtls_md_info_from_type (md_type), 0);
    mbedtls_md_starts (&ctx);
    mbedtls_md_update (&ctx, (const unsigned char *) clearText, strlen (clearText));
    mbedtls_md_finish (&ctx, shaByteResult);
    mbedtls_md_free (&ctx);
    for (int i = 0; i < 32; i++) 
        sprintf (shaCharResult + 2 * i, "%02x", (int) shaByteResult [i]);
    if (bufferSize > strlen (shaCharResult)) { 
        strcpy (buffer, shaCharResult); 
        return true; 
    }
    return false; 
}

bool userManagement_t::__writePasswd__ (const char *buffer) { // buffer of MAX_ETC_PASSWD_SIZE bytes
    threadSafeFS::File f = __fileSystem__.open ("/etc/passwd", "w");
    if (f && !f.isDirectory ())
        if (f.print (buffer) > 0)
            return true;
    return false;
}

bool userManagement_t::__readPasswd__ (char *buffer) { // buffer of MAX_ETC_PASSWD_SIZE bytes
    threadSafeFS::File f = __fileSystem__.open ("/etc/passwd", "r");
    if (!f || f.isDirectory ())
        return false;

    size_t bytesRead = f.read ((byte *) buffer, MAX_ETC_PASSWD_SIZE);
    if (bytesRead == MAX_ETC_PASSWD_SIZE)
        return false; // file contet too long for buffer
    // buffer [bytesRead] = 0; // close C string        
    // close and rtrim buffer
    for (char *p = buffer + bytesRead; p > buffer && *p <= ' '; p--)
        *p = 0;

    return true;
}

bool userManagement_t::__writeShadow__ (const char *buffer) { // buffer of MAX_ETC_SHADOW_SIZE bytes
    threadSafeFS::File f = __fileSystem__.open ("/etc/shadow", "w");
    if (f && !f.isDirectory ())
        if (f.print (buffer) > 0)
            return true;
    return false;
}

bool userManagement_t::__readShadow__ (char *buffer) { // buffer of MAX_ETC_SHADOW_SIZE bytes
    threadSafeFS::File f = __fileSystem__.open ("/etc/shadow", "r");
    if (!f || f.isDirectory ())
        return false;

    size_t bytesRead = f.read ((byte *) buffer, MAX_ETC_SHADOW_SIZE);
    if (bytesRead == MAX_ETC_SHADOW_SIZE)
        return false; // file contet too long for buffer
    // buffer [bytesRead] = 0; // close C string
    // close and rtrim buffer
    for (char *p = buffer + bytesRead; p > buffer && *p <= ' '; p--)
        *p = 0;

    return true;
}
