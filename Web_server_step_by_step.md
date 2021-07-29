# Step-by-step guide to web server of ESP32_web_ftp_telnet_server_template

The initial idea was to develop a tiny web server for ESP32 projects that allows fast development of web user interface. Two things were needed:

  -	A  httpRequestHandler function that would handle (small) programmable responses. httpRequestHandler function takes HTTP request as an argument and returns HTTP response. 
  -	Ability to send (large) .html files as responses to HTTP requests. As it turned out this also required FTP server to be able to upload the .html files to ESP32 in the first place.

Web server can handle both or just one of them. If you need only programmable responses than you can get rid of file system and FTP server. If you only need to serve .html files than you can get rid of httpRequestHandler function. All you have to do in this case is to upload .html files to /var/www/html directory. But you have to set up FTP server
and file system, of course. Don't forget to change partition scheme to one that uses FAT (Tools | Partition scheme |  ...) befor uploading the code to ESP32.

Everything is already prepared in the template. Whatever you are doing it would be a good idea to start with the template and then just start deleting everything you don’t need.


... to be continued ...
