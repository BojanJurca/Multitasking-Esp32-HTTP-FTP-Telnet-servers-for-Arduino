/*

    ftpClient.hpp 
 
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    June 25, 2023, Bojan Jurca

*/


    // ----- includes, definitions and supporting functions -----

    #include <WiFi.h>
    // fixed size strings
    #include "fsstring.h"


#ifndef __FTP_CLIENT__
    #define __FTP_CLIENT__

    #ifndef __FILE_SYSTEM__
        #error "You can't use ftpClient.h without file_system.h. Either #include file_system.h prior to including ftpClient.h or exclude ftpClient.h"
    #endif


    // ----- functions and variables in this modul -----

    string ftpPut (char *, char *, char *, char *, int, char *);
    string ftpGet (char *, char *, char *, char *, int, char *);

    
    // TUNNING PARAMETERS

    #define FTP_CLIENT_TIME_OUT 3000              // 3000 ms = 3 sec 
    #define FTP_CMD_BUFFER_SIZE 300               // sending FTP commands and readinf replies from the server
    // ftpClientBuffer will be used first to read configuration file /etc/mail/sendmail.cf file in the memory and then for data transmission, fro both purposes 1 KB seems OK
    #define FTP_CLIENT_BUFFER_SIZE 1024           // MTU = 1500 (maximum transmission unit), TCP_SND_BUF = 5744 (a maximum block size that ESP32 can send), FAT cluster size = n * 512. 1024 seems a good trade-off


    // ----- CODE -----

    #include "dmesg_functions.h"

       
    // sends or receives a file via FTP, ftpCommand is eighter "PUT" or "GET", returns error or success text from FTP server, uses ftpClientBuffer that should be allocated in advance
    string __ftpClient__ (char *ftpCommand, char *clientFile, char *serverFile, char *password, char *userName, int ftpPort, char *ftpServer, char *ftpClientBuffer) {
      // get server address
      struct hostent *he = gethostbyname (ftpServer);
      if (!he) {
        return string ("gethostbyname error: ") + string (h_errno) + (char *) " " + hstrerror (h_errno);
      }
      // create socket
      int controlConnectionSocket = socket (PF_INET, SOCK_STREAM, 0);
      if (controlConnectionSocket == -1) {
        return string ("socket error: ") + string (errno) + (char *) " " + strerror (errno);
      }
      // make the socket not-blocking so that time-out can be detected
      if (fcntl (controlConnectionSocket, F_SETFL, O_NONBLOCK) == -1) {
        string e = string (errno) + (char *) " " + strerror (errno);
        close (controlConnectionSocket);
        return string ("fcntl error: ") + e;
      }
      // connect to server
      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_port = htons (ftpPort);
      serverAddress.sin_addr.s_addr = *(in_addr_t *) he->h_addr; 
      if (connect (controlConnectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
        if (errno != EINPROGRESS) {
          string e = string (errno) + (char *) " " + strerror (errno); 
          close (controlConnectionSocket);
          return string ("connect error: ") + e;
        }
      } // it is likely that socket is not connected yet at this point

      char buffer [FTP_CMD_BUFFER_SIZE];
      int dataConnectionSocket = -1;
     
      // read and process FTP server replies in an endless loop
      int receivedTotal = 0;
      do {
        receivedTotal = recvAll (controlConnectionSocket, buffer + receivedTotal, sizeof (buffer) - 1 - receivedTotal, (char *) "\n", FTP_CLIENT_TIME_OUT);
        if (receivedTotal <= 0) {
          string e = string (errno) + (char *) " " + strerror (errno);
          close (controlConnectionSocket);
          return string ("recv error: ") + e;
        }
        // DEBUG: Serial.printf ("ftpServerReply = |%s|\n", buffer);
        char *endOfCommand = strstr (buffer, "\n");
        while (endOfCommand) {
          *endOfCommand = 0;
          // process command in the buffer
          {
            // DEBUG: Serial.printf ("ftpServerReply = |%s|\n", buffer);
            #define ftpReplyIs(X) (strstr (buffer, X) == buffer)

                  if (ftpReplyIs ("220 "))  { // server wants client to log in
                                              string s = string ("USER ") + userName + (char *) "\r\n";
                                              if (sendAll (controlConnectionSocket, s, FTP_CLIENT_TIME_OUT) == -1) {
                                                string e = string (errno) + (char *) " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return string ("send error: ") + e;
                                              }
                                            }
            else if (ftpReplyIs ("331 "))   { // server wants client to send password
                                              string s = string ("PASS ") + password + (char *) "\r\n";
                                              if (sendAll (controlConnectionSocket, s, FTP_CLIENT_TIME_OUT) == -1) {
                                                string e = string (errno) + (char *) " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return string ("send error: ") + e;
                                              }
                                            }
            else if (ftpReplyIs ("230 "))   { // server said that we have logged in, initiate pasive data connection
                                              if (sendAll (controlConnectionSocket, "PASV\r\n", FTP_CLIENT_TIME_OUT) == -1) {
                                                string e = string (errno) + (char *) " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return string ("send error: ") + e;
                                              }
                                            }
            else if (ftpReplyIs ("227 "))   { // server send connection information like 227 entering passive mode (10,18,1,66,4,1)
                                              // open data connection
                                              int ip1, ip2, ip3, ip4, p1, p2; // get FTP server IP and port
                                              if (6 != sscanf (buffer, "%*[^(](%i,%i,%i,%i,%i,%i)", &ip1, &ip2, &ip3, &ip4, &p1, &p2)) { // should always succeed
                                                close (controlConnectionSocket);
                                                return buffer; 
                                              }
                                              char pasiveDataIP [46]; sprintf (pasiveDataIP, "%i.%i.%i.%i", ip1, ip2, ip3, ip4);
                                              int pasiveDataPort = p1 << 8 | p2; 
                                              // DEBUG: Serial.printf ("Connecting to: %s:%i\n", pasiveDataIP, pasiveDataPort);
                                              // establish data connection now
                                              /* // we already heve client's IP so we don't have to reslove its name first
                                              struct hostent *he = gethostbyname (pasiveDataIP);
                                              if (!he) {
                                                dmesg ("[httpClient] gethostbyname() error: ", h_errno);
                                                return F ("425 can't open active data connection\r\n");
                                              }
                                              */
                                              // create socket
                                              dataConnectionSocket = socket (PF_INET, SOCK_STREAM, 0);
                                              if (dataConnectionSocket == -1) {
                                                string e = string (errno) + (char *) " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return string ("socket error: ") + e; 
                                              }
                                              // make the socket not-blocking so that time-out can be detected
                                              if (fcntl (dataConnectionSocket, F_SETFL, O_NONBLOCK) == -1) {
                                                string e = string (errno) + (char *) " " + strerror (errno);
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return string ("fcntl error: ") + e; 
                                              }
                                              // connect to client that acts as a data server 
                                              struct sockaddr_in serverAddress;
                                              serverAddress.sin_family = AF_INET;
                                              serverAddress.sin_port = htons (pasiveDataPort);
                                              serverAddress.sin_addr.s_addr = inet_addr (pasiveDataIP); // serverAddress.sin_addr.s_addr = *(in_addr_t *) he->h_addr; 
                                              /*
                                              int retVal;
                                              unsigned long startMillis = millis ();
                                              do {
                                                retVal = connect (dataConnectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress));
                                                if (retVal != -1) break; // success
                                                if (errno == EINPROGRESS && millis () - startMillis < FTP_CLIENT_TIME_OUT) delay (1); // continue waiting
                                                else {
                                                  string e (errno);
                                                  close (dataConnectionSocket);
                                                  close (controlConnectionSocket);
                                                  return ("connect() error: " + e);                                                 
                                                }
                                              } while (true);
                                              */
                                              if (connect (dataConnectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
                                                if (errno != EINPROGRESS) {
                                                  string e = string (errno) + (char *) " " + strerror (errno);
                                                  close (dataConnectionSocket);
                                                  close (controlConnectionSocket);
                                                  return string ("connect error: ") + e;
                                                } 
                                              }
                                              // it is likely that socket is not connected yet at this point (the socket is non-blocking)
                                              // are we GETting or PUTting the file?
                                              string s;
                                                      if (!strcmp (ftpCommand, "GET")) {
                                                        s = string ("RETR ") + serverFile + (char *) "\r\n";
                                              } else if (!strcmp (ftpCommand, "PUT")) {
                                                        s = string ("STOR ") + serverFile + (char *) "\r\n";
                                              } else  {
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return string ("Unknown FTP command ") + ftpCommand; 
                                              }
                                              if (sendAll (controlConnectionSocket, s, FTP_CLIENT_TIME_OUT) == -1) {
                                                string e = string (errno) + (char *) " " + strerror (errno);
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return string ("send error: ") + e;
                                              }
                                            }

            else if (ftpReplyIs ("150 "))   { // server wants to start data transfer
                                              // are we GETting or PUTting the file?
                                                      if (!strcmp (ftpCommand, "GET")) {
                                                        
                                                          int bytesRecvTotal = 0; int bytesWrittenTotal = 0;
                                                          File f = fileSystem.open (clientFile, "w", true);
                                                          if (f) {
                                                            // read data from data connection and store it to the file
                                                            do {
                                                              int bytesRecvThisTime = recvAll (dataConnectionSocket, ftpClientBuffer, FTP_CLIENT_BUFFER_SIZE, NULL, FTP_CLIENT_TIME_OUT);
                                                              if (bytesRecvThisTime < 0)  { bytesRecvTotal = -1; break; } // to detect error later
                                                              if (bytesRecvThisTime == 0) { break; } // finished, success
                                                              bytesRecvTotal += bytesRecvThisTime;
                                                              int bytesWrittenThisTime = f.write ((uint8_t *) ftpClientBuffer, bytesRecvThisTime);
                                                              bytesWrittenTotal += bytesWrittenThisTime;
                                                              if (bytesWrittenThisTime != bytesRecvThisTime) { bytesRecvTotal = -1; break; } // to detect error later
                                                            } while (true);
                                                            f.close ();
                                                          } else {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return string ("Can't open ") + clientFile + (char *) " for writting";
                                                          }
                                                          if (bytesWrittenTotal != bytesRecvTotal) {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return string ("Can't write ") + clientFile;
                                                          }
                                                          close (dataConnectionSocket);

                                                          diskTrafficInformation.bytesWritten += bytesWrittenTotal; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way

                                              } else if (!strcmp (ftpCommand, "PUT")) {
                                                          int bytesReadTotal = 0; int bytesSentTotal = 0;
                                                          File f = fileSystem.open (clientFile, "r", false);
                                                          if (f) {
                                                            // read data from file and transfer it through data connection
                                                            do {
                                                              int bytesReadThisTime = f.read ((uint8_t *) ftpClientBuffer, FTP_CLIENT_BUFFER_SIZE);
                                                              if (bytesReadThisTime == 0) { break; } // finished, success
                                                              bytesReadTotal += bytesReadThisTime;
                                                              int bytesSentThisTime = sendAll (dataConnectionSocket, ftpClientBuffer, bytesReadThisTime, FTP_CLIENT_TIME_OUT);
                                                              if (bytesSentThisTime != bytesReadThisTime) { bytesSentTotal = -1; break; } // to detect error later
                                                              bytesSentTotal += bytesSentThisTime;
                                                            } while (true);
                                                            f.close ();
                                                          } else {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return string ("Can't open ") + clientFile + (char *) " for reading";
                                                          }
                                                          if (bytesSentTotal != bytesReadTotal) {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return string ("Can't send ") + clientFile;                                                            
                                                          }
                                                          close (dataConnectionSocket);

                                                          diskTrafficInformation.bytesRead += bytesReadTotal; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                                                       
                                              } else  {
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return string ("Unknown FTP command ") + ftpCommand; 
                                              }
                                            }
            else                            {
                                              if (buffer [3] == ' ')  {
                                                                        close (controlConnectionSocket);
                                                                        return buffer;
                                                                      }
                                              // DEBUG: Serial.printf ("Unknown FTP server reply: %s\n", buffer);
                                            }
          }
          // is there more commands in the buffer?
          strcpy (buffer, endOfCommand + 1);
          endOfCommand = strstr (buffer, "\n");
        }
        receivedTotal = strlen (buffer);
      } while (controlConnectionSocket > -1); // while the connection is still opened
      return "";
  }

  // sends file, returns error or success text, fills empty parameters with the ones from configuration file /etc/ftp/ftpclient.cf
  string ftpPut (char *localFileName = (char *) "", char *remoteFileName = (char *) "", char *password = (char *) "", char *userName = (char *) "", int ftpPort = 0, char *ftpServer = (char *) "") {
    char ftpClientBuffer [FTP_CLIENT_BUFFER_SIZE]; // ftpClientBuffer will be used first to read configuration file /etc/mail/sendmail.cf file in the memory and then for data transmission, fro both purposes 1 KB seems OK
    // DEBUG: Serial.printf ("entering: ftpPut (%s, %s, %s, %s, %i, %s)\n", localFileName, remoteFileName, password, userName, ftpPort, ftpServer);
    if (fileSystem.mounted ()) { 
      strcpy (ftpClientBuffer, "\n");
      if (fileSystem.readConfigurationFile (ftpClientBuffer + 1, sizeof (ftpClientBuffer) - 2, (char *) "/etc/ftp/ftpclient.cf")) {
        strcat (ftpClientBuffer, "\n");
        char *p = NULL;
        char *q;
        if (!*ftpServer) if ((ftpServer = strstr (ftpClientBuffer, "\nftpServer "))) ftpServer += 11;
        if (!ftpPort)    if ((p = strstr (ftpClientBuffer, "\nftpPort ")))           p += 9;
        if (!*userName)  if ((userName = strstr (ftpClientBuffer, "\nuserName ")))   userName += 10;
        if (!*password)  if ((password = strstr (ftpClientBuffer, "\npassword ")))   password += 10;

        for (q = ftpClientBuffer; *q; q++)
          if (*q < ' ') *q = 0;
        if (p) ftpPort = atoi (p);
      }
    } else {
        dmesg ("[ftpClient] file system not mounted, can't read /etc/ftp/ftpclient.cf");
    }
    // DEBUG: Serial.printf ("defaults filled in\n"); Serial.printf ("with defaults: ftpPut (%s, %s, %s, %s, %i, %s)\n", localFileName, remoteFileName, password, userName, ftpPort, ftpServer);
    if (!*password || !*userName || !ftpPort || !*ftpServer) {
      dmesg ("[ftpClient] not all the arguments are set, if you want to use the default values, write them to /etc/ftp/ftpclient.cf");
      return "Not all the arguments are set, if you want to use ftpPut default values, write them to /etc/ftp/ftpclient.cf";
    } else {
      return __ftpClient__ ((char *) "PUT", localFileName, remoteFileName, password, userName, ftpPort, ftpServer, ftpClientBuffer);
    }
  }

  // retrieves file, returns error or success text, fills empty parameters with the ones from configuration file /etc/ftp/ftpclient.cf
  string ftpGet (char *localFileName = (char *) "", char *remoteFileName = (char *) "", char *password = (char *) "", char *userName = (char *) "", int ftpPort = 0, char *ftpServer = (char *) "") {
    char ftpClientBuffer [FTP_CLIENT_BUFFER_SIZE]; // ftpClientBuffer will be used first to read configuration file /etc/mail/sendmail.cf file in the memory and then for data transmission, fro both purposes 1 KB seems OK
    if (fileSystem.mounted ()) {
      strcpy (ftpClientBuffer, "\n");
      if (fileSystem.readConfigurationFile (ftpClientBuffer + 1, sizeof (ftpClientBuffer) - 2, (char *) "/etc/ftp/ftpclient.cf")) {
        strcat (ftpClientBuffer, "\n");
        char *p = NULL;
        char *q;
        if (!*ftpServer) if ((ftpServer = strstr (ftpClientBuffer, "\nftpServer "))) ftpServer += 11;
        if (!ftpPort)    if ((p = strstr (ftpClientBuffer, "\nftpPort ")))           p += 9;
        if (!*userName)  if ((userName = strstr (ftpClientBuffer, "\nuserName ")));  userName += 10;
        if (!*password)  if ((password = strstr (ftpClientBuffer, "\npassword ")));  password += 10;

        if ((q = strstr (ftpServer, "\n"))) *q = 0;
        if (p && (q = strstr (p, "\n"))) { *q = 0; ftpPort = atoi (p); }
        if ((q = strstr (userName, "\n"))) *q = 0;
        if ((q = strstr (password, "\n"))) *q = 0;
      }
    } else {
        dmesg ("[ftpClient] file system not mounted, can't read /etc/ftp/ftpclient.cf");
    }
    if (!*password || !*userName || !ftpPort || !*ftpServer) {
      dmesg ("[ftpClient] not all the arguments are set, if you want to use the default values, write them to /etc/ftp/ftpclient.cf");
      return "Not all the arguments are set, if you want to use ftpPut default values, write them to /etc/ftp/ftpclient.cf";
    } else {
      return __ftpClient__ ((char *) "GET", localFileName, remoteFileName, password, userName, ftpPort, ftpServer, ftpClientBuffer);
    }
  }

#endif
