# ESP32 with Web Server, Telnet Server, file system, FTP Server and SMTP client

**Want to try live demo? All three servers are available on demo ESP32 at [193.77.159.208](http://193.77.159.208) (login as root/rootpassword). Please be lenient, ESP32 is not primarily intended for WAN.**

It is more or less about user interface:

- **very fast and easy development of Telnet command-line user interface for your project** 
- **easy development of nice-looking Web user interface for your project**
- **working with files (uploading and downloading .html and other files)**
- **run-time monitoring of ESP32 behaviour with built-in dmesg Telnet command**
- **run-time monitoring of ESP32 signals with built-in Web-based oscilloscope**

![Screenshot](presentation.gif)


In case of Web user interface all you need to do is to upload (with FTP) .html (.png, .jpg, …) files into your ESP32 /var/www/html directory and/or modify httpRequestHandler function that already exists in Esp32_web_ftp_telnet_server_template.ino according to your needs (see examples below).

In case of Telnet user interface all you need to do is to modify telnetCommandHandler function that already exists in Esp32_web_ftp_telnet_server_template.ino according to your needs (see example below).

You can go directly to Setup instructions and examples now or continue reading the rest of this text.

## Features

**webServer** can handle HTTP requests in two different ways. As a programmed response to some requests (typically small replies – see examples) or by sending .html files that have been previously uploaded into /var/www/html directory. Features:

   - HTTP protocol,
   - Cookies
   - WS protocol – only basic support for WebSockets is included so far,
   - webClient function is included for making simple HTTP requests from ESP32 to other servers,
   - threaded web server sessions,
   - time-out set to 1,5 seconds for HTTP protocol and 5 minutes for WS protocol to free up limited ESP32 resources used by inactive sessions,
   - support for HTTP 1.1 keep-alive directive,
   - optional firewall for incoming requests.

-----> Please see [Step by step guide to web server](Web_server_step_by_step.md) 

-----> or [Reference manual](Reference_manual.md) for more information.


**telnetServer** can, similarly to webserver, handle commands in two different ways. As a programmed response to some commands or it can handle some already built-in commands by itself. 

Built-in commands implemented so far:

  - quit                                                  - Stops telnet session. You can also use Ctrl-C instead.
  - clear                                                 - Clears screen. You can also use cls instead.
  - help                                                  - Displays this file.
  - uname                                                 - Displays basic ESP32 server information.
  - uptime                                                - Displays the time ESP32 server has been running.
  - reboot                                                - Causes soft reboot of ESP32.
  - reset                                                 - Causes hard reboot of ESP32.
  - date [-s <YYYY/MM/DD hh:mm:ss>]                       - Displays and sets date/time (in 24-hour format).
  - ntpdate [-u [<ntpServer>]]                            - Makes ESP32 to synchronize time with NTP server.
  - crontab [-l]                                          - Displays current content of cron table in ESP32 server memory.
  - free [-s <n>] (where 0 < n < 300)                     - Displays free ESP32 memory.
  - dmesg [--follow] [-T]                                 - Displays ESP32 server messages.
  - mkfs.fat                                              - Formats (also erases) ESP32 flash disk with FAT file system.
  - fs_info                                               - Displays basic information about flash disk FAT file system.
  - ls [<directoryName>]                                  - Lists directory content. You can also use dir instead.
  - tree [<directoryName>]                                - Lists directory subtree content.
  - mkdir <directoryName>                                 - Makes a new directory.
  - rmdir <directoryName>                                 - Removes existing directory.
  - cd <directoryName> or cd ..                           - Changes current working directory.
  - pwd                                                   - Displays current working directory name.
  - cat <fileName>                                        - Displays content of text file. You can also use type instead.
  - cat > <fileName>                                      - Creates a new file from what you type on console. End it with Ctrl-Z.
  - vi <fileName>                                         - A basic text editor for small files.
  - cp <existingFileName> <newFileName>                   - Copies content of one file to another. You can also use copy instead.
  - mv <existingFileName> <newFileName>                   - Renames the file.
  - mv <existingDirectoryName> <newDirectoryName>         - Renames the directory
  - rm <fileName>                                         - Removes (deletes) the file.
  - passwd [<userName>]                                   - Changes password of current user or another user (only root).
  - useradd -u <userId> -d <userHomeDirectory> <userName> - Creates a new user. Unique userId should be > 1000 (only root).
  - userdel <userName>                                    - Deletes the user (only root).
  - ifconfig or ipconfig                                  - Displays basic network configuration.
  - iw                                                    - Displays basic WiFi information.
  - arp                                                   - Displays current content of ARP table.
  - ping <targetIP>                                       - Tests if another network device is reachable form ESP32 server.
  - telnet <targetIP>                                     - Establishes telnet session from ESP32 server to another device.
  - sendmail [-S smtpServer] [-P smtpPort] [-u userName] [-p password] [-f from address] [t to address list] [-s subject] [-m messsage]
  - curl [method] http://url (where method is GET, PUT, ...)

Other features:

   - threaded Telnet sessions,
   - time-out set to 5 minutes to free up limited ESP32 resources used by inactive sessions,  
   - optional firewall for incoming connections.

-----> Please see [Step by step guide to telnet server](Telnet_server_step_by_step.md) 

-----> or [Reference manual](Reference_manual.md) for more information.


**ftpServer** is needed for uploading configuration files, .html files, etc. onto ESP32 file system. Unlike webServer and telnetServer it does not expose a programming interface. 

Built-in commands that are implemented so far:

   - pwd
   - cd
   - ls
   - rmdir
   - mkdir
   - rm
   - put osFileName esp32FileName
   - get esp32FileName osFileName

Other features:

   - active and passive data connections (you can use command line FTP client or Windows Explorer for example) 
   - threaded FTP sessions,
   - time-out set to 5 minutes to free up limited ESP32 the resources used by inactive sessions,  
   - optional firewall for incoming connections.

-----> Please see [Step by step guide to FTP server](FTP_server_step_by_step.md)

-----> or [Reference manual](Reference_manual.md) for more information.



**SMTP client** 

Features:

   - sendMail function and sendmail telnet command. Default values for sendMail function and sendmail telnet command can be provided in /etc/mail/sendmail.cf file. Plain text, UTF-8 and HTML are supported. 


-----> Please see [Reference manual](Reference_manual.md) for more information.



**TcpServer** is the heart of all three servers mentioned above but it can also be used as stand-alone (see example). 

Features:

   - threaded TCP server,
   - non-threaded TCP server,
   - non-threaded TCP client,
   - optional time-out to free up limited ESP32 resources used by inactive sessions,  
   - optional firewall for incoming connections.


**File system** is needed for storing configuration files, .html files used by web server, etc. FAT flash file system is used. Make sure you compile your sketch for one of FAT partition schemas (Tool | Partition Scheme | ...).


**Configuration files** Esp32_web_ftp_telnet_server_template uses Unix/Linux like network configuration files (you can edit these files yourself with vi telnet editor):

   - /etc/passwd                                     - Contains users' accounts information.
   - /etc/shadow                                     - Contains hashed users' passwords.
   - /network/interfaces                             - Contains WiFi STA(tion) configuration.
   - /etc/wpa_supplicant/wpa_supplicant.conf         - Contains WiFi STA(tion) credentials.
   - /etc/dhcpcd.conf                                - Contains WiFi A(ccess) P(oint) configuration.
   - /etc/hostapd/hostapd.conf                       - Contains WiFi A(ccess) P(oint) credentials.
   - /etc/ntp.conf                                   - Contains NTP time servers names.
   - /etc/crontab                                    - Contains scheduled tasks.
   -   /etc/mail/sendmail.cf                         - contains sendMail default settings.

These files (except for /etc/mail/sendmail.cf) are created at first run of your sketch with default settings (you can modify default settings in source code before you run the sketch for the first time). 


**User accounts**. Three types of managing user login are supported (depending on how USER_MANAGEMENT is #define-d):

   - Unix/Linux like (using user management files - this is the default setting) - use #define USER_MANAGEMENT   UNIX_LIKE_USER_MANAGEMENT,
   - hardcoded (username is root, password is hardcoded in to Arduino sketch constants) - use #define USER_MANAGEMENT   HARDCODED_USER_MANAGEMENT,
   - no user management at all (everyone can Telnet or FTP to ESP32 servers without password) - use #define USER_MANAGEMENT   NO_USER_MANAGEMENT.

Only "root" user with "rootpassword" password, "webadmin" user with "webadminpassword" password, "webserver" (with no password, this is system account intended only to hold webserver home directory) and "telnetserver" (with no password, this is system account intended only to hold telnetserver home directory) users are created at initialization. You can create additional users if you need them or change their passwords at initialization (by modifying the part of code in user_management.h) or upload your own files onto ESP32 with FTP. User management implemented in Esp32_web_ftp_telnet_server_template is very basic, only 3 fields are used: user name, hashed password and home directory. The following files are used to store user management information:

   - /etc/passwd contains users' accounts information
   - /etc/shadow contains users' passwords

-----> Please see [Step by step guide to user management](User_management_step_by_step.md) for more information.


**Time functions**. Time_functions.h provides GMT to local time conversion from 35 different time zones. #define TIMEZONE to one of the following (or add your own and modify timeToLocalTime function appropriately): 

KAL_TIMEZONE, MSK_TIMEZONE, SAM_TIMEZONE, YEK_TIMEZONE, OMS_TIMEZONE, KRA_TIMEZONE, IRK_TIMEZONE, YAK_TIMEZONE, VLA_TIMEZONE, SRE_TIMEZONE, PET_TIMEZONE, JAPAN_TIMEZONE, CHINA_TIMEZONE, WET_TIMEZONE, ICELAND_TIMEZONE, CET_TIMEZONE, EET_TIMEZONE, FET_TIMEZONE, NEWFOUNDLAND_TIMEZONE, ATLANTIC_TIME_ZONE, ATLANTIC_NO_DST_TIMEZONE, EASTERN_TIMEZONE, EASTERN_NO_DST_TIMEZONE, CENTRAL_TIMEZONE, CENTRAL_NO_DST_TIMEZONE, MOUNTAIN_TIMEZONE, MOUNTAIN_NO_DST_TIMEZONE, PACIFIC_TIMEZONE, ATLANTIC_NO_DST_TIMEZONE, EASTERN_TIMEZONE, CENTRAL_TIMEZONE, MOUNTAIN_TIMEZONE, PACIFIC_TIMEZONE, ALASKA_TIMEZNE, HAWAII_ALEUTIAN_TIMEZONE, HAWAII_ALEUTIAN_NO_DST_TIMEZONE, AMERICAN_SAMOA_TIMEZONE, BAKER_HOWLAND_ISLANDS_TIMEZONE, WAKE_ISLAND_TIMEZONE, CHAMORRO_TIMEZONE.

By default, TIMEZONE is #define-d as: #define TIMEZONE CET_TIMEZONE. Time_functions.h also takes care of synchronizing ESP32 clock with NTP servers once a day.


**cronDaemon** scans crontab table and executes tasks at specified time. You can schedule tasks from code or write them in to /etc/crontab file (see examples). 

-----> Please see [Step-by-step guide to do something at specific time](cronDaemon_step_by_step.md) for more information.

-----> or [Reference manual](Reference_manual.md).


## Setup instructions

1. Copy all files in this package into Esp32_web_ftp_telnet_server_template directory.
2. Open Esp32_web_ftp_telnet_server_template.ino with Arduino IDE.
3. modify the default #definitions that will be written into configuration files when sketch runs for the first time.
```C++
   #define HOSTNAME    "MyESP32Server" // define the name of your ESP32 here
   #define MACHINETYPE "ESP32 NodeMCU" // describe your hardware here

   #define DEFAULT_STA_SSID          "YOUR_STA_SSID"               // define default WiFi settings (see network.h)
   #define DEFAULT_STA_PASSWORD      "YOUR_STA_PASSWORD"
   #define DEFAULT_AP_SSID           HOSTNAME                      // set it to "" if you don't want ESP32 to act as AP 
   #define DEFAULT_AP_PASSWORD       "YOUR_AP_PASSWORD"            // must be at leas 8 characters long	

   #define DEFAULT_NTP_SERVER_1          "1.si.pool.ntp.org"       // define default NTP severs ESP32 will synchronize its time with
   #define DEFAULT_NTP_SERVER_2          "2.si.pool.ntp.org"
   #define DEFAULT_NTP_SERVER_3          "3.si.pool.ntp.org"
	
   #define TIMEZONE  CET_TIMEZONE                                  // define time zone you are in (see time_functions.h)  
	
   #define USER_MANAGEMENT UNIX_LIKE_USER_MANAGEMENT               // define the kind of user management project is going to use (see user_management.h)
```
4. Select one of FAT partition schemas (Tool | Partition Scheme).
5. Compile sketch and run it on your ESP32.

Doing this the following will happen:

  - ESP32 flash memory will be formatted with FAT file system. WARNING: every information you have stored into ESP32’s flash memory will be lost.
  - network configuration files will be created with the following settings:
  - your ESP32 will be configured to use DHCP in STAtion mode (with DEFAULT_STA_SSID / DEFAULT_STA_PASSWORD #definitions),
  - your ESP32 AccessPoint SSID will be MyESP32Server (with DEFAULT_AP_PASSWORD #definition),
  - your ESP32 AccessPoint IP will be 10.0.1.3,
   - user management files will be created with the following user accounts:
      - root / rootpassword,
      - webadmin / webadminpassword,
      - webserver with no password since this is a system account used only to define webserver home directory,
      - telnetserver with no password since this is also a system account used only to define telnetserver home directory.

At this point, you can already test if everything is going on as planned by http, FTP or telnet to your ESP32. Your ESP32 is already working as a server but there are a few minor things yet left to be done.

6. FTP to your ESP32 as webadmin / webadminpassword (with command line fto or Windows Explorer) and upload the following files into /var/www/html/ directory:

   - index.html,
   - android-192.png,
   - apple-180.png,
   - android-192-osc.png,
   - apple-180-osc.png,
   - example02.html,
   - example03.html,
   - example04.html,
   - example05.html,
   - login.html
   - logout.html
   - example10.html,
   - oscilloscope.html.

```
C:\esp32_web_ftp_telnet_server_template>ftp yourEsp32IP
Connected to 10.0.0.22.
220-MyESP32Server FTP server - please login
200 UTF8 enabled
User (10.0.0.22:(none)): webadmin
331 enter password: webadminpassword
230 logged on, your home directory is "/var/www/html/"
ftp> put index.html
226 transfer complete
ftp> put android-192.png
226 transfer complete
ftp> put apple-180.png
226 transfer complete
ftp> put android-192-osc.png
226 transfer complete
ftp> put apple-180-osc.png
226 transfer complete
ftp> put example02.html
226 transfer complete
ftp> put example03.html
226 transfer complete
ftp> put example04.html
226 transfer complete
ftp> put example05.html
226 transfer complete
ftp> put login.html
226 transfer complete
ftp> put logout.html
226 transfer complete
ftp> put example10.html
226 transfer complete
ftp> put oscilloscope.html
226 transfer complete
ftp> ls /var/www/html/
200 port ok
150 starting transfer
-rw-rw-rw-   1 root     root             1818  Nov 14 21:33      android-192.png
-rw-rw-rw-   1 root     root             1596  Nov 14 21:33      apple-180.png
-rw-rw-rw-   1 root     root             1818  Nov 14 21:33      android-192-osc.png
-rw-rw-rw-   1 root     root             1596  Nov 14 21:33      apple-180-osc.png
-rw-rw-rw-   1 root     root             6807  Nov 14 21:33      index.html
-rw-rw-rw-   1 root     root            39561  Nov 14 21:35      oscilloscope.html
-rw-rw-rw-   1 root     root             1147  Nov 14 21:33      example02.html
-rw-rw-rw-   1 root     root             1905  Nov 14 21:34      example03.html
-rw-rw-rw-   1 root     root             3521  Nov 14 21:34      example04.html
-rw-rw-rw-   1 root     root             9355  Nov 14 21:34      example05.html
-rw-rw-rw-   1 root     root             3177  Nov 14 21:34      
login.html
-rw-rw-rw-   1 root     root             4847  Nov 14 21:34      logout.html
-rw-rw-rw-   1 root     root             2633  Nov 14 21:34      
example10.html
226 transfer complete
```

Files will be placed into webadmin home directory, which is /var/www/html/.

7. FTP to your ESP32 as root / rootpassword and upload help.txt into /var/telnet/ directory, which is a home directory of telnetserver system account.

```
C:\esp32_web_ftp_telnet_server_template>ftp yourEsp32IP
Connected to 10.0.0.22.
220-MyESP32Server FTP server - please login
200 UTF8 enabled
User (10.0.0.22:(none)): root
331 enter password: rootpassword
230 logged on, your home directory is "/"
ftp> put help.txt /var/telnet/help.txt
226 transfer complete
```

8. Delete all the examples and functionalities that don't need and all the references to them in the code. They are included just to make the development easier for you.


## How to continue from here?

Esp32_web_ftp_telnet_server_template is what its name says, just a working template. A programmer is highly encouraged to add or change each piece of code as he or she sees appropriate for his or her projects. Esp32_web_ftp_telnet_server_template.ino is pretty comprehensive, small and easy to modify so it may be a good starting point.


## Building HTML user interface for your ESP32 project

A series of examples will demonstrate how to create a neat HTML user interface for your ESP32 project.


**Example 01 - dynamic HTML page**

You can always use static HTML that can be uploaded (with FTP) as .html files into /var/www/html directory but they would always display the same content. If you want to show what is going on in your ESP32 you can generate a dynamic HTML page for each HTTP request. The easiest way is modifying httpRequestHandler function that already exists in Esp32_web_ftp_telnet_server_template.ino according to your needs. For example:

```C++
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  if (httpRequest.substring (0, 20) == "GET /example01.html ") 
    return String ("<HTML>Example 01 - dynamic HTML page<br><br><hr />") + (digitalRead (2) ? "Led is on." : "Led is off.") + String ("<hr /></HTML>");
                                                                   
  return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}
```


**Example 02 - static HTML page calling REST functions**

Once HTML pages get large enough dynamic generation becomes impractical. The preferred way is building a large static HTML page that will be accessed by web browser and would call smaller server-side functions. In this case, we have two sides where programming code is located at run time. The server-side code is still in ESP32 whereas client-side code is in your web browser. They communicate with each other in standardised way - through REST functions. Although REST functions could respond in HTML manner they usually use JSON format.

Let's take a look at server-side first. Change code in httpRequestHandler function that already exists in Esp32_web_ftp_telnet_server_template.ino:

```C++
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!
  
  if (httpRequest.substring (0, 16) == "GET /builtInLed ") {
      return "{\"id\":\"esp32\",\"builtInLed\":\"" + (digitalRead (2) ? String ("on") : String ("off")) + "\"}\r\n";

  return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}
```

We do not have C++ compiler available in browser, but Javascript will do the job as well. See example02.html:

```HTML
<html>
  Example 02 - static HTML page calling REST functions<br><br>
  <hr />
  Led is <span id='ledState'>...</span>.
  <hr />
  <script type='text/javascript'>

    // mechanism that makes REST calls
    var httpClient = function () { 
      this.request = function (url, method, callback) {
        var httpRequest = new XMLHttpRequest ();
        httpRequest.onreadystatechange = function () {
          if (httpRequest.readyState == 4 && httpRequest.status == 200) callback (httpRequest.responseText);
        }
        httpRequest.open (method, url, true);
        httpRequest.send (null);
      }
    }

    // make a REST call and initialize/populate this page
    var client = new httpClient ();
    client.request ('/builtInLed', 'GET', function (json) {
                                                            // json reply looks like: {"id":"ESP32_SRV","builtInLed":"on"}
                                                            document.getElementById ('ledState').innerText = (JSON.parse (json).builtInLed);
                                                          });

  </script>
</html>
```


**Example 03 - HTML page interacting with ESP32**

We had only one-way server-client (ESP32-HTML) communication so far. It was used to initialize/populate HTML page. However, the communication can also be bi-directional – client (HTML page) telling the server (ESP32) what to do. We will use the same mechanism except for the use of PUT method instead of GET in REST calls (the latter is only the matter of understanding; GET method would do the job just as well).

Server will have to handle two additional cases:

```C++
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!

  if (httpRequest.substring (0, 16) == "GET /builtInLed ") {
getBuiltInLed:
      return "{\"id\":\"esp32\",\"builtInLed\":\"" + (digitalRead (2) ? String ("on") : String ("off")) + "\"}\r\n";
  } else if (httpRequest.substring (0, 19) == "PUT /builtInLed/on ") {
      digitalWrite (2, HIGH);
      goto getBuiltInLed;
  } else if (httpRequest.substring (0, 20) == "PUT /builtInLed/off ") {
      digitalWrite (2, LOW);
      goto getBuiltInLed;
  } 

  return  // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}
```

In HTML we use input tag of checkbox type. See example03.html:

```HTML
<html>
  Example 03 - HTML page interacting with ESP32<br><br>
  <hr />
  Led: <input type='checkbox' disabled id='ledSwitch' onClick='turnLed (this.checked)'>
  <hr />
  <script type='text/javascript'>

    // mechanism that makes REST calls
    var httpClient = function () { 
      this.request = function (url, method, callback) {
        var httpRequest = new XMLHttpRequest ();
        httpRequest.onreadystatechange = function () {
          if (httpRequest.readyState == 4 && httpRequest.status == 200) callback (httpRequest.responseText);
        }
        httpRequest.open (method, url, true);
        httpRequest.send (null);
      }
    }

    // make a REST call and initialize/populate this page
    var client = new httpClient ();
    client.request ('/builtInLed', 'GET', function (json) {
                                                            // json reply will look like: {"id":"ESP32_SRV","builtInLed":"on"}
                                                            var obj=document.getElementById ('ledSwitch'); 
                                                            obj.disabled = false; 
                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');
                                                          });

  function turnLed (switchIsOn) { // send desired led state to ESP32 and refresh ledSwitch state
    client.request (switchIsOn ? '/builtInLed/on' : '/builtInLed/off' , 'PUT', function (json) {
                                                            var obj = document.getElementById ('ledSwitch'); 
                                                            obj.checked = (JSON.parse (json).builtInLed == 'on');                                                           
                                                          });
  }

  </script>
</html>
```

Everything works fine now but it looks awful.


**Example 04 - user interface with style**
   
Style user interface with CSS like in example04.html:

```HTML
<html>

  <style>
   /* nice page framework */
   hr {border: 0; border-top: 1px solid lightgray; border-bottom: 1px solid lightgray}
   h1 {font-family: verdana; font-size: 40px; text-align: center}
   div.d1 {position: relative; overflow: hidden; width: 100%; height: 40px}
   div.d2 {position: relative; float: left; width: 15%; font-family: verdana; font-size: 30px; color: gray;}
   div.d3 {position: relative; float: left; width: 85%; font-family: verdana; font-size: 30px; color: black;}

   /* nice switch control */
   .switch {position: relative; display: inline-block; width: 60px; height: 34px}
   .slider {position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; -webkit-transition: .4s; transition: .4s}
   .slider:before {position: absolute; content: ''; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; -webkit-transition: .4s; transition: .4s}
   input:checked+.slider {background-color: #2196F3}
   input:focus+.slider {box-shadow: 0 0 1px #2196F3}
   input:checked+.slider:before {-webkit-transform: translateX(26px); -ms-transform: translateX(26px); transform: translateX(26px)}
   .switch input {display: none}
   .slider.round {border-radius: 34px}
   .slider.round:before {border-radius: 50%}
   input:disabled+.slider {background-color: #aaa}
  </style>

  <body>

    <br><h1>Example 04 - user interface with style</h1>

    <hr />
    <div class='d1'>
     <div class='d2'>&nbsp;Led</div>
     <div class='d3'>
      <label class='switch'><input type='checkbox' id='ledSwitch' disabled onClick='turnLed(this.checked)'><div class='slider round'></div></label>
     </div>
    </div>
    <hr />

    <script type='text/javascript'>

      // mechanism that makes REST calls
      var httpClient = function () { 
        this.request = function (url, method, callback) {
          var httpRequest = new XMLHttpRequest ();
          httpRequest.onreadystatechange = function () {
            if (httpRequest.readyState == 4 && httpRequest.status == 200) callback (httpRequest.responseText);
          }
          httpRequest.open (method, url, true);
          httpRequest.send (null);
        }
      }

      // make a REST call and initialize/populate this page
      var client = new httpClient ();
      client.request ('/builtInLed', 'GET', function (json) {
                                                              // json reply will be in a form: {"id":"ESP32_SRV","builtInLed":"on"}
                                                              var obj = document.getElementById ('ledSwitch'); 
                                                              obj.disabled = false; 
                                                              obj.checked = (JSON.parse (json).builtInLed == 'on');
                                                            });

    function turnLed (switchIsOn) { // send desired led state to ESP and refresh ledSwitch state
      client.request (switchIsOn ? '/builtInLed/on' : '/builtInLed/off' , 'PUT', function (json) {
                                                              var obj = document.getElementById ('ledSwitch');  
                                                              obj.checked = (JSON.parse (json).builtInLed == 'on');                                                           
                                                            });
    }

    </script>
  </body>
</html>
```

More controls with style can be found in **example05.html**.


## Building Telnet user interface for your ESP32 project

**Example 06 - processing Telnet command line**

Compared to HTML user interface Telnet user interface is a piece of cake. Modify telnetCommandHandler function that already exists in Esp32_web_ftp_telnet_server_template.ino according to your needs. For example:

```C++
String telnetCommandHandler (int argc, String argv [], telnetServer::telnetSessionParameters *tsp) { // - must be reentrant!

  if (argc == 2 && argv [0] == "led" && argv [1] == "state") {
getBuiltInLed:
    return "Led is " + (digitalRead (2) ? String ("on.\n") : String ("off.\n"));
  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "on") {
    digitalWrite (2, HIGH);
    goto getBuiltInLed;
  } else if (argc == 3 && argv [0] == "turn" && argv [1] == "led" && argv [2] == "off") {
    digitalWrite (2, LOW);
    goto getBuiltInLed;
  }

  return; // telnetCommand has not been handled by telnetCommandHandler - tell telnetServer to handle it internally by returning "" reply
}
```


## Other examples

**Example 07 - cookies**

Example 07 demonstrates the way web server handles cookies.

```C++
String httpRequestHandler (String& httpRequest, httpServer::wwwSessionParameters *wsp) { // - must be reentrant!
  
  if (httpRequest.substring (0, 20) == "GET /example07.html ") { // used by example 07
                                                                 String refreshCounter = wsp->getHttpRequestCookie ("refreshCounter");           // get cookie that browser sent in HTTP request
                                                                 if (refreshCounter == "") refreshCounter = "0";
                                                                 refreshCounter = String (refreshCounter.toInt () + 1);
                                                                 wsp->setHttpResponseCookie ("refreshCounter", refreshCounter, getGmt () + 60);  // set 1 minute valid cookie that will be send to browser in HTTP reply
                                                                 return String ("<HTML>Example 07<br><br>This page has been refreshed " + refreshCounter + " times. Click refresh to see more.</HTML>");
                                                               }

  return ""; // httpRequestHandler did not handle the request - tell httpServer to handle it internally by returning "" reply
}
```


**Example 08 - reading time**

Example 08 demonstrates the use of time_functions.h.

```C++
time_t t = getGmt ();
if (!t) {
  Serial.printf ("[example 08] current time has not been obtained from NTP server(s) yet\n");
} else {
  Serial.printf ("[example 08] current Unix time is %li\n", t);

  char str [30];
  time_t l = timeToLocalTime (t); // alternativelly you can use l = getLocalTime ();
  strftime (str, 30, "%d.%m.%y %H:%M:%S", gmtime (&l));
  Serial.printf ("[example 08] current local time is %s\n", str); // alternativelly you can use timeToString (l)
} 
```


**Example 09 - making HTTP requests (REST calls for example) directly from ESP32**

Up to now, we have only made REST calls from within Javascript (from browser). However, for establishing machine-to-machine communication REST calls should be made directly from C++ code residing in ESP32. The only drawback is that C++ does not support parsing JSON answers natively. You have to program the parsing yourself (not included in this example).

Example 09 shows how we can use TcpServer objects to make HTTP requests.

```C++
void example09_makeRestCall () {
  String s = webClient ((char *) "127.0.0.1", 80, 5000, "GET /builtInLed"); // send request to local loopback port 80, wait max 5 s (time-out)
  if (s > "")
    Serial.print ("[example 09] " + s);
  else
    Serial.printf ("[example 09] the reply didn't arrive (in time) or it is corrupt or too long\n");

  return;
}
```


## WebSockets

**Example 10 - WebSockets**

A basic WebSocket support is built-in into webServer. Text and binary data can be exchanged between browser and ESP32 server in both ways. I only tested the example below on little endian machines. The code is supposed to work on big endian machines as well (I was not able to test it though).

Example 10 demonstrates how ESP32 server could handle WebSockets:

```C++
void wsRequestHandler (String& wsRequest, WebSocket *webSocket) { // - must be reentrant!

  if (wsRequest.substring (0, 26) == "GET /example10_WebSockets ") { // Example 10

    while (true) {
      switch (webSocket->available ()) {
        case WebSocket::NOT_AVAILABLE:  delay (1);
                                        break;
        case WebSocket::STRING:       { // text received
                                        String s = webSocket->readString ();
                                        Serial.printf ("[example 10] got text from browser over webSocket: %s\n", s.c_str ());
                                        break;
                                      }
        case WebSocket::BINARY:       { // binary data received
                                        char buffer [256];
                                        int bytesRead = webSocket->readBinary (buffer, sizeof (buffer));
                                        Serial.printf ("[example 10] got %i bytes of binary data from browser over webSocket\n", bytesRead);
                                        // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 16 bit integers
                                        // (I know they are 16 bit integers because I have written javascript client example myself but this information can not be obtained from webSocket)
                                        int16_t *i = (int16_t *) buffer;
                                        while ((char *) (i + 1) <= buffer + bytesRead) Serial.printf (" %i", *i ++);
                                        Serial.printf ("\n[example 10] Looks like this is the Fibonacci sequence,\n"
                                                         "[example 10] which means that both, endianness and complements are compatible with javascript client.\n");
                                        
                                        // send text data
                                        if (!webSocket->sendString ("Thank you webSocket client, I'm sending back 8 32 bit binary floats.")) goto errorInCommunication;
  
                                        // send binary data
                                        float geometricSequence [8] = {1.0}; for (int i = 1; i < 8; i++) geometricSequence [i] = geometricSequence [i - 1] / 2;
                                        if (!webSocket->sendBinary ((byte *) geometricSequence, sizeof (geometricSequence))) goto errorInCommunication;
                                                         
                                        break; // this is where webSocket connection ends - in our simple "protocol" browser closes the connection but it could be the server as well ...
                                               // ... just "return;" in this case (instead of break;)
                                      }
        case WebSocket::ERROR:          
  errorInCommunication:     
                                        Serial.printf ("[example 10] error in communication, closing connection\n");
                                        return; // close this connection
      }
    }
}
```

On the browser side Javascript program could look something like example10.html:

```HTML
<html>
  <body>

    Example 10 - using WebSockets<br><br>

    <script type='text/javascript'>

      if ("WebSocket" in window) {
        var ws = new WebSocket ("ws://" + self.location.host + "/example10_WebSockets"); // open webSocket connection
				
        ws.onopen = function () {
          alert ("WebSocket connection established.");

          // ----- send text data -----

          ws.send ("Hello webSocket server, after this text I'm sending 32 16 bit binary integers."); 
          // alert ("Text sent over WebSocket.");

          // ----- send binary data -----

          const fibonacciSequence = new Uint16Array (32);
          fibonacciSequence [0] = -21;
          fibonacciSequence [1] = 13;
          for (var i = 2; i < 32; i ++) fibonacciSequence [i] = fibonacciSequence [i - 1] + fibonacciSequence [i - 2]; 
          ws.send (fibonacciSequence);
          // alert ("Binary data sent over WebSocket.");
        };

        ws.onmessage = function (evt) { 
          if (typeof(evt.data) === 'string' || evt.data instanceof String) { // UTF-8 formatted string data

            // ----- receive text data -----

            alert ("[example 10] got text from server over webSocket: " + evt.data);
	  }
          if (evt.data instanceof Blob) { // binary data

            // ----- receive binary data as blob and then convert it into array buffer -----

            var myFloat32Array = null;
            var myArrayBuffer = null;
            var myFileReader = new FileReader ();
            myFileReader.onload = function (event) {
              myArrayBuffer = event.target.result;
              myFloat32Array = new Float32Array (myArrayBuffer); // <= our data is now here, in the array of 32 bit floating point numbers

              var myMessage = "[example 10] got " + myArrayBuffer.byteLength + " bytes of binary data from server over webSocket\n[example 09]";
              for (var i = 0; i < myFloat32Array.length; i++) myMessage += " " + myFloat32Array [i];
                alert (myMessage);
                // note that we don't really know anything about format of binary data we have got, we'll just assume here it is array of 32 bit floating point numbers
                // (I know they are 32 bit floating point numbers because I have written server C++ example myself but this information can not be obtained from webSocket)

                alert ("[example 10] Looks like this is the geometric sequence,\n" + 
                       "[example 10] which means that 32 bit floating point format is compatible with ESP32 C++ server.\n");

                ws.close (); // this is where webSocket connection ends - in our simple "protocol" browser closes the connection but it could be the server as well

            };
            myFileReader.readAsArrayBuffer (evt.data);

          }
        };
				
        ws.onclose = function () { 
          alert ("WebSocket connection is closed."); 
        };

      } else {
        alert ("WebSockets are not supported by your browser.");
      }

    </script>
  </body>
</html>
```


## Run-time monitoring ESP32 behaviour

**Example 12 - monitor your ESP32 behaviour with dmesg C++ function and dmesg Telnet command**

Telnet server provides Unix/Linux like dmesg circular message queue. You can monitor your ESP32 behaviour even when it is not connected to a computer with USB cable. How does it work? In your C++ code use dmesg (String); function to insert important message about the state of your code into dmesg circular queue. When you want to view it, connect to your ESP32 with telnet client and type dmesg command.

![Screenshot](dmesg.png)


## Run-time monitoring ESP32 signals

**Oscilloscope - see the signals the way ESP32 sees them through Web interface**

ESP32 oscilloscope is web-based application included in Esp32_web_ftp_telnet_server_template. It is accessible through web browser once oscilloscope.html is uploaded to ESP32 (with FTP). ESP32 Oscilloscope is using WebSockets to exchange measured signal values between ESP32 and web browser.

The first picture below was generated by bouncing of an end switch. ESP32 Oscilloscope performed digitalRead-s after pin has been initialised in INPUT_PULLUP mode. The second picture shows noise coming from poorly regulated power supply. ESP32 Oscilloscope used analogRead-s on an unconnected pin.

![Screenshot](oscilloscope.png)