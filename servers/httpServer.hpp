/*

    httpServer.hpp
  
    This file is part of Multitasking Esp32 HTTP FTP http servers for Arduino project: https://github.com/BojanJurca/Multitasking-Esp32-HTTP-FTP-http-servers-for-Arduino
  
    May 22, 2025, Bojan Jurca


    Classes implemented/used in this module:

        httpServer_t
        httpServer_t::httpConnection_t
        httpServer_t::webSocket_t

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


    Nomenclature used here for easier understaning of the code:

     - "connection" normally applies to TCP connection from when it was established to when it is terminated

                  Looking back to HTTP 1.0 protocol one TCP connection was used to transmit oparamene HTTP request from browser to web server
                  and then HTTP response back to the browser. After this TCP connection was terminated. HTTP 1.1 introduced "keep-alive"
                  directive. When the browser requests several pages from server it can do it over the same TCP connection in short time period 
                  (one after another) reducing the need of establishing and terminating TCP connections several times.   

     - "session" applies to user interaction between login and logout

                  Taking account the web technology one web session tipically consists of many TCP connections. Cookies are involved
                  in order to preserve the state of the web session since HTTP requests and replies themselves are stateless. A list of 
                  valid "session tokens" is keept on the web server so web server can check the session validity.

      - "buffer size" is the number of bytes that can be placed in a buffer. 
      
                  In case of C 0-terminated strings the terminating 0 character should be included in "buffer size".


      - "HTTP request":

                  ┌──────────────── GET / HTTP/1.1
                  │                 Host: 10.18.1.200
                  │                 Connection: keep-alive    field value
                  │                 Cache-Control: max-age=0   |
         request  │    field name - Upgrade-Insecure-Requests: 1
                  │                 User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.45 Safari/537.36
                  │                 Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng;q=0.8,application/signed-exchange;v=b3;q=0.9
                  │                 Accept-Encoding: gzip, deflate
                  │                 Accept-Language: sl-SI,sl;q=0.9,en-GB;q=0.8,en;q=0.7
                  └──────────────── Cookie: refreshCounter=1
                                                |          |
                                           cookie name   cookie value

      - "HTTP reply":
                                                    status
                                                       |         cookie name, value, path and expiration
                  ┌─────────┬──────────────── HTTP/1.1 200 OK      |     |    |         |
                  │         │                 Set-Cookie: refreshCounter=2; Path=/; Expires=Thu, 09 Dec 2021 19:07:04 GMT
            reply │  header │    field name - Content-Type: text/html
                  │         │                 Content-Length: 96   |
                  │         └────────────────                     field value
                  └─ content ──────────────── <HTML>Web cookies<br><br>This page has been refreshed 2 times. Click refresh to see more.</HTML>

*/


#include <sha/sha_parallel_engine.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include "tcpServer.hpp"
#include "tcpConnection.hpp"    
#include "std/Cstring.hpp"
                                                            

#ifndef __HTTP_SERVER__
    #define __HTTP_SERVER__

    #ifdef SHOW_COMPILE_TIME_INFORMATION
        #pragma message "__HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__ __HTTP_SERVER__"
    #endif

    #ifndef __FILE_SYSTEM__
      #ifdef SHOW_COMPILE_TIME_INFORMATION
          #pragma message "Compiling httpServer.hpp without file system (fileSystem.hpp), httpServer will not be able to serve files"
      #endif
    #endif


    // ----- TUNNING PARAMETERS -----

    #define HTTP_SERVER_HOME_DIRECTORY "/var/www/html/"         // home directory path should always end with '/'
    #define HTTP_CONNECTION_STACK_SIZE (8 * 1024)               // a good first estimate how to set this parameter would be to always leave at least 1 KB of each httpConnection stack unused
    #define HTTP_BUFFER_SIZE 1440                               // reading and temporary keeping HTTP requests, buffer for constructing HTTP reply - 1440 is the largest payload that fits into one packet: MTU = 1500, 20 bytes are used for TCP header and 40 for IPv6 header (for IPv4 sligltly less, but we want a general solution)
    #define HTTP_CONNECTION_TIME_OUT 5                          // 5 s, 0 for infinite
    #define HTTP_REPLY_STATUS_MAX_LENGTH 32                     // only first 3 characters are important
    #define HTTP_WS_FRAME_MAX_SIZE 1440                         // WebSocket send frame size and also read frame size (there are 2 buffers), 1440 is the largest payload that fits into one packet: MTU = 1500, 20 bytes are used for TCP header and 40 for IPv6 header (for IPv4 sligltly less, but we want a general solution), WebSocket frame (of 6 or 8 bytes) is included in HTTP_WS_FRAME_MAX_SIZE
    #define HTTP_CONNECTION_WS_TIME_OUT 300                     // 300 s = 5 min
    // #define USE_WEB_SESSIONS                                     // comment this line out if you won't use web sessions
    #define WEB_SESSION_TIME_OUT 300                            // only makes sense if WEB_SESSIONS is #defined, 300 s = 5 min, 0 for infinite 
    #define WEB_SESSION_TOKEN_DATABASE "/var/www/webSessionTokens.db" 

    #ifndef USE_WEB_SESSIONS
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Compiling httpServer.hpp without web sessions, web session login/logout will not be available"
        #endif
    #endif


    // ----- CODE -----


    #define reply400 "HTTP/1.0 400 Bad request\r\nConnection: close\r\nContent-Length:34\r\n\r\nFormat of HTTP request is invalid."
    #define reply404 "HTTP/1.0 404 Not found\r\nConnection: close\r\nContent-Length:10\r\n\r\nNot found."
    #define reply503 "HTTP/1.0 503 Service unavailable\r\nConnection: close\r\nRetry-After: 3\r\nContent-Length:40\r\n\r\nHTTP service is not available right now."
    #define reply507 "HTTP/1.0 507 Insuficient storage\r\nConnection: close\r\nContent-Length:41\r\n\r\nThe buffers of HTTP server are too small."


    #ifdef USE_WEB_SESSIONS
        #ifndef __FILE_SYSTEM__
            #pragma error "fileSystem.hpp must be included in order to use WEB_SESSIONs - key-value session token database needs flash disk"
        #endif

        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #ifndef __KEY_VALUE_DATABASE_HPP__
                #pragma message "Implicitly including keyValueDatabase.hpp"
            #endif
        #endif
        #include "keyValueDatabase.hpp"

        typedef Cstring<64> webSessionToken_t;
        struct webSessionTokenInformation_t {
            time_t expires; // web session toke expiration time in GMT
            Cstring<64> userName; // USER_PASSWORD_MAX_LENGTH = 64
        };

        keyValueDatabase<webSessionToken_t, webSessionTokenInformation_t> webSessionTokenDatabase; // a database containing valid web session tokens
    #endif
    // please note that the limit for getHttpRequestHeaderField and getHttpRequestCookie return values are defined by cstring #definition in Cstring.h


    #ifndef __SHA256__
      #define __SHA256__
        bool sha256 (char *buffer, size_t bufferSize, const char *clearText) { // converts clearText to 265 bit SHA, returns character representation in hexadecimal format of hash value
          *buffer = 0;
          byte shaByteResult [32];
          static char shaCharResult [65];
          mbedtls_md_context_t ctx;
          mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
          mbedtls_md_init (&ctx);
          mbedtls_md_setup (&ctx, mbedtls_md_info_from_type (md_type), 0);
          mbedtls_md_starts (&ctx);
          mbedtls_md_update (&ctx, (const unsigned char *) clearText, strlen (clearText));
          mbedtls_md_finish (&ctx, shaByteResult);
          mbedtls_md_free (&ctx);
          for (int i = 0; i < 32; i++) sprintf (shaCharResult + 2 * i, "%02x", (int) shaByteResult [i]);
          if (bufferSize > strlen (shaCharResult)) { strcpy (buffer, shaCharResult); return true; }
          return false; 
        }
    #endif    


    class httpServer_t : public tcpServer_t {

        public:

            /*
                This is rather unusual class hierarhy. Normally webSocket calss would inherit from httpConnection and add some additional functions to it but here is just the other way arround.
                The reason for this is that once the instance (say httpConnection) is already created we can not extend it to its inherited class (say webSocket), but we can always do the
                other way arround. The implementation is easier this way.
            */

            class httpConnection_t;

            class webSocket_t : public tcpConnection_t {

               friend httpServer_t;

                private:

                    String (*__httpRequestHandlerCallback__) (char *httpRequest, httpConnection_t *hcn);  // will be initialized in constructor
                    void (*__wsRequestHandlerCallback__) (char *httpRequest, webSocket_t *webSck);        // will be initialized in constructor

                    static UBaseType_t __lastHighWaterMark__;


                    // WebSocket definitions 
                    #define STRING_FRAME_TYPE 1
                    #define BINARY_FRAME_TYPE 2 
                    #define CLOSE_FRAME_TYPE 8


                public:

                    webSocket_t (int connectionSocket,                                                            // socket number
                                 char *clientIP,                                                                  // client's IP
                                 char *serverIP,                                                                  // server's IP (the address to which the connection arrived)
                                 String (*httpRequestHandlerCallback) (char *httpRequest, httpConnection_t *hcn), // httpRequestHandlerCallback function provided by calling program
                                 void (*wsRequestHandlerCallback) (char *httpRequest, webSocket_t *webSck)        // wsRequestHandlerCallback function provided by calling program
                                ) : tcpConnection_t (connectionSocket, clientIP, serverIP) {
                        // create a local copy of parameters for later use
                        __httpRequestHandlerCallback__ = httpRequestHandlerCallback;
                        __wsRequestHandlerCallback__ = wsRequestHandlerCallback;
                    }

                    ~webSocket_t () {
                        if (__recvFrameBuffer__) { // assuming WebSocket connection has been established
                            __sendFrame__ (NULL, 0, CLOSE_FRAME_TYPE); // try sending closing WebSocket frame to the browser
                            free (__recvFrameBuffer__);              
                        }
                    }


                    // HTTP connection related functions

                    char *getHttpRequest () { return __httpRequestAndReplyBuffer__; }

                    cstring getHttpRequestHeaderField (const char *fieldName) { // HTTP header fields are in format \r\nfieldName: fieldValue\r\n
                        char *p = strstr (__httpRequestAndReplyBuffer__, fieldName);
                        if (p && p != __httpRequestAndReplyBuffer__ && *(p - 1) == '\n' && *(p + strlen (fieldName)) == ':') { // p points to fieldName in HTTP request
                            p += strlen (fieldName) + 1; 
                            while (*p == ' ') 
                                p++; // p points to field value in HTTP request
                            cstring s; 
                            while (*p >= ' ') 
                                s += *(p ++);
                            return s;
                        }
                        return "";
                    }

                    cstring getHttpRequestCookie (const char *cookieName) { // cookies are passed from browser to http server in "cookie" HTTP header field
                        char *p = strstr (__httpRequestAndReplyBuffer__, (char *) "\nCookie:"); // find cookie field name in HTTP header          
                        if (p) {
                            p = strstr (p, cookieName); // find cookie name in HTTP header
                            if (p && p != __httpRequestAndReplyBuffer__ && *(p - 1) == ' ' && *(p + strlen (cookieName)) == '=') {
                                p += strlen (cookieName) + 1; while (*p == ' ' || *p == '=' ) p++; // p points to cookie value in HTTP request
                                cstring s; while (*p > ' ' && *p != ';') s += *(p ++);
                                return s;
                            }
                        }
                        return "";
                    }

                    void setHttpReplyStatus (const char *status) { strncpy (__httpReplyStatus__, status, HTTP_REPLY_STATUS_MAX_LENGTH); __httpReplyStatus__ [HTTP_REPLY_STATUS_MAX_LENGTH - 1] = 0; }
        
                    void setHttpReplyHeaderField (cstring fieldName, cstring fieldValue) { 
                        __httpReplyHeader__ += fieldName;
                        __httpReplyHeader__ +=  + ": ";
                        __httpReplyHeader__ += fieldValue;
                        __httpReplyHeader__ += "\r\n"; 
                    }

                    void setHttpReplyCookie (cstring cookieName, cstring cookieValue, time_t expires = 0, cstring path = "/") { 
                        char e [50] = "";
                        if (expires) {
                            if (expires < 1687461154) { // 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
                                #ifdef __DMESG__
                                    dmesgQueue << "[httpConn] can't set cookie expiratin time for the time has not been set yet";
                                #endif
                            }
                            struct tm st;
                            gmtime_r (&expires, &st);
                            strftime (e, sizeof (e), "; Expires=%a, %d %b %Y %H:%M:%S GMT", &st);
                        }
                        // save whole fieldValue into cookieName to save stack space
                        cookieName += "=";
                        cookieName += cookieValue;
                        cookieName += "; Path=";
                        cookieName += path;
                        cookieName += e;
                        setHttpReplyHeaderField ("Set-Cookie", cookieName);
                    }


                    // support for web sessions
                    #ifdef USE_WEB_SESSIONS

                        // combination of clientIP and User-Agent HTTP header field - used for calculation of web session token
                        String getClientSpecificInformation () { return String (getClientIP ()) + getHttpRequestHeaderField ("User-Agent"); }

                        // calculates new token from random number and client specific information
                        webSessionToken_t newWebSessionToken (char *userName, time_t expires) {

                            // first delete all already expired tokens
                            if (time (NULL) > 1736439853) { // 1736439853 = 2025/01/09 15:... is the time when I'm writing this code, any valid time should be greater than this
                                webSessionToken_t expiredToken;
                                webSessionTokenInformation_t webSessionTokenInformation;
                                while (true) {
                                    expiredToken = "";
                                    for (auto p: webSessionTokenDatabase) {
                                        signed char e = webSessionTokenDatabase.FindValue (p.key, &webSessionTokenInformation, p.blockOffset);
                                        if (e == err_ok && webSessionTokenInformation.expires && webSessionTokenInformation.expires <= time (NULL)) {
                                            expiredToken = p.key;
                                            break;
                                        } 
                                    }
                                    if (expiredToken != "") {
                                        // DEBUG: cout << "[httpConn] deleteing expired token " << expiredToken << endl;
                                        signed char e = webSessionTokenDatabase.Delete (expiredToken); // can't delete the token while scanning with for loop
                                        if (e) { // != OK
                                            #ifdef __DMESG__
                                                dmesgQueue << "[httpConn] webSessionTokenDatabase.Delete error: " << e; 
                                            #endif
                                            // DEBUG: cout << "[httpConn] webSessionTokenDatabase.Delete error: " << e << endl; 
                                        }                                        
                                    } else {
                                        break;
                                    }     
                                }
                            }

                            // generate new token
                            static int tokenCounter = 0;
                            webSessionToken_t webSessionToken;
                            sha256 ((char *) webSessionToken.c_str (), 64 + 1, (char *) (String (tokenCounter ++) + String (esp_random ()) + String (getClientIP ()) + getHttpRequestHeaderField ("User-Agent")).c_str ());
                            
                            // insert the new token into token database together with information associated with it
                            signed char e = webSessionTokenDatabase.Insert (webSessionToken, {expires, userName}); // if Insert fails, the token will just not work
                            if (e) { // != OK
                                #ifdef __DMESG__
                                    dmesgQueue << "[httpConn] webSessionTokenDatabase.Insert error: " << e; 
                                #endif
                                // DEBUG: cout << "[httpConn] webSessionTokenDatabase.Insert error: " << e << endl; 
                            }             
                            return webSessionToken;
                        }

                        // check validity of a token in webSessionTokenDatabase
                        Cstring<64> getUserNameFromToken (webSessionToken_t& webSessionToken) {
                            // find token in the webSessionTokenDatabase
                            webSessionTokenInformation_t webSessionTokenInformation;
                            signed char e = webSessionTokenDatabase.FindValue (webSessionToken, &webSessionTokenInformation);
                            switch (e) {
                                case err_ok:
                                                    if ((time (NULL) > 1687461154 && webSessionTokenInformation.expires > time (NULL)) || webSessionTokenInformation.expires == 0)  // 1687461154 = 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
                                                        return webSessionTokenInformation.userName;
                                                    else
                                                        return "";
                                case err_not_found: return "";
                                default:                                
                                                #ifdef __DMESG__
                                                    dmesgQueue << "[httpConn] webSessionTokenDatabase.FindValue error: " << e; 
                                                #endif
                                                return "";
                            }
                        }

                        // updates the token in webSessionTokenDatabase
                        bool updateWebSessionToken (webSessionToken_t& webSessionToken, Cstring<64>& userName, time_t expires) {
                            return (webSessionTokenDatabase.Update (webSessionToken, {expires, userName}) == err_ok);
                        }

                        // deletes the token from webSessionTokenDatabase
                        bool deleteWebSessionToken (webSessionToken_t& webSessionToken) {
                            return (webSessionTokenDatabase.Delete (webSessionToken) == err_ok);
                        }

                    #endif


                    // WebSocket implementation of recvBlock, sendBlock, recvString, sendString functions

                    int recvBlock (void *buf, size_t len) {
                        while (true) {
                            switch (peek ()) {
                                case 0:                 break;  // not read yet, continue waiting
                                case BINARY_FRAME_TYPE: __recvFrameState__ = EMPTY; 
                                                        memcpy (buf, __payload__, min (__payloadLength__, (int) len));
                                                        return __payloadLength__; 
                                default:                return -1; // error, wrong frame type, ...
                            }
                            delay (1);
                        }
                        return 0; // ever executes
                    }

                    bool sendBlock (void *buf, size_t bufSize) { return __sendFrame__ ((byte *) buf, bufSize, BINARY_FRAME_TYPE); } // returns success
                    
                    bool recvString (char *buf, size_t len) {
                        while (true) {
                            switch (peek ()) {
                                case 0:                 break;  // not read yet, continue waiting
                                case STRING_FRAME_TYPE: __recvFrameState__ = EMPTY;
                                                        len = min (__payloadLength__, (int) len - 1);
                                                        memcpy (buf, __payload__, len);
                                                        buf [len] = 0;
                                                        return true;
                                default:                return false; // error, wrong frame type, ...
                            }
                            delay (1);
                        }
                        return 0; // ever executes
                    }

                    bool sendString (const char *buf) { return __sendFrame__ ((byte *) buf, strlen (buf), STRING_FRAME_TYPE); } // returns success

                    int peek ()  { // returns the type of the frame received or 0 if the frame has not arrived (yet completely), -1 in case of error
                        switch (__recvFrameState__) {
                            case EMPTY:                     {
                                                                // prepare data structure for the first read operation
                                                                __bytesReceived__ = 0;
                                                                __recvFrameState__ = READING_SHORT_HEADER;
                                                                // continue reading immediately
                                                            }
                            case READING_SHORT_HEADER:      { 
                                                                // check socket if data is pending to be read
                                                                switch (tcpConnection_t::peek (__recvFrameBuffer__, 6)) {
                                                                    case -1:  // error
                                                                              return -1;
                                                                    case 0:   // no data is available
                                                                              return 0;
                                                                    default:  break; // continue
                                                                }
                                                                // read 6 bytes of short header
                                                                tcpConnection_t::recvBlock (__recvFrameBuffer__ + __bytesReceived__, 6 - __bytesReceived__); // can't fail now

                                                                // check if this frame type is supported
                                                                if (!(__recvFrameBuffer__ [0] & 0b10000000)) { // check if fin bit is set
                                                                    #ifdef __DMESG__
                                                                        dmesgQueue << "[webSocket] frame type not supported";
                                                                    #endif
                                                                    return -1;
                                                                }

                                                                // read frame type
                                                                byte frameType = __recvFrameBuffer__ [0] & 0b00001111; // check opcode, 1 = text, 2 = binary, 8 = close, ...
                                                                if (frameType == CLOSE_FRAME_TYPE)
                                                                    return CLOSE_FRAME_TYPE;

                                                                if (frameType != STRING_FRAME_TYPE && frameType != BINARY_FRAME_TYPE) {
                                                                    #ifdef __DMESG__
                                                                        dmesgQueue << "[webSocket] frame type not supported";
                                                                    #endif
                                                                    return -1;
                                                                } 
                                                                // NOTE: after this point only TEXT and BINRY frames are processed!
                                                              
                                                                // check payload length that also determines frame type
                                                                __payloadLength__ = __recvFrameBuffer__ [1] & 0b01111111; // byte 1: mask bit is always 1 for packets that come from browsers, cut it off
                                                                if (__payloadLength__ <= 125) { // short payload
                                                                    __frameMask__ = __recvFrameBuffer__ + 2; // bytes 2, 3, 4, 5
                                                                    __payload__  = __recvFrameBuffer__ + 6; // skip 6 bytes of header, note that __recvFrameBuffer__ is large enough
                                                                    // continue with reading payload immediatelly
                                                                    __recvFrameState__ = READING_PAYLOAD;
                                                                    goto readingPayload;
                                                                } else if (__payloadLength__ == 126) { // 126 means medium payload, read additional 2 bytes of header
                                                                    __recvFrameState__ = READING_MEDIUM_HEADER;
                                                                    // continue reading immediately
                                                                } else { // 127 means large data block - not supported since ESP32 doesn't have enough memory
                                                                    #ifdef __DMESG__
                                                                        dmesgQueue << "[webSocket] frame type not supported";
                                                                    #endif
                                                                    return -1;
                                                                }
                                                            }
                              case READING_MEDIUM_HEADER:   {
                                                                // we don't have to repeat the checking already done in short header case, just read additional 2 bytes and update the data structure
                                                                // read additional 2 bytes (8 altogether) bytes of medium header
                                                                switch (tcpConnection_t::peek (__recvFrameBuffer__ + 6, 2)) {
                                                                    case -1:  // error
                                                                              return -1;
                                                                    case 0:   // no data is available
                                                                              return 0;
                                                                    default:  break; // continue
                                                                }
                                                                // read additional 2 bytes of header
                                                                tcpConnection_t::recvBlock (__recvFrameBuffer__ + __bytesReceived__, 8 - __bytesReceived__); // can't fail now

                                                                // correct internal structure for reading into extended buffer and continue immediately
                                                                __payloadLength__ = __recvFrameBuffer__ [2] << 8 | __recvFrameBuffer__ [3];
                                                                __frameMask__ = __recvFrameBuffer__ + 4; // bytes 4, 5, 6, 7
                                                                __payload__ = __recvFrameBuffer__ + 8; // skip 8 bytes of header, check if __readFrame is large enough:
                                                                if (__payloadLength__ > HTTP_WS_FRAME_MAX_SIZE - 8) {
                                                                    #ifdef __DMESG__
                                                                        dmesgQueue << "[webSocket] buffer too small";
                                                                    #endif
                                                                    Serial.println ("[webSocket] buffer too small");
                                                                    return -1;
                                                                }
                                                                __recvFrameState__ = READING_PAYLOAD;
                                                                // continue with reading payload immediatelly
                                                            }
                              case READING_PAYLOAD:
                          readingPayload:
                                                            {
                                                                __bytesReceived__ = 0; // reset the counter, count only payload from now on
                                                                // read all the payload bytes if possible
                                                                int i = tcpConnection_t::recv (__payload__ + __bytesReceived__, __payloadLength__ - __bytesReceived__);
                                                                if (i <= 0)
                                                                    return -1;
                                                                __bytesReceived__ += i;
                                                                if (__bytesReceived__ != __payloadLength__) 
                                                                    return 0;

                                                                // all is read, decode (unmask) the data
                                                                for (int i = 0; i < __payloadLength__; i++)
                                                                    __payload__ [i] ^= __frameMask__ [i % 4];
                                                                // conclude payload with 0 in case this is going to be interpreted as text - like C string
                                                                __payload__ [__payloadLength__] = 0;
                                                                __recvFrameState__ = FULL;     // stop reading until buffer is read by the calling program
                                                                return __recvFrameBuffer__ [0] & 0b00001111; // check opcode, 1 = text, 2 = binary, 8 = close, ...
                                                            }

                              case FULL:                    // return immediately, there is no space left to read additional incoming data
                                                            return __recvFrameBuffer__ [0] & 0b00001111; // check opcode, 1 = text, 2 = binary, 8 = close, ...
                          }
                          // for every case that has not been handeled earlier return not available
                          return 0;
                      }


                private:

                    // HTTP connection related variables
                    Cstring <HTTP_BUFFER_SIZE> __httpRequestAndReplyBuffer__ = ""; // by default
                    char  __httpReplyStatus__ [HTTP_REPLY_STATUS_MAX_LENGTH] = "200 OK"; // by default
                    cstring __httpReplyHeader__ = ""; // by default

                    // WebSocket related variables
                    byte *__recvFrameBuffer__ = NULL;
                    byte *__sendFrameBuffer__ = NULL;
                    int __bytesReceived__;
                    byte *__frameMask__;
                    int __payloadLength__;
                    byte *__payload__; // will be initialized when the header is read

                    // WebSocket functions

                    enum RECV_FRAME_STATE {
                        EMPTY = 0,                                  // frame is empty
                        READING_SHORT_HEADER = 1,
                        READING_MEDIUM_HEADER = 2,
                        READING_PAYLOAD = 3,
                        FULL = 4                                    // frame is full and waiting to be read by the calling program
                    }; 
                    RECV_FRAME_STATE __recvFrameState__ = EMPTY;

                    bool __sendFrame__ (byte *buffer, size_t bufferSize, byte frameType) {
                        if (bufferSize > 0xFFFF) { // this size fits in large frame size - not supported here
                            #ifdef __DMESG__
                                dmesgQueue << "[webSocket] large frame size is not supported";
                            #endif
                            return false;
                        } 
                        // byte *frame = NULL;
                        if (bufferSize > HTTP_WS_FRAME_MAX_SIZE - 4) { // 4 bytes are needed for header
                            #ifdef __DMESG__
                                dmesgQueue << "[webSocket] frame size > " << HTTP_WS_FRAME_MAX_SIZE - 4 << "is not supported";
                            #endif
                            return false;                         
                        }           
                        int sendFrameSize;
                        if (bufferSize > 125) { // medium frame size
                            // frame type
                            __sendFrameBuffer__ [0] = 0b10000000 | frameType; // set FIN bit and frame data type
                            __sendFrameBuffer__ [1] = 126; // medium frame size, without masking (we won't do the masking, we won't set the MASK bit)
                            __sendFrameBuffer__ [2] = bufferSize >> 8; // / 256;
                            __sendFrameBuffer__ [3] = bufferSize; // % 256;
                            memcpy (__sendFrameBuffer__ + 4, buffer, bufferSize);  
                            sendFrameSize = bufferSize + 4;
                        } else { // small frame size
                            __sendFrameBuffer__ [0] = 0b10000000 | frameType; // set FIN bit and frame data type
                            __sendFrameBuffer__ [1] = bufferSize; // small frame size, without masking (we won't do the masking, we won't set the MASK bit)
                            if (bufferSize) memcpy (__sendFrameBuffer__ + 2, buffer, bufferSize);  
                            sendFrameSize = bufferSize + 2;
                        }
                        if (tcpConnection_t::sendBlock (__sendFrameBuffer__, sendFrameSize) != sendFrameSize)
                            return false;
                        return true;
                    }


                    void __runConnectionTask__ () {

                        // ----- this is where HTTP connection starts, the socket is already opened -----

                        bool keepAlive;
                        do { // while keepAlive

                            // 1. read the request
                            switch (tcpConnection_t::recvString (__httpRequestAndReplyBuffer__, HTTP_BUFFER_SIZE, "\r\n\r\n")) {
                                case -1:                // error
                                case 0:                 // connection closed by peer
                                                        return;
                                case HTTP_BUFFER_SIZE:  // buffer full but end of request did not arrive
                                                        #ifdef __DMESG__
                                                            dmesgQueue << (const char *) "[httpConn] buffer too small for " << __httpRequestAndReplyBuffer__;
                                                        #endif
                                                        Serial.print ((const char *) "[httpConn] buffer too small for "); Serial.println (__httpRequestAndReplyBuffer__);
                                                        tcpConnection_t::sendString (reply507);
                                                        return;
                                default:                break; // continue
                            }

                            // connection: keep-alive?
                            keepAlive = (strstr (__httpRequestAndReplyBuffer__, "Connection: keep-alive") != NULL);

                            // 2. is it a websocket request?
                            if (strstr (__httpRequestAndReplyBuffer__, (char *) "Upgrade: websocket")) {

                                // a. change timeout for WebSocket
                                if (setTimeout (WEB_SESSION_TIME_OUT) < 0) {
                                    #ifdef __DMESG__
                                        dmesgQueue << (const char *) "[httpConn] setsockopt error: " << errno << " " << strerror (errno);
                                    #endif
                                    return;
                                }

                                // b. get additional memory for frame buffers
                                __recvFrameBuffer__ = (byte *) malloc (2 * HTTP_WS_FRAME_MAX_SIZE);
                                if (!__recvFrameBuffer__) {
                                    #ifdef __DMESG__
                                        dmesgQueue << (const char *) "[httpConn] out of memory";
                                    #endif
                                    return;                                  
                                }
                                __sendFrameBuffer__ = __recvFrameBuffer__ + HTTP_WS_FRAME_MAX_SIZE;

                                // c. do the handshake with the browser so it would consider webSocket connection established
                                char *i = strstr (__httpRequestAndReplyBuffer__, (char *) "Sec-WebSocket-Key: ");
                                if (i) {             
                                    i += 19;
                                    char *j = strstr (i, "\r\n");
                                    if (j) {
                                        if (j - i <= 24) { // Sec-WebSocket-Key is not supposed to exceed 24 characters
                                            char key [25]; 
                                            memcpy (key, i, j - i); 
                                            key [j - i] = 0; 
                                            // calculate Sec-WebSocket-Accept
                                            char s1 [64]; 
                                            strcpy (s1, key); 
                                            strcat (s1, (char *) "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); 
                                            #define SHA1_RESULT_SIZE 20
                                            unsigned char s2 [SHA1_RESULT_SIZE]; 
                                            esp_sha (SHA1, (unsigned char *) s1, strlen (s1), s2);
                                            #define WS_CLIENT_KEY_LENGTH 24
                                            size_t olen = WS_CLIENT_KEY_LENGTH;
                                            char s3 [32];
                                            mbedtls_base64_encode ((unsigned char *) s3, 32, &olen, s2, SHA1_RESULT_SIZE);
                                            // compose websocket accept response and send it back to the client
                                            char buffer  [255]; // 255 will do
                                            sprintf (buffer, "HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", s3);
                                            if (tcpConnection_t::sendString (buffer) <= 0)
                                                return;
                                        } else { // |key| > 24
                                            #ifdef __DMESG__
                                                dmesgQueue << "[webSocket] WsRequest key too long";
                                            #endif
                                            return;
                                        }
                                    } else { // j == NULL
                                        #ifdef __DMESG__
                                            dmesgQueue << "[webSocket] WsRequest without key";
                                        #endif
                                        return;
                                    }

                                    // d. call __wsRequestHandlerCallback__
                                    if (__wsRequestHandlerCallback__) {
                                        __wsRequestHandlerCallback__ (__httpRequestAndReplyBuffer__, this);
                                    } else {
                                        #ifdef __DMESG__
                                            dmesgQueue << (const char *) "[httpConn]", " wsRequestHandlerCallback was not provided to handle WebSocket";
                                        #endif
                                        return;
                                    }

                                } else { // i == NULL
                                    #ifdef __DMESG__
                                        dmesgQueue << "[webSocket] WsRequest without key";
                                    #endif
                                }
                                return;
                            }

                            // 3. will httpRequestHandlerCallback function provide (at least content part of) the reply?
                            String httpReplyContent ("");
                            if (__httpRequestHandlerCallback__) {

                                httpReplyContent = __httpRequestHandlerCallback__ (__httpRequestAndReplyBuffer__, (httpConnection_t *) this);

                                if (!httpReplyContent) { // out of memory
                                    #ifdef __DMESG__
                                        dmesgQueue << (const char *) "[httpConn] out of memory";
                                    #endif
                                    tcpConnection_t::sendString (reply503);
                                    return;
                                }
                            }

                            if (httpReplyContent != "") {

                                // if content-type was not provided with __httpRequestHandlerCallback__ try guessing what it is
                                if (!strstr (__httpReplyHeader__, "Content-Type")) { 
                                    if (stristr ((char *) httpReplyContent.c_str (), "<HTML")) {
                                        setHttpReplyHeaderField ("Content-Type", "text/html");
                                    } else if (strstr ((char *) httpReplyContent.c_str (), "{")) {
                                        setHttpReplyHeaderField ("Content-Type", "application/json");
                                    // ... add more if needed
                                    } else {
                                        setHttpReplyHeaderField ("Content-Type", "text/plain");
                                    }
                                }
                                if (__httpReplyHeader__.errorFlags ()) {

                                    #ifdef __DMESG__
                                        dmesgQueue << (const char *) "[httpConn] header reply buffer too small";
                                    #endif
                                    Serial.print ((const char *) "[httpConn] reply header buffer too small\n");
                                    tcpConnection_t::sendString (reply507);
                                    return;
                                }

                                // construct the whole HTTP reply from differrent pieces and send it to the client (browser)
                                // if the reply is short enough send it in one block, 
                                // if not send header first and then the content, so we won't have to move hughe blocks of data
                                __httpRequestAndReplyBuffer__ = "";
                                __httpRequestAndReplyBuffer__ += "HTTP/1.1 ";
                                __httpRequestAndReplyBuffer__ += __httpReplyStatus__;
                                __httpRequestAndReplyBuffer__ += "\r\n";
                                __httpRequestAndReplyBuffer__ += __httpReplyHeader__;
                                __httpRequestAndReplyBuffer__ += "Content-Length: ";
                                __httpRequestAndReplyBuffer__ += httpReplyContent.length ();
                                __httpRequestAndReplyBuffer__ += "\r\n\r\n";

                                unsigned long httpReplyHeaderLen = __httpRequestAndReplyBuffer__.length ();

                                // can not happen due to the difference in lengths, we can skip checking:
                                // if (ths->__httpRequestAndReplyBuffer__.error ()) {
                                //    dmesg ("[httpConn] __httpRequestAndReplyBuffer__ too small");
                                //    sendAll (ths->__connectionSocket__, reply507, HTTP_CONNECTION_TIME_OUT);
                                //    goto endOfConnection;
                                // }

                                unsigned long contentBytesToSendThisTime = min ((unsigned long) httpReplyContent.length (), HTTP_BUFFER_SIZE - httpReplyHeaderLen);
                                unsigned long contentBytesSentTotal = 0;
                                memcpy (&__httpRequestAndReplyBuffer__ [httpReplyHeaderLen], (char *) httpReplyContent.c_str (), contentBytesToSendThisTime);
                                while (true) {
                                    if (tcpConnection_t::sendBlock (__httpRequestAndReplyBuffer__, httpReplyHeaderLen + contentBytesToSendThisTime) <= 0)
                                        return;

                                    httpReplyHeaderLen = 0; // we won't send the header any more
                                    contentBytesSentTotal += contentBytesToSendThisTime;
                                    contentBytesToSendThisTime = min (httpReplyContent.length () - contentBytesSentTotal, (unsigned long) HTTP_BUFFER_SIZE);
                                    if (!contentBytesToSendThisTime)
                                        break;
                                    memcpy (__httpRequestAndReplyBuffer__, (char *) httpReplyContent.c_str () + contentBytesSentTotal, contentBytesToSendThisTime);
                                }

                                // content-type provided with __httpRequestHandlerCallback__ was sent to the browser, continue with the next request on this connection
                                goto nextHttpRequest;
                            }

                            // 4. if the request is a file name
                            bool browserAcceptsGzip = (getHttpRequestHeaderField ("Accept-Encoding").indexOf ("gzip") >= 0);

                            #ifdef __FILE_SYSTEM__
                                if (fileSystem.mounted ()) {
                                    if (strstr (__httpRequestAndReplyBuffer__.c_str (), "GET ") == __httpRequestAndReplyBuffer__.c_str ()) {
                                        char *p = strstr (__httpRequestAndReplyBuffer__.c_str () + 4, " ");
                                        if (p) {
                                            *p = 0;
                                            // get file name from HTTP request
                                            cstring fileName (__httpRequestAndReplyBuffer__.c_str () + 4);
                                            if (fileName == "" || fileName == "/") fileName = "/index.html";
                                            fileName = cstring (HTTP_SERVER_HOME_DIRECTORY) + (fileName.c_str () + 1); // HTTP_SERVER_HOME_DIRECTORY always ends with /
                                            // if Content-type was not provided in __httpRequestHandlerCallback__ try guessing what it is
                                            if (!strstr ((char *) __httpReplyHeader__.c_str (), (char *) "Content-Type")) { 
                                                     if (fileName.endsWith (".bmp"))                                        setHttpReplyHeaderField ("Content-Type", "image/bmp");
                                                else if (fileName.endsWith (".css"))                                        setHttpReplyHeaderField ("Content-Type", "text/css");
                                                else if (fileName.endsWith (".csv"))                                        setHttpReplyHeaderField ("Content-Type", "text/csv");
                                                else if (fileName.endsWith (".gif"))                                        setHttpReplyHeaderField ("Content-Type", "image/gif");
                                                else if (fileName.endsWith (".htm") || fileName.endsWith (".html"))         setHttpReplyHeaderField ("Content-Type", "text/html");
                                                else if (fileName.endsWith (".htm.gz") || fileName.endsWith (".html.gz"))   setHttpReplyHeaderField ("Content-Type", "text/html");
                                                else if (fileName.endsWith (".jpg") || fileName.endsWith (".jpeg"))         setHttpReplyHeaderField ("Content-Type", "image/jpeg");
                                                else if (fileName.endsWith (".js"))                                         setHttpReplyHeaderField ("Content-Type", "text/javascript");
                                                else if (fileName.endsWith (".json"))                                       setHttpReplyHeaderField ("Content-Type", "application/json");
                                                else if (fileName.endsWith (".mpeg"))                                       setHttpReplyHeaderField ("Content-Type", "video/mpeg");
                                                else if (fileName.endsWith (".pdf"))                                        setHttpReplyHeaderField ("Content-Type", "application/pdf");
                                                else if (fileName.endsWith (".png"))                                        setHttpReplyHeaderField ("Content-Type", "image/png");
                                                else if (fileName.endsWith (".tif") || fileName.endsWith (".tiff"))         setHttpReplyHeaderField ("Content-Type", "image/tiff");
                                                else if (fileName.endsWith (".txt"))                                        setHttpReplyHeaderField ("Content-Type", "text/plain");
                                                // ... add more if needed but Contet-Type can often be omitted without problems ...
                                            }

                                            // serving files works better if served one at a time
                                            xSemaphoreTakeRecursive (__tcpServerSemaphore__, portMAX_DELAY);

                                            File f;
                                            // check if there is a compressed .gz file available
                                            if (fileName.endsWith (".gz")) {
                                                setHttpReplyHeaderField ("Content-Encoding", "gzip");
                                                f = fileSystem.open (fileName, "r");
                                            } else {
                                                // if the browser accepts gzip
                                                if (browserAcceptsGzip) {
                                                    f = fileSystem.open (fileName + ".gz", "r");
                                                    if (f) {
                                                        fileName += ".gz";
                                                        setHttpReplyHeaderField ("Content-Encoding", "gzip");
                                                    } else {
                                                        // no, open uncompressed fileName
                                                        f = fileSystem.open (fileName, "r");
                                                    }
                                                } else {
                                                    // no, open uncompressed fileName
                                                    f = fileSystem.open (fileName, "r");
                                                }
                                            }

                                            if (f) {
                                                if (!f.isDirectory ()) {                                  
                                                    fileSystem.diskTraffic.bytesRead += f.size (); // asume the whole file will be read - update performance counter without semaphore - values may not be perfectly exact but we won't loose time this way

                                                    if (__httpReplyHeader__.errorFlags ()) {
                                                        xSemaphoreGiveRecursive (__tcpServerSemaphore__);
                                                        f.close ();
                                                        #ifdef __DMESG__
                                                            dmesgQueue << "[httpConn] reply header buffer too small";
                                                        #endif
                                                        Serial.print ((const char *) "[httpConn] reply header buffer too small\n");
                                                        tcpConnection_t::sendString (reply507);
                                                        return;
                                                    }
                    
                                                    // construct the whole HTTP reply from differrent pieces and send it to client (browser)
                                                    // if the reply is short enough send it in one block,
                                                    // if not send header first and then the content, so we won't have to move hughe blocks of data
                                                    unsigned long httpReplyContentLen = f.size ();
                                                    __httpRequestAndReplyBuffer__ = "";
                                                    __httpRequestAndReplyBuffer__ += "HTTP/1.1 ";
                                                    __httpRequestAndReplyBuffer__ += __httpReplyStatus__;
                                                    __httpRequestAndReplyBuffer__ += "\r\n";
                                                    __httpRequestAndReplyBuffer__ += __httpReplyHeader__;
                                                    __httpRequestAndReplyBuffer__ += "Content-Length: ";
                                                    __httpRequestAndReplyBuffer__ += httpReplyContentLen; 
                                                    __httpRequestAndReplyBuffer__ += "\r\n\r\n";

                                                    unsigned long httpReplyHeaderLen = __httpRequestAndReplyBuffer__.length ();

                                                    // can not happen due to the difference in lengths, we can skip checking:
                                                    // if (ths->__httpRequestAndReplyBuffer__.error ()) {
                                                    //    dmesg ("[httpConn] __httpRequestAndReplyBuffer__ too small");
                                                    //    sendAll (ths->__connectionSocket__, reply507, HTTP_CONNECTION_TIME_OUT);
                                                    //    goto endOfConnection;
                                                    // }

                                                    int bytesToReadThisTime = min (httpReplyContentLen, HTTP_BUFFER_SIZE - httpReplyHeaderLen); 
                                                    int bytesReadThisTime = f.read ((uint8_t *) &__httpRequestAndReplyBuffer__ [httpReplyHeaderLen], bytesToReadThisTime);

                                                    __httpRequestAndReplyBuffer__ [HTTP_BUFFER_SIZE] = 0;
                                                    if (tcpConnection_t::sendBlock (__httpRequestAndReplyBuffer__, httpReplyHeaderLen + bytesReadThisTime) <= 0) {
                                                        f.close ();
                                                        xSemaphoreGiveRecursive (__tcpServerSemaphore__);
                                                        #ifdef __DMESG__
                                                            dmesgQueue << (const char *) "[httpConn] send failed";
                                                        #endif
                                                        Serial.print ((const char *) "[httpConn] send failed");
                                                        return;
                                                    }
                                            
                                                    int bytesSent = bytesReadThisTime; // already sent
                                                    while (bytesSent < httpReplyContentLen) {
                                                        bytesToReadThisTime = min (httpReplyContentLen - bytesSent, (unsigned long) HTTP_BUFFER_SIZE);
                                                        bytesReadThisTime = f.read ((uint8_t *) &__httpRequestAndReplyBuffer__ [0], (unsigned long) bytesToReadThisTime);

                                                        delay (10); // WiFi STAtion sometimes disconnects at heavy load - maybe giving it some time would make things better? 
                                                        if (!bytesReadThisTime) {
                                                            f.close ();
                                                            xSemaphoreGiveRecursive (__tcpServerSemaphore__);
                                                            #ifdef __DMESG__
                                                                dmesgQueue << (const char *) "[httpConn] can't read " << fileName;
                                                            #endif
                                                            Serial.print ((const char *) "[httpConn] can't read "); Serial.print (bytesToReadThisTime); Serial.print (" bytes of "); Serial.println (fileName);
                                                            return;
                                                        }
                                        
                                                        if (tcpConnection_t::sendBlock (__httpRequestAndReplyBuffer__, bytesReadThisTime) <= 0) {
                                                            f.close ();
                                                            xSemaphoreGiveRecursive (__tcpServerSemaphore__);
                                                            #ifdef __DMESG__
                                                                dmesgQueue << (const char *) "[httpConn] can't send " << fileName;
                                                            #endif
                                                            Serial.print ((const char *) "[httpConn] can't send "); Serial.print (bytesReadThisTime); Serial.printf (" bytes of "); Serial.println (fileName);
                                                            return;
                                                        }
                                                        bytesSent += bytesReadThisTime; // already sent
                                                    }
                                                    // HTTP reply sent
                                                    f.close ();
                                                    xSemaphoreGiveRecursive (__tcpServerSemaphore__);                                                   
                                                    goto nextHttpRequest;
                                                } // if file is a file
                                                f.close ();

                                            } // if file is open

                                            xSemaphoreGiveRecursive (__tcpServerSemaphore__);
                                        }
                                    }
                                }
                            #endif

                            // 5. HTTP request was not handeled above
                            if (tcpConnection_t::sendString (reply404) <= 0)
                                return;

                      nextHttpRequest:
                            // check how much stack did we use
                            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark (NULL);
                            if (__lastHighWaterMark__ > highWaterMark) {
                                #ifdef __DMESG__
                                    dmesgQueue << (const char *) "[httpConn] new HTTP connection stack high water mark reached: " << highWaterMark << (const char *) " not used bytes";
                                #endif
                                // DEBUG: 
                                Serial.print ((const char *) "[httpConn] new HTTP connection stack high water mark reached: "); Serial.print (highWaterMark); Serial.println ((const char *) " not used bytes");
                                __lastHighWaterMark__ = highWaterMark;
                            }

                            // if we are running out of ESP32's resources we won't try to keep the connection alive, this would slow down the server a bit but it would let still it handle requests from different clients
                            if (getSocket () >= LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN - 2)  // running out of sockets
                                return;
                            if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) < 2 * HTTP_CONNECTION_STACK_SIZE) // there is not a memory block large enough evailable to start 2 new tasks that would handle the connection
                                return;

                            // restore default values of member variables for the next HTTP request on this connection                              
                            __httpRequestAndReplyBuffer__ = "";
                            strcpy (__httpReplyStatus__, "200 OK");
                            __httpReplyHeader__ = "";
                        
                        } while (keepAlive); // while

                        // ----- this is where HTTP connection ends, the socket will be closed upon return -----

                    }

            };

            class httpConnection_t : public webSocket_t {
                // make it look like httpConnection is actually inherited from tcpConnection directly and not from webSocket
                using tcpConnection_t::recvBlock;
                using tcpConnection_t::sendBlock;
                using tcpConnection_t::recvString;
                using tcpConnection_t::sendString;
                using tcpConnection_t::peek;
            };


            String (*__httpRequestHandlerCallback__) (char *httpRequest, httpConnection_t *hcn);            // will be initialized in constructor
            void (*__wsRequestHandlerCallback__) (char *httpRequest, webSocket_t *webSck);  // will be initialized in constructor


        public:

            httpServer_t (String (*httpRequestHandlerCallback) (char *httpRequest, httpConnection_t *hcn) = NULL, // httpRequestHandlerCallback function provided by calling program
                          void (*wsRequestHandlerCallback) (char *httpRequest, webSocket_t *webSck) = NULL,       // wsRequestHandlerCallback function provided by calling program
                          int serverPort = 80,                                                                    // HTTP server port
                          bool (*firewallCallback) (char *clientIP, char *serverIP) = NULL,                       // a reference to callback function that will be celled when new connection arrives 
                          bool runListenerInItsOwnTask = true                                                     // a calling program may repeatedly call accept itself to save some memory tat listener task would use
                         ) : tcpServer_t (serverPort, firewallCallback, runListenerInItsOwnTask) {
                // create a local copy of parameters for later use
                __httpRequestHandlerCallback__ = httpRequestHandlerCallback;
                __wsRequestHandlerCallback__ = wsRequestHandlerCallback;

                #ifdef __FILE_SYSTEM__
                    if (fileSystem.mounted ()) {
                        cstring d = HTTP_SERVER_HOME_DIRECTORY;
                        if (!fileSystem.isDirectory ((char *) d)) {
                            // DEBUG: Serial.print ("[httpServer] creating "); Serial.println (HTTP_SERVER_HOME_DIRECTORY);

                            bool b = false;
                            for (int i = 0; d [i] && d [i]; i++) 
                                if (i && d [i] == '/') {
                                  d [i] = 0; 
                                  b = fileSystem.makeDirectory (d); 
                                  d [i] = '/'; 
                              }
                            if (!b) {
                                // DEBUG: Serial.print ("[httpServer] error creating "); Serial.println (HTTP_SERVER_HOME_DIRECTORY);
                                #ifdef __DMESG__
                                    dmesgQueue << "[httpServer] can't create " << HTTP_SERVER_HOME_DIRECTORY;
                                #endif
                            }
                        }
                    }

                    #ifdef USE_WEB_SESSIONS
                        // load existing tokens
                        signed char e = webSessionTokenDatabase.Open (WEB_SESSION_TOKEN_DATABASE);
                        if (e != err_ok) {
                            #ifdef __DMESG__
                                dmesgQueue << "[httpServer] webSessionCleaner: webSessionTokenDatabase.loadData error: " << e << ", truncating webSessionTokenDatabase";
                            #endif
                            webSessionTokenDatabase.Truncate (); // forget all stored data and try to make it work from the start
                        }
                    #endif

                #endif

                #ifdef USE_mDNS
                    // register mDNS servise
                    MDNS.addService ("http", "tcp", serverPort); // won't work if not connected yet
                    #ifdef __NETWK__
                        __httpPort__ = serverPort; // remember HTTP port so __NETWK__ onEvent handler would call MDNS.addService again when connected
                    #endif
                #endif
            }

            tcpConnection_t *__createConnectionInstance__ (int connectionSocket, char *clientIP, char *serverIP) override {
                webSocket_t *connection = new (std::nothrow) webSocket_t (connectionSocket, clientIP, serverIP, __httpRequestHandlerCallback__, __wsRequestHandlerCallback__);
                if (!connection) {
                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[httpServer] " << "can't create connection instance, out of memory";
                    #endif
                    send (connectionSocket, reply503, strlen (reply503), 0);
                    close (connectionSocket); // normally tcpConnection would do this but if it is not created we have to do it here since the connection was not created
                    return NULL;
                }

                if (connection->setTimeout (HTTP_CONNECTION_TIME_OUT) == -1) {
                    #ifdef __DMESG__
                        dmesgQueue << "[httpConn] setsockopt error: " << errno << strerror (errno);
                    #endif
                    send (connectionSocket, reply503, strlen (reply503), 0);
                    delete (connection); // normally tcpConnection would do this but if it is not running we have to do it here
                    return NULL;
                }

                if (pdPASS != xTaskCreate ([] (void *thisInstance) {
                                                                        httpConnection_t* ths = (httpConnection_t *) thisInstance; // get "this" pointer

                                                                        xSemaphoreTakeRecursive (__tcpServerSemaphore__, portMAX_DELAY);
                                                                            __runningTcpConnections__ ++;
                                                                        xSemaphoreGiveRecursive (__tcpServerSemaphore__);

                                                                        // DEBUG: cout << "httpConnection task started: " << (unsigned long) xTaskGetCurrentTaskHandle << ", currently running: " << __runningTcpConnections__ << endl;

                                                                        ths->__runConnectionTask__ ();

                                                                        xSemaphoreTakeRecursive (__tcpServerSemaphore__, portMAX_DELAY);
                                                                            __runningTcpConnections__ --;
                                                                        xSemaphoreGiveRecursive (__tcpServerSemaphore__);

                                                                        // DEBUG: cout << " GG httpConnection task ended: " << (unsigned long) xTaskGetCurrentTaskHandle << ", currently running: " << __runningTcpConnections__ << endl;

                                                                        delete ths;
                                                                        vTaskDelete (NULL); // it is connection's responsibility to close itself
                                                                    }
                                            , "httpConn", HTTP_CONNECTION_STACK_SIZE, connection, tskNORMAL_PRIORITY, NULL)) {

                    #ifdef __DMESG__
                        dmesgQueue << (const char *) "[httpServer] " << "can't create connection task, out of memory";
                    #endif
                    send (connectionSocket, reply503, strlen (reply503), 0);
                    delete (connection); // normally tcpConnection would do this but if it is not running we have to do it here
                    return NULL;
                }
            
                return NULL; // success, but don't return connection, since it may already been closed and destructed by now
            }


            // accept any connection, the client will get notified in __createConnectionInstance__ if there arn't enough resources
            inline tcpConnection_t *accept () __attribute__((always_inline)) { return tcpServer_t::accept (); }
            /*
            tcpConnection_t *accept () { 
                if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) > HTTP_CONNECTION_STACK_SIZE && esp_get_free_heap_size () > HTTP_CONNECTION_STACK_SIZE + sizeof (httpConnection_t)) {
                    last = millis () - 1000;
                    return tcpServer_t::accept (); 
                } else {
                    if (millis () - last > 1000) {
                        last = millis ();
                        return tcpServer_t::accept (); 
                    }
                }
                if (heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) > HTTP_CONNECTION_STACK_SIZE && esp_get_free_heap_size () > HTTP_CONNECTION_STACK_SIZE + sizeof (httpConnection_t)) 
                    return tcpServer_t::accept (); 
                else // do no accept new connection if there is not enough memory left to handle it
                    return NULL;
            }
            */

    };

    // declare static member outside of class definition
    UBaseType_t httpServer_t::webSocket_t::__lastHighWaterMark__ = HTTP_CONNECTION_STACK_SIZE;

#endif
