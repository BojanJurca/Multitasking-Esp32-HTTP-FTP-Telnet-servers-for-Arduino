# A_kind_of_esp32_OS_template

While working on my ESP32 / Arduino home automation project I was often missing functionalities that are available on bigger computers. This template is an attempt of providing some functionalities of operating system to an ESP32 programmer. Rather then making a complete and finished software I decided to go with a working template that could easily be modified to each individual needs. In order to achieve this goal the code should be:

- small,
- efficient,
- easily modifiable,
- understandable to programmers,
- and should use standard approaches wherever possible.

## Features

Here is a list of functionalities that I consider home automation projects should have.

- Real time clock. If you want to do something like turning the light on at certain time for example, ESP32 should be aware of current time. In A_kind_of_esp32_OS_template real time clock reads current GMT time from NTP servers and synchronize internal clock once a day with them. You can define three NTP servers ESP32 will read GMT time from. Local time on the other hand is not covered adequately since different countries have different rules how to calculate it from GMT and I cannot prepare all the possible options myself. You may have to modify getLocalTime () function to match your country and location.

- File system is needed for storing configuration files, .html files used by web server, etc. A SPIFFS flash file system is used in A_kind_of_esp32_OS_template. Documentation on SPIFFS can be found at http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html.

- Network configuration files. A_kind_of_esp32_OS_template uses UNIX, LINUX, Raspbian like network configuration files (although it is a little awkward how network configuration is implemented in these operating systems). The following files are used to store STAtion and AccessPoint configuration parameters:

   - /network/interfaces
   - /etc/wpa_supplicant.conf
   - /etc/dhcpcd.conf
   - /etc/hostapd/hostapd.conf  

Modify these files according to your needs or upload your own files onto ESP32 by using FTP. 

- User management. A_kind_of_esp32_OS_template uses UNIX, LINUX, Raspbian like user management files. Only "root" user with "rootpassword" password, "webadmin" user with "webadminpassword" password, "webserver" and "telnetserver" users are created at initialization. You can create additional users if you need them or change their passwords at initialization or upload your own files onto ESP32 by using FTP. User management implemented in A_kind_of_esp32_OS_template is very basic, only 3 fields are used: user name, hashed password and home directory. The following files are used to store user information:

   - /etc/passwd
   - /etc/shadow

- FTP server is needed for uploading configuration files, .html files, etc onto ESP32 file system.

- Web server. HTTP seems to offer a convenient user interface with ESP32 projects. Web server included in A_kind_of_esp32_OS_template can handle requests in two different ways. As a programmed response to some requests (typically small REST calls in similar way as, for example, PHP) or by sending .html files that has been previously uploaded to /var/www/html directory.

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

More will be implemented in the future. Help command displays help.txt file. Help.txt is included in A_kind_of_esp32_OS_template package. Use FTP to upload it into /var/telnet directory.

## Setup Instructions

1. Copy all files in this package into A_kind_of_esp32_OS_template directory.
2. Open A_kind_of_esp32_OS_template.ino with Arduino IDE.
3. Go to Network.h and do the following changes:
   - find and change YOUR-STA-SSID to your WiFi SSID,
   - find and change YOUR-STA-PASSWORD to your WiFi password,
   - find and change YOUR-AP-PASSWORD to the password your access point will be using.
4. Compile sketch and run it on your ESP32 for the first time.

Doing this the following will happen:
   - ESP32 flash memory will be formatted with SPIFFS file system,
   - network configuration files will be created with the following settings:
      - your ESP32 will be configured to use DHCP in STAtion mode,
      - your ESP32 AccessPoint name will be ESP32_SRV,
      - your ESP32 AccessPoint IP will be 10.0.1.4,
   - user management files will be created with the following user accounts:
      - root / rootpassword,
      - webadmin / webadminpassword,
      - webserver with no password since this is a system account used only to define webserver home directory,
      - telnetserver with no password since this is also a system account used only to define telnetserver home directory.

At this point, you can already test if everything goes on as planned by www, FTP or Telnet to your ESP32.

Once all this is done, there is no need of doing it again.

5. Go to File_system.h and change definition FILE_SYSTEM_MOUNT_METHOD to DO_NOT_FORMAT.
6. Go to Network.h and change definition NETWORK_CONNECTION_METHOD to ONLY_READ_NETWORK_CONFIGURATION.
7. Go to User_management.h and change definition INITIALIZE_USERS to DO_NOT_INITIALIZE_USERS.
8. Compile sketch and run it on your ESP32 for the second time.

Now you are almost there. Your ESP32 is already woriking as a server but there are a few minor things yet left to do.

9. FTP to your ESP32 as webadmin / webadminpassword and upload the following files:
   - index.html,
   - android-192.png,
   - apple-180.png.

Files will be placed into webadmin home directory, which is configured to be /var/www/html/.

10. FTP to your ESP32 as root / rootpassword and upload help.txt into /var/telnet/ directory (put help.txt /var/telnet/help.txt), which is a home directory for telnetserver system account.

## How to continue from here?

A_kind_of_esp32_OS_template is what its name says, just a working template. A programmer is highly encouraged to add or change each piece of code as he or she sees appropriate for his or her projects.
