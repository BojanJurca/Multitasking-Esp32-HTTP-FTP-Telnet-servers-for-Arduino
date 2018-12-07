/*
 * 
 * Debugmsg.h 
 * 
 *  This file is part of TcpServer project: https://github.com/BojanJurca/esp32_threaded_TCP_server
 *
 *  Debugmsg.h contains definitions that help debug the code.
 * 
 *  Adjust DEBUG_LEVEL to the level of details you need to display during program execution 
 * 
 * History:
 *          - first release, October 31, 2018, Bojan Jurca
 *  
 */


#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL 0
#endif

#define dbgprintf01(format) ;
#define dbgprintf02(format,param1) ;
#define dbgprintf03(format,param1,param2) ;
#define dbgprintf04(format,param1,param2,param3);
#define dbgprintf05(format,param1,param2,param3,param4) ;
#define dbgprintf11(format) ;
#define dbgprintf12(format,param1) ;
#define dbgprintf13(format,param1,param2) ;
#define dbgprintf14(format,param1,param2,param3) ;
#define dbgprintf15(format,param1,param2,param3,param4) ;
#define dbgprintf21(format) ;
#define dbgprintf22(format,param1) ;
#define dbgprintf23(format,param1,param2) ;
#define dbgprintf24(format,param1,param2,param3) ;
#define dbgprintf25(format,param1,param2,param3,param4) ;
#define dbgprintf31(format) ;
#define dbgprintf32(format,param1) ;
#define dbgprintf33(format,param1,param2) ;
#define dbgprintf34(format,param1,param2,param3) ;
#define dbgprintf35(format,param1,param2,param3,param4) ;
#define dbgprintf41(format) ;
#define dbgprintf42(format,param1) ;
#define dbgprintf43(format,param1,param2) ;
#define dbgprintf44(format,param1,param2,param3) ;
#define dbgprintf45(format,param1,param2,param3,param4) ;
#define dbgprintf51(format) ;
#define dbgprintf52(format,param1) ;
#define dbgprintf53(format,param1,param2) ;
#define dbgprintf54(format,param1,param2,param3) ;
#define dbgprintf55(format,param1,param2,param3,param4) ;
#define dbgprintf61(format) ;
#define dbgprintf62(format,param1) ;
#define dbgprintf63(format,param1,param2) ;
#define dbgprintf64(format,param1,param2,param3) ;
#define dbgprintf65(format,param1,param2,param3,param4) ;
#define dbgprintf71(format) ;
#define dbgprintf72(format,param1) ;
#define dbgprintf73(format,param1,param2) ;
#define dbgprintf74(format,param1,param2,param3) ;
#define dbgprintf75(format,param1,param2,param3,param4) ;
#define dbgprintf81(format) ;
#define dbgprintf82(format,param1) ;
#define dbgprintf83(format,param1,param2) ;
#define dbgprintf84(format,param1,param2,param3) ;
#define dbgprintf85(format,param1,param2,param3,param4) ;
#define dbgprintf91(format) ;
#define dbgprintf92(format,param1) ;
#define dbgprintf93(format,param1,param2) ;
#define dbgprintf94(format,param1,param2,param3) ;
#define dbgprintf95(format,param1,param2,param3,param4) ;
#if DEBUG_LEVEL >= 0
  #define dbgprintf01(format) Serial.printf(format);
  #define dbgprintf02(format,param1) Serial.printf(format,param1);
  #define dbgprintf03(format,param1,param2) Serial.printf(format,param1,param2); 
  #define dbgprintf04(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);   
  #define dbgprintf05(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 1
  #define dbgprintf11(format) Serial.printf(format);
  #define dbgprintf12(format,param1) Serial.printf(format,param1);
  #define dbgprintf13(format,param1,param2) Serial.printf(format,param1,param2); 
  #define dbgprintf14(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);
  #define dbgprintf15(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 2
  #define dbgprintf21(format) Serial.printf(format);
  #define dbgprintf22(format,param1) Serial.printf(format,param1);
  #define dbgprintf23(format,param1,param2) Serial.printf(format,param1,param2); 
  #define dbgprintf24(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);  
  #define dbgprintf25(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 3
  #define dbgprintf31(format) Serial.printf(format);
  #define dbgprintf32(format,param1) Serial.printf(format,param1);
  #define dbgprintf33(format,param1,param2) Serial.printf(format,param1,param2); 
  #define dbgprintf34(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);  
  #define dbgprintf35(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 4
  #define dbgprintf41(format) Serial.printf(format);
  #define dbgprintf42(format,param1) Serial.printf(format,param1);
  #define dbgprintf43(format,param1,param2) Serial.printf(format,param1,param2);
  #define dbgprintf44(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);
  #define dbgprintf45(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 5
  #define dbgprintf51(format) Serial.printf(format);
  #define dbgprintf52(format,param1) Serial.printf(format,param1);
  #define dbgprintf53(format,param1,param2) Serial.printf(format,param1,param2); 
  #define dbgprintf54(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);  
  #define dbgprintf55(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 6
  #define dbgprintf61(format) Serial.printf(format);
  #define dbgprintf62(format,param1) Serial.printf(format,param1);
  #define dbgprintf63(format,param1,param2) Serial.printf(format,param1,param2);
  #define dbgprintf64(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);
  #define dbgprintf65(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 7
  #define dbgprintf71(format) Serial.printf(format);
  #define dbgprintf72(format,param1) Serial.printf(format,param1);
  #define dbgprintf73(format,param1,param2) Serial.printf(format,param1,param2);
  #define dbgprintf74(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);
  #define dbgprintf75(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 8
  #define dbgprintf81(format) Serial.printf(format);
  #define dbgprintf82(format,param1) Serial.printf(format,param1);
  #define dbgprintf83(format,param1,param2) Serial.printf(format,param1,param2);
  #define dbgprintf84(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);
  #define dbgprintf85(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
#if DEBUG_LEVEL >= 9
  #define dbgprintf91(format) Serial.printf(format);
  #define dbgprintf92(format,param1) Serial.printf(format,param1);
  #define dbgprintf93(format,param1,param2) Serial.printf(format,param1,param2); 
  #define dbgprintf94(format,param1,param2,param3) Serial.printf(format,param1,param2,param3);  
  #define dbgprintf95(format,param1,param2,param3,param4) Serial.printf(format,param1,param2,param3,param4);  
#endif
