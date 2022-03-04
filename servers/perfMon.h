/*

    perfMon.h
  
    This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
  
    perfMon.h contains definitions of different performance counters that may be useful
  
    Januar, 22, 2022, Bojan Jurca
         
*/



// different performance counters


#ifndef __PERFMON__
  #define __PERFMON__

  // file system performance counters
  unsigned long __perfFSBytesRead__ = 0;
  unsigned long __perfFSBytesWritten__ = 0;

  // WiFi performance counters
  unsigned long __perfWiFiBytesReceived__ = 0;
  unsigned long __perfWiFiBytesSent__ = 0;

  // HTTP performance counters
  unsigned long __perfOpenedHttpConnections__ = 0;
  unsigned long __perOpenedfWebSockets__ = 0;  
  unsigned long __perfConcurrentHttpConnections__ = 0;
  unsigned long __perfConcurrentWebSockets__ = 0;
  unsigned long __perfHttpRequests__ = 0;

  // Telnet performance counters
  unsigned long __perfOpenedTelnetConnections__ = 0;
  unsigned long __perfConcurrentTelnetConnections__ = 0;

  // FTP performance counters
  unsigned long __perfOpenedFtpControlConnections__ = 0;
  unsigned long __perfConcurrentFtpControlConnections__ = 0;  

  // oscilloscope performance counters
  unsigned long __perfConcurrentOscWebSockets__ = 0;

#endif
