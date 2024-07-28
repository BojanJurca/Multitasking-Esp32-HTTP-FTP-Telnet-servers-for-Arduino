
#define DEFAULT_STA_SSID                "YOUR_STA_SSID"
#define DEFAULT_STA_PASSWORD            "YOUR_STA_PASSWORD"

#include <esp_wifi.h>
#include "servers/ESP32_ping.hpp"


void setup () {
    Serial.begin (115200);

    WiFi.begin (DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD);

    while (WiFi.localIP ().toString () == "0.0.0.0") { // wait until we get IP from the router
        delay (1000);
        Serial.printf ("   .\n");
    } 

    Serial.printf ("Got IP: %s\n", (char *) WiFi.localIP ().toString ().c_str ());
}

void loop () {

    static unsigned long lastConnectionCheckMillis = millis ();
    if (millis () - lastConnectionCheckMillis > 3600000) { // 1 hour
        lastConnectionCheckMillis = millis ();

        // check if ESP32 works in WIFI_STA mode
        wifi_mode_t wifiMode = WIFI_OFF;
        if (esp_wifi_get_mode (&wifiMode) != ESP_OK) {
            Serial.println ("Couldn't get WiFi mode");
        } else {
            if (wifiMode & WIFI_STA) { // WiFi works in STAtion mode  
                // is  STAtion mode is diconnected?
                if (!WiFi.isConnected ()) { 
                    Serial.println ("STAtion disconnected, reconnecting to WiFi");
                    WiFi.reconnect (); 
                } else { // check if it really works anyway 
                    esp32_ping routerPing;
                    routerPing.ping (WiFi.gatewayIP ());
                    for (int i = 0; i < 4; i++) {
                        routerPing.ping (1);
                        if (routerPing.received ())
                            break;
                        delay (1000);
                    }
                    if (!routerPing.received ()) {
                        Serial.println ("ping of router failed, reconnecting WiFi STAtion");
                        WiFi.reconnect (); 
                    }
                }
            }
        }

    }
    
}
