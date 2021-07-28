# Step-by-step guide to user management in ESP32_web_ftp_telnet_server_template
User management is composed of functions in user_management.h, built-in telnet commands (useradd, userdel and passwd) and two Unix/Linux like configuration files:

  - /etc/passwd   - this is where user information is stored
  - /etc/shadow   - this is where hashed passwords are stored

Although it is not meant for these files to be edited manually you can easily view their content if you have set up telnet server:

```
C:\>telnet <YOUR ESP32 IP>
Hello 10.18.1.88!
user: root
password: rootpassword

Welcome root,
your home directory is /,
use "help" to display available commands.

# cat /etc/passwd
root:x:0:::/:
webserver::100:::/var/www/html/:
telnetserver::101:::/var/telnet/:
webadmin:x:1000:::/var/www/html/:

# cat /etc/shadow
root:$5$de362bbdf11f2df30d12f318eeed87b8ee9e0c69c8ba61ed9a57695bbd91e481:::::::
webadmin:$5$40c6af3d1540ca2af132e1e93e7f5a5f624280b9d4d552a0bb103afe17c75c53:::::::

#
```

You can not set access rights for different objects, files for example, so everything a logged in user can or can't do must be checked by your code. A rough idea is that the level of access rights is determined by user’s home directory (as stored in /etc/passwd). This principle is fully respected by built-in telnet and FTP commands, but you must take care of everything else you write yourself, meaning if you don't prevent something based on user's name or user's home directory then the code will be executed. You can use all the functions in user_management.h for his purpose like: checkUserNameAndPassword, getUserHomeDirectory, userAdd, userDel, …

What we can see from the files above is that by default ESP32_web_ftp_telnet_server_template already creates two real users with their passwords: root/rootpassword and webadmin/webadminpassword. Webserver and telnetserver doesn’t have their passwords since they are not real users. The records in /etc/passwd only hold their home directories. This is where web server is going to look for .html, .png and other files and where telnet server is going to look for help.txt file.  

## 1. Checking user rights inside telnet session (a working example)
Telnet session is just a TCP connection through which characters are being send in both directions. At the beginning, the user introduces himself by logging in and this stays the same until telnet session or TCP connection ends. It is easy to remember which user has logged in since one TCP connection is handled by one thread/task/process on the server side. This is not something you would do from your code, but user's name and user’s home directory are passed to telnetCommandHandler by telnet server each time the user sends a command to execution by pressing enter. You can access this information through telnetSessionParameters and do with it whatever you consider necessary.

```C++
... demo code to be added ...
```

## 2. Checking user rights inside FTP session
Since there is nothing you can do from your code to interfere with FTP sessions you can’t do anything about user access rights as well. 

## 3. Checking user rights inside web session (a working example)
Normally everybody can use web server but if you would like to add login/logout functionality things are more complicated than with telnet sessions. The main difference comes from the fact that one telnet session consists of one TCP connection whereas one web session consists of many TCP connections. Normally there is one TCP connection for each web page that browser requests. When handling one TCP connection you cannot automatically know what happened in another. For example: if the user successfully passes user name and password to web server through one web page (one TCP connection handled by one server thread/task/process) there is no way you can get this information when the user tries to open another web page (in another TCP connection handled by another server thread/task/process). To overcome this difficulty web server and browser exchange pieces of information stored in cookies here and there, each time the browser requests a new page from web server.

```C++
... demo code to be added ...
```

## 4. Simplified user management
There are three levels of user management. You can choose which one to use by #define-ing USER_MANAGEMENT inside your code before #include-ing user_management.h. By default, USER_MANAGEMENT is #define-d as UNIX_LIKE_USER_MANAGEMENT. This is the case that is described above.

If you want to simplify thing a little bit you can #define USER_MANAGEMENT to be HARDCODED_USER_MANAGEMENT. In this case you will only have one user: root. Its ROOT_PASSWORD is #defined as “rootpassword” which is stored in ESP32’s RAM during start up. You can change this definition and keep it secret if you wish.

Another simplification is to #define USER_MANAGEMENT as NO_USER_MANAGEMENT. In this case user password and access rights are not going to be checked at all so everyone can do everithing.
