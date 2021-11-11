/*

    TcpServer.hpp

    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template

    TcpServer.hpp contains a minimalistic IPv4 threaded TCP server for ESP32 / Arduino environment with:
      - time-out functionality,
      - firewall functionality.

    Five types of objects are introduced in order to make programming interface as simple as possible
    (a programmer does not have to deal with sockets and threads by himself):
    
      - threaded TcpConnection (with time-out functionality while handling a connection),
      - not-threaded TcpConnection (with time-out functionality while handling a connection),
      - threaded TcpServer (with firewall functionality),
      - not-threaded TcpServer (with time-out functionality while waiting for a connection, firewall functionality),
      - TcpClient (with time-out functionality while handling the connection).

    November, 2, 2021, Bojan Jurca

*/


#ifndef __TCP_SERVER__
  #define __TCP_SERVER__

  #include <WiFi.h>
  #include <lwip/sockets.h>
  #include "common_functions.h" // inet_ntos, __newInstanceSemaphore__

  
  /*

     Support for telnet dmesg command. If telnetServer.hpp is included in the project __TcpDmesg__ function will be redirected
     to message queue defined there and dmesg command will display its contetn. Otherwise it will just display message on the
     Serial console.
     
  */


  void __TcpServerDmesg__ (String message) { 
    #ifdef __TELNET_SERVER__  // use dmesg from telnet server if possible
      dmesg (message);
    #else                     // if not just output to Serial
      Serial.printf ("[%10lu] %s\n", millis (), message.c_str ()); 
    #endif
  }
  void (* TcpServerDmesg) (String) = __TcpServerDmesg__; // use this pointer to display / record system messages

  #ifndef dmesg
    #define dmesg TcpServerDmesg
  #endif
  

  // control TcpServer critical sections
  static SemaphoreHandle_t __TcpServerSemaphore__ = xSemaphoreCreateMutex (); 
  

  /*
      There are two types of TcpConnections:
       
        Threaded TcpConnections run in their own threads and communicate with calling program through connection handler callback function.
        This type of TcpConnections is normally used by TcpServers.
      
        Not-threaded TcpConnections run in threads of calling programs. Calling program can call instance member functions. 
        This type of TcpConnections is normally used by TcpClients.

  */


  // define time-out data type
  enum TIME_OUT_TYPE: unsigned long {
    INFINITE = 0  // infinite time-out
                  // everything else is time-out in milliseconds
  };


  class TcpConnection {
  
    public:

    
      /*
    
        TcpConnection constructor that creates threaded TcpConnection. Threaded TcpConnections run in their own threads and communicate with 
        calling program through connection handler callback function.
          
        TcpConnection state is for instance internal use, only to assure proper closing of the socket and unloading of the instance. 
        Threaded TcpConnections goes through the following states:

                                                                 (connection thread has stopped, it does not access the instance
          (initial state)    (connection thread is running)       internal memory any more so it is safe to unload the instance)
          NOT_STARTED=9 -+------------> RUNNING=1 -----------+-> FINISHED=2
                         |                                   |
                         +-----------------------------------+
                         (creation of connection thread was not successful)

      */

      
      TcpConnection (void (* connectionHandlerCallback) (TcpConnection *connection, void *parameter), // a reference to callback function that will handle the connection
                     void *connectionHandlerCallbackParamater,                                        // a reference to parameter that will be passed to connectionHandlerCallback
                     size_t stackSize,                                                                // stack size of a thread where connection runs - this value depends on what server really does (see connectionHandler function) should be set appropriately
                     int socket,                                                                      // connection socket
                     String otherSideIP,                                                              // IP address of the other side of connection
                     TIME_OUT_TYPE timeOutMillis)                                                     // connection time-out in milli seconds
      {
        // copy constructor parameters to internal instance memory
        __connectionHandlerCallback__ = connectionHandlerCallback;
        __connectionHandlerCallbackParamater__ = connectionHandlerCallbackParamater;
        __socket__ = socket;
        strcpy (__otherSideIP__,otherSideIP.c_str ()); // __otherSideIP__ = otherSideIP;
        __timeOutMillis__ = timeOutMillis;
  
        // start thread that will handle the connection
        if (connectionHandlerCallback) {
          #define tskNORMAL_PRIORITY 1
          __connectionState__ = TcpConnection::RUNNING;
          if (pdPASS != xTaskCreate (__connectionHandler__,
                                     "TcpConnection",
                                     stackSize,
                                     this, // pass "this" pointer to static member function
                                     tskNORMAL_PRIORITY,
                                     NULL)) {
            // __connectionState__ = TcpConnection::FINISHED; // not really necessary since "throw" will cause constructor to fail and destructor will not get called at all
            throw 1; // out of memory
          }
        }
      }


      /*
    
        TcpConnection constructor that creates not-threaded TcpConnection. Not-threaded TcpConnections run in threads of calling programs.
        Calling program can call instance member functions.

        It is due of a calling program to make use of the instance properly, so no special mechanism is needed to
        control unloading of the instance. The instance stays in NOT_THREADED state all the time.
        
          (the only state)
          NOT_THREADED=8
     
      */

      
      TcpConnection (int socket,                                   // connection socket
                     String otherSideIP,                           // IP address of the other side of connection
                     TIME_OUT_TYPE timeOutMillis)                  // connection time-out in milli seconds
      {
        // copy constructor parameters to internam instance memory
        __socket__ = socket;
        strcpy (__otherSideIP__, otherSideIP.c_str ()); // __otherSideIP__ = otherSideIP;
        __timeOutMillis__ = timeOutMillis;

        // set state
        __connectionState__ = TcpConnection::NOT_THREADED;
      }
  
      virtual ~TcpConnection () {
        closeConnection (); // close connection socket if it is still open - this will, in consequence, cause __connectionHandlerCallback__ to finish regulary by itself and clean up ist memory before returning (if threaded mode is used)
        while (__connectionState__ < TcpConnection::FINISHED) delay (1); // wait for __connectionHandler__ to finish before releasing the memory occupied by this instance (if threaded mode is used)
      }
  
      void closeConnection () {
        int connectionSocket;
        xSemaphoreTake (__TcpServerSemaphore__, portMAX_DELAY);
          connectionSocket = __socket__;
          __socket__ = -1;
        xSemaphoreGive (__TcpServerSemaphore__);
        if (connectionSocket != -1) close (connectionSocket); // can't close socket inside critical section, close it now
      }
  
      bool isClosed () { return __socket__ == -1; }
  
      bool isOpen () { return __socket__ != -1; }
  
      int getSocket () { return __socket__; } // if the calling program needs a lower level programming

      String getOtherSideIP () { return __otherSideIP__;  }
      
      String getThisSideIP () { // we can not get this information from constructor since connection is not necessarily established when constructor is called
        struct sockaddr_in thisAddress = {};
        socklen_t len = sizeof (thisAddress);
        if (getsockname (__socket__, (struct sockaddr *) &thisAddress, &len) == -1) return "";
        else                                                                        return inet_ntos (thisAddress.sin_addr);
        // port number could also be obtained if needed: ntohs (thisAddress.sin_port);
      }

      // define available data types
      enum AVAILABLE_TYPE {
        NOT_AVAILABLE = 0,  // no data is available to be read
        AVAILABLE = 1,      // data is available to be read
        ERROR = 3           // error in communication
      };
      AVAILABLE_TYPE available () { // checks if incoming data is pending to be read
        char buffer;
        if (-1 == recv (__socket__, &buffer, sizeof (buffer), MSG_PEEK)) {
          #define EAGAIN 11
          if (errno == EAGAIN || errno == EBADF) {
            if ((__timeOutMillis__ != INFINITE) && (millis () - __lastActiveMillis__ >= __timeOutMillis__)) {
              // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] time-out: %lu in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), __socket__, millis () - __lastActiveMillis__, __func__);              
              __timeOut__ = true;
              closeConnection ();
              return TcpConnection::ERROR;
            }
            return TcpConnection::NOT_AVAILABLE;
          } else {
            // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] MSG_PEEK error: %i, time-out: %lu in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), __socket__, errno, millis () - __lastActiveMillis__, __func__);
            return TcpConnection::ERROR;
          }
        } else {
          return TcpConnection::AVAILABLE;
        }
      }

      int recvData (char *buffer, int bufferSize) { // returns the number of bytes actually received or 0 indicating error or closed connection
        while (true) {
          yield ();
          if (__socket__ == -1) return 0;
          switch (int recvTotal = recv (__socket__, buffer, bufferSize, 0)) {
            case -1:
              // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] recv error: %i, time-out: %lu in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), __socket__, errno, millis () - __lastActiveMillis__, __func__);
              #define EAGAIN 11
              #define ENAVAIL 119
              if (errno == EAGAIN || errno == ENAVAIL) {
                if ((__timeOutMillis__ == INFINITE) || (millis () - __lastActiveMillis__ < __timeOutMillis__)) {
                  delay (1);
                  break;
                }
              }
              // else close and continue to case 0
              // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] time-out: %lu in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), __socket__, millis () - __lastActiveMillis__, __func__);              
              __timeOut__ = true;
              closeConnection ();
            case 0:   // connection is already closed
              return 0;
            default:
              __lastActiveMillis__ = millis ();
              return recvTotal;
          }
        }
      }

      int recvChar (char *c) { return (recvData (c, 1)); }

      int sendData (char *buffer, int bufferSize) { // returns the number of bytes actually sent or 0 indicatig error or closed connection
        int writtenTotal = 0;
        #define min(a,b) ((a)<(b)?(a):(b))
        while (bufferSize) {
          yield ();
          if (__socket__ == -1) return writtenTotal;
          switch (int written = send (__socket__, buffer, bufferSize, 0)) { // seems like ESP32 can send even larger packets
            case -1:
              // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] send error: %i, time-out: %lu in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), __socket__, errno, millis () - __lastActiveMillis__, __func__);
              #define EAGAIN 11
              #define ENAVAIL 119
              if (errno == EAGAIN || errno == ENAVAIL) {
                if ((__timeOutMillis__ == INFINITE) || (millis () - __lastActiveMillis__ < __timeOutMillis__)) {
                  delay (1);
                  break;
                }
              }
              // else close and continue to case 0
              // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] time-out: %lu in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), __socket__, millis () - __lastActiveMillis__, __func__);              
              __timeOut__ = true;
              closeConnection ();
            case 0:   // socket is already closed
              return writtenTotal;
            default:
              writtenTotal += written;
              buffer += written;
              bufferSize -= written;
              __lastActiveMillis__ = millis ();
              break;
          }
        }
        return writtenTotal;
      }

      int sendData (char string []) { return (sendData (string, strlen (string))); }
      
      int sendData (String string) { return (sendData ((char *) string.c_str (), strlen (string.c_str ()))); }
    
      bool timeOut () { return __timeOut__; } // if time-out occured
  
      void setTimeOut (TIME_OUT_TYPE timeOutMillis) { // user defined time-out if it differs from default one
        __timeOutMillis__ = timeOutMillis;
        __lastActiveMillis__ = millis ();
      }
  
      TIME_OUT_TYPE getTimeOut () { return __timeOutMillis__; } // returns time-out milliseconds

    private:
    
      void (* __connectionHandlerCallback__) (TcpConnection *, void *) = NULL;  // local copy of constructor parameters
      void *__connectionHandlerCallbackParamater__ = NULL;
      int __socket__ = -1;
      char __otherSideIP__ [46]; // keep it on the stack, 45 characters is enought for IPv6 although IPv6 is not supported (yet) // String __otherSideIP__;
      TIME_OUT_TYPE __timeOutMillis__;
  
      unsigned long __lastActiveMillis__ = millis ();                   // needed for time-out detection
      bool __timeOut__ = false;                                         // "time-out" flag
  
      enum CONNECTION_THREAD_STATE_TYPE {
        NOT_STARTED = 9,                                                // initial state
        NOT_THREADED = 8,                                               // the only state for not-threaded connections
        RUNNING = 1,                                                    // connection thread has started
        FINISHED = 2                                                    // connection thread has finished, instance can unload
      };
      
      CONNECTION_THREAD_STATE_TYPE __connectionState__ = TcpConnection::NOT_STARTED;
    
      static void __connectionHandler__ (void *threadParameters) {      // envelope for connection handler callback function
        TcpConnection *ths = (TcpConnection *) threadParameters;        // get "this" pointer
        if (ths->__connectionHandlerCallback__) ths->__connectionHandlerCallback__ (ths, ths->__connectionHandlerCallbackParamater__);
        ths->__connectionState__ = TcpConnection::FINISHED;
        delete (ths);                                                   // unload instnance, do not address any instance members form this point further
        vTaskDelete (NULL);
      }
  
  };

  // Arduino has serious problem with "new" - if it can not allocat memory for a new object it should
  // return a NULL pointer but it just crashes ESP32 instead. The way around it to test if there is enough 
  // memory available first, before calling "new". Since this is multi-threaded environment both should be
  // done inside a critical section. Each class we create will implement a function that would create a
  // new object and would follow certain rules. 

  inline TcpConnection *newTcpConnection (void (* connectionHandlerCallback) (TcpConnection *connection, void *parameter), 
                                          void *connectionHandlerCallbackParamater,                                        
                                          size_t stackSize,                                                                
                                          int socket,                                                                      
                                          String otherSideIP,                                                              
                                          TIME_OUT_TYPE timeOutMillis) {
    TcpConnection *p = NULL;
    xSemaphoreTakeRecursive (__newInstanceSemaphore__, portMAX_DELAY);
      if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) >= sizeof (TcpConnection) + 256) { // don't go below 256 B (live some memory for error messages ...)
        try {
          p = new TcpConnection (connectionHandlerCallback, connectionHandlerCallbackParamater, stackSize, socket, otherSideIP, timeOutMillis);   
        } catch (int e) {
          ; // threaded TcpConnection constructor throws exception if it can not start a new thread to handle the connection
        }
      }
    xSemaphoreGiveRecursive (__newInstanceSemaphore__);
    if (!p) dmesg ("[TcpConnection] out of memory, could not create thereaded TcpConnection instance."); // not enough memory for TcpConnection instance or for new connection task stack
    return p;
  }

  inline TcpConnection *newTcpConnection (int socket,
                                          String otherSideIP,
                                          TIME_OUT_TYPE timeOutMillis) {
    TcpConnection *p = NULL;
    xSemaphoreTakeRecursive (__newInstanceSemaphore__, portMAX_DELAY);
      if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) >= sizeof (TcpConnection) + 256) { // don't go below 256 B (live some memory for error messages ...)
        p = new TcpConnection (socket, otherSideIP, timeOutMillis); // not-threaded TcpConnection constructor does not throw any exception   
      }
    xSemaphoreGiveRecursive (__newInstanceSemaphore__);
    if (!p) dmesg ("[TcpConnection] out of memory, could not create not-thereaded TcpConnection instance."); // not enough memory for TcpConnection instance
    return p;
  }

  
  /*
      There are two types of TcpServers:
       
        Threaded TcpServers run listeners in their own threads, accept incoming connections and start new threaded TcpConnections for each one.
        This is an usual type of TcpServer.
      
        Not-threaded TcpServers run in threads of calling programs. They only wait or a single connection to arrive and handle it.
        This type of TcpServer is only used for pasive FTP data connections.

  */

  
  class TcpServer {                                                
    
    public:
  

      /*
    
        TcpServer constructor that creates threaded TcpServer. Listener runs in its own thread and creates a new thread for each
        arriving connection.

        TcpServer state is for instance internal use, only to assure proper closing of the listening socket and unloading of the instance. 
        Threaded TcpServer goes through the following states:
        
                                                                 (listener is accepting new
                                                                  connections - at this point
                                                                  we consider server as running    (destructor signaled
                                                                  and constructor can return        listener thread to        (listener thread has stopped, it does not access the instance
          (initial state)    (listener thread is running)         a success - not NULL pointer)     stop)                      internal memory any more so it is safe to unload the instance)
          NOT_RUNNING=9 -+------------> RUNNING=1 -------------> ACCEPTING_CONNECTIONS=2 ---------> UNLOADING=3 ----------+-> FINISHED=4
                         |                                                                                                |
                         +------------------------------------------------------------------------------------------------+
                         (creation of listener thread was not successful)

      */

      
      TcpServer      (void (* connectionHandlerCallback) (TcpConnection *connection, void *parameter),  // a reference to callback function that will handle the connection
                      void *connectionHandlerCallbackParameter,                                         // a reference to parameter that will be passed to connectionHandlerCallback
                      size_t connectionStackSize,                                                       // stack size of a thread where connection runs - this value depends on what server really does (see connectionHandler function) should be set appropriately (not to use too much memory and not to crush ESP)
                      TIME_OUT_TYPE timeOutMillis,                                                      // connection time-out in milli seconds
                      String serverIP,                                                                  // server IP address, 0.0.0.0 for all available IP addresses
                      int serverPort,                                                                   // server port
                      bool (* firewallCallback) (String connectingIP)                                   // a reference to callback function that will be celled when new connection arrives
                     ) {
        // copy constructor parameters to local structure
        __connectionHandlerCallback__ = connectionHandlerCallback;
        __connectionHandlerCallbackParameter__ = connectionHandlerCallbackParameter;
        __connectionStackSize__ = connectionStackSize;
        __timeOutMillis__ = timeOutMillis;
        strcpy (__serverIP__, serverIP.c_str ()); // __serverIP__ = serverIP;
        __serverPort__ = serverPort;
        __firewallCallback__ = firewallCallback;
  
        // start listener thread
        #define tskNORMAL_PRIORITY 1
        if (pdPASS == xTaskCreate (__listener__,
                                   "TcpServer",
                                   2048, // 2 KB stack is large enough for TCP listener
                                   this, // pass "this" pointer to static member function
                                   tskNORMAL_PRIORITY,
                                   NULL)) {
          while (__listenerState__ < TcpServer::ACCEPTING_CONNECTIONS) delay (1); // after this listener is accepting new connections
        } else {
          // __listenerState__ = TcpServer::FINISHED; // not really necessary since "throw" will cause constructor to fail and destructor will not get called at all
          throw 1; // out of memory
        }
      }
  

      /*
    
        TcpServer constructor that creates not-threaded TcpServer. Listener still runs in its own thread and creates only one TcpConnection
        once the connection occurs. When constructor returns there are two possible outcomes. If a connection is established then
        connection () member function returns a pointer to it and the calling program can use it to control the communication. If time-out
        occurs while waiting for a connection then connection () member function returns NULL;

        The only practical use of not-threaded TcpServer is for handling passive data FTP connections.

        TcpServer state is for instance internal use, only to assure proper closing of the listening socket and unloading of the instance. 
        Not-threaded TcpServer goes through the following states:
        
                                                                 (listener is accepting new
                                                                  connections - at this point
                                                                  we consider server as running    
                                                                  and constructor can return       (listener thread has stopped, it does not access the instance
          (initial state)    (listener thread is running)         a success - not NULL pointer)     internal memory any more so it is safe to unload the instance)
          NOT_RUNNING=9 -+------------> RUNNING=1 -------------> ACCEPTING_CONNECTIONS=2 -------+-> FINISHED=4
                         |                                                                      |
                         +----------------------------------------------------------------------+
                         (creation of listener thread was not successful)

      */

      
      TcpServer (TIME_OUT_TYPE timeOutMillis,                    // connection time-out in milli seconds
                 String serverIP,                                // server IP address, 0.0.0.0 for all available IP addresses
                 int serverPort,                                 // server port
                 bool (* firewallCallback) (String connectingIP) // a reference to callback function that will be celled when new connection arrives
                ) {
        // copy constructor parameters to local structure
        __timeOutMillis__ = timeOutMillis;
        strcpy (__serverIP__, serverIP.c_str ()); // __serverIP__ = serverIP;
        __serverPort__ = serverPort;
        __firewallCallback__ = firewallCallback;
        
        // start listener thread
        #define tskNORMAL_PRIORITY 1
        if (pdPASS == xTaskCreate (__listener__,
                                   "TcpServer",
                                   2048, // 2 KB stack is large enough for TCP listener
                                   this, // pass "this" pointer to static member function
                                   tskNORMAL_PRIORITY,
                                   NULL)) {
          while (__listenerState__ < TcpServer::FINISHED) delay (1); // listener has already accepted the connection or time-out occured
        } else {
          // __listenerState__ = TcpServer::FINISHED; // not really necessary since "throw" will cause constructor to fail and destructor will not get called at all
          throw 1; // out of memory
        }
      }
  
      virtual ~TcpServer () {
        if (__connection__) delete (__connection__); // close not-threaded connection if it has been established
        xSemaphoreTake (__TcpServerSemaphore__, portMAX_DELAY);
          if (__listenerState__ < TcpServer::UNLOADING) __listenerState__ = TcpServer::UNLOADING; // signal __listener__ to stop
        xSemaphoreGive (__TcpServerSemaphore__);
        while (__listenerState__ < TcpServer::FINISHED) delay (1); // wait for __listener__ to finish before releasing the memory occupied by this instance
      }
  
      String getServerIP () { return __serverIP__; } // information from constructor
  
      int getServerPort () { return __serverPort__; } // information from constructor
  
      TcpConnection *connection () { return __connection__; } // it make sense for not-threaded TcpServer only since it accepts only one connection: calling program will handle the connection through this reference
  
      bool timeOut () { // not-threaded TcpServer only: returns true if time-out has occured while not-threaded TcpServer is waiting for a connection
        if (__threadedMode__ ()) return false; 
        if (__timeOutMillis__ == INFINITE) return false;
        if (millis () - __lastActiveMillis__ > __timeOutMillis__) return true;
        return false;
      }

    private:
      friend class telnetServer;
      friend class ftpServer;
      friend class httpServer;
      friend class httpsServer;
  
      void (* __connectionHandlerCallback__) (TcpConnection *, void *) = NULL; // local copy of constructor parameters
      void *__connectionHandlerCallbackParameter__ = NULL;
      size_t __connectionStackSize__;
      TIME_OUT_TYPE __timeOutMillis__;
      char __serverIP__ [46]; // keep it on the stack, 45 characters is enought for IPv6 although IPv6 is not supported (yet) // String __serverIP__;
      int __serverPort__;
      bool (* __firewallCallback__) (String);
  
      TcpConnection *__connection__ = NULL;                           // pointer to TcpConnection instance (not-threaded TcpServer only)
      enum LISTENER_STATE_TYPE {
        NOT_RUNNING = 9,                                              // initial state
        RUNNING = 1,                                                  // preparing listening socket to start accepting connections
        ACCEPTING_CONNECTIONS = 2,                                    // listening socket started accepting connections
        UNLOADING = 3,                                                // instance is unloading
        FINISHED = 4                                                  // listener thread has finished, instance can unload
      };
      LISTENER_STATE_TYPE __listenerState__ = NOT_RUNNING;
  
      unsigned long __lastActiveMillis__ = millis ();                 // used for time-out detection (not-threaded TcpServer only)
  
      bool __threadedMode__ () { return (__connectionHandlerCallback__ != NULL); } // returns true if server is working in threaded mode - in this case it has to have a callback function
  
      static void __listener__ (void *taskParameters) { // listener running in its own thread imlemented as static memeber function
        TcpServer *ths = (TcpServer *) taskParameters; // get "this" pointer 
        ths->__listenerState__ = TcpServer::RUNNING; 
        int listenerSocket = -1;
        while (ths->__listenerState__ <= TcpServer::ACCEPTING_CONNECTIONS) { // prepare listener socket - repeat this in a loop in case something goes wrong
          // make listener TCP socket (SOCK_STREAM) for internet protocol family (PF_INET) - protocol family and address family are connected (PF__INET protokol and AF_INET)
          listenerSocket = socket (PF_INET, SOCK_STREAM, 0);
          if (listenerSocket == -1) {
            delay (1000); // try again after 1 s
            continue;
          }
          // make address reusable - so we won't have to wait a few minutes in case server will be restarted
          int flag = 1;
          setsockopt (listenerSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));
          // bind listener socket to IP address and port
          struct sockaddr_in serverAddress;
          memset (&serverAddress, 0, sizeof (struct sockaddr_in));
          serverAddress.sin_family = AF_INET;
          serverAddress.sin_addr.s_addr = inet_addr (ths->getServerIP ().c_str ());
          serverAddress.sin_port = htons (ths->getServerPort ());
          if (bind (listenerSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) goto terminateListener;
          // mark socket as listening socket
          #define BACKLOG 16 // queue lengthe of (simultaneously) arrived connections - actual active connection number might be larger 
          if (listen (listenerSocket, BACKLOG) == -1) goto terminateListener;
          // make socket not-blocking
          if (fcntl (listenerSocket, F_SETFL, O_NONBLOCK) == -1) goto terminateListener;
          // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] listener started on %s:%i\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), listenerSocket, ths->getServerIP ().c_str (), ths->getServerPort ());
          while (ths->__listenerState__ <= TcpServer::ACCEPTING_CONNECTIONS) { // handle incoming connections
            delay (1);
            if (!ths->__threadedMode__ () && ths->timeOut ()) goto terminateListener; // checking time-out makes sense only when working as not-threaded TcpServer

            // accept new connection
            xSemaphoreTake (__TcpServerSemaphore__, portMAX_DELAY);
              if (ths->__listenerState__ < TcpServer::ACCEPTING_CONNECTIONS) ths->__listenerState__ = TcpServer::ACCEPTING_CONNECTIONS; 
            xSemaphoreGive (__TcpServerSemaphore__);
            int connectionSocket;
            struct sockaddr_in connectingAddress;
            socklen_t connectingAddressSize = sizeof (connectingAddress);
            connectionSocket = accept (listenerSocket, (struct sockaddr *) &connectingAddress, &connectingAddressSize);
            if (connectionSocket != -1) { // not-blocking socket keeps returning -1 until new connection arrives
              if (ths->__firewallCallback__ && !ths->__firewallCallback__ (inet_ntos (connectingAddress.sin_addr))) {
                close (connectionSocket);
                dmesg ("[TcpServer] firewall rejected connection from " + inet_ntos (connectingAddress.sin_addr));
                continue;
              }
              if (fcntl (connectionSocket, F_SETFL, O_NONBLOCK) == -1) {
                close (connectionSocket);
                continue;
              }

              TcpConnection *p = newTcpConnection (ths->__connectionHandlerCallback__, ths->__connectionHandlerCallbackParameter__, ths->__connectionStackSize__, connectionSocket, inet_ntos (connectingAddress.sin_addr), ths->__timeOutMillis__); 
              if (!p) close (connectionSocket); // if constructor did't succeed close the socket, otherwise the instance will close it itself in destructor
              if (!ths->__threadedMode__ ()) {  // in not-threaded mode ... 
                ths->__connection__ = p;        // ... we only accept (and remember) one connection ...
                goto terminateListener;         // ... and stop listening for new ones
              }

            } // new connection
          } // handle incoming connections
        } // prepare listener socket
  terminateListener:
        close (listenerSocket);
        xSemaphoreTake (__TcpServerSemaphore__, portMAX_DELAY);
          ths->__listenerState__ = TcpServer::FINISHED;
        xSemaphoreGive (__TcpServerSemaphore__);
        vTaskDelete (NULL); // terminate this thread
      }
  
  };

  // Arduino has serious problem with "new" - if it can not allocat memory for a new object it should
  // return a NULL pointer but it just crashes ESP32 instead. The way around it to test if there is enough 
  // memory available first, before calling "new". Since this is multi-threaded environment both should be
  // done inside a critical section. Each class we create will implement a function that would create a
  // new object and would follow certain rules. 

  inline TcpServer *newTcpServer (void (* connectionHandlerCallback) (TcpConnection *connection, void *parameter),
                                  void *connectionHandlerCallbackParameter,
                                  size_t connectionStackSize,
                                  TIME_OUT_TYPE timeOutMillis,
                                  String serverIP,
                                  int serverPort,
                                  bool (* firewallCallback) (String connectingIP)) {
    TcpServer *p = NULL;
    xSemaphoreTakeRecursive (__newInstanceSemaphore__, portMAX_DELAY);
      if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) >= sizeof (TcpServer) + 256 ) { // don't go below 256 B, just in case ...
        try {
          p = new TcpServer (connectionHandlerCallback, connectionHandlerCallbackParameter, connectionStackSize, timeOutMillis, serverIP, serverPort, firewallCallback);   
        } catch (int e) {
          ; // threaded TcpServer constructor throws exception if it can not start a new listener thread
        }
      }
    xSemaphoreGiveRecursive (__newInstanceSemaphore__);
    if (!p) dmesg ("[TcpServer] out of memory, could not create thereaded TcpServer instance."); // not enough memory for TcpServer instance or for new listner task stack
    return p;
  }

  inline TcpServer *newTcpServer (TIME_OUT_TYPE timeOutMillis,
                                  String serverIP,
                                  int serverPort,
                                  bool (* firewallCallback) (String connectingIP)) {
    TcpServer *p = NULL;
    xSemaphoreTakeRecursive (__newInstanceSemaphore__, portMAX_DELAY);
      if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) >= sizeof (TcpServer) + 256) { // don't go below 256 B, just in case ...
        try {
          p = new TcpServer (timeOutMillis, serverIP, serverPort, firewallCallback);   
        } catch (int e) {
          ; // not-threaded TcpServer constructor throws exception if it can not start a new listener thread
        }
      }
    xSemaphoreGiveRecursive (__newInstanceSemaphore__);
    if (!p) dmesg ("[TcpServer] out of memory, could not create not-thereaded TcpServer instance."); // not enough memory for TcpServer instance or for new listner task stack
    return p;
  }
  
      
  /*

    TcpClient creates not-threaded TcpConnection. 
    
    Successful instance creation can be tested using connection () != NULL after constructor returns, but that doesn't mean that
    the TcpConnections is already established, it has only been requested so far. If connection fails it will be shown through
    TcpConnection time-out (). Therefore it may not be a good idea to set time-out to INFINITE.

  */
  

  class TcpClient {
  
    public:
  
      TcpClient (String serverName,                            // server name or IP address
                 int serverPort,                               // server port
                 TIME_OUT_TYPE timeOutMillis)                  // connection time-out in milli seconds
      {
        // get server IP address
        IPAddress serverIP;
        if (!WiFi.hostByName (serverName.c_str (), serverIP)) { 
          // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] hostByName error in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), connectionSocket, __func__);
          dmesg ("[TcpClient] hostByName could not find host " + serverName);
          throw 1;
        } 
                              
        // make TCP socket (SOCK_STREAM) for internet protocol family (PF_INET) - protocol family and address family are connected (PF__INET protokol and AF__INET)
        int connectionSocket = socket (PF_INET, SOCK_STREAM, 0);
        if (connectionSocket == -1) {
          // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] socket error in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), connectionSocket, __func__);
          dmesg ("[TcpClient] socket error.");
          throw 2;
        }
        // make the socket not-blocking - needed for time-out detection
        if (fcntl (connectionSocket, F_SETFL, O_NONBLOCK) == -1) {
          // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] fcntl error in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), connectionSocket, __func__);
          dmesg ("[TcpClient] fcntl error.");          
          close (connectionSocket);
          throw 3;
        }
        // connect to server
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons (serverPort);
        serverAddress.sin_addr.s_addr = inet_addr (serverIP.toString ().c_str ());
        if (connect (connectionSocket, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1) {
          // DEBUG: Serial.printf ("[Thread: %lu][Core: %i][Socket: %i] connect error in %s\n", (unsigned long) xTaskGetCurrentTaskHandle (), xPortGetCoreID (), connectionSocket, __func__);
          #define EINPROGRESS 119
          if (errno != EINPROGRESS) {
            dmesg ("[TcpClient] connect error " + String (errno) + ".");          
            close (connectionSocket);
            throw 4;
          }
        } // it is likely that socket is not open yet at this point
        __connection__ = newTcpConnection (connectionSocket, serverIP.toString (), timeOutMillis); // load not-threaded TcpConnection instance
        if (!__connection__) {
          dmesg ("[TcpClient] could not create a new TcpConnection.");
          close (connectionSocket);
          throw 5;
        }
        // success
      }
  
      ~TcpClient () { delete (__connection__); } // destructor is called only if constructor succeeds
  
      TcpConnection *connection () { return __connection__; }         // calling program will handle the connection through this reference - connection is set, before constructor returns but TCP connection may not be established (yet) at this time or possibly even not at all

    private:
  
      TcpConnection *__connection__ = NULL;                           // TcpConnection instance used by TcpClient instance
  
  };

  // Arduino has serious problem with "new" - if it can not allocat memory for a new object it should
  // return a NULL pointer but it just crashes ESP32 instead. The way around it to test if there is enough 
  // memory available first, before calling "new". Since this is multi-threaded environment both should be
  // done inside a critical section. Each class we create will implement a function that would create a
  // new object and would follow certain rules. 

  inline TcpClient *newTcpClient (String serverName,
                                  int serverPort,
                                  TIME_OUT_TYPE timeOutMillis) {
    TcpClient *p = NULL;
    xSemaphoreTakeRecursive (__newInstanceSemaphore__, portMAX_DELAY);
      if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) >= sizeof (TcpClient) + 256) { // don't go below 256 B (live some memory for error messages ...)
        try {
          p = new TcpClient (serverName, serverPort, timeOutMillis);
        } catch (int e) {
          ; // TcpClient throws exceptions if constructor fails
        }
      }
    xSemaphoreGiveRecursive (__newInstanceSemaphore__);
    if (!p) dmesg ("[TcpClient] could not create TcpClient instance."); // not enough memory for TcpClient instance or failed to create TcpConnection instance
    return p;
  }
  
#endif
