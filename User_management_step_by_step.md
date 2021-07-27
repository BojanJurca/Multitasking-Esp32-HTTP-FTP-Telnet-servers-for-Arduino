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

You can not set access rights for different objects, files for example, so everything a logged in user can do must be checked by your code. A rough idea for the level of access rights is to be determined by user’s home directory (as stored in /etc/passwd). This principle is fully respected by built-in commands in telnet and FTP servers, but you must take care of everything else you write yourself. You can use all the functions in user_management.h for his purpose like: checkUserNameAndPassword, getUserHomeDirectory, userAdd, userDel, …

What we can see from the files above is that by default ESP32_web_ftp_telnet_server_template already creates two real users with their passwords: root/rootpassword and webadmin/webadminpassword. Webserver and telnetserver doesn’t have their password since they are not real users. The records in /etc/passwd only hold their home directories. This is where web server is going to look for .html, .png and other files and where telnet server is going to look for help.txt file.  

## 1. Checking user rights inside telnet session (a working example)
… missing ...

## 2. Checking user rights inside FTP session
Since there is nothing you can do from your code to interfere with FTP sessions you can’t do anything about user access rights as well. 

## 3. Checking user rights inside web session (a working example)
Normally you wouldn’t like to do this but if you do then this is more complicated … missing ...

## 4. Simplified user management
There are three levels of user management. You can choose which one to use by #define-ing USER_MANAGEMENT inside your code before #include-ing user_management.h. By default, USER_MANAGEMENT is #define-d as UNIX_LIKE_USER_MANAGEMENT. This is the case that is described above.

If you want to simplify thing a little bit you can #define USER_MANAGEMENT to be HARDCODED_USER_MANAGEMENT. In this case you will only have one user: root. Its ROOT_PASSWORD is #defined as “rootpassword” which is stored in ESP32’s RAM during start up. You can change this definition if you wish.

Another simplification is to #define USER_MANAGEMENT as NO_USER_MANAGEMENT. In this case user password and access rights are not going to be checked at all.
