// #include "./servers/fileSystem.hpp"  // all file name and file info related functions are there, by default FILE_SYSTEM is #defined as FILE_SYSTEM_LITTLEFS, other options are: FILE_SYSTEM_FAT, FILE_SYSTEM_SD_CARD and (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)

#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"
#include "./servers/netwk.h"            // if #included after fileSystem.hpp these files will be created: /network/interfaces, /etc/wpa_supplicant/wpa_supplicant.conf, /etc/dhcpcd.conf, /etc/hostapd/hostapd.conf

#include "./servers/httpServer.hpp"     // if #included after fileSystem.hpp httpServer will also serve files stored in /var/www/html directory


String httpRequestHandlerCallback (char *httpRequest, httpServer_t::httpConnection_t *httpConnection) {  // callback function to handle HTTP requests and prepare (content part of) HTTP replies

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

    Serial.print (httpRequest);

    // allow all requests from other server HTML files to this server
    httpConnection->setHttpReplyHeaderField ("Access-Control-Allow-Origin", "*");  
    httpConnection->setHttpReplyHeaderField ("Access-Control-Allow-Methods", "*");
    // before PUT, POST, ... (except GET) methods the browser always sends OPTIONS method
    if (httpRequestStartsWith ("OPTIONS ")) return "ok"; // whatever


        if (httpRequestStartsWith ("GET / "))             {   // send the content part of the default page, header will be added by httpServer itself before sending the whole reply to the browser

                                                              String textForOtherWebPage =  " &lt;html&gt;<br>"
                                                                                            "&nbsp; &lt;body&gt;<br>"
                                                                                            "&nbsp;&nbsp; &lt;br&gt;&lt;br&gt;&lt;p id = 'hostname_placeholder'&gt;...&lt;/p&gt;<br>"
                                                                                            "&nbsp; &lt;/body&gt;<br>"
                                                                                            "<br>"
                                                                                            "&nbsp; &lt;script type='text/javascript'&gt;<br>"
                                                                                            "<br>"
                                                                                            "&nbsp;&nbsp; var httpClient = function () {<br>"
                                                                                            "&nbsp;&nbsp;&nbsp; this.request = function (url, method, callback) {<br>"
                                                                                            "&nbsp;&nbsp;&nbsp; var httpRequest = new XMLHttpRequest ();<br>"
                                                                                            "&nbsp;&nbsp;&nbsp; httpRequest.onreadystatechange = function () {<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp; if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; switch (httpRequest.status) {<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; case 200: 	callback (httpRequest.responseText); // 200 = OK<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; break;<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; case 0:		break;<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; default: 	alert ('Server reported error ' + httpRequest.status + ' ' + httpRequest.responseText);<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; break;<br>"
                                                                                            "&nbsp;&nbsp;&nbsp;&nbsp; }<br>"
                                                                                            "&nbsp;&nbsp;&nbsp; }<br>"
                                                                                            "&nbsp;&nbsp; }<br>"
                                                                                            "&nbsp;&nbsp; httpRequest.open (method, url, true);<br>"
                                                                                            "&nbsp;&nbsp; httpRequest.send (null);<br>"
                                                                                            "&nbsp;&nbsp; }<br>"
                                                                                            "&nbsp; }<br>"
                                                                                            "<br>"
                                                                                            "&nbsp; var client = new httpClient ();<br>"
                                                                                            "&nbsp; client.request ('http://" + WiFi.localIP ().toString () + "/who/are/you', 'GET', function (replyText) {<br>"
                                                                                            "&nbsp;&nbsp; document.getElementById ('hostname_placeholder').innerText = \"Server's name is \" + replyText + '.';<br>"
                                                                                            "&nbsp; });<br>"
                                                                                            "<br>"
                                                                                            "&nbsp; &lt;/script&gt;<br>"
                                                                                            "&lt;/html&gt;";

                                                              return "<html>\n"
                                                                     "  <head>\n"
                                                                     "\n" 
                                                                     "    <title>Cross Origin Resource Sharing (CORS)</title>\n"
                                                                     "\n" 
                                                                     "  </head>\n"
                                                                     "   <body>\n"
                                                                     "\n" 
                                                                     "     <br><h1>Cross Origin Resource Sharing (CORS)</h1>\n"
                                                                     "     The pages on this server "
                                                                         + WiFi.localIP ().toString () +
                                                                     "     are directly acessible from HTML files resinding elsewhere. Copy the following page to HTML file on the disk an open it: <br><br>"
                                                                     "     <p style=\"font-family: 'Courier New'\">"
                                                                     "\n"
                                                                        + textForOtherWebPage +
                                                                     "\n"
                                                                     "     </p>"
                                                                     "  </body>\n"
                                                                     "</html>\n";
                                                            }

    else if (httpRequestStartsWith ("GET /who/are/you "))   {   // RESful API
                                                                return HOSTNAME;
                                                            }

    return "";  // if not handeled above then let the HTTP server send the reply itself (server files stored in /var/www/html directory or send 404 reply)
}


httpServer_t *httpServer = NULL;

void setup () {
    Serial.begin (115200);

    // fileSystem.mountLittleFs (true); // or just call    LittleFS.begin (true);

    startWiFi (); // or just call    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    httpServer = new (std::nothrow) httpServer_t (httpRequestHandlerCallback); // there are other arguments available to handle WebSockets, firewall, ... if httpServer will only serve files stored in /var/www/html directory this argument can be omitted
    if (httpServer && *httpServer)
        Serial.printf ("HTTP server started\n");
    else
        Serial.printf ("HTTP server did not start\n");

    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP from the router
        delay (1000); 
        Serial.printf ("   .\n"); 
    } 
    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
}

void loop () {

}
