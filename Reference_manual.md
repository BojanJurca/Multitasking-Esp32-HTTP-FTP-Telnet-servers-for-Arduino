# Programmer's reference manual for network.h


## startWiFi () function

startWiFi reads WiFi configuration parameters from files: /network/interfaces, /etc/wpa_supplicant/wpa_supplicant.conf, /etc/dhcpcd.conf and /etc/hostapd/hostapd.conf and starts WiFi accordingly. These files are pretty much the same as it is normal in Linux and can be edited by vi telnet command. If the file system is not mounted then it just take network configuration parameters from #definitions: DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD, DEFAULT_AP_SSID (#define it as "" if you don't want AP), DEFAULT_AP_PASSWORD (at least 8 charCTERS), HOSTNAME, DEFAULT_STA_IP, DEFAULT_STA_SUBNET_MASK, DEFAULT_STA_GATEWAY, DEFAULT_STA_DNS_1, DEFAULT_STA_DNS_2, DEFAULT_AP_IP, DEFAULT_AP_SUBNET_MASK. When being called the first time after the file system is formatted it creates configuration files with default #definitions.

**Declaration**

```C++
void startWiFi ();
```
 
## ifconfig () function

**Declaration**

```C++
String ifconfig ();
```

**Return value**

ifconfig returns basic network configuration information, the result is the same as with ifconfig telnet command.


## ping (String, int, int, int, int, TcpConnection) function

ping function pings target with ping packets and waits for the replies. ping function is also called internally by the ping telnet command.

**Declaration**

```C++
uint32_t ping (String targetName, int pingCount = 1, int pingInterval = 1, int pingSize = 32, int timeOut = 1, TcpConnection *telnetConnection = NULL);
```

**Parameters**

  - String targetName is target device name or IP number to be pinged.

  - int pingCount = 1 is the number of ping packets to be sent.

  - int pingInterval = 1 is the number of seconds between ping packets.  

  - int pingSize = 32 is the number of bytes in the ping packet.

  - int timeOut = 1 is the number of seconds ping is supposed to wait for a reply from the target.

  - TcpConnection *telnetConnection = NULL is only used by the telnet server to let ping display intermediate results. It should be left NULL otherwise.

**Return value**

ping returns the number of packets received from target.


## arp () function

**Declaration**

```C++
String arp ();
```

**Return value**

arp returns the current content of ARP table, the result is the same as with arp telnet command.


## iw (TcpConnection) function

**Declaration**

```C++
String iw (TcpConnection *connection = NULL)
```

**Parameter**

   - TcpConnection *connection = NULL is only used by the telnet server to display intermediate results. It should be left NULL otherwise.

**Return value**

iw returns some WiFi information, the result is the same as with iw telnet command.



# Programmer's reference manual for webServer.hpp


## httpServer class constructor


**Declaration**

```C++
httpServer (String (*httpRequestHandler) (String&, httpServer::wwwSessionParameters *), void (*wsRequestHandler) (String&, WebSocket *), size_t, String, int, bool (*firewallCallback) (String))
```

**Parameters**

  - String (*httpRequestHandler) (String& httpRequest, httpServer::wwwSessionParameters *wsp) is a pointer to the callback function provided by the calling program. Pass NULL if you are not going to use httpRequestHandler. Please see examples to learn how to use the httpRequestHandler function and wwwSessionParameters class.  

  - void (*wsRequestHandler) (String& wsRequest, WebSocket *webSocket) is a pointer to the callback function provided by the calling program. Pass NULL if you are not going to use wsRequestHandler. Please see examples to learn how to use wsRequestHandler function and WebSocket class.

  - size_t stackSize is the stack size that each HTTP session is going to use. 8 KB will usually do but if your httpRequestHandler or wsRequestHandler need more, increase this value until the server is stable.

  - String serverIP is the IP address which httpServer will be using. Pass "0.0.0.0" for all IP addresses available to httpServer.

  - int serverPort is the port number httpServer will be using. The default HTTP port is 80.

  - bool (*firewallCallback) (String connectingIP) is a pointer to the callback function provided by the calling program. Pass NULL if you are not going to use firewallCallback. Please see examples to learn how to use firewallCallback function.


**Testing success**

Test started () member function after the constructor returns.


**Example**

```C++
httpServer *httpSrv = new httpServer (NULL, NULL, 8 * 1024, "0.0.0.0", 80, NULL);

if (!httpSrv || (httpSrv && !httpSrv->started ())) Serial.println ("httpServer did not start.");
```


## httpServer class destructor


**Declaration**

```C++
~httpServer ()
```


***Note***

All active HTTP and WS sessions will continue to run even after destructor returns, until they end by themselves.


## wwwSessionParameters class members

**String homeDir**

Web server home directory. This is where .html and other files are stored.


**TcpConnection *connection**

TcpConnection instance for lower level programming if needed.


**String getHttpRequestHeaderField (String fieldName)**

Extracts and returns field value form accepted HTTP request header. Returns "" if the field is not found in the HTTP header.


**String getHttpRequestCookie (String cookieName)**

Extracts and returns cookie value accepted from HTTP request header. Returns "" if the cookie is not found in the HTTP header.


**String httpResponseStatus = "200 OK"**

The response status that will be sent to the client browser. By default it is "200 OK" but you can set it to anything else that the client browser would know how to handle.


**void setHttpResponseHeaderField (String fieldName, String fieldValue)**

Sets a HTTP response header field that would be sent to the client browser together with HTTP reply.


**void setHttpResponseCookie (String cookieName, String cookieValue, time_t expires = 0, String path = "/")**

Sets cookie value that would be sent to the client browser together with HTTP reply.

time_t expires is an optional expirational date for the cookie. If used it should be in GMT. A cookie with no expiration date specified will expire when the browser is closed.

path is optional filter for pages to which the cookie applies.


## WebSocket class members

Please note that not all websocket frames are supported. Out of 3 frame sizes only the first 2 are supported, the third is too large for ESP32 anyway. Continuation frames are not supported hence all the exchange information should fit into one frame. Beside this only non-control frames are supported. Basically WebSocket is a TCP connection with some additional reading and sending functions like sending and receiving String information or sending and receiving binary data.


**String getWsRequest ()**

Returns websocket request.


**Declaration**

```C++
String getWsRequest ();
```


**void closeWebSocket ()**

Closes websocket.


**Declaration**

```C++
String void closeWebSocket ();
```

**bool isOpen ()**

Returns status of websocket.


**Declaration**

```C++
bool isOpen ();
```


**WEBSOCKET_DATA_TYPE enumeration**

```C++
enum WEBSOCKET_DATA_TYPE {
  NOT_AVAILABLE = 0,          // no data is available to be read 
  STRING = 1,                 // text data is available to be read
  BINARY = 2,                 // binary data is available to be read
  CLOSE = 8,        
  ERROR = 3
};
```


**WEBSOCKET_DATA_TYPE available ()**

Checks if there is data available to be read. Returns the type of data that is available.


**Declaration**

```C++
WEBSOCKET_DATA_TYPE available ();
```


**String readString ()**

Reads String data tape from websocket. If String is not available the function blocks until String arrives.


**Declaration**

```C++
String readString ()
```


**Return value**

An UTF-8 string in case of success. "" in case of error or closed connection.


**size_t binarySize ()**

Returns the number of bytes of binary data ready to be read.


**Declaration**

```C++
size_t binarySize ();
```


**Return value**

Returns the number of bytes that has arrived from the browser or 0 if data is not ready (yet) to be read.


**size_t readBinary (byte *, size_t)**

Reads binary data from websocket.


**Declaration**

```C++
size_t readBinary (byte *buffer, size_t bufferSize);
```


**Parameters**

  - byte *buffer is a pointer to a buffer that will receive binary data.

  - size_t bufferSize it the size of the buffer.


**Return value**

Function returns number bytes copied into buffer.


**bool sendString (String)**

Sends a String data to websocket.


**Declaration**

```C++
bool sendString (String text);
```

**Parameter**

  - String text is String data to be sent.


**Return value**

Function returns success.


**bool sendBinary (byte *, size_t)**

Sends binary data to websocket.


**Declaration**

```C++
bool sendBinary (byte *buffer, size_t bufferSize);
```


**Parameters**

  - byte *buffer is a pointer to a buffer containing binary data.

  - size_t bufferSize it the number of bytes binary data in the buffer.


**Return value**

Function returns success.


## String webClient (String, int, TIME_OUT_TYPE, String);


**Description**

webClient passes HTTP request to (remote) web server and returns its reply.


**Declaration**

```C++
String webClient (String serverName, int serverPort, TIME_OUT_TYPE timeOutMillis, String httpRequest);
```


**Parameter**

  - String serverName is the name of (remote) web server.

  - int serverPort is the port number of (remote) web server.

  - TIME_OUT_TYPE timeOutMillis is the number of milliseconds webClient is going to wait for a reply before returning with error. Pass INFINITE if you don’t intend to use time-out detection.

  - String httpRequest is a HTTP request to be passed to a (remote) web server. webClient will add the necessary ending "\r\n\r\n" to httpRequest itself so you don’t need to include it in your request.


**Return value**

  - HTTP reply (including HTTP header) if succeeded

  - "" in case of error. You can check dmesg for error description.


**Example**

```C++
String s = webClient ("127.0.0.1", 80, (TIME_OUT_TYPE) 5000, "GET /builtInLed");

if (s > "") Serial.printf ("%s\r\n", s.c_str ());
else Serial.printf ("HTTP reply didn't arrive (in time) or it is corrupt or too long\r\n");
```


# Programmer's reference manual for telnetServer.hpp


## void dmesg (String)


**Declaration**

```C++
void dmesg (String message);
```


**Parameter**

  - String message is an ESP32 server message to be added to the message queue. Queued messages can be displayed using dmesg telnet command.


## telnetServer class constructor


**Declaration**

```C++
telnetServer (String (*telnetCommandHandler) (int argc, String argv [], telnetServer::telnetSessionParameters *tsp), size_t stackSize, String serverIP, int serverPort, bool (*firewallCallback) (String connectingIP))
```


**Parameters**

  - String (*telnetCommandHandler) (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) is a pointer to the callback function provided by the calling program. Pass NULL if you are not going to use telnetCommandHandler. Please see examples to learn how to use telnetCommandHandler function and telnetSessionParameters structure.  

  - size_t stackSize is the stack size that each telnet session is going to use. 16 KB will usually do but if your telnetCommandHandler needs more increase this value until the server is stable. If you are not going to use stack consuming telnet commands like tree or vi stackSize can be much smaller.   

  - String serverIP is the IP address which telnetServer will be using. Pass "0.0.0.0" for all IP addresses available to telnetServer.

  - int serverPort is the port number telnetServer will be using. The default HTTP port is 23.

  - bool (*firewallCallback) (String connectingIP) is a pointer to the callback function provided by the calling program. Pass NULL if you are not going to use firewallCallback. Please see examples to learn how to use firewallCallback function.


**Testing success**
Test started () member function after the constructor returns.


**Example**

```C++
telnetServer *telnetSrv = new telnetServer (NULL, 16 * 1024, "0.0.0.0", 23, NULL);

if (!telnetSrv || (telnetSrv && !telnetSrv->started ())) Serial.println ("telnetServer did not start."); 
```


## telnetServer class destructor


**Declaration**

```C++
~telnetServer ()
```


***Note***

All active telnet sessions will continue to run even after destructor returns, until they end by themselves.


## telnetServer member functions and structures

**telnetSessionParameters**

typedef struct {
  String userName;            // the name of logged in user
  String homeDir;             // user's home directory determines the level of access rights
  String workingDir;          // user's current working directory
  char *prompt;               // prompt character
  TcpConnection *connection;  // TcpConnection instance for lower level programming if needed
  byte clientWindowCol1;      // telnet client's window size
  byte clientWindowCol2;      // telnet client's window size
  byte clientWindowRow1;      // telnet client's window size
  byte clientWindowRow2;      // telnet client's window size
  // feel free to add more
} telnetSessionParameters;


# Programmer's reference manual for ftpServer.hpp


## ftpServer class constructor


**Declaration**

```C++
ftpServer (String serverIP, int serverPort, bool (* firewallCallback) (String connectingIP))
```


**Parameters**

  - String serverIP is the IP address which ftpServer will be using. Pass "0.0.0.0" for all IP addresses available to ftpServer.

  - int serverPort is the port number ftpServer will be using for control connection. The default FTP port is 21.

  - bool (*firewallCallback) (String connectingIP) is a pointer to the callback function provided by the calling program. Pass NULL if you are not going to use firewallCallback. Please see examples to learn how to use firewallCallback function.


**Testing success**
Test started () member function after the constructor returns.


**Example**

```C++
ftpServer *ftpSrv = new ftpServer ("0.0.0.0", 21, NULL);

if (!ftpSrv || (ftpSrv && !ftpSrv->started ())) Serial.println ("ftpServer did not start."); 
```


## ftpServer class destructor


**Declaration**

```C++
~ftpServer ()
```


***Note***

All active FTP sessions will continue to run even after destructor returns, until they end by themselves.


# Programmer's reference manual for TcpServer.hpp


## TIME_OUT_TYPE enumeration

```C++
enum TIME_OUT_TYPE: unsigned long {
  INFINITE = 0  // infinite time-out
                // everything else is time-out in milliseconds
};
```


## TcpServer class threaded constructor

TcpServer class has two constructors: threaded and non-threaded. The TcpServer constructor creates a new thread/task/process for the listener and a new thread/task/process for each TcpConnection it creates when new TCP connections are requested by connecting clients. Non-threaded constructor creates a new thread/task/process only for the listener and accepts only one TCP connection. The handling of TcpConnection is left to the calling program. The only use of the latter I found is for handling passive data FTP connections.


## TcpServer class constructor


**Declaration**

```C++
TcpServer (void (* connectionHandlerCallback) (TcpConnection *, void *), void *connectionHandlerCallbackParameter, size_t connectionStackSize, TIME_OUT_TYPE timeOutMillis, String serverIP, int serverPort, bool (* firewallCallback) (String connectingIp));
```

**Parameters**

  - void (* connectionHandlerCallback) (TcpConnection *, void *parameter) is a pointer to callback function provided by a calling program through which TCP connections would be managed.

  - void *connectionHandlerCallbackParamater is a reference to parameter that will be passed to connectionHandlerCallback function.

  - size_t stackSize is the stack size that the newly created thread/task/process for each TCP connection is going to use.

  - TIME_OUT_TYPE timeOutMillis is the maximum inactive time before TCP connections get closed.

  - String serverIP is the IP address server listener process is going to use, pass "0.0.0.0" for all available IP addresses.

  - int serverPort is the port number the listener process is going to use.

  - bool (* firewallCallback) (String connectingIP) is a reference to a callback function that will be called when a new connection arrives. If firewallCallback returns false the connection will be rejected.


**Testing success**

Test started () member function after the constructor returns.


## TcpServer class non-threaded constructor

TcpServer class has two constructors: threaded and non-threaded. The TcpServer constructor creates a new thread/task/process for the listener and a new thread/task/process for each TcpConnection it creates when new TCP connections are requested by connecting clients. Non-threaded constructor creates a new thread/task/process only for the listener and accepts only one TCP connection. The handling of TcpConnection is left to the calling program. The only use of the latter I found is for handling passive data FTP connections.

**Declaration**

```C++
TcpServer (TIME_OUT_TYPE timeOutMillis, String serverIP, int serverPort, bool (* firewallCallback) (String connectingIP));
```

**Parameters**

  - TIME_OUT_TYPE timeOutMillis is the maximum inactive time before TCP connections get closed.

  - String serverIP is the IP address server listener process is going to use, pass "0.0.0.0" for all available IP addresses.

  - int serverPort is the port number the listener process is going to use.

  - bool (* firewallCallback) (String connectingIP) is a reference to a callback function that will be called when a new connection arrives. If firewallCallback returns false the connection will be rejected.



## TcpServer class destructor


**Declaration**

```C++
~TcpServer ()
```


***Note***

All active TCP connections will continue to run even after destructor returns, until they end by themselves.


## TcpServer member functions and structures

**bool started ()**

Provides information if TcpServer started successfully.


**String getServerIP ()**

Provides IP address ESP32 server is using.


**int getServerPort ()**

Provides port number ESP32 server is using.


**TcpConnection *connection ()**

Non-threaded TcpServers only, since non-threaded TcpServers only accept one TCP connection: provides reference to this connection when it is established.


**bool timeOut ()**

Non-threaded TcpServers only, since non-threaded TcpServers only accept one TCP connection: provides information if time-out occurred while waiting for a connection.


## TcpConnection class threaded constructor

TcpConnection class has two constructors: threaded and non-threaded. Threaded TcpConnection constructor creates a new thread/task/process for newly created TCP connection whereas non-threaded constructor uses the same thread/task/process that called the constructor. The first case is more useful for TCP servers whereas the second is more useful for TCP clients.


**Declaration**

```C++
TcpConnection (void (* connectionHandlerCallback) (TcpConnection *, void *), void *connectionHandlerCallbackParamater, size_t stackSize, int socket, String otherSideIP, TIME_OUT_TYPE timeOutMillis);
```


**Parameters**

  - void (* connectionHandlerCallback) (TcpConnection *, void *parameter) is a pointer to callback function provided by a calling program through which a new TCP connection would be managed.

  - void *connectionHandlerCallbackParamater is a reference to parameter that will be passed to connectionHandlerCallback function.

  - size_t stackSize is the stack size that the newly created thread/task/process is going to use to handle a new TCP connection.

  - int socket is already open socket

  - String otherSideIP is IP address of the other side of connection - it is just an information provided by calling program that can be used later
 
  - TIME_OUT_TYPE timeOutMillis is the maximum inactive time before a TCP connection gets closed.


**Testing success**

Test started () member function after the constructor returns.


## TcpConnection class non-threaded constructor

TcpConnection class has two constructors: threaded and non-threaded. Threaded TcpConnection constructor creates a new thread/task/process for newly created TCP connection whereas non-threaded constructor uses the same thread/task/process that called the constructor. The first case is more useful for TCP servers whereas the second is more useful for TCP clients.


**Declaration**

```C++
TcpConnection (int socket, String otherSideIP, TIME_OUT_TYPE timeOutMillis);
```


**Parameters**

  - int socket is already open socket

  - String otherSideIP is IP address of the other side of connection - it is just an information provided by calling program that can be used later
 
  - TIME_OUT_TYPE timeOutMillis is the maximum inactive time before a TCP connection gets closed.


## TcpConnection member functions and structures


**bool started ()**

Provides information if TCP connection started successfully.


**void closeConnection ()**

Closes TCP connection. If it is a threaded connection ths function also ends the thread/task/process created by constructor and unloads class instance as soon as connectionHandlerCallback function returns.


**isClosed ()**

Provides information if TCP connection is already closed.


**bool isOpen ()**

Provides information if TCP connection is still open.


**int getSocket ()**

Provides information about the sockets used by the class instance to handle TCP connection for lower level programming if needed.


**String getThisSideIP ()**

Provides IP address ESP32 server is using for TCP connection.

** String getOtherSideIP ()**

Provides IP address the connecting program is using for TCP connection.


**AVAILABLE_TYPE enumneration**

```C++
enum AVAILABLE_TYPE {
  NOT_AVAILABLE = 0,  // no data is available to be read
  AVAILABLE = 1,      // data is available to be read
  ERROR = 3           // error in communication
};
```


**AVAILABLE_TYPE available ()**

Checks if incoming data is pending to be read.


**int recvData (char *buffer, int bufferSize)**

Reads incoming data into buffer and returns the number of bytes actually received or 0 indicating error or closed connection.


**int sendData (char *buffer, int bufferSize)**

Sends data in the buffer and returns the number of bytes actually sent or 0 an indicating error or closed connection.


**int sendData (char string [])**

Sends C formatted string and returns the number of bytes actually sent or 0 an indicating error or closed connection.


**int sendData (String string)**

Sends Arduino String and returns the number of bytes actually sent or 0 indicating an error or closed connection.


**bool timeOut ()**

Provides information if time-out occurred.


**TIME_OUT_TYPE getTimeOut ()**

Returns current time-out setting.


**void setTimeOut (TIME_OUT_TYPE timeOutMillis)**

Sets time-out for class instance.


## TcpClient class constructor


**Declaration**

```C++
TcpClient (String serverName, int serverPort, TIME_OUT_TYPE timeOutMillis)
```


**Parameters**

  - String serverName is a server name or IP address.

  - int serverPort is a server port number.
 
  - TIME_OUT_TYPE timeOutMillis is the maximum inactive time before a TCP connection gets closed.


**Testing success**

Test connection () member function after the constructor returns.


## TcpClient member functions and structures


**TcpConnection *connection ()**

Provides a reference to established TcpConnection or NULL if failed.


# Programmer's reference manual for time_functions.h


## Compile time definitions

Before #include-ing "time_functions.h" the following definitions can be #defined:

```C++
#define TIMEZONE   CET_TIMEZONE   // this is the default definition
```

  - TIMEZONE tells ESP32 server how it should calculate local time from GMT. You can change the default setting to one of already supported time zones: KAL_TIMEZONE, MSK_TIMEZONE, SAM_TIMEZONE, YEK_TIMEZONE, OMS_TIMEZONE, KRA_TIMEZONE, IRK_TIMEZONE, YAK_TIMEZONE, VLA_TIMEZONE, SRE_TIMEZONE, PET_TIMEZONE, JAPAN_TIMEZONE, CHINA_TIMEZONE, WET_TIMEZONE, ICELAND_TIMEZONE, CET_TIMEZONE, EET_TIMEZONE, FET_TIMEZONE, NEWFOUNDLAND_TIMEZONE, ATLANTIC_TIME_ZONE, ATLANTIC_NO_DST_TIMEZONE, EASTERN_TIMEZONE, EASTERN_NO_DST_TIMEZONE, CENTRAL_TIMEZONE, CENTRAL_NO_DST_TIMEZONE, MOUNTAIN_TIMEZONE, MOUNTAIN_NO_DST_TIMEZONE, PACIFIC_TIMEZONE, ATLANTIC_NO_DST_TIMEZONE, EASTERN_TIMEZONE, CENTRAL_TIMEZONE, MOUNTAIN_TIMEZONE, PACIFIC_TIMEZONE, ALASKA_TIMEZNE, HAWAII_ALEUTIAN_TIMEZONE, HAWAII_ALEUTIAN_NO_DST_TIMEZONE, AMERICAN_SAMOA_TIMEZONE, BAKER_HOWLAND_ISLANDS_TIMEZONE, WAKE_ISLAND_TIMEZONE, CHAMORRO_TIMEZONE.

```C++
#define DEFAULT_NTP_SERVER_1   "1.si.pool.ntp.org"   // this is the default definition 
#define DEFAULT_NTP_SERVER_2   "2.si.pool.ntp.org"   // this is the default definition
#define DEFAULT_NTP_SERVER_3   "3.si.pool.ntp.org"   // this is the default definition
```

  - NTP_SERVER definitions tell ESP32 server where to get current time (GMT) from, when certain functions are being called (see startCronDaemon). You can change the default settings to NTP servers close to you. These #definitions will be written to /etc/ntp.conf file which ESP32 reads each time it starts up. 


## void setGmt (time_t);


**Description**

setGmt sets ESP32 built-in clock with current time (GMT). If setLocalTime has already been called there is no need of calling also setGmt. This function is also called internally by ntpDate function or ntpdate telnet command.


**Declaration**

```C++
void setGmt (time_t t);
```

**Parameter**

  - time_t t is the number of seconds that elapsed since midnight of January 1, 1970 (GMT).


## void setLocalTime (time_t);


**Description**

setLocalTime sets ESP32 built-in clock with current (local) time. It is different from setGmt only in parameter which holds local time instead of GMT (this function is also called internally by date telnet command). If setGmt has already been called there is no need of calling also setLocalTime. This function is also called internally by date telnet command.


**Declaration**

```C++
void setLocalTime (time_t t);
```


**Parameter**

  - time_t t is the number of seconds that elapsed since midnight of January 1, 1970 (local time).


## time_t getGmt ();


**Description**

getGmt returns the state of ESP32 built-in clock in GMT. 


**Declaration**

```C++
time_t getGmt ();
```


**Return value**

  - 0 if current time has not been set yet (by calling setGmt or setLocalTime)

  - GMT otherwise


## time_t getLocalTime ();


**Description**

getLocalTime returns the state of ESP32 built-in clock in local time. 


**Declaration**

```C++
time_t getLocalTime ();
```


**Return value**

  - 0 if current time has not been set yet (by calling setGmt or setLocalTime)
  - local time otherwise


## time_t timeToLocalTime (time_t);


**Description**

timeToLocalTime coverts GMT to local time. This function is the only function that does the conversion. It is affected by TIMEZONE #definition.


**Declaration**

```C++
time_t timeToLocalTime (time_t t);
```


**Parameter**

  - time_t t is time (GMT) to be converted to local time.


**Return value**

  - local time converted from GMT.


## struct tm timeToStructTime (time_t);


**Description**

timeToStructTime converts time_t to struct tm. 


**Declaration**

```C++
struct tm timeToStructTime (time_t t);
```


**Parameter**

  - time_t t is the time to be converted to struct tm.


**Return value**

  - struct_tm converted from time_t. 


## String timeToString (time_t);


**Description**

timeToString converts time_t to String. 


**Declaration**

```C++
String timeToString (time_t t);
```


**Parameter**

  - time_t t is the time to be converted to String.


**Return value**

  - String in a form of YYYY/MM/DD hh:mm:ss (24 hour format). 


## String ntpDate (String);   


**Description**

ntpDate synchronizes ESP32 built-in clock with NTP server. ESP32 must have an internet connection before ntpDate is called. This function is also called internally by ntpdate telnet command.


**Declaration**

```C++
String ntpDate (String ntpServer);
```


**Parameter**

  - String ntpServer is the name of NTP server by which ESP32 built-in clock is to be synchronized.


**Return value**

  - "" if synchronization succeeded
  - error message if not


## String ntpDate ();


**Description**

ntpDate synchronizes ESP32 built-in clock with one of NTP servers specified in /etc/ntp.conf configuration file. ESP32 must have an internet connection before ntpDate is called. File system must be mounted and /etc/ntp.conf configured. If this is not the case, then ntpDate will try the default NTP_SERVER #definitions. This function is also called internally by ntpdate telnet command and cronDaemon.


**Declaration**

```C++
String ntpDate ();
```


**Return value**

  - “” if synchronization succeeded
  - error message if not


## startCronDaemon (void (* cronHandler) (String&), size_t);


**Description**

startCronDaemon starts special task or process called cronDaemon that:

  - Updates internal ESP32 clock with NTP server(s) once a day.

  - constantly checks the content of cron table inside ESP32 server memory and calls cornHandler function when the time is right. cronDaemon will read its configuration from /etc/ntp.conf and /etc/crontab files when ESP32 starts up. If the files do not exist yet it will create new ones from compile time #definitions with default values.


**Declaration**

```C++
void startCronDaemon (void (* cronHandler) (String&), size_t cronStack = (3 * 1024));
```


**Parameters**

  - void (* cronHandler) (String&) is a callback function that cronDaemon will call as defined by cron table. Cron table resides in ESP32 memory and is filled with the content of /etc/crontab file each time ESP32 starts up. Beside this it can also be managed through cronTabAdd and cronTabDel functions. If you are not going to use a cron table then this parameter can be NULL.

  - size_t cronStack is stack size cronDaemon will be using. cronDaemon runs as separate thread/task/process so it needs its own stack. 3 KB is only enough for daily time synchronizations with NTP servers. If you are going to use cronHandler you better increase it to 8 KB. If your cronHandler is stack-hungry you may have to add more stack.


## bool cronTabAdd (uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, String, bool);


**Description**

cronTabAdd adds a new cron command definition into cron table.


**Declaration**

```C++
bool cronTabAdd (uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t day_of_week, String cronCommand, bool readFromFile = false); 
```


**Parameters**

  - uint8_t second defines the second (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t minute defines the minute (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t hour defines the hour (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t day defines the day of month (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t month defines the month of year (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t day_of_week defines the day of week (local time, Sunday = 1, …) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - String cronCommand is the croncommand that is going to be passed to cronHandler.

  - bool readFromFile = false is used internally, normally it should be left false.


**Return value**

  - true if succeeded

  - false if there is no more space in cron table. Cron table can include up to 32 entries, including those that are uploaded from /etc/crontab file during ESP32 start up.


## bool cronTabAdd (String, bool);


**Description**

cronTabAdd adds a new cron command definition into cron table.


**Declaration**

```C++
bool cronTabAdd (String cronTabLine, bool readFromFile = false); 
```

**Parameters**

  - String cronTabLine contains definition cron command and when it should be called in text format. This is the same format as in /etc/crontab file: 6 numbers and cron command text. The numbers correspond to second, minute, hour, day of month, month of year, day of week when cronHandler will be called with cronCommand parameter. If it is not important set it to *.

  - bool readFromFile = false is used internally, normally it should be left false.


**Return value**

  - true if succeeded

  - false if there is no more space in cron table. Cron table can include up to 32 entries, including those that are uploaded from /etc/crontab file during ESP32 start up.


## int cronTabDel (String);


**Description**

cronTabDel deletes cron command definition from cron table.


**Declaration**

```C++
int cronTabDel (String cronCommand);
```


**Parameters**

  - String cronCommand contains the nme of the command to be deleted from cron table.


**Return value**

The number of deleted cron tab entries. This number can be 0 (if cronCommand did not exist in cron table), 1 (if it did) and even > 1 since there may be more than 1 entry for the same cronCommand in cron table.


# Programmer's reference manual for smtpClient.h


## String sendMail (String, String, String, String, String, String, int, String);


**Description**

sendMail sends out an email message through SMTP server.


**Declaration**

```C++
String sendMail (String message = "", String subject = "", String to = "", String from = "", String password = "", String userName = "", int smtpPort = 0, String smtpServer = "");
```


**Parameters**

  - String message is the text in email body. It can be plain text, UTF-8 and HTML. 

  - String subject is the text in email Subject field.

  - String to are recipients' email addresses. This parameter is actualy a "to" field of SMTP protocol.    

  - String from is usually your email address. This field is actualy a "from" field of SMTP protocol. 

  - String password is your SMTP server password.

  - String userName is your SMTP server user name.

  - int smtpPort is your SMTP server port number. Defuult SMTP port is 25. 

  - String smtpServer is your SMTP server name.


**Return value**

  - success message from SMTP server if succeeded. SMTP success messages start with code 250.

  - error message if failed


***Note***

Unlike any other configuration file Esp32_web_ftp_telnet_server_template is using, /etc/mail/sendmail.cf is not generated by the template with default values. You have to create it manually yurself if you want to use it.


**Example (of /etc/mail/sendmail.cf)**

```
Hello 10.18.1.88!
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# mkdir /etc/mail
/etc/mail made.
#
#vi etc/mail/sendmail.cf

---+--------------------------------------------------------------------------------------------- Save: Ctrl-S, Exit: Ctrl-X
  1|# This configuration file contains default values for sendMail function. 
  2|# You can proivide all, some or none default values. 
  3|
  4|# SMTP server name (also -S field of sendmail telnet command)
  5|smtpServer   smtp.siol.net
  6|
  7|# SMTP port (also -P field of sendmail telnet command,  default SMTP port is 25)
  8|smtpPort     25
  9|
 10|# your user name for SMTP server (also -u field of sendmail telnet command)
 11|userName     yourSmtpServerUserName
 12|
 13|# your password for SMTP server (also -p field of sendmail telnet command)
 14|password     yourSmtpServerPassword
 15|
 16|# From field of SMTP protocol usually contains your email address (also -f field of sendmail telnet command)
 17|from         "My ESP32 Server"<my.name@somewhere.net>
 18|
 19|# To field of SMTP protocol (also -t field of sendmail telnet command)
 20|to           "me"<my.name@somewhere.net>, "other"<someone.else@somewhere.com>
 21|
 22|# Subject field of SMTP profocol (also -s field of sendmail telnet command)
 23|subject      Test
 24|
 25|# message body text (also -m field of sendmail telnet command)
 26|message      This is just a test message.
 27|
---+-------------------------------------------------------------------------------------------------------------------------
```