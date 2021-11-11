/*

    ftpServer.hpp 
 
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    FtpServer is built upon TcpServer with connectionHandler that handles TcpConnection according to FTP protocol.
    This goes for control connection at least. FTP is a little more complicated since it uses another TCP connection
    for data transfer. Beside that, data connection can be initialized by FTP server or FTP client and it is FTP client's
    responsibility to decide which way it is going to be.
   
    November, 6, 2021, Bojan Jurca
    
*/


#ifndef __FTP_SERVER__
  #define __FTP_SERVER__

  #ifndef HOSTNAME
    #define HOSTNAME "MyESP32Server" // use default if not defined
  #endif

  #include <WiFi.h>


  /*

     Support for telnet dmesg command. If telnetServer.hpp is included in the project __ftpDmesg__ function will be redirected
     to message queue defined there and dmesg command will display its contetn. Otherwise it will just display message on the
     Serial console.
     
  */  
  
  void __ftpDmesg__ (String message) { 
    #ifdef __TELNET_SERVER__ // use dmesg from telnet server if possible
      dmesg (message);
    #else
      Serial.printf ("[%10lu] %s\n", millis (), message.c_str ()); 
    #endif
  }
  void (* ftpDmesg) (String) = __ftpDmesg__; // use this pointer to display / record system messages

  #ifndef dmesg
    #define dmesg ftpDmesg
  #endif

  /*

     When FTP server is working in passive mode it tells FTP client on which port to establish data connection. Since there may be
     multiple FTP sessions running at the same time the server must pass a different port number for passive data connection
     to each client.  
     
  */  


  int __pasiveDataPort__ () {
    static SemaphoreHandle_t __pasiveDataPortSemaphore__= xSemaphoreCreateMutex (); 
    static int lastPasiveDataPort = 1024;                                               // change pasive data port range if needed
    xSemaphoreTake (__pasiveDataPortSemaphore__, portMAX_DELAY);
      int pasiveDataPort = lastPasiveDataPort = (((lastPasiveDataPort + 1) % 16) + 1024); // change pasive data port range if needed
    xSemaphoreGive (__pasiveDataPortSemaphore__);
    return pasiveDataPort;
  }


  #include "TcpServer.hpp"        // ftpServer.hpp is built upon TcpServer.hpp  
  #include "file_system.h"        // ftpServer.hpp needs file_system.h
  #include "user_management.h"    // ftpServer.hpp needs user_management.h


  class ftpServer: public TcpServer {                                             
  
    public:

      // keep ftp session parameters in a structure
      typedef struct {
        String userName;
        String homeDir;
        String workingDir;
        TcpConnection *controlConnection; // ftp control connection
        TcpServer *pasiveDataServer;      // for ftp pasive mode data connection
        TcpClient *activeDataClient;      // for ftp active mode data connection
        // feel free to add more
      } ftpSessionParameters;        
  
      ftpServer (String serverIP,                                      // FTP server IP address, 0.0.0.0 for all available IP addresses
                 int serverPort,                                       // FTP server port
                 bool (* firewallCallback) (String connectingIP)       // a reference to callback function that will be celled when new connection arrives 
                ): TcpServer (__staticFtpConnectionHandler__, (void *) this, 8 * 1024, (TIME_OUT_TYPE) 300000, serverIP, serverPort, firewallCallback)
                                                {
                                                  dmesg ("[ftpServer] started on " + serverIP + ":" + serverPort + (firewallCallback ? F (" with firewall.") : F (".")));
                                                }
      
      ~ftpServer ()                             { dmesg (F ("[ftpServer] stopped.")); }
                                     
    private:

      static void __staticFtpConnectionHandler__ (TcpConnection *connection, void *ths) {  // connectionHandler callback function
        ((ftpServer *) ths)->__ftpConnectionHandler__ (connection);
      }

      virtual void __ftpConnectionHandler__ (TcpConnection *connection) { // connectionHandler callback function

        // this is where ftp session begins
        
        ftpSessionParameters fsp = {"", "", "", connection, NULL, NULL};
        
        String cmdLine; cmdLine.reserve (80);

        if (!__fileSystemMounted__) {
          connection->sendData ("220-" + String (HOSTNAME) + F (" file system file system not mounted. You may have to use telnet and mkfs.fat to format flash disk first."));
          goto closeFtpConnection;
        }
  
        #if USER_MANAGEMENT == NO_USER_MANAGEMENT
          if (!connection->sendData ("220-" + String (HOSTNAME) + F (" FTP server - everyone is allowed to login\r\n220 \r\n"))) goto closeFtpConnection;
        #else
          if (!connection->sendData ("220-" + String (HOSTNAME) + F (" FTP server - please login\r\n220 \r\n"))) goto closeFtpConnection;
        #endif  

        while (13 == __readLineFromClient__ (cmdLine, connection)) { // read and process comands in a loop        

          if (cmdLine.length ()) {

            // ----- parse command line into arguments (max 32) -----
            
            int argc = 0; String argv [8] = {"", "", "", "", "", "", "", ""}; 
            argv [0] = String (cmdLine); argv [0].trim ();
            String param = ""; // everything behind argv [0]
            if (argv [0] != "") {
              argc = 1;
              while (argc < 8) {
                int l = argv [argc - 1].indexOf (" ");
                if (l > 0) {
                  argv [argc] = argv [argc - 1].substring (l + 1);
                  argv [argc].trim ();
                  if (argc == 1) param = argv [argc]; // remember the whole parameter line in its original form
                  argv [argc - 1] = argv [argc - 1].substring (0, l); // no need to trim
                  argc ++;
                } else {
                  break;
                }
              }
            }

            //debug FTP protocol: Serial.print ("<--"); for (int i = 0; i < argc; i++) Serial.print (argv [i] + " "); Serial.println ();

            // ----- try to handle ftp command -----
            String s = __internalFtpCommandHandler__ (argc, argv, param, &fsp);
            if (!connection->sendData (s)) goto closeFtpConnection; // send reply to FTP client
              
            //debug FTP protocol: Serial.println ("-->" + s);

          } // if cmdLine is not empty
        } // read and process comands in a loop

closeFtpConnection:      
        if (fsp.userName != "") dmesg ("[ftpServer] " + fsp.userName + F (" logged out."));
      }
    
      // returns last chracter pressed (Enter or 0 in case of error
      static char __readLineFromClient__ (String& line, TcpConnection *connection) {
        line = "";
        
        char c;
        while (connection->recvChar (&c)) { // read and process incoming data in a loop 
          switch (c) {
              case 10:  // ignore
                        break;
              case 13:  // end of command line
                        return 13;
              default:  // fill the buffer 
                        line += c;
                        break;              
          } // switch
        } // while
        return false;
      }

      // ----- built-in commands -----

      String __internalFtpCommandHandler__ (int argc, String argv [], String param, ftpServer::ftpSessionParameters *fsp) {
        argv [0].toUpperCase ();

        if (argv [0] == "OPTS") { //------------------------------------------- OPTS (before login process)
          
          if (argc == 3) return this->__OPTS__ (argv [1], argv [2]);

        } else if (argv [0] == "USER") { //------------------------------------ USER (login process)
          
          if (argc <= 2) return this->__USER__ (argv [1], fsp); // not entering user name may be ok for anonymous login

        } else if (argv [0] == "PASS") { //------------------------------------ PASS (login process)
          
          if (argc <= 2) return this->__PASS__ (argv [1], fsp); // not entering password may be ok for anonymous login

        } else if (argv [0] == "CWD") { //------------------------------------ CWD (cd directory)
          
          if (argc >= 2) return this->__CWD__ (param, fsp); // use param instead of argv [1] to enable the use of long file names

        } else if (argv [0] == "PWD" || argv [0] == "XPWD") { //-------------- PWD, XPWD
          
          if (argc == 1) return this->__PWD__ (fsp);

        } else if (argv [0] == "TYPE") { //----------------------------------- TYPE
          
          return this->__TYPE__ (fsp);

        } else if (argv [0] == "NOOP") { //----------------------------------- NOOP
          
          return this->__NOOP__ ();

        } else if (argv [0] == "SYST") { //----------------------------------- SYST
          
          return this->__SYST__ (fsp);

        } else if (argv [0] == "SIZE") { //----------------------------------- SIZE 
          
          if (argc == 2) return this->__SIZE__ (argv [1], fsp);

        } else if (argv [0] == "PASV") { //----------------------------------- PASV - start pasive data transfer (always followed by one of data transfer commands)
          
          if (argc == 1) return this->__PASV__ (fsp);

        } else if (argv [0] == "PORT") { //----------------------------------- PORT - start active data transfer (always followed by one of data transfer commands)
          
          if (argc == 2) return this->__PORT__ (argv [1], fsp);

        } else if (argv [0] == "NLST" || argv [0] == "LIST") { //------------- NLST, LIST (ls directory) - also ends pasive or active data transfer

          if (argc == 1) return this->__NLST__ (fsp->workingDir, fsp);
          if (argc == 2) return this->__NLST__ (argv [1], fsp);

        } else if (argv [0] == "CWD") { //------------------------------------ CWD (cd directory)

          if (argc >= 2) return this->__CWD__ (param, fsp); // use param instead of argv [1] to enable the use of long file names

        } else if (argv [0] == "XMKD" || argv [0] == "MKD") { //-------------- XMKD, MKD (mkdir directory)

          if (argc >= 2) return this->__XMKD__ (param, fsp); // use param instead of argv [1] to enable the use of long file names

        } else if (argv [0] == "RNFR" || argv [0] == ".....") { //-------------- RNFR, MKD (... file or directory)

          if (argc >= 2) return this->__RNFR__ (param, fsp); // use param instead of argv [1] to enable the use of long file names
          
        } else if (argv [0] == "XRMD" || argv [0] == "DELE") { //------------- XRMD, DELE (rm file or directory)

          if (argc >= 2) return this->__XRMD__ (param, fsp); // use param instead of argv [1] to enable the use of long file names

        } else if (argv [0] == "RETR") { //----------------------------------- RETR (get fileName) - also ends pasive or active data transfer

          if (argc >= 2) return this->__RETR__ (param, fsp); // use param instead of argv [1] to enable the use of long file names

        } else if (argv [0] == "STOR") { //----------------------------------- STOR (put fileName) - also ends pasive or active data transfer

          if (argc >= 2) return this->__STOR__ (param, fsp); // use param instead of argv [1] to enable the use of long file names

        } else if (argv [0] == "QUIT") { //----------------------------------- QUIT
          
          return this->__QUIT__ (fsp);
                    
        }

        // -------------------------------------------------------------------- INVALID COMMAND
                      
        return F ("502 command not implemented\r\n"); // "220 unsupported command\r\n";                               
      }

      inline String __OPTS__ (String argv1, String argv2) { // enable UTF8
        argv1.toUpperCase (); argv2.toUpperCase ();
        if (argv1 == "UTF8" && argv2 == "ON")         return F ("200 UTF8 enabled\r\n");
                                                      return F ("502 command not implemented\r\n");
      }

      inline String __USER__ (String userName, ftpSessionParameters *fsp) { // save user name and require password
        fsp->userName = userName;                     return F ("331 enter password\r\n");
      }

      String __PASS__ (String password, ftpSessionParameters *fsp) { // login
        if (checkUserNameAndPassword (fsp->userName, password)) fsp->workingDir = fsp->homeDir = getUserHomeDirectory (fsp->userName);
        if (fsp->homeDir > "") { // if logged in
                                                      dmesg ("[ftpServer] " + fsp->userName + F (" logged in."));
                                                      return "230 logged on, your home directory is \"" + fsp->homeDir + F ("\"\r\n");
        } else { 
                                                      dmesg ("[ftpServer] " + fsp->userName + F (" login attempt failed."));
                                                      return F ("530 user name or password incorrect\r\n"); 
        }
      }

      inline String __PWD__ (ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                      return F ("530 not logged in\r\n");
                                                     return "257 \"" + fsp->workingDir + F ("\"\r\n");
      }

      inline String __TYPE__ (ftpSessionParameters *fsp) { // command needs to be implemented but we are not going to repond to it
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");
                                                      return F ("200 ok\r\n");
      }

      inline String __NOOP__ () { 
                                                      return F ("200 ok\r\n");
      }
      
      inline String __SYST__ (ftpSessionParameters *fsp) { // command needs to be implemented but we are going to pretend this is a UNIX system
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");
                                                      return F ("215 UNIX Type: L8\r\n");
      }

      inline String __SIZE__ (String fileName, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");

        String fp = fullFilePath (fileName, fsp->workingDir);
        if (fp == "")                                 return F ("550 invalid file name\r\n");
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        unsigned long fSize = 0;
        File f = FFat.open (fp, FILE_READ);
        if (f) {
          fSize = f.size ();                        
          f.close ();
        }
                                                      return "213 " + String (fSize) + "\r\n";
      }

      inline String __PASV__ (ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");

        int ip1, ip2, ip3, ip4, p1, p2; // get (this) server IP and next free port
        if (4 == sscanf (fsp->controlConnection->getThisSideIP ().c_str (), "%i.%i.%i.%i", &ip1, &ip2, &ip3, &ip4)) {
          // get next free port
          int pasiveDataPort = __pasiveDataPort__ ();
           // open a new TCP server to accept pasive data connection
          fsp->pasiveDataServer = newTcpServer ((TIME_OUT_TYPE) 5000, fsp->controlConnection->getThisSideIP (), pasiveDataPort, NULL);
          // report to ftp client through control connection how to connect for data exchange
          p2 = pasiveDataPort % 256;
          p1 = pasiveDataPort / 256;
          if (fsp->pasiveDataServer)                  return "227 entering passive mode (" + String (ip1) + "," + String (ip2) + "," + String (ip3) + "," + String (ip4) + "," + String (p1) + "," + String (p2) + ")\r\n";
        } 
                                                      return F ("425 can't open data connection\r\n");
      }

      inline String __PORT__ (String dataConnectionInfo, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");
        
        int ip1, ip2, ip3, ip4, p1, p2; // get client IP and port
        if (6 == sscanf (dataConnectionInfo.c_str (), "%i,%i,%i,%i,%i,%i", &ip1, &ip2, &ip3, &ip4, &p1, &p2)) {
          char activeDataIP [16];
          int activeDataPort;
          sprintf (activeDataIP, "%i.%i.%i.%i", ip1, ip2, ip3, ip4); 
          activeDataPort = 256 * p1 + p2;
          fsp->activeDataClient = newTcpClient (activeDataIP, activeDataPort, (TIME_OUT_TYPE) 5000); // open a new TCP client for active data connection
          if (fsp->activeDataClient) {
            if (fsp->activeDataClient->connection ()) return F ("200 port ok\r\n"); 
            delete (fsp->activeDataClient);
            fsp->activeDataClient = NULL;
          }
        } 
                                                      return F ("425 can't open data connection\r\n");
      }

      inline String __NLST__ (String& directory, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");

        String fp = fullDirectoryPath (directory, fsp->workingDir);

        if (fp == "" || !isDirectory (fp))            return F ("550 invalid directory\r\n");
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        if (fp.substring (fp.length () - 1) != "/") fp += '/'; // full directoy path always ends with /

        if (!fsp->controlConnection->sendData ((char *) F ("150 starting transfer\r\n"))) return ""; // control connection closed
        
        int bytesWritten = 0;
        TcpConnection *dataConnection = NULL;
        if (fsp->pasiveDataServer) while (!(dataConnection = fsp->pasiveDataServer->connection ()) && !fsp->pasiveDataServer->timeOut ()) delay (1); // wait until a connection arrives to non-threaded server or time-out occurs
        if (fsp->activeDataClient) dataConnection = fsp->activeDataClient->connection (); // non-threaded client differs from non-threaded server - connection is established before constructor returns or not at all

        if (dataConnection) bytesWritten = dataConnection->sendData (listDirectory (fp) + "\r\n");
        
        if (fsp->activeDataClient) { delete (fsp->activeDataClient); fsp->activeDataClient = NULL; }
        if (fsp->pasiveDataServer) { delete (fsp->pasiveDataServer); fsp->pasiveDataServer = NULL; }
        if (bytesWritten)                             return F ("226 transfer complete\r\n");
                                                      return F ("425 can't open data connection\r\n");
      }

      inline String __CWD__ (String directory, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");
        String fp = fullDirectoryPath (directory, fsp->workingDir);
        if (fp == "" || !isDirectory (fp))            return "501 invalid directory " + fp +"\r\n";
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        fsp->workingDir = fp;                         return "250 your working directory is now " + fsp->workingDir + "\r\n";
      }

      inline String __XMKD__ (String directory, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");
        String fp = fullDirectoryPath (directory, fsp->workingDir);
        if (fp == "")                                 return F ("501 invalid directory\r\n");
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        if (makeDir (fp))                             return "257 \"" + fp + "\" created\r\n";
                                                      return "550 could not create \"" + fp + "\"\r\n";
      }

      inline String __RNFR__ (String fileOrDirName, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");

        if (!fsp->controlConnection->sendData ((char *) F ("350 need more information\r\n"))) return "";
        String s; s.reserve (80);
        if (13 != __readLineFromClient__ (s, fsp->controlConnection)) return "503 wrong command syntax\r\n";

        if (s.substring (0, 4) != "RNTO")             return F ("503 wrong command syntax\r\n");
        s = s.substring (4); s.trim ();
        String fp1 = fullFilePath (fileOrDirName, fsp->workingDir);
        if (fp1 == "")                                return F ("501 invalid directory\r\n");
        if (!userMayAccess (fp1, fsp->homeDir))       return F ("553 access denyed\r\n");

        String fp2 = fullFilePath (s, fsp->workingDir);
        if (fp2 == "")                                return F ("501 invalid directory\r\n");
        if (!userMayAccess (fp2, fsp->homeDir))       return F ("553 access denyed\r\n");

        if (FFat.rename (fp1, fp2))                   return "250 renamed to " + s + "\r\n";
                                                      return "553 unable to rename " + fileOrDirName + "\r\n";
      }

      inline String __XRMD__ (String fileOrDirName, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");
        String fp = fullFilePath (fileOrDirName, fsp->workingDir);
        if (fp == "")                                 return F ("501 invalid file or directory name\r\n");
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        if (isFile (fp)) {
          if (deleteFile (fp))                        return "250 " + fp + " deleted\r\n";
                                                      return "452 could not delete " + fp + "\r\n";
        } else {
          if (fp == fsp->homeDir)                     return F ("550 you can't remove your home directory\r\n");
          if (fp == fsp->workingDir)                  return F ("550 you can't remove your working directory\r\n");
          if (removeDir (fp))                         return "250 " + fp + " removed\r\n";
                                                      return "452 could not remove " + fp + "\r\n";
        }
                             
      }

      inline String __RETR__ (String fileName, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");

        String fp = fullFilePath (fileName, fsp->workingDir);

        if (fp == "" || !isFile (fp))                 return F ("501 invalid file name\r\n");
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        if (!fsp->controlConnection->sendData ((char *) F ("150 starting transfer\r\n"))) return ""; // control connection closed 

        int bytesWritten = 0; int bytesRead = 0;
        TcpConnection *dataConnection = NULL;
        if (fsp->pasiveDataServer) while (!(dataConnection = fsp->pasiveDataServer->connection ()) && !fsp->pasiveDataServer->timeOut ()) delay (1); // wait until a connection arrives to non-threaded server or time-out occurs
        if (fsp->activeDataClient) dataConnection = fsp->activeDataClient->connection (); // non-threaded client differs from non-threaded server - connection is established before constructor returns or not at all

        if (dataConnection) {
          File f = FFat.open (fp, FILE_READ);
          if (f) {
            if (!f.isDirectory ()) {
              // read data from file and transfer it through data connection
              #define BUFF_SIZE 2048
              byte buff [2048]; // get 2 KB of memory from the stack
              int i = bytesWritten = 0;
              while (f.available ()) {
                buff [i++] = f.read ();
                if (i == BUFF_SIZE) { bytesRead += BUFF_SIZE; bytesWritten += dataConnection->sendData ((char *) buff, BUFF_SIZE); i = 0; }
              }
              if (i) { bytesRead += i; bytesWritten += dataConnection->sendData ((char *) buff, i); }
            }
            f.close ();
          }
        }

        if (fsp->activeDataClient) { delete (fsp->activeDataClient); fsp->activeDataClient = NULL; }
        if (fsp->pasiveDataServer) { delete (fsp->pasiveDataServer); fsp->pasiveDataServer = NULL; }
        if (bytesRead && bytesWritten == bytesRead)   return F ("226 transfer complete\r\n");
                                                      return F ("550 file could not be transfered\r\n");      
      }

      inline String __STOR__ (String fileName, ftpSessionParameters *fsp) { 
        if (fsp->homeDir == "")                       return F ("530 not logged in\r\n");

        String fp = fullFilePath (fileName, fsp->workingDir);

        if (fp == "" || isDirectory (fp))             return F ("501 invalid file name\r\n");
        if (!userMayAccess (fp, fsp->homeDir))        return F ("550 access denyed\r\n");

        if (!fsp->controlConnection->sendData ((char *) F ("150 starting transfer\r\n"))) return ""; // control connection closed 

        int bytesWritten = 0; int bytesRead = 0;
        TcpConnection *dataConnection = NULL;
        if (fsp->pasiveDataServer) while (!(dataConnection = fsp->pasiveDataServer->connection ()) && !fsp->pasiveDataServer->timeOut ()) delay (1); // wait until a connection arrives to non-threaded server or time-out occurs
        if (fsp->activeDataClient) dataConnection = fsp->activeDataClient->connection (); // non-threaded client differs from non-threaded server - connection is established before constructor returns or not at all

        if (dataConnection) {
          File f = FFat.open (fp, FILE_WRITE);
          if (f) {
            #define BUFF_SIZE 2048
            byte buff [BUFF_SIZE]; // get 2048 B of memory from from the stack
            int received;
            do {
              bytesRead += (received = dataConnection->recvData ((char *) buff, BUFF_SIZE));
              int written = f.write (buff, received);                   
              if (received && (written == received)) bytesWritten += written;
            } while (received);
            f.close ();
          } else {
            dmesg ("[ftpServer] could not open " + fp + F (" for writing."));
          }
        }

        if (fsp->activeDataClient) { delete (fsp->activeDataClient); fsp->activeDataClient = NULL; }
        if (fsp->pasiveDataServer) { delete (fsp->pasiveDataServer); fsp->pasiveDataServer = NULL; }
        if (bytesRead && bytesWritten == bytesRead)   return F ("226 transfer complete\r\n");
                                                      return F ("550 file could not be transfered\r\n");      
      }

      inline String __QUIT__ (ftpSessionParameters *fsp) { 
        // close data connection if open
        if (fsp->activeDataClient) delete (fsp->activeDataClient);
        if (fsp->pasiveDataServer) delete (fsp->pasiveDataServer); 
        // report client we are closing contro connection
        fsp->controlConnection->sendData ((char *) F ("221 closing control connection\r\n"));
        fsp->controlConnection->closeConnection ();
        return "";
      }
      
  };


  // Arduino has serious problem with "new" - if it can not allocat memory for a new object it should
  // return a NULL pointer but it just crashes ESP32 instead. The way around it to test if there is enough 
  // memory available first, before calling "new". Since this is multi-threaded environment both should be
  // done inside a critical section. Each class we create will implement a function that would create a
  // new object and would follow certain rules. 

  inline ftpServer *newFtpServer (String serverIP,
                                  int serverPort,
                                  bool (* firewallCallback) (String connectingIP)) {
    ftpServer *p = NULL;
    xSemaphoreTake (__newInstanceSemaphore__, portMAX_DELAY);
      if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) >= sizeof (ftpServer) + 256) { // don't go below 256 B (live some memory for error messages ...)
        try {
          p = new ftpServer (serverIP, serverPort, firewallCallback);
        } catch (int e) {
          ; // TcpServer thows exception if constructor fails
        }
      }
    xSemaphoreGive (__newInstanceSemaphore__);
    return p;
  }  
    
#endif
