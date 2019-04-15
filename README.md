# Esp32_web_ftp_telnet_server_template

While working on my ESP32 / Arduino home automation project I was often missing functionalities that are available on bigger computers. This template is an attempt of providing some functionalities of operating system such as file system (SPIFFS is used), threaded Web, FTP and Telnet servers (all three are built upon threaded TCP server which is also included) to an ESP32 programmer. Rather then making a complete and finished software I decided to go with a working template that could easily be modified to each individual needs. The template demonstrates the use of Web interface to turn LED built into Esp32 on and off using REST calls and basically the same through the use of Telnet interface. It creates Unix like environment so Unix / Linux / Raspian programmers will be familiar with.

## Features

Here is a list of functionalities that I consider home automation projects should have.

- Real time clock. If you want to do something like turning the light on at certain time for example, ESP32 should be aware of current time. In Esp32_web_ftp_telnet_server_template real time clock reads current GMT time from NTP servers and synchronize internal clock once a day with them. You can define three NTP servers ESP32 will read GMT time from. Local time on the other hand is not covered adequately since different countries have different rules how to calculate it from GMT and I cannot prepare all the possible options myself. You may have to modify getLocalTime () function to match your country and location.

- File system is needed for storing configuration files, .html files used by web server, etc. A SPIFFS flash file system is used in Esp32_web_ftp_telnet_server_template. Documentation on SPIFFS can be found at http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html.

- Network configuration files. Esp32_web_ftp_telnet_server_template uses Unix / Linux / Raspbian like network configuration files (although it is a little awkward how network configuration is implemented in these operating systems). The following files are used to store STAtion and AccessPoint configuration parameters:

   - /network/interfaces
   - /etc/wpa_supplicant.conf
   - /etc/dhcpcd.conf
   - /etc/hostapd/hostapd.conf  

Modify these files according to your needs or upload your own files onto ESP32 by using FTP. 

- User management. Esp32_web_ftp_telnet_server_template uses UNIX, LINUX, Raspbian like user management files. Only "root" user with "rootpassword" password, "webadmin" user with "webadminpassword" password, "webserver" and "telnetserver" users are created at initialization. You can create additional users if you need them or change their passwords at initialization or upload your own files onto ESP32 by using FTP. User management implemented in Esp32_web_ftp_telnet_server_template is very basic, only 3 fields are used: user name, hashed password and home directory. The following files are used to store user information:

   - /etc/passwd
   - /etc/shadow

- FTP server is needed for uploading configuration files, .html files, etc onto ESP32 file system.

- Web server. HTTP seems to offer a convenient user interface with ESP32 projects. Web server included in Esp32_web_ftp_telnet_server_template can handle requests in two different ways. As a programmed response to some requests (typically small REST calls in similar way as, for example, PHP) or by sending .html files that has been previously uploaded to /var/www/html directory.

- Telnet server, similarly to Web server can also handle commands in two different ways. As a programmed response to some commands or it can handle some already built in commands by itself. Only a few commands are implemented so far:
   - passwd
   - ls ([directoryName]) or dir ([directoryName])
   - cat [fileName] or type [fileName]
   - rm [fileName] or del [fileName]
   - ping [target]   (ping used here was taken and modified from https://github.com/pbecchi/ESP32_ping)
   - ifconfig or ipconfig
   - arp (synonym for "arp -a" as implemented here)
   - iw (synonym for "iw dev wlan1 station dump" as implemented here)
   - uptime
   - reboot
   - help
   - quit

More will be implemented in the future. Help command displays help.txt file. Help.txt is included in Esp32_web_ftp_telnet_server_template package. Use FTP to upload it into /var/telnet directory.

## Setup Instructions

1. Copy all files in this package into Esp32_web_ftp_telnet_server_template directory.
2. Open Esp32_web_ftp_telnet_server_template.ino with Arduino IDE.
3. Go to Network.h and do the following changes:
   - find and change YOUR-STA-SSID to your WiFi SSID,
   - find and change YOUR-STA-PASSWORD to your WiFi password,
   - find and change YOUR-AP-PASSWORD to the password your access point will be using.
4. Compile sketch and run it on your ESP32 for the first time.

Doing this the following will happen:
   - ESP32 flash memory will be formatted with SPIFFS file system. WARNING: every information you have stored into ESP32’s flash memory will be lost.
   - network configuration files will be created with the following settings:
      - your ESP32 will be configured to use DHCP in STAtion mode,
      - your ESP32 AccessPoint name will be ESP32_SRV,
      - your ESP32 AccessPoint IP will be 10.0.1.3,
   - user management files will be created with the following user accounts:
      - root / rootpassword,
      - webadmin / webadminpassword,
      - webserver with no password since this is a system account used only to define webserver home directory,
      - telnetserver with no password since this is also a system account used only to define telnetserver home directory.

At this point, you can already test if everything goes on as planned by www, FTP or Telnet to your ESP32. Your ESP32 is already working as a server but there are a few minor things yet left to do.

5. FTP to your ESP32 as webadmin / webadminpassword and upload the following files:
   - index.html,
   - android-192.png,
   - apple-180.png.

Files will be placed into webadmin home directory, which is configured to be /var/www/html/.

6. FTP to your ESP32 as root / rootpassword and upload help.txt into /var/telnet/ directory (put help.txt /var/telnet/help.txt), which is a home directory for telnetserver system account.

## How to continue from here?

Esp32_web_ftp_telnet_server_template is what its name says, just a working template. A programmer is highly encouraged to add or change each piece of code as he or she sees appropriate for his or her projects. Esp32_web_ftp_telnet_server_template.ino is pretty comprehansive, small and easy to modify so it may be a good starting point.

## Programming code tips

Example 01 shows how to access files on SPIFFS and/or perform delays properly in a multi-threaded environment. Why are those two connected? The issue is well described in https://www.esp32.com/viewtopic.php?t=7876: "vTaskDelay() cannot be called whilst the scheduler is disabled (i.e. in between calls of vTaskSuspendAll() and vTaskResumeAll()). The assertion failure you see is vTaskDelay() checking if it was called whilst the scheduler is disabled." Some SPIFFS functions internally call vTaskSuspendAll() hence no other thread should call delay()  (which internally calls vTaskDelay()) since we cannot predict if this would happen while the scheduler is disabled.

Esp32_web_ftp_telnet_server_template introduces a SPIFFSsemaphore mutex that prevents going into delay() and SPIFSS functions simultaneously from different threads. Calling a delay() function is not thread safe (when used together with SPIFFS) and would crash ESP32 occasionally. Use SPIFFSsafeDelay() instead.

```C++
for (int i = 0; i < 3; i++) {
  String s = "";
  File file;

  xSemaphoreTake (SPIFFSsemaphore, portMAX_DELAY); // always take SPIFFSsemaphore before any SPIFSS operations
  
  if ((bool) (file = SPIFFS.open ("/ID", FILE_READ)) && !file.isDirectory ()) {
    while (file.available ()) s += String ((char) file.read ());
    file.close (); 
    Serial.printf ("[example 01] %s\n", s.c_str ());
  } else {
    Serial.printf ("[example 01] can't read file /ID\n");
  }
  
  xSemaphoreGive (SPIFFSsemaphore); // alwys give SPIFFSsemaphore when SPIFSS operation completes
  
  SPIFFSsafeDelay (1000); // always use SPIFFSsafeDelay() instead of delay()
}
```

Example 02 demonstrates the use of rtc (real time clock) instance built into Esp32_web_ftp_telnet_server_template.

```C++
if (rtc.isGmtTimeSet ()) {
  time_t now;
  now = rtc.getGmtTime ();
  Serial.printf ("[example 02] current UNIX time is %lu\n", (unsigned long) now);

  char str [30];
  now = rtc.getLocalTime ();
  strftime (str, 30, "%d.%m.%y %H:%M:%S", gmtime (&now));
  Serial.printf ("[example 02] current local time is %s\n", str);
} else {
  Serial.printf ("[example 02] rtc has not obtained time from NTP server yet\n");
}
```


Example 03 shows how we can use TcpServer objects to make HTTP requests.

```C++
char buffer [256]; *buffer = 0; // reserve some space to hold the response
// create non-threaded TCP client instance
TcpClient myNonThreadedClient ("127.0.0.1", // server's IP address (local loopback in this example)
                               80,          // server port (usual HTTP port)
                               3000);       // time-out (3 s in this example)
// get reference to TCP connection. Befor non-threaded constructor of TcpClient returns the 
// connection is established if this is possible
TcpConnection *myConnection = myNonThreadedClient.connection ();

if (myConnection) { // test if connection is established
  int sendTotal = myConnection->sendData ("GET /upTime \r\n\r\n", strlen ("GET /upTime \r\n\r\n")); // send REST request
  // Serial.printf ("[example 03] a request of %i bytes sent to the server\n", sendTotal);
  int receivedTotal = 0;
  // read response in a loop untill 0 bytes arrive - this is a sign that connection has ended 
  // if the response is short enough it will normally arrive in one data block although
  // TCP does not guarantee that it would
  while (int received = myConnection->recvData (buffer + receivedTotal, sizeof (buffer) - receivedTotal - 1)) {
    buffer [receivedTotal += received] = 0; // mark the end of the string we have just read
  }
  
} 

// check if the reply is correct - the best way is to parse the response but here we'll just check if 
// the whole reply arrived - our JSON reponse ends with "}\r\n"
if (strstr (buffer, "}\r\n")) {
  Serial.printf ("[example 03] %s\n", buffer);
} else {
  Serial.printf ("[example 03] %s ... the reply didn't arrive (in time) or it is corrupt or too long\n", buffer);
}
```


Example 04 shows how we can handle incoming HTTP requests. If the reply is static then the easiest way is to upload (FTP) file with a name corresponding to a request into /var/www/html directory. If it is not then you can modify httpRequestHandler function (already existing in Esp32_web_ftp_telnet_server_template) accordingly. . Make sure httpRequestHandler is re-entrant for it can run in many threads simultaneously. For example:

```C++
String httpRequestHandler (String httpRequest) {  
  if (httpRequest.substring (0, 12) == "GET /upTime ") {
    if (rtc.isGmtTimeSet ()) {
      unsigned long long l = rtc.getGmtTime () - rtc.getGmtStartupTime ();
      return "{\"id\":\"esp32\",\"upTime\":\"" + String ((unsigned long) l) + " sec\"}\r\n";
    } else {
      return "{\"id\":\"esp32\",\"upTime\":\"unknown\"}\r\n";
    }
  }

  else return ""; // HTTP request has not been handled by httpRequestHandler - let the webServer handle it itself
}
```

Example 05 shows how we can handle incoming telnet commands. The easiest way is to modify  telnetCommandHandler function (already existing in Esp32_web_ftp_telnet_server_template) accordingly. Make sure telnetCommandHandler is re-entrant for it can run in many threads simultaneously. For example:

```C++
String telnetCommandHandler (String command, String parameter, String homeDirectory) { 
  if (command + " " + parameter  == "led state")
    return "Led is " + (digitalRead (2) ? String ("on") : String ("off"));
  else                                                   
    return ""; // telnetCommand has not been handled by telnetCommandHandler - let the telnetServer handle it itself
}
```

## Plans for the future

I’m currently working on a comprehensive example of a TcpServer that will be available here soon. In meantime, you can look at telnetServer.hpp, webServer.hpp or ftpServer.hpp. All three are using underlying TcpServer although some knowledge of telnet, HTTP and FTP protocols is required in order to understand how they work.




