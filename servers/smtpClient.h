/*

    smtpClient.hpp 
 
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    November 26, 2023, Bojan Jurca

*/


    // ----- includes, definitions and supporting functions -----

    #include <WiFi.h>
    // base64 encoding
    #include <mbedtls/base64.h>
    // fixed size strings
    #include "fsString.h"


#ifndef __SMTP_CLIENT__
    #define __SMTP_CLIENT__

    // ----- functions and variables in this modul -----

    string sendMail (const char *, const char *, const char *, const char *, const char *, const char *, int, const char *);
    
    
    // TUNNING PARAMETERS

    #define SMTP_TIME_OUT 1500                    // 1500 ms = 1,5 sec 
    #define SMTP_BUFFER_SIZE 256                  // constructing SMTP commands and reading SMTP reply
    #ifndef HOSTNAME
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "HOSTNAME was not defined previously, #defining the default MyESP32Server in smtpClient.h"
        #endif
        #define "MyESP32Server"                     // use default if not defined previously
    #endif
    #define MAX_ETC_MAIL_SENDMAIL_CF 1 * 1024     // 1 KB will usually do - sendmail reads the whole /etc/mail/sendmail.cf file in the memory 


    // ----- CODE -----

    #include "dmesg_functions.h"

       
    // sends message, returns error or success text (from SMTP server)
    string __sendMail__ (const char *message, const char *subject, const char *to, const char *from, const char *password, const char *userName, int smtpPort, const char *smtpServer) {
        // DEBUG: Serial.printf ("[__sendMail__] ()\nsmtpServer=%s\nsmtpPort=%i\nusername=%s\npassword=%s\nfrom=%s\nto=%s\nsubject=%s\nmessage=%s\n", smtpServer, smtpPort, userName, password, from, to, subject, message);

      // get server address
      struct hostent *he = gethostbyname (smtpServer);
      if (!he) {
        dmesg ("[smtpClient] gethostbyname() error: ", h_errno, hstrerror (h_errno));
        return "";
      }
      // create socket
      int connectionSocket = socket (PF_INET, SOCK_STREAM, 0);
      if (connectionSocket == -1) {
        dmesg ("[smtpClient] socket() error: ", errno, strerror (errno));
        return "";
      }
      // make the socket not-blocking so that time-out can be detected
      if (fcntl (connectionSocket, F_SETFL, O_NONBLOCK) == -1) {
        dmesg ("[smtpClient] fcntl() error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // connect to server
      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_port = htons (smtpPort);
      serverAddress.sin_addr.s_addr = *(in_addr_t *) he->h_addr; 
      if (connect (connectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
        if (errno != EINPROGRESS) {
          dmesg ("[smtpClient] connect() error: ", errno, strerror (errno)); 
          close (connectionSocket);
          return "";
        }
      } // it is likely that socket is not connected yet at this point

      char buffer [SMTP_BUFFER_SIZE];
      
      // read welcome message from SMTP server
      if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) {
        dmesg ("[smtpClient] read error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);

      if (strstr (buffer, "220") != buffer) goto closeSmtpConnection; // 220 mail.siol.net ESMTP server ready

      // send EHLO to SMTP server and read its reply
      if (sizeof (buffer) < 5 + strlen (HOSTNAME) + 3) {
        dmesg ("[sendMail] buffer too small");
        close (connectionSocket);
        return "";
      }
      sprintf (buffer, "EHLO %s\r\n", (char *) HOSTNAME);
      if (sendAll (connectionSocket, buffer, SMTP_TIME_OUT) <= 0) {
        dmesg ("[smtpClient] send error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";        
      }
      // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", buffer);
      if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
        dmesg ("[smtpClient] read error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
      if (strstr (buffer, "250") != buffer) goto closeSmtpConnection; // 250-mail.siol.net ...

      // send login request to SMTP server and read its reply
      if (sendAll (connectionSocket, "AUTH LOGIN\r\n", SMTP_TIME_OUT) <= 0) {
        dmesg ("[smtpClient] send error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";        
      }
      // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", "AUTH LOGIN\r\n");
      if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
        dmesg ("[smtpClient] read error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
      if (strstr (buffer, "334") != buffer) goto closeSmtpConnection; // 334 VXNlcm5hbWU6 (= base64 encoded)
  
      // send base64 encoded user name to SMTP server and read its reply
      size_t encodedLen;
      mbedtls_base64_encode ((unsigned char *) buffer, sizeof (buffer) - 3, &encodedLen, (const unsigned char *) userName, strlen (userName));
      strcpy (buffer + encodedLen, "\r\n");
      if (sendAll (connectionSocket, buffer, SMTP_TIME_OUT) <= 0) {
        dmesg ("[smtpClient] send error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";        
      }
      // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", buffer);      
      if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
        dmesg ("[smtpClient] read error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
      if (strstr (buffer, "334") != buffer) goto closeSmtpConnection; // 334 UGFzc3dvcmQ6 (= base64 encoded Password:)

      // send base64 encoded password to SMTP server and read its reply
      mbedtls_base64_encode ((unsigned char *) buffer, sizeof (buffer) - 3, &encodedLen, (const unsigned char *) password, strlen (password));
      strcpy (buffer + encodedLen, "\r\n");
      if (sendAll (connectionSocket, buffer, SMTP_TIME_OUT) <= 0) {
        dmesg ("[smtpClient] send error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";        
      }
      // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", buffer);      
      if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
        dmesg ("[smtpClient] read error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
      if (strstr (buffer, "235") != buffer) goto closeSmtpConnection; // 235 2.7.0 Authentication successful 

      // send MAIL FROM to SMTP server and read its reply, there should be only one address in from string (there is onlyone sender) - parse it against @ chracter 
      if (sizeof (buffer) < 13 + strlen (from) + 3) {
        dmesg ("[sendMail] buffer too small");
        close (connectionSocket);
        return "";
      }
      strcpy (buffer + 13, from);
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
                          // send buffer now
                          if (sendAll (connectionSocket, buffer, SMTP_TIME_OUT) <= 0) {
                            dmesg ("[smtpClient] send error: ", errno, strerror (errno));
                            close (connectionSocket);
                            return "";        
                          }
                          // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", buffer);                          
                          if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
                            dmesg ("[smtpClient] read error: ", errno, strerror (errno));
                            close (connectionSocket);
                            return "";
                          }
                          // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
                          if (strstr (buffer, "250") != buffer) goto closeSmtpConnection; // 250 2.1.0 Ok
                        }
            default:    break;
          }
        }
      }

      // send RCPT TO for each address in to-list, to SMTP server and read its reply - parse it against @ chracter 
      if (sizeof (buffer) < 13 + strlen (to) + 1 + 1) { // there are going to be only one 'to' adress at a time, we can move this checking down the code to be more effective if needed
        dmesg ("[sendMail] buffer too small");
        close (connectionSocket);
        return "";
      }
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
                          // send buffer now
                          if (sendAll (connectionSocket, buffer, SMTP_TIME_OUT) <= 0) {
                            dmesg ("[smtpClient] send error: ", errno, strerror (errno));
                            close (connectionSocket);
                            return "";        
                          }
                          // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", buffer);                          
                          if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
                            dmesg ("[smtpClient] read error: ", errno, strerror (errno));
                            close (connectionSocket);
                            return "";
                          }
                          // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
                          if (strstr (buffer, "250") != buffer) goto closeSmtpConnection; // 250 2.1.5 Ok
                        }
            default:    break;
          }
        }
      }

      // send DATA command to SMTP server and read its reply
      if (sendAll (connectionSocket, "DATA\r\n", SMTP_TIME_OUT) <= 0) {
        dmesg ("[smtpClient] send error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";        
      }
      // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n","DATA\r\n");      
      if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
        dmesg ("[smtpClient] read error: ", errno, strerror (errno));
        close (connectionSocket);
        return "";
      }
      // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
      if (strstr (buffer, "354") != buffer) goto closeSmtpConnection; // 354 End data with <CR><LF>.<CR><LF>

      // send DATA content to SMTP server and read its reply
      {
        String s = "From:";
               s += from;
               s += "\r\nTo:";
               s += to;
               s += "\r\n";
        #ifdef __TIME_FUNCTIONS__
          time_t now = time (NULL);
          if (now) { // if we know the time than add this information also
            struct tm structNow;
            gmtime_r (&now, &structNow);
            char stringNow [128];
            strftime (stringNow, sizeof (stringNow), "%a, %d %b %Y %H:%M:%S %Z", &structNow);
                s += "Date:";
                s += stringNow;
                s += "\r\n";
          }
        #endif
                s += "Subject:";
                s += subject;
                s += "\r\nContent-Type: text/html; charset=\"utf8\"\r\n" // add this directive to enable HTML content of message
                     "\r\n";
                s += message;
                s += "\r\n.\r\n"; // end-of-message mark
        if (sendAll (connectionSocket, s.c_str (), SMTP_TIME_OUT) <= 0) {
          dmesg ("[smtpClient] send error: ", errno, strerror (errno));
          close (connectionSocket);
          return "";        
        }
        // DEBUG: Serial.printf ("[__sendMail__] =>\n%s\n", (char *) s);             
        if (recvAll (connectionSocket, buffer, SMTP_BUFFER_SIZE, (char *) "\n", SMTP_TIME_OUT) <= 0) { 
          dmesg ("[smtpClient] read error: ", errno, strerror (errno));
          close (connectionSocket);
          return "";
        }
        // DEBUG: Serial.printf ("[__sendMail__] <=\n%s\n", buffer);
        if (strstr (buffer, "250") != buffer) goto closeSmtpConnection; // 250 2.0.0 Ok: queued as 5B05E52A278
      }

  closeSmtpConnection:
      close (connectionSocket);
      return buffer;
  }

  // sends message, returns error or success text, fills empty parameters with the ones from configuration file /etc/mail/sendmail.cf
  string sendMail (const char *message = "", const char *subject = "", const char *to = "", const char *from = "", const char *password = "", const char *userName = "", int smtpPort = 0, const char *smtpServer = "") {
      // DEBUG: Serial.printf ("sendMail (call):\nsmtpServer=%s\nsmtpPort=%i\nusername=%s\npassword=%s\nfrom=%s\nto=%s\nsubject=%s\nmessage=%s\n", smtpServer, smtpPort, userName, password, from, to, subject, message);

    #ifdef __FILE_SYSTEM__
      if (fileSystem.mounted ()) { 
        char buffer [MAX_ETC_MAIL_SENDMAIL_CF + 1];
        strcpy (buffer, "\n");
        if (fileSystem.readConfigurationFile (buffer + 1, sizeof (buffer) - 2, (char *) "/etc/mail/sendmail.cf")) {
          strcat (buffer, "\n");
          // DEBUG: Serial.printf ("/etc/mail/sendmail.cf:%s", buffer);
          char *p = NULL;
          char *q;
          if (!*smtpServer) if ((smtpServer = strstr (buffer, "\nsmtpServer ")))  { smtpServer += 12; 
                                                                                    // DEBUG: Serial.printf ("smtpServer from .cf=%s\n", smtpServer); delay (1000);
                                                                                  }
          if (!smtpPort)    if ((p = strstr (buffer, "\nsmtpPort ")))             { p += 10;
                                                                                    // DEBUG: Serial.printf ("smtpPort from .cf=%s\n", p); delay (1000);
                                                                                  }
          if (!*userName)   if ((userName = strstr (buffer, "\nuserName ")))      { userName += 10;
                                                                                    // DEBUG: Serial.printf ("userName from .cf=%s\n", userName); delay (1000);
                                                                                  }
          if (!*password)   if ((password = strstr (buffer, "\npassword ")))      { password += 10;
                                                                                    // DEBUG: Serial.printf ("password from .cf=%s\n", password); delay (1000);
                                                                                  }
          if (!*from)       if ((from = strstr (buffer, "\nfrom ")))              { from += 6;
                                                                                    // DEBUG: Serial.printf ("from from .cf=%s\n", from); delay (1000);
                                                                                  }
          if (!*to)         if ((to = strstr (buffer, "\nto ")))                  { to += 4;
                                                                                    // DEBUG: Serial.printf ("to from .cf=%s\n", to); delay (1000);
                                                                                  }          
          if (!*subject)    if ((subject = strstr (buffer, "\nsubject ")))        { subject += 9;
                                                                                    // DEBUG: Serial.printf ("subject from .cf=%s\n", subject); delay (1000);
                                                                                  }
          if (!*message)    if ((message = strstr (buffer, "\nmessage ")))        { message += 9;
                                                                                    // DEBUG: Serial.printf ("message from .cf=%s\n", message); delay (1000);
                                                                                  }
          for (q = buffer; *q; q++)
            if (*q < ' ') *q = 0;
          if (p) smtpPort = atoi (p);
        }
      } else {
          dmesg ("[smtpCient] file system not mounted, can't read /etc/mail/sendmail.cf");
      }

      // DEBUG: Serial.printf ("sendMail (filled missing information):\nsmtpServer=%s\nsmtpPort=%i\nusername=%s\npassword=%s\nfrom=%s\nto=%s\nsubject=%s\nmessage=%s\n", smtpServer, smtpPort, userName, password, from, to, subject, message);
    #endif

    if (!*to || !*from || !*password || !*userName || !smtpPort || !*smtpServer) {
      dmesg ("[smtpClient] not all the arguments are set, if you want to use the default values, write them to /etc/mail/sendmail.cf");
      return "Not all the arguments are set, if you want to use sendMail default values, write them to /etc/mail/sendmail.cf";
    } else {
      return __sendMail__ (message, subject, to, from, password, userName, smtpPort, smtpServer);    
    }
  }
  
#endif
