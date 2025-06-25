/*

    smtpClient.hpp 
 
    This file is part of Multitasking Esp32 HTTP FTP Telnet servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-Telnet-servers-for-Arduino
  
    February 6, 2025, Bojan Jurca


    Classes used in this module:

        tcpClient_t

    Inheritance diagram:    

             ┌─────────────┐
             │ tcpServer_t ┼┐
             └─────────────┘│
                            │
      ┌─────────────────┐   │
      │ tcpConnection_t ┼┐  │
      └─────────────────┘│  │
                         │  │
                         │  │  ┌─────────────────────────┐
                         │  ├──┼─ httpServer_t           │
                         ├─────┼──── webSocket_t         │
                         │  │  │     └── httpConection_t │
                         │  │  └─────────────────────────┘
                         │  │  logicly webSocket_t would inherit from httpConnection_t but it is easier to implement it the other way around
                         │  │
                         │  │
                         │  │  ┌────────────────────────┐
                         │  ├──┼─ telnetServer_t        │
                         ├─────┼──── telnetConnection_t │
                         │  │  └────────────────────────┘
                         │  │
                         │  │
                         │  │  ┌────────────────────────────┐
                         │  └──┼─ ftpServer_t               │
                         ├─────┼──── ftpControlConnection_t │
                         │     └────────────────────────────┘
                         │  
                         │  
                         │     ┌──────────────┐
                         └─────┼─ tcpClient_t │
                               └──────────────┘

*/


#include <WiFi.h>
#include "tcpClient.hpp"
#include <mbedtls/base64.h>
#include "std/Cstring.hpp"


#ifndef __SMTP_CLIENT__
    #define __SMTP_CLIENT__

    // ----- functions and variables in this modul -----

    cstring sendMail (const char *message, const char *subject, const char *to, const char *from, const char *password, const char *userName, int smtpPort, const char *smtpServer);
    
    
    // TUNNING PARAMETERS

    #define SMTP_TIME_OUT 3                       // 3 sec 
    #define SMTP_BUFFER_SIZE 256                  // for constructing SMTP commands and reading SMTP reply
    #ifndef HOSTNAME
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "HOSTNAME was not defined previously, #defining the default MyESP32Server in smtpClient.h"
        #endif
        #define HOSTNAME "MyESP32Server"          // use default if not defined previously
    #endif
    #define MAX_ETC_MAIL_SENDMAIL_CF 1 * 1024     // 1 KB will usually do - sendmail reads the whole /etc/mail/sendmail.cf file in the memory 


    // ----- CODE -----


    // sends message, returns error or success text (from SMTP server)
    cstring __sendMail__ (const char *message, const char *subject, const char *to, const char *from, const char *password, const char *userName, int smtpPort, const char *smtpServer) {

        if (WiFi.status () != WL_CONNECTED) {
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[smtpClient] not connected to WiFi";
            #endif
            return (const char *) "[smtpClient] not connected to WiFi";
        }

        tcpClient_t smtpClient (smtpServer, smtpPort, SMTP_TIME_OUT);
        if (smtpClient.errText ())
            return smtpClient.errText ();

        // connection is established now and the socket os opened

        char buffer [SMTP_BUFFER_SIZE];

        // 1. read welcome message from SMTP server
        switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
            case -1:                return strerror (errno);
            case 0:                 return (const char *) "Connection closed by peer";
            case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
            default:                break; // continue
        }
        // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
        if (strstr (buffer, "220") != buffer) // 220 mail.siol.net ESMTP server ready
            return buffer;

        // 2. send EHLO to SMTP server (the buffer should have enough capacity)
        if (sizeof (buffer) < 5 + strlen (HOSTNAME) + 3)
            return (const char *) "Buffer too small";
        sprintf (buffer, "EHLO %s\r\n", (char *) HOSTNAME);
        switch (smtpClient.sendString (buffer)) {
            case -1:  return strerror (errno);
            case 0:   return (const char *) "Connection closed by peer";
            default:  break;
        }

        // 3. get the reply from SMTP server
        switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
            case -1:                return strerror (errno);
            case 0:                 return (const char *) "Connection closed by peer";            
            case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
            default:                break; // continue
        }
        // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
        if (strstr (buffer, "250") != buffer) // 250-mail.siol.net ...
            return buffer;

        // 4. send login request to SMTP server
        switch (smtpClient.sendString ((char *) "AUTH LOGIN\r\n")) {
            case -1:  return strerror (errno);
            case 0:   (const char *) "Connection closed by peer";
            default:  break;
        }

        // 5. get the reply from SMTP server
        switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
            case -1:                return strerror (errno);
            case 0:                 return (const char *) "Connection closed by peer"; 
            case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
            default:                break; // continue
        }
        // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
        if (strstr (buffer, "334") != buffer) // 334 VXNlcm5hbWU6 (= base64 encoded "Username:")
            return buffer;

        // 6. send base64 encoded user name to SMTP server
        size_t encodedLen;
        mbedtls_base64_encode ((unsigned char *) buffer, sizeof (buffer) - 3, &encodedLen, (const unsigned char *) userName, strlen (userName));
        strcpy (buffer + encodedLen, "\r\n");
        switch (smtpClient.sendString (buffer)) {
            case -1:  return strerror (errno);
            case 0:   (const char *) "Connection closed by peer";
            default:  break;
        }

        // 7. get the reply from SMTP server
        switch (smtpClient.recvString ( buffer, SMTP_BUFFER_SIZE, "\n")) {
            case -1:                return strerror (errno);
            case 0:                 return (const char *) "Connection closed by peer"; 
            case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
            default:                break; // continue
        }
        // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
        if (strstr (buffer, "334") != buffer) // 334 UGFzc3dvcmQ6 (= base64 encoded "Password:")
            return buffer;

        // 8. send base64 encoded password to SMTP server
        mbedtls_base64_encode ((unsigned char *) buffer, sizeof (buffer) - 3, &encodedLen, (const unsigned char *) password, strlen (password));
        strcpy (buffer + encodedLen, "\r\n");
        switch (smtpClient.sendString (buffer)) {
            case -1:  return strerror (errno);
            case 0:   (const char *) "Connection closed by peer";
            default:  break;
        }

        // 9. get the reply from SMTP server
        switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
            case -1:                return strerror (errno);
            case 0:                 return (const char *) "Connection closed by peer";
            case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
            default:                break; // continue
        }
        // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
        if (strstr (buffer, "235") != buffer) // 235 2.7.0 Authentication successful 
            return buffer;

        // 10. send MAIL FROM to SMTP server - there should be only one address in from string (there is onlyone sender) - parse it against @ chracter and get the reply(es)
        if (sizeof (buffer) < 13 + strlen (from) + 3)
            return (const char *) "Buffer too small";
        strcpy (buffer + 13, from); // leave some spoace at hte start of the buffer
        {
            bool quotation = false;
            for (int i = 13; buffer [i]; i++) {  
                switch (buffer [i]) {
                    case '\"':  quotation = !quotation; 
                                break;
                    case '@':   if (!quotation) { // we found @ in email address
                                    int j; for (j = i - 1; j > 0 && buffer [j] > '\"' && buffer [j] != ','; j--); // go back to the beginning of email address
                                    if (j == 0) j--;
                                    int k; for (k = i + 1; buffer [k] > '\"' && buffer [k] != ',' && buffer [k] != ';'; k++); buffer [k] = 0; // go forward to the end of email address
                                    i = k;
                                    strcpy (buffer, "MAIL FROM:"); strcat (buffer, buffer + j + 1); strcat (buffer, (char *) "\r\n");

                                    // send the buffer now
                                    switch (smtpClient.sendString (buffer)) {
                                        case -1:  return strerror (errno);
                                        case 0:   (const char *) "Connection closed by peer";
                                        default:  break;
                                    }

                                    // get the reply from SMTP server
                                    switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
                                        case -1:                return strerror (errno);
                                        case 0:                 return (const char *) "Connection closed by peer";
                                        case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
                                        default:                break; // continue
                                    }
                                    // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
                                    if (strstr (buffer, "250") != buffer) // 250 2.1.0 Ok
                                        return buffer;

                                }
                    default:    break;
                }
            }
        }

        // 11. send RCPT TO for each address in to-list, to SMTP server and read its reply - parse it against @ chracter and get the reply(es)
        if (sizeof (buffer) < 13 + strlen (to) + 1 + 1)
            return (const char *) "Buffer too small";
        memset (buffer, 0, sizeof (buffer)); // fill the buffer with 0
        strcpy (buffer + 13, to);
        {
            bool quotation = false;
            for (int i = 13; buffer [i]; i++) {  
                switch (buffer [i]) {
                    case '\"':  quotation = !quotation; 
                                break;
                    case '@':   if (!quotation) { // we found @ in email address
                                    int j; for (j = i - 1; j > 0 && buffer [j] > '\"' && buffer [j] != ','; j--); // go back to the beginning of email address
                                    if (j == 0) j--;
                                    int k; for (k = i + 1; buffer [k] > '\"' && buffer [k] != ',' && buffer [k] != ';'; k++); buffer [k] = 0; // go forward to the end of email address
                                    i = k;
                                    strcpy (buffer, "RCPT TO:"); strcat (buffer, buffer + j + 1); strcat (buffer, (char *) "\r\n");

                                    // send the buffer now
                                    switch (smtpClient.sendString (buffer)) {
                                        case -1:  return strerror (errno);
                                        case 0:   (const char *) "Connection closed by peer";
                                        default:  break;
                                    }

                                    // get the reply from SMTP server
                                    switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
                                        case -1:                return strerror (errno);
                                        case 0:                 return (const char *) "Connection closed by peer";
                                        case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
                                        default:                break; // continue
                                    }
                                    // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
                                    if (strstr (buffer, "250") != buffer) // 250 2.1.0 Ok
                                        return buffer;

                                }
                    default:    break;
                }
            }
        }

        // 12. send DATA command to SMTP server
        switch (smtpClient.sendString ((char *) "DATA\r\n")) {
            case -1:  return strerror (errno);
            case 0:   (const char *) "Connection closed by peer";
            default:  break;
        }

        // 13. get the reply from SMTP server
        switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
            case -1:                return strerror (errno);
            case 0:                 return (const char *) "Connection closed by peer";
            case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
            default:                break; // continue
        }
        // DEBUG: Serial.printf ("[smtpClient] <- %s", buffer);
        if (strstr (buffer, "354") != buffer) // 354 End data with <CR><LF>.<CR><LF>
            return buffer;

        // 14. send DATA content to SMTP server
        {
            String s ("");
            int errCount = 0;
                  errCount += !s.concat ("From:");
                  errCount += !s.concat (from);
                  errCount += !s.concat ("\r\nTo:");
                  errCount += !s.concat (to);
                  errCount += !s.concat ("\r\n");

            #ifdef __TIME_FUNCTIONS__
                time_t now = time (NULL);
                if (now) { // if we know the time than add this information also
                    struct tm structNow;
                    gmtime_r (&now, &structNow);
                    char stringNow [128];
                    strftime (stringNow, sizeof (stringNow), "%a, %d %b %Y %H:%M:%S %Z", &structNow);
                        errCount += !s.concat ("Date:");
                        errCount += !s.concat (stringNow);
                        errCount += !s.concat ("\r\n");
                }
            #endif

                  errCount += !s.concat ("Subject:");
                  errCount += !s.concat (subject);
                  errCount += !s.concat ("\r\nContent-Type: text/html; charset=\"utf8\"\r\n" // add this directive to enable HTML content of message
                                         "\r\n");
                  errCount += !s.concat (message);
                  errCount += !s.concat ("\r\n.\r\n"); // end-of-message mark
            if (errCount)
                return "Out of memory";

            switch (smtpClient.sendString ((char *) s.c_str ())) {
                case -1:  return strerror (errno);
                case 0:   (const char *) "Connection closed by peer";
                default:  break;
            }

            // 15. get the reply from SMTP server which also indicates the success of the whole procedure
            switch (smtpClient.recvString (buffer, SMTP_BUFFER_SIZE, "\n")) {
                case -1:                return strerror (errno);
                case 0:                 return (const char *) "Connection closed by peer";
                case SMTP_BUFFER_SIZE:  return (const char *) "Buffer too small";
                default:                break; // continue
            }
            // DEBUG:  Serial.printf ("[smtpClient] <- %s", buffer);

            return buffer; // whatever the SMTP server sent
        }
    }


    // sends message, returns error or success text, fills empty parameters with the ones from configuration file /etc/mail/sendmail.cf
    cstring sendMail (const char *message = "", const char *subject = "", const char *to = "", const char *from = "", const char *password = "", const char *userName = "", int smtpPort = 0, const char *smtpServer = "") {

        #ifdef __FILE_SYSTEM__
            if (fileSystem.mounted ()) { 
                char buffer [MAX_ETC_MAIL_SENDMAIL_CF + 1];
                strcpy (buffer, "\n");
                if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 2, "/etc/mail/sendmail.cf")) {
                    strcat (buffer, "\n");
                    char *p = NULL;
                    char *q;
                    if (!*smtpServer) if ((smtpServer = stristr (buffer, "\nsmtpServer ")))  { smtpServer += 12; }
                    if (!smtpPort)    if ((p = stristr (buffer, "\nsmtpPort ")))             { p += 10; }
                    if (!*userName)   if ((userName = stristr (buffer, "\nuserName ")))      { userName += 10; }
                    if (!*password)   if ((password = stristr (buffer, "\npassword ")))      { password += 10; }
                    if (!*from)       if ((from = stristr (buffer, "\nfrom ")))              { from += 6; }
                    if (!*to)         if ((to = stristr (buffer, "\nto ")))                  { to += 4; }          
                    if (!*subject)    if ((subject = stristr (buffer, "\nsubject ")))        { subject += 9; }
                    if (!*message)    if ((message = stristr (buffer, "\nmessage ")))        { message += 9; }
                    for (q = buffer; *q; q++)
                        if (*q < ' ') *q = 0;
                    if (p) smtpPort = atoi (p);
                }
            } else {
                #ifdef __DMESG__
                    dmesgQueue << F ("[smtpCient] file system not mounted, can't read /etc/mail/sendmail.cf");
                #endif
            }
        #endif

        if (!*to || !*from || !*password || !*userName || !smtpPort || !*smtpServer) {
            #ifdef __DMESG__
                dmesgQueue << (const char *) "[smtpClient] not all the arguments are set in /etc/mail/sendmail.cf";
            #endif
            return (const char *) "[smtpClient] not all the arguments are set in /etc/mail/sendmail.cf";
        } else {
            return __sendMail__ (message, subject, to, from, password, userName, smtpPort, smtpServer); 
        }
    }


#endif
