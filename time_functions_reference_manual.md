# Programmer's reference manual for time_functions.h


## Compile time definitions

Before #include-ing “time_functions.h” the following definitions can be #defined:

'''C++
#define TIMEZONE   CET_TIMEZONE   // this is the default definition
'''

  - TIMEZONE tells ESP32 server how it should calculate local time from GMT. You can change the default setting to one of already supported time zones: KAL_TIMEZONE, MSK_TIMEZONE, SAM_TIMEZONE, YEK_TIMEZONE, OMS_TIMEZONE, KRA_TIMEZONE, IRK_TIMEZONE, YAK_TIMEZONE, VLA_TIMEZONE, SRE_TIMEZONE, PET_TIMEZONE, JAPAN_TIMEZONE, CHINA_TIMEZONE, WET_TIMEZONE, ICELAND_TIMEZONE, CET_TIMEZONE, EET_TIMEZONE, FET_TIMEZONE, NEWFOUNDLAND_TIMEZONE, ATLANTIC_TIME_ZONE, ATLANTIC_NO_DST_TIMEZONE, EASTERN_TIMEZONE, EASTERN_NO_DST_TIMEZONE, CENTRAL_TIMEZONE, CENTRAL_NO_DST_TIMEZONE, MOUNTAIN_TIMEZONE, MOUNTAIN_NO_DST_TIMEZONE, PACIFIC_TIMEZONE, ATLANTIC_NO_DST_TIMEZONE, EASTERN_TIMEZONE, CENTRAL_TIMEZONE, MOUNTAIN_TIMEZONE, PACIFIC_TIMEZONE, ALASKA_TIMEZNE, HAWAII_ALEUTIAN_TIMEZONE, HAWAII_ALEUTIAN_NO_DST_TIMEZONE, AMERICAN_SAMOA_TIMEZONE, BAKER_HOWLAND_ISLANDS_TIMEZONE, WAKE_ISLAND_TIMEZONE, CHAMORRO_TIMEZONE.

'''C++
#define DEFAULT_NTP_SERVER_1   "1.si.pool.ntp.org"   // this is the default definition 
#define DEFAULT_NTP_SERVER_2   "2.si.pool.ntp.org"   // this is the default definition
#define DEFAULT_NTP_SERVER_3   "3.si.pool.ntp.org"   // this is the default definition
'''

  - NTP_SERVER definitions tell ESP32 server where to get current time (GMT) from, when certain functions are being called (see startCronDaemonAndInitializeItAtFirstCall). You can change the default settings to NTP servers close to you. These #definitions will be written to /etc/ntp.conf file which ESP32 reads each time it starts up. 


## void setGmt (time_t t);

**Description**

setGmt sets ESP32 built-in clock with current time (GMT). If setLocalTime has already been called there is no need of calling also setGmt. This function is also called internally by ntpDate function or ntpdate telnet command.

**Declaration**

'''C++
void setGmt (time_t t)
'''

**Parameter**

  - time_t t is the number of seconds that elapsed since midnight of January 1, 1970 (GMT).


## void setLocalTime (time_t t);

**Description**

setLocalTime sets ESP32 built-in clock with current (local) time. It is different from setGmt (this function is also called internally by ntpDate function or ntpdate telnet command) only in parameter which holds local time instead of GMT. If setGmt has already been called there is no need of calling also setLocalTime. This function is also called internally by date telnet command.

**Declaration**

'''C++
void setLocalTime (time_t t)
'''

**Parameter**

  - time_t t is the number of seconds that elapsed since midnight of January 1, 1970 (local time).


## time_t getGmt ();

**Description**

getGmt returns the state of ESP32 built-in clock in GMT. 

**Declaration**

'''C++
time_t getGmt ();
'''

**Return value**

  - 0 if current time has not been set yet (by calling setGmt or setLocalTime)
  - GMT otherwise


## time_t getLocalTime ();

**Description**

getLocalTime returns the state of ESP32 built-in clock in local time. 

**Declaration**

'''C++
time_t getLocalTime ();
'''

**Return value**

  - 0 if current time has not been set yet (by calling setGmt or setLocalTime)
  - local time otherwise


## time_t timeToLocalTime (time_t t);

**Description**

timeToLocalTime coverts GMT to local time. This function is the only function that does the conversion. It is affected by TIMEZONE #definition.

**Declaration**

'''C++
time_t timeToLocalTime (time_t t)
'''

**Parameter**

  - time_t t is GMT time to be converted to local time.

**Return value**

  - local time converted from GMT.


## struct tm timeToStructTime (time_t t);

**Description**

timeToStructTime converts time_t to struct tm. 

**Declaration**

'''C++
struct tm timeToStructTime (time_t t);
'''

**Parameter**

  - time_t t is the time to be converted to struct tm.

**Return value**

  - struct_tm converted from time_t. 


## String timeToString (time_t t);

**Description**

timeToString converts time_t to String. 

**Declaration**

'''C++
String timeToString (time_t t);
'''

**Parameter**

  - time_t t is the time to be converted to String.

**Return value**

  - String in a form of YYYY/MM/DD hh:mm:ss (24 hour format). 


## String ntpDate (String ntpServer);   

**Description**

ntpDate synchronizes ESP32 built-in clock with NTP server. ESP32 must have an internet connection before ntpDate is called. This function is also called internally by ntpdate telnet command.


**Declaration**

'''C++
String ntpDate (String ntpServer);
'''

**Parameter**

  - String ntpServer is the name of NTP server by which ESP32 built-in clock is to be synchronized.

**Return value**

  - “” if synchronization succeeded
  - error message if not


## String ntpDate ();

**Description**

ntpDate synchronizes ESP32 built-in clock with one of NTP servers specified in /etc/ntp.conf configuration file. ESP32 must have an internet connection before ntpDate is called. File system must be mounted and /etc/ntp.conf configured. If this is not the case, then ntpDate will try the default NTP_SERVER #definitions. This function is also called internally by ntpdate telnet command and cronDaemon.

**Declaration**

'''C++
String ntpDate ();
'''

**Return value**

  - “” if synchronization succeeded
  - error message if not


## startCronDaemonAndInitializeItAtFirstCall (void (* cronHandler) (String&), size_t cronStack);

**Description**

startCronDaemonAndInitializeItAtFirstCall starts special task or process called cronDaemon that:

  - Updates internal ESP32 clock with NTP server(s) once a day.

  - constantly checks the content of cron table inside ESP32 server memory and calls cornHandler function when the time is right. cronDaemon will read its configuration from /etc/ntp.conf and /etc/crontab files when ESP32 starts up. If the files do not exist yet it will create new ones from compile time #definitions with default values.

**Declaration**

'''C++
void startCronDaemonAndInitializeItAtFirstCall (void (* cronHandler) (String&), size_t cronStack = (3 * 1024));
'''

**Parameters**

  - void (* cronHandler) (String&) is a callback function that cronDaemon will call as defined by cron table. Cron table resides in ESP32 memory and is filled with the content of /etc/crontab file each time ESP32 starts up. Beside this it can also be managed through cronTabAdd and cronTabDel functions. If you are not going to use cron table then this parameter can be NULL.

  - size_t cronStack is stack size cronDaemon will be using. cronDaemon runs as separate thread/task/process so it needs its own stack. 3 KB is only enough for daily time synchronizations with NTP servers. If you are going to use cronHandler you better increase it to 8 KB. If your cronHandler is stack-hungry you may have to add more stack.


## bool cronTabAdd (uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t day_of_week, String cronCommand, bool readFromFile = false);

**Description**

cronTabAdd adds a new cron command definition into cron table.

**Declaration**

'''C++
bool cronTabAdd (uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t day_of_week, String cronCommand, bool readFromFile = false); 
'''

**Parameters**

  -  uint8_t second defines the second (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t minute defines the minute (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t hour defines the hour (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t day defines the day of month (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t month defines the month of year (local time) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - uint8_t day_of_week defines the day of week (local time, Sunday = 1, …) when cronHandler will be called with cronCommand parameter. If it is not important set it to ANY.

  - String cronCommand is the croncommand that is going to be passed to cronHandler.

  - bool readFromFile = false is used internally, normally it should be left false.


## bool cronTabAdd (String cronTabLine, bool readFromFile = false);

**Description**

cronTabAdd adds a new cron command definition into cron table.

**Declaration**

'''C++
bool cronTabAdd (String cronTabLine, bool readFromFile = false); 
'''

**Parameters**

  - String cronTabLine contains definition cron command and when it should be called in text format. This is the same format as in /etc/crontab file: 6 numbers and cron command text. The numbers correspond to second, minute, hour, day of month, month of year, day of week when cronHandler will be called with cronCommand parameter. If it is not important set it to *.

  - bool readFromFile = false is used internally, normally it should be left false.


## int cronTabDel (String cronCommand);

**Description**

cronTabDel deletes cron command definition from cron table.

**Declaration**

'''C++
int cronTabDel (String cronCommand);
'''

**Parameters**

  - String cronCommand contains the nme of the command to be deleted from cron table.

**Return value**

The number of deleted cron tab entries.
