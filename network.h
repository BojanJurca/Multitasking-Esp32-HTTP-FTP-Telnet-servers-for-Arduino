/*
 * 
 * Network.h
 * 
 *  This file is part of A_kind_of_esp32_OS_template.ino project: https://github.com/BojanJurca/A_kind_of_esp32_OS_template
 * 
 *  Network.h reads network configuration from file system and sets both ESP32 network interfaces accordingly
 * 
 * INSTRUCTIONS
 * 
 *  Run A_kind_of_esp32_OS_template the first time with NETWORK_CONNECTION_METHOD == PREPARE_AND_READ_NETWORK_CONFIGURATION 
 *  to generate network configuration files (it is a little awkward why UNIX, LINUX, Raspbian are using so many network 
 *  configuration files and how they are used):
 * 
 *    /network/interfaces             - modify the code below with your IP addresses
 *    /etc/wpa_supplicant.conf        - modify the code below with your WiFi SSID and password (this file is used instead of /etc/wpa_supplicant/wpa_supplicant.conf, the latter name is too long to fit into SPIFFS)
 *    /etc/dhcpcd.conf                - modify the code below with your access point IP addresses 
 *    /etc/hostapd/hostapd.conf       - modify the code below with your access point SSID and password
 * 
 *  Once network configuration files are generated there's no need of generating them again, you can switch
 *  NETWORK_CONNECTION_METHOD to ONLY_READ_NETWORK_CONFIGURATION
 * 
 * History:
 *          - first release, November 16, 2018, Bojan Jurca
 *  
 */
 

#ifndef __NETWORK__
  #define __NETWORK__
  #include "file_system.h"  // network.h needs file_system.h
  
  
  // change this definitions according to your needs
  
  #define PREPARE_AND_READ_NETWORK_CONFIGURATION 1                // this option is normally used ony the first time you run A_kind_of_est32_OS_template on ESP32
                                                                  // change networkConnect function below for initial network setup
  #define ONLY_READ_NETWORK_CONFIGURATION        2                // once the configuration is set, this option is preferable
  // select one of the above network connection methods
  #define NETWORK_CONNECTION_METHOD  PREPARE_AND_READ_NETWORK_CONFIGURATION
  
  bool networkStationWorking = false;                                         // use this variable to check ESP32 has connected in STAtion mode
  bool networkAccesPointWorking = false;                                      // use this variable to check ESP32 access point is in function
  #define networkWorking (networkStationWorking || networkAccesPointWorking)  // use this definition to check in there is some kond of network connection
  
  
  String __compactNetworkConfiguration__ (String inp);
  String __insideBrackets__ (String inp, String opening, String closing);
  IPAddress IPAddressFromString (String ipAddress);
  
  
  void connectNetwork () {                                        // connect to the network by calling this function
    if (!SPIFFSmounted) { Serial.printf ("[network] can't read or write network configuration from/to file system since file system is not mounted"); return; }
  
    #if NETWORK_CONNECTION_METHOD == PREPARE_AND_READ_NETWORK_CONFIGURATION
      // it is a little awkward why UNIX, LINUX, Raspbian are using so many network configuration files and how they are used
  
      // prepare configuration for network interface 1 that will be used latter to connect in STA-tion mode to WiFi (skip this if you don't want ESP32 to connect to eisting WiFi)
      if (File file = SPIFFS.open ("/network/interfaces", FILE_WRITE)) {
      
        String s =  "# only wlan1 can be used to connecto to your WiFi\r\n"
                    "\r\n"
                    "# get IP address from DHCP\r\n"
                    "   iface wlan1 inet dhcp\r\n"                  // this method is preferable, you can configure your router to always get the same IP address
                    "\r\n"
                    "# use static IP address (example below)\r\n"   // comment upper line and uncomment this lines if you want to set a static IP address
                    "#   iface wlan1 inet static\r\n"
                    "#      address 10.0.0.4\r\n"
                    "#      netmask 255.255.255.0\r\n"
                    "#      gateway 10.0.0.1\r\n";
      
        if (file.print (s) != s.length ()) { file.close (); Serial.printf ("[network] can't write network configuration to file system"); return; }
        file.close ();
      }
      if (File file = SPIFFS.open ("/etc/wpa_supplicant.conf", FILE_WRITE)) {
      
        String s =  "network = {\r\n"
                    "   ssid = \"YOUR-STA-SSID\"\r\n"               // use your WiFI SSID here
                    "   psk  = \"YOUR-STA-PASSWORD\"\r\n"           // use your WiFi password here
                    "}\r\n";
      
        if (file.print (s) != s.length ()) { file.close (); Serial.printf ("[network] can't write network configuration to file system"); return; }
        file.close ();
      } else {
        Serial.printf ("[network] can't write network configuration to file system");
        return;      
      }
  
      // prepare configuration for network interface 0 that will be used latter to start WiFi access point (skip this if you don't want ESP32 to be an access point)
      if (File file = SPIFFS.open ("/etc/dhcpcd.conf", FILE_WRITE)) {
      
        String s =  "# only static IP addresses can be used for acces point and only wlan0 can be used (example below)\r\n"
                    "\r\n"
                    "interface wlan1\r\n"
                    "   static ip_address = 10.0.1.4\r\n"           // set your access point IP addresses here
                    "          netmask = 255.255.255.0\r\n"
                    "          gateway = 10.0.1.4\r\n";
      
        if (file.print (s) != s.length ()) { file.close (); Serial.printf ("[network] can't write network configuration to file system"); return; }
        file.close ();
      }
      if (File file = SPIFFS.open ("/etc/hostapd/hostapd.conf", FILE_WRITE)) {
      
        String s =  "# only wlan1 can be used for access point\r\n"
                    "\r\n"
                    "interface = wlan1\r\n"
                    "   ssid = ESP32_SRV\r\n"                       // set your access point SSID here
                    "   # use at least 8 characters for wpa_passphrase\r\n"
                    "   wpa_passphrase = YOUR-AP-PASSWORD\r\n";     // set your access point password here
      
        if (file.print (s) != s.length ()) { file.close (); Serial.printf ("[network] can't write network configuration to file system"); return; }
        file.close ();
      }
    #endif
  
    // read network configuration from configuration files and set it accordingly
    String staSSID = "";
    String staPassword = "";
    String staIP = "";
    String staSubnetMask = "";
    String staGateway = "";
  
    String apSSID = "";
    String apPassword = "";
    String apIP = "";
    String apSubnetMask = "";
    String apGateway = "";
    String s;
    int i;
    
    s = __insideBrackets__ (__compactNetworkConfiguration__ (readEntireTextFile ("/etc/wpa_supplicant.conf")), "network {", "}"); 
    staSSID     = __insideBrackets__ (s, "ssid ", "\n");
    staPassword = __insideBrackets__ (s, "psk ", "\n");
  
    s = __compactNetworkConfiguration__ (readEntireTextFile ("/network/interfaces") + "\n"); 
    i = s.indexOf ("iface wlan1 inet static");
    if (i >= 0) {
      s = s.substring (i);
      staIP         = __insideBrackets__ (s, "address ", "\n");
      staSubnetMask = __insideBrackets__ (s, "netmask ", "\n");
      staGateway    = __insideBrackets__ (s, "gateway ", "\n");
    }
  
    s = __compactNetworkConfiguration__ (readEntireTextFile ("/etc/hostapd/hostapd.conf") + "\n"); 
    i = s.indexOf ("interface wlan1");
    if (i >= 0) {
      s = s.substring (i);
      apSSID        = __insideBrackets__ (s, "ssid ", "\n");
      apPassword    = __insideBrackets__ (s, "wpa_passphrase ", "\n");
    }
  
    s = __compactNetworkConfiguration__ (readEntireTextFile ("/etc/dhcpcd.conf") + "\n"); 
    i = s.indexOf ("interface wlan1");
    if (i >= 0) {
      s = s.substring (i);
      apIP          = __insideBrackets__ (s, "static ip_address ", "\n");
      apSubnetMask  = __insideBrackets__ (s, "netmask ", "\n");
      apGateway     = __insideBrackets__ (s, "gateway ", "\n");
    }
  
    // connect STA and AP if defined
    WiFi.disconnect (true);
    WiFi.mode (WIFI_OFF);             // no WiFi
    if (staSSID > "") {
      if (apSSID > "") {             
        WiFi.mode (WIFI_AP_STA);      // both, AP and STA modes
      } else {                       
        WiFi.mode (WIFI_STA);         // only STA mode
      }
    } else {
      if (apSSID > "") {              
        WiFi.mode (WIFI_AP);          // only AP mode
      }
    }
    
    if (apSSID > "") {
        if (WiFi.softAP (apSSID.c_str (), apPassword.c_str ())) { 
          WiFi.softAPConfig (IPAddressFromString (apIP), IPAddressFromString (apGateway), IPAddressFromString (apSubnetMask));
          WiFi.begin ();
          Serial.print ("[network][AP] IP: "); Serial.println (WiFi.softAPIP ());
          networkAccesPointWorking = true;
        } else {
          // ESP.restart ();
          Serial.printf ("[network][AP] failed to initialize access point mode\n"); 
        }
    }    
    if (staSSID > "") {
      if (staIP > "") { // configure static IP address
        WiFi.config (IPAddressFromString (staIP), IPAddressFromString (staGateway), IPAddressFromString (staSubnetMask));
      } // else go with DHCP
      WiFi.begin (staSSID.c_str (), staPassword.c_str ());
      Serial.printf ("[network][STA] connecting ...\n");
      for (unsigned long i = 0; i < 240000; i++) if (WiFi.status () == WL_CONNECTED) break; else {if (!(i % 1000)) Serial.printf ("[network][STA] connecting ...\n"); delay (1);}
      if (WiFi.status () != WL_CONNECTED) {
        // ESP.restart ();
        Serial.printf ("[network][STA] failed to connect to %s in station mode\n", staSSID.c_str ());
      } else {
        Serial.print ("[network][STA] IP: "); Serial.println (WiFi.localIP ());
        networkStationWorking = true;
      }
    }
  }

  
  String __compactNetworkConfiguration__ (String inp) { // skips comments, ...
    String outp = "";
    bool inComment = false;  
    bool inQuotation = false;
    String c;
    bool lastCharacterIsSpace = false;
    for (int i = 0; i < inp.length (); i++) {
      c = inp.substring (i, i + 1);
  
           if (c == "#")                                        {inComment = true;}
      else if (c == "\"")                                       {inQuotation = !inQuotation;}
      else if (c == "\n")                                       {if (!outp.endsWith ("\n")) {if (!inQuotation && outp.endsWith (" ")) outp = outp.substring (0, outp.length () - 1); outp += "\n";} inComment = inQuotation = false;}
      else if (c == "{")                                        {if (!inComment) {while (outp.endsWith ("\n") || outp.endsWith (" ")) outp = outp.substring (0, outp.length () - 1); outp += "\n{\n";}}
      else if (c == "}")                                        {if (!inComment) {while (outp.endsWith ("\n") || outp.endsWith (" ")) outp = outp.substring (0, outp.length () - 1); outp += "\n}\n";}}    
      else if (c == " " || c == "\t" || c == "=" || c == "\r")  {if (!inComment && !outp.endsWith (" ") && !outp.endsWith ("\n")) outp += " ";}
      else if (!inComment) {outp += c;}
   
    }
    if (outp.endsWith (" ")) outp = outp.substring (0, outp.length () - 1);
    return outp;
  }
  
  String __insideBrackets__ (String inp, String opening, String closing) { // returns content inside of opening and closing brackets
    int i = inp.indexOf (opening);
    if (i >= 0) {
      inp = inp.substring (i +  opening.length ());
      i = inp.indexOf (closing);
      if (i >= 0) inp = inp.substring (0, i);
    }
    return inp;
  }
  
  IPAddress IPAddressFromString (String ipAddress) { // converts dotted IP address into IPAddress structure
    int ip1, ip2, ip3, ip4; 
    if (4 == sscanf (ipAddress.c_str (), "%i.%i.%i.%i", &ip1, &ip2, &ip3, &ip4)) {
      return IPAddress (ip1, ip2, ip3, ip4);
    } else {
      Serial.printf ("[network] invalid IP address %s\n", ipAddress.c_str ());
      return IPAddress (0, 42, 42, 42); // invalid address - first byte of class A can not be 0
    }
  }
  
#endif
