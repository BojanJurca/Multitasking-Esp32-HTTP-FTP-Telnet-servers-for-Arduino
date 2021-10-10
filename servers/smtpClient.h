/*

    smtpClient.hpp 
 
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    smtpClient is built upon non-threadet TCP client.

    August, 12, 2021, Bojan Jurca

 */


#ifndef __SMTP_CLIENT__
  #define __SMTP_CLIENT__

  #ifndef HOSTNAME
    #define HOSTNAME "MyESP32Server" // use default if not defined
  #endif

  #include <WiFi.h>

  #include "TcpServer.hpp"  
  #include "mbedtls/base64.h" // needed to base64 encode username and password for SMTP server
  

  String __smtpReply__ (TcpConnection *smtpConnection) {
    char buffer [256]; *buffer = 0;                                                            
    int receivedTotal = 0;
    int received;
    while ((received = smtpConnection->recvData (buffer + receivedTotal, sizeof (buffer) - receivedTotal - 1))) {
      buffer [receivedTotal += received] = 0; // mark the end of the string we have just read
      if (strstr (buffer, "\n")) break;  // read until the end of response
    }
    if (!received) return "lost connection.";
    return String (buffer);
  }

    
  // sends message, returns error or success text
  String __sendMail__ (String message, String subject, String to, String from, String password, String userName, int smtpPort, String smtpServer) {
    String r;
  
    // 1. get SMTP server IP number
    IPAddress smtpServerIP;
    if (!WiFi.hostByName (smtpServer.c_str (), smtpServerIP)) return "Could not find host " + smtpServer;
    // debug: Serial.println ("[SMTP] getHostByName: " + smtpServerIP.toString ()); 
  
    // 2. create non-threaded TCP client instance and connect to SMTP server
    TcpClient *smtpClient = new TcpClient (smtpServerIP.toString (), smtpPort, (TIME_OUT_TYPE) 5000); 
    TcpConnection *smtpConnection = smtpClient ? smtpClient->connection () : NULL;
    if (!smtpConnection) return "Could not connect to " + smtpServer + " ("+ smtpServerIP.toString () + ") on port " + String (smtpPort) + ".";
    // debug: Serial.println ("[SMTP] connected."); 
  
    // 3. read welcome message from SMTP server
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "220") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
    
    // 4. send EHLO
    smtpConnection->sendData ("EHLO " + String (HOSTNAME) + "\r\n");
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "250") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
  
    // 5. send login request
    smtpConnection->sendData ((char *) "AUTH LOGIN\r\n");
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "334") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
  
    // 6. send base64 encoded user name and password
    unsigned char encoded [256];
    size_t encodedLen;
    mbedtls_base64_encode (encoded, sizeof (encoded) - 3, &encodedLen, (const unsigned char *) userName.c_str (), userName.length ());
    strcpy ((char *) encoded + encodedLen, "\r\n");
    smtpConnection->sendData ((char *) encoded);
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "334") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
    mbedtls_base64_encode (encoded, sizeof (encoded) - 3, &encodedLen, (const unsigned char *) password.c_str (), password.length ());
    strcpy ((char *) encoded + encodedLen, "\r\n");
    smtpConnection->sendData ((char *) encoded);
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "235") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
    // debug: Serial.println ("[SMTP] authenticated."); 
  
    // 7. MAIL FROM there should be only one address in from string - parse it according to @ chracter 
    bool quotation = false;
    for (int i = 0; i < from.length (); i++) {  
      switch (from.charAt (i)) {
        case '\"':  quotation = !quotation; 
                    break;
        case '@':   if (!quotation) { // we found @ in email address
                      int j; for (j = i - 1; j > 0 && from.charAt (j) > '\"' && from.charAt (j) != ','; j --); // go back to the beginning of email address
                      if (j == 0) j --;
                      int k; for (k = i + 1; k < from.length () && from.charAt (k) > '\"' && from.charAt (k) != ','; k ++); // go forward to the end of email address
                      
                        smtpConnection->sendData ("MAIL FROM:" + from.substring (j + 1, k) + "\r\n");
                        r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "250") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
                        // debug: Serial.println ("[SMTP] MAIL FROM:" + from.substring (j + 1, k)); 
                    }
        default:    break;
      }
    }
  
    // 8. RCPT TO for each address in to list - parse it according to @ chracter 
    quotation = false;
    for (int i = 0; i < to.length (); i++) {  
      switch (to.charAt (i)) {
        case '\"':  quotation = !quotation; 
                    break;
        case '@':   if (!quotation) { // we found @ in email address
                      int j; for (j = i - 1; j > 0 && to.charAt (j) > '\"' && to.charAt (j) != ','; j --); // go back to the beginning of email address
                      if (j == 0) j --;
                      int k; for (k = i + 1; k < to.length () && to.charAt (k) > '\"' && to.charAt (k) != ','; k ++); // go forward to the end of email address
                        smtpConnection->sendData ("RCPT TO:" + to.substring (j + 1, k) + "\r\n");
                        r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "250") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
                        // debug: Serial.println ("[SMTP] RCPT TO:" + to.substring (j + 1, k)); 
                    }
        default:    break;
      }
    }
  
    // 9. DATA
    smtpConnection->sendData ((char *) "DATA\r\n");
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "354") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
  
    String s = "From:" + from + "\r\n"
               "To:" +  to + "\r\n";
    time_t now = getGmt ();
    if (now) { // if we know the time than add this information also
      struct tm structNow = timeToStructTime (now);
      char stringNow [128];
      strftime (stringNow, sizeof (stringNow), "%a, %d %b %Y %H:%M:%S %Z", &structNow);
      s += "Date:" + String (stringNow) + "\r\n";
    }
    s += "Subject:" + subject + "\r\n"
         "Content-Type: text/html; charset=\"utf8\"\r\n" // add this directive to enable HTML content of message
          "\r\n" + 
          message + 
          "\r\n.\r\n"; // end-of-message mark
    smtpConnection->sendData (s);        
    r = __smtpReply__ (smtpConnection); if (r.substring (0, 3) != "250") { smtpConnection->closeConnection (); return "SMTP error: " + r; }
  
    // 10. QUIT
    smtpConnection->sendData ((char *) "QUIT\r\n");
  
    return r; // return (last) OK response from SMTP server
  }


  #include "file_system.h"        // smtpClient.h needs file_system.h to read /etc/mail/sendmail.cf configuration file from

  // sends message, returns error or success text, fills empty parameters with the ones from configuration file /etc/mail/sendmail.cf
  String sendMail (String message = "", String subject = "", String to = "", String from = "", String password = "", String userName = "", int smtpPort = 0, String smtpServer = "") {

    #ifdef __FILE_SYSTEM__
      if (__fileSystemMounted__) { 
        String fileContent = "";
        readFile (fileContent, "/etc/mail/sendmail.cf"); 
        if (fileContent != "") { 
          fileContent = compactConfigurationFileContent (fileContent) + "\n"; 
          // replace empty parameter values with the ones in configuration file
          if (smtpServer == "") smtpServer  = between (fileContent, "smtpServer ", "\n");
          if (smtpPort == 0)    smtpPort    = between (fileContent, "smtpPort ", "\n").toInt ();
          if (userName == "")   userName    = between (fileContent, "userName ", "\n");
          if (password == "")   password    = between (fileContent, "password ", "\n");
          if (from == "")       from        = between (fileContent, "from ", "\n");
          if (to == "")         to          = between (fileContent, "to ", "\n");
          if (subject == "")    subject     = between (fileContent, "subject ", "\n");
          if (message == "")    subject     = between (fileContent, "message ", "\n");
        }
      } else
    #endif
              {
                #ifdef dmesg
                  dmesg ("[SMTP client] file system not mounted, can't read /etc/mail/sendmail.cf");
                #endif
              }
    
    return __sendMail__ (message, subject, to, from, password, userName, smtpPort, smtpServer);    
  }
  
#endif
