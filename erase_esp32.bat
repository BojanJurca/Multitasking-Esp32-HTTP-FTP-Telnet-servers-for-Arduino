:: To erase esp32 completely, do not rely on Arduino IDE and code upload, it has cluster and odd thing when uses FATFS.
:: xiaolaba, 2020-MAR-02

:: Arduino 1.8.13, esptool and path,
set esptool=C:\Users\user0\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\3.0.0/esptool.exe

:: erase whole flash of esp32
%esptool% --chip esp32 --port com5 --baud 921600 erase_flash

pause


