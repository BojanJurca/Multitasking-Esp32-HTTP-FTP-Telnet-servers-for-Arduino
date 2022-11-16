/*

    ftpClient.hpp 
 
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    October, 23, 2022, Bojan Jurca

*/


    // ----- includes, definitions and supporting functions -----

    #include <WiFi.h>


#ifndef __FTP_CLIENT__
  #define __FTP_CLIENT__

  #ifndef __FILE_SYSTEM__
    #error "You can't use ftpClient.h without file_system.h. Either #include file_system.h prior to including ftpClient.h or exclude ftpClient.h"
  #endif
  #ifndef __PERFMON__
    #pragma message "Compiling ftpClient.h without performance monitors (perfMon.h)"
  #endif


    // ----- functions and variables in this modul -----

    String ftpPut (char *, char *, char *, char *, int, char *);
    String ftpGet (char *, char *, char *, char *, int, char *);

    
    // TUNNING PARAMETERS

    #define FTP_CLIENT_TIME_OUT 3000              // 3000 ms = 3 sec 
    #define FTP_CMD_BUFFER_SIZE 300               // sending FTP commands and readinf replies from the server
    #define FTP_CLIENT_BUFFER_SIZE TCP_SND_BUF    // for file transfer, 5744 bytes is maximum block that ESP32 can send
    #define MAX_ETC_FTP_FTPCLIENT_CF 1 * 1024     // 1 KB will usually do - sendmail reads the whole /etc/mail/sendmail.cf file in the memory 


    // ----- CODE -----

    #include "dmesg_functions.h"

       
    // sends or receives a file via FTP, ftpCommand is eighter "PUT" or "GET", returns error or success text from FTP server
    String __ftpClient__ (char *ftpCommand, char *clientFile, char *serverFile, char *password, char *userName, int ftpPort, char *ftpServer) {
      // get server address
      struct hostent *he = gethostbyname (ftpServer);
      if (!he) {
        return "gethostbyname() error: " + String (h_errno) + " " + hstrerror (h_errno);
      }
      // create socket
      int controlConnectionSocket = socket (PF_INET, SOCK_STREAM, 0);
      if (controlConnectionSocket == -1) {
        return "socket() error: " + String (errno) + " " + strerror (errno);
      }
      // make the socket not-blocking so that time-out can be detected
      if (fcntl (controlConnectionSocket, F_SETFL, O_NONBLOCK) == -1) {
        String e = String (errno) + " " + strerror (errno);
        close (controlConnectionSocket);
        return "fcntl() error: " + e;
      }
      // connect to server
      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_port = htons (ftpPort);
      serverAddress.sin_addr.s_addr = *(in_addr_t *) he->h_addr; 
      if (connect (controlConnectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
        if (errno != EINPROGRESS) {
          String e = String (errno) + " " + strerror (errno); 
          close (controlConnectionSocket);
          return "connect() error: " + e;
        }
      } // it is likely that socket is not connected yet at this point

      char buffer [FTP_CMD_BUFFER_SIZE]; 
      int dataConnectionSocket = -1;
     
      // read and process FTP server replies in an endless loop
      int receivedTotal = 0;
      do {
        receivedTotal = recvAll (controlConnectionSocket, buffer + receivedTotal, sizeof (buffer) - 1 - receivedTotal, (char *) "\n", FTP_CLIENT_TIME_OUT);
        if (receivedTotal <= 0) {
          String e = String (errno) + " " + strerror (errno);
          close (controlConnectionSocket);
          return "recv() error: " + e;
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
                                              String s = "USER " + String (userName) + "\r\n";
                                              if (sendAll (controlConnectionSocket, (char *) s.c_str (), s.length (), FTP_CLIENT_TIME_OUT) == -1) {
                                                String e = String (errno) + " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return "send() error: " + e;
                                              }
                                            }
            else if (ftpReplyIs ("331 "))   { // server wants client to send password
                                              String s = "PASS " + String (password) + "\r\n";
                                              if (sendAll (controlConnectionSocket, (char *) s.c_str (), s.length (), FTP_CLIENT_TIME_OUT) == -1) {
                                                String e = String (errno) + " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return "send() error: " + e;
                                              }
                                            }
            else if (ftpReplyIs ("230 "))   { // server said that we have logged in, initiate pasive data connection
                                              if (sendAll (controlConnectionSocket, (char *) "PASV\r\n", strlen ("PASV\r\n"), FTP_CLIENT_TIME_OUT) == -1) {
                                                String e = String (errno) + " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return "send() error: " + e;
                                              }
                                            }
            else if (ftpReplyIs ("227 "))   { // server send connection information like 227 entering passive mode (10,18,1,66,4,1)
                                              // open data connection
                                              int ip1, ip2, ip3, ip4, p1, p2; // get FTP server IP and port
                                              if (6 != sscanf (buffer, "%*[^(](%i,%i,%i,%i,%i,%i)", &ip1, &ip2, &ip3, &ip4, &p1, &p2)) { // shoul always succeed
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
                                                String e = String (errno) + " " + strerror (errno);
                                                close (controlConnectionSocket);
                                                return "socket() error: " + e; 
                                              }
                                              // make the socket not-blocking so that time-out can be detected
                                              if (fcntl (dataConnectionSocket, F_SETFL, O_NONBLOCK) == -1) {
                                                String e = String (errno) + " " + strerror (errno);
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return "fcntl() error: " + e; 
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
                                                  String e (errno);
                                                  close (dataConnectionSocket);
                                                  close (controlConnectionSocket);
                                                  return ("connect() error: " + e);                                                 
                                                }
                                              } while (true);
                                              */
                                              if (connect (dataConnectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
                                                if (errno != EINPROGRESS) {
                                                  String e = String (errno) + " " + strerror (errno);
                                                  close (dataConnectionSocket);
                                                  close (controlConnectionSocket);
                                                  return "connect() error: " + e;
                                                } 
                                              }
                                              // it is likely that socket is not connected yet at this point (the socket is non-blocking)
                                              // are we GETting or PUTting the file?
                                              String s;
                                                      if (!strcmp (ftpCommand, "GET")) {
                                                        s = "RETR " + String (serverFile) + "\r\n";
                                              } else if (!strcmp (ftpCommand, "PUT")) {
                                                        s = "STOR " + String (serverFile) + "\r\n";
                                              } else  {
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return "Unknown FTP command " + String (ftpCommand); 
                                              }
                                              if (sendAll (controlConnectionSocket, (char *) s.c_str (), s.length (), FTP_CLIENT_TIME_OUT) == -1) {
                                                String e = String (errno) + " " + strerror (errno);
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return "send() error: " + e;
                                              }
                                            }

            else if (ftpReplyIs ("150 "))   { // server wants to start data transfer
                                              // are we GETting or PUTting the file?
                                                      if (!strcmp (ftpCommand, "GET")) {
                                                        
                                                          int bytesRecvTotal = 0; int bytesWrittenTotal = 0;
                                                          File f = fileSystem.open (clientFile, FILE_WRITE);
                                                          if (f) {
                                                            // read data from data connection and store it to the file
                                                            #define BUFF_SIZE TCP_SND_BUF // TCP_SND_BUF = 5744, a maximum block size that ESP32 can send 
                                                            char *buff = (char *) malloc (BUFF_SIZE);
                                                            if (!buff) {
                                                              close (dataConnectionSocket);
                                                              close (controlConnectionSocket);
                                                              return "Out of memory.";
                                                            }
                                                            do {
                                                              int bytesRecvThisTime = recvAll (dataConnectionSocket, buff, BUFF_SIZE, NULL, FTP_CLIENT_TIME_OUT);
                                                              if (bytesRecvThisTime < 0)  { bytesRecvTotal = -1; break; } // to detect error later
                                                              if (bytesRecvThisTime == 0) { break; } // finished, success
                                                              bytesRecvTotal += bytesRecvThisTime;
                                                              int bytesWrittenThisTime = f.write ((uint8_t *) buff, bytesRecvThisTime);
                                                              bytesWrittenTotal += bytesWrittenThisTime;
                                                              if (bytesWrittenThisTime != bytesRecvThisTime) { bytesRecvTotal = -1; break; } // to detect error later
                                                            } while (true);
                                                            f.close ();
                                                            free (buff);
                                                          } else {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return "Can't open " + String (clientFile) + " for writting";
                                                          }
                                                          if (bytesWrittenTotal != bytesRecvTotal) {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return "Can't write " + String (clientFile);
                                                          }
                                                          close (dataConnectionSocket);
                                                          #ifdef __PERFMON__
                                                            __perfFSBytesWritten__ += bytesWrittenTotal; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                                                          #endif

                                              } else if (!strcmp (ftpCommand, "PUT")) {

                                                          int bytesReadTotal = 0; int bytesSentTotal = 0;
                                                          File f = fileSystem.open (clientFile, FILE_READ);
                                                          if (f) {
                                                            // read data from file and transfer it through data connection
                                                            #define BUFF_SIZE TCP_SND_BUF // TCP_SND_BUF = 5744, a maximum block size that ESP32 can send 
                                                            char *buff = (char *) malloc (BUFF_SIZE);
                                                            if (!buff) {
                                                              close (dataConnectionSocket);
                                                              close (controlConnectionSocket);
                                                              return "Out of memory.";
                                                            }
                                                            do {
                                                              int bytesReadThisTime = f.read ((uint8_t *) buff, BUFF_SIZE);
                                                              if (bytesReadThisTime == 0) { break; } // finished, success
                                                              bytesReadTotal += bytesReadThisTime;
                                                              int bytesSentThisTime = sendAll (dataConnectionSocket, buff, bytesReadThisTime, FTP_CLIENT_TIME_OUT);
                                                              if (bytesSentThisTime != bytesReadThisTime) { bytesSentTotal = -1; break; } // to detect error later
                                                              bytesSentTotal += bytesSentThisTime;
                                                            } while (true);
                                                            f.close ();
                                                            free (buff);
                                                          } else {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return "Can't open " + String (clientFile) + " for reading";
                                                          }
                                                          if (bytesSentTotal != bytesReadTotal) {
                                                            close (dataConnectionSocket);
                                                            close (controlConnectionSocket);
                                                            return "Can't send " + String (clientFile);                                                            
                                                          }
                                                          close (dataConnectionSocket);
                                                          #ifdef __PERFMON__
                                                            __perfFSBytesRead__ += bytesReadTotal; // update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way
                                                          #endif
                                                       
                                              } else  {
                                                close (dataConnectionSocket);
                                                close (controlConnectionSocket);
                                                return "Unknown FTP command " + String (ftpCommand); 
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
  String ftpPut (char *localFileName = (char *) "", char *remoteFileName = (char *) "", char *password = (char *) "", char *userName = (char *) "", int ftpPort = 0, char *ftpServer = (char *) "") {
    // DEBUG:
    Serial.printf ("entering: ftpPut (%s, %s, %s, %s, %i, %s)\n", localFileName, remoteFileName, password, userName, ftpPort, ftpServer);
    if (__fileSystemMounted__) { 
      char buffer [MAX_ETC_FTP_FTPCLIENT_CF + 1];
      strcpy (buffer, "\n");
      if (readConfigurationFile (buffer + 1, sizeof (buffer) - 2, (char *) "/etc/ftp/ftpclient.cf")) {
        strcat (buffer, "\n");
        char *p = NULL;
        char *q;
        if (!*ftpServer) if ((ftpServer = strstr (buffer, "\nftpServer "))) ftpServer += 11;
        if (!ftpPort)    if ((p = strstr (buffer, "\nftpPort ")))           p += 9;
        if (!*userName)  if ((userName = strstr (buffer, "\nuserName ")));  userName += 10;
        if (!*password)  if ((password = strstr (buffer, "\npassword ")));  password += 10;

        if ((q = strstr (ftpServer, "\n"))) *q = 0;
        if (p && (q = strstr (p, "\n"))) { *q = 0; ftpPort = atoi (p); }
        if ((q = strstr (userName, "\n"))) *q = 0;
        if ((q = strstr (password, "\n"))) *q = 0;
      }
    } else {
        dmesg ("[ftpClient] file system not mounted, can't read /etc/ftp/ftpclient.cf");
    }
    // DEBUG:
    Serial.printf ("defaults filled in\n");
    Serial.printf ("with defaults: ftpPut (%s, %s, %s, %s, %i, %s)\n", localFileName, remoteFileName, password, userName, ftpPort, ftpServer);
    if (!*password || !*userName || !ftpPort || !*ftpServer) {
      dmesg ("[ftpClient] not all the arguments are set, if you want to use the default values, write them to /etc/ftp/ftpclient.cf");
      return "Not all the arguments are set, if you want to use ftpPut default values, write them to /etc/ftp/ftpclient.cf";
    } else {
      return __ftpClient__ ((char *) "PUT", localFileName, remoteFileName, password, userName, ftpPort, ftpServer);
    }
  }

  // retrieves file, returns error or success text, fills empty parameters with the ones from configuration file /etc/ftp/ftpclient.cf
  String ftpGet (char *localFileName = (char *) "", char *remoteFileName = (char *) "", char *password = (char *) "", char *userName = (char *) "", int ftpPort = 0, char *ftpServer = (char *) "") {
    if (__fileSystemMounted__) { 
      char buffer [MAX_ETC_FTP_FTPCLIENT_CF + 1];
      strcpy (buffer, "\n");
      if (readConfigurationFile (buffer + 1, sizeof (buffer) - 2, (char *) "/etc/ftp/ftpclient.cf")) {
        strcat (buffer, "\n");
        char *p = NULL;
        char *q;
        if (!*ftpServer) if ((ftpServer = strstr (buffer, "\nftpServer "))) ftpServer += 11;
        if (!ftpPort)    if ((p = strstr (buffer, "\nftpPort ")))           p += 9;
        if (!*userName)  if ((userName = strstr (buffer, "\nuserName ")));  userName += 10;
        if (!*password)  if ((password = strstr (buffer, "\npassword ")));  password += 10;

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
      return __ftpClient__ ((char *) "GET", localFileName, remoteFileName, password, userName, ftpPort, ftpServer);
    }
  }

#endif
