
// #include "servers/fileSystem.hpp"       // all file name and file info related functions are there, by default FILE_SYSTEM is #defined as FILE_SYSTEM_LITTLEFS, other options are: FILE_SYSTEM_FAT, FILE_SYSTEM_SD_CARD and (FILE_SYSTEM_FAT | FILE_SYSTEM_SD_CARD)

#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"
#include "servers/netwk.h"              // sendAll and recvAll functions are declared there, if #included after fileSystem.hpp these files will be created: /network/interfaces, /etc/wpa_supplicant/wpa_supplicant.conf, /etc/dhcpcd.conf, /etc/hostapd/hostapd.conf

#include "servers/httpServer.hpp"       // if #included after fileSystem.hpp httpServer will also serve files stored in /var/www/html directory


String httpRequestHandlerCallback (char *httpRequest, httpConnection *hcn) {  // callback function to handle HTTP requests and prepare (content part of) HTTP replies

    #define httpRequestStartsWith(X) (strstr(httpRequest,X)==httpRequest)

    Serial.print (httpRequest);

    if (httpRequestStartsWith ("GET / ")) { // send the content part of the default page, header will be added by httpServer itself before sending the whole reply to the browser

                                              return F ("<html>\n"
                                                        "  <head>\n"
                                                        "\n" 
                                                        "    <title>Index</title>\n"
                                                        "\n" 
                                                        "  </head>\n"
                                                        "   <body>\n"
                                                        "\n"     
                                                        "     <br><h1>Index</h1>\n"
                                                        "  </body>\n"
                                                        "</html>\n");
                                            }

    return "";  // if not handeled above then let the HTTP server send the reply itself (server files stored in /var/www/html directory or send 404 reply)
}


httpServer *myHttpServer = NULL;

void setup () {
    Serial.begin (115200);

    // fileSystem.mountLittleFs (true); // or just call    LittleFS.begin (true);

    startWiFi (); // or just call    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    myHttpServer = new httpServer (httpRequestHandlerCallback); // there are other arguments available to handle WebSockets, firewall, ... if httpServer will only serve files stored in /var/www/html directory this argument can be omitted
    if (myHttpServer && myHttpServer->state () == httpServer::RUNNING)
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
