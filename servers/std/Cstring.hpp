/*
 *  Cstring.hpp for Arduino (ESP boards)
 * 
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 * 
 *  May 22, 2024, Bojan Jurca
 *  
 */


#ifndef __CSTRING_HPP__
    #define __CSTRING_HPP__


        // missing (Windows) C functions in Arduino

        char *stristr (const char *haystack, const char *needle) { 
            if (!haystack || !needle) return NULL; // nonsense
            int n = strlen (haystack) - strlen (needle) + 1;
            for (int i = 0; i < n; i++) {
                int j = i;
                int k = 0;
                char hChar, nChar;
                bool match = true;
                while (*(needle + k)) {
                    hChar = *(haystack + j); if (hChar >= 'a' && hChar <= 'z') hChar -= 32; // convert to upper case
                    nChar = *(needle + k); if (nChar >= 'a' && nChar <= 'z') nChar -= 32; // convert to upper case
                    if (hChar != nChar && nChar) {
                        match = false;
                        break;
                    }
                    j ++;
                    k ++;
                }
                if (match) return (char *) haystack + i; // match!
            }
            return NULL; // no match
        }

        int stricmp (const char *str1, const char *str2) { 
            while (*str1 && *str2) {
                char chr1 = *str1; if (chr1 >= 'a' && chr1 <= 'z') chr1 -= 32; // convert to upper case
                char chr2 = *str2; if (chr2 >= 'a' && chr2 <= 'z') chr2 -= 32; // convert to upper case
                if (chr1 < chr2) return -1;
                if (chr1 > chr2) return 1;
                str1 ++;
                str2 ++;
            }
            return 0; // stings are equal
        }


    // ----- TUNNING PARAMETERS -----

    #ifndef string
        #ifdef SHOW_COMPILE_TIME_INFORMATION
            #pragma message "Cstring with default Cstring size was not defined previously, #defining the default string as Cstring<300> in Cstring.hpp" 
        #endif
        #define cstring Cstring<300>
    #endif


    // error flags: there are only two types of error flags that can be set: err_overflow and err_out_of_range- please note that all errors are negative (char) numbers
    #define err_ok           ((signed char) 0b00000000) //    0 - no error    
    #define err_overflow     ((signed char) 0b10000001) // -127 - buffer overflow
    #define err_out_of_range ((signed char) 0b10000010) // -126 - invalid index


    // fixed size string, actually C char arrays with additional functions and operators
    template<size_t N> struct Cstring {

        private: 

            // internal storage: char array (= 0-terminated C string)
            char __c_str__ [N + 2] = {}; // add 2 characters at the end __c_str__ [N] will detect if string gets too long (owerflow), __c_str__ [N + 1] will guard the end of the string and will always be 0, initialize it with zeros
            signed char __errorFlags__ = 0;

        public:

            signed char errorFlags () { return __errorFlags__ & 0b01111111; }
            void clearErrorFlags () { __errorFlags__ = 0; }
        
        
            // constructors
            Cstring () {}                                      // for declarations like Cstring<15> a;

            Cstring (const char *other) {                      // for declarations with initialization, like Cstring<15> b ("abc");
                if (other) {                                                  // check if NULL char * pointer to overcome from possible programmer's errors
                    strncpy (this->__c_str__, other, N + 1);
                    if (this->__c_str__ [N]) {
                         __errorFlags__ = err_overflow;                       // err_overflow
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                        #ifdef __LOCALE_HPP__
                            if (__use_utf8__)
                                this->__rTrimUnfinishedUtf8Character__ ();
                        #endif
                    }
                }
            }

            Cstring (const Cstring& other) {                  // for declarations with initialization, like Cstring<15> c = a;            
                strncpy (this->__c_str__, other.__c_str__, N + 1);
                this->__errorFlags__ = other.__errorFlags__;                  // inherit all errors from other Cstring
            }
        
            Cstring (const char& other) {                      // for declarations with initialization, like Cstring<15> d ('c'); (convert char to Cstring)
                this->__c_str__ [0] = other;
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                    #ifdef __LOCALE_HPP__
                        if (__use_utf8__)
                            this->__rTrimUnfinishedUtf8Character__ ();
                    #endif
                }
            }

            Cstring (int number) {                             // for declarations with initialization, like Cstring<15> e (3); (convert int to Cstring)
                snprintf (this->__c_str__, N + 2, "%i", number);
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                }
            }

            Cstring (unsigned int number) {                    // for declarations with initialization, like Cstring<15> e (3); (convert unsigned int to Cstring)
                snprintf (this->__c_str__, N + 2, "%u", number);
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                }
            }

            Cstring (long number) {                            // for declarations with initialization, like Cstring<15> e (3); (convert long to Cstring)
                snprintf (this->__c_str__, N + 2, "%l", number);
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                }
            }

            Cstring (unsigned long number) {                   // for declarations with initialization, like Cstring<15> e (3); (convert unsigned long to Cstring)
                snprintf (this->__c_str__, N + 2, "%lu", number);
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                }
            }

            Cstring (float number) {                           // for declarations with initialization, like Cstring<15> e (3.1); (convert float to Cstring)
                snprintf (this->__c_str__, N + 2, "%f", number);
                #ifdef __LOCALE_HPP__
                    // replace decimal and thousand separators with '.' and ','
                    for (int i = 0; this->__c_str__ [i]; i++) {
                        if (this->__c_str__ [i] == __locale_decimalSeparator__)
                            this->__c_str__ [i] = '.';
                        else if (this->__c_str__ [i] == __locale_thousandSeparator__)
                            this->__c_str__ [i] = ',';
                    }
                #endif                
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                }
            }

            Cstring (double number) {                          // for declarations with initialization, like Cstring<15> e (3.1); (convert float to Cstring)
                snprintf (this->__c_str__, N + 2, "%lf", number);
                #ifdef __LOCALE_HPP__
                    // replace decimal and thousand separators with '.' and ','
                    for (int i = 0; this->__c_str__ [i]; i++) {
                        if (this->__c_str__ [i] == __locale_decimalSeparator__)
                            this->__c_str__ [i] = '.';
                        else if (this->__c_str__ [i] == __locale_thousandSeparator__)
                            this->__c_str__ [i] = ',';
                    }
                #endif                
                if (this->__c_str__ [N]) {
                    __errorFlags__ = err_overflow;                            // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                }
            }

            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                Cstring (time_t t) {                                // for declarations with initialization, like Cstring<15> t (time (NULL)); (convert time_t to Cstring)
                    struct tm st;
                    localtime_r (&t, &st);
                    char buf [80];
                    #ifdef __LOCALE_HPP__
                        strftime (buf, sizeof (buf), __locale_time__, &st);
                    #else
                        strftime (buf, sizeof (buf), "%Y/%m/%d %T", &st);
                    #endif

                    strncpy (this->__c_str__, buf, N + 1);
                    if (this->__c_str__ [N]) {
                        __errorFlags__ = err_overflow;                          // err_overflow
                        this->__c_str__ [N] = 0;                                // mark the end of the string regardles OVERFLOW
                    }
                }
            #endif


            // char *() operator so that Cstring can be used the same way as C char arrays, like; Cstring<5> a = "123"; Serial.printf ("%s\n", a);
            inline operator char *() __attribute__((always_inline)) { return __c_str__; }
            inline operator const char *() __attribute__((always_inline)) { return (const char *) __c_str__; }
            

            // = operator
            Cstring operator = (const char *other) {           // for assigning C char array to Cstring, like: a = "abc";
                if (other) {                                                  // check if NULL char * pointer to overcome from possible programmers' errors
                    strncpy (this->__c_str__, other, N + 1);
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ = err_overflow;                  // err_overflow
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                        #ifdef __LOCALE_HPP__
                            if (__use_utf8__)
                                this->__rTrimUnfinishedUtf8Character__ ();
                        #endif
                    } else {
                        this->__errorFlags__ = 0;                             // clear error after new assignment
                    }
                }
                return *this;
            }
    
            Cstring operator = (const Cstring& other) {       // for assigning other Cstring to Cstring, like: a = b;
                if (this != &other) {
                    strncpy (this->__c_str__, other.__c_str__, N + 1);
                    this->__errorFlags__ = other.__errorFlags__;              // inherit all errors from original string
                }
                return *this;
            }

            template<size_t M>
            Cstring operator = (Cstring<M>& other) {    // for assigning other Cstring to Cstring of different size, like: a = b;
                strncpy (this->__c_str__, other.c_str (), N + 1);
                this->__errorFlags__ = other.errorFlags ();                   // inherit all errors from original string
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                    #ifdef __LOCALE_HPP__
                        if (__use_utf8__)
                            this->__rTrimUnfinishedUtf8Character__ ();
                    #endif
                }
                return *this;
            }

            Cstring operator = (const char& other) {           // for assigning character to Cstring, like: a = 'b';
                this->__c_str__ [0] = other; 
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    #ifdef __LOCALE_HPP__
                        if (__use_utf8__)
                            this->__rTrimUnfinishedUtf8Character__ ();
                    #endif
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                #if N > 0
                    this->__c_str__ [1] = 0;                                  // mark the end of the string
                #else
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                #endif
                return *this;
            }   

            Cstring operator = (int number) {                   // for assigning int to Cstring, like: a = 1234;
                snprintf (this->__c_str__, N + 2, "%i", number);
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                return *this;
            }

            Cstring operator = (unsigned int number) {           // for assigning unsigned int to Cstring, like: a = 1234;
                snprintf (this->__c_str__, N + 2, "%u", number);
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles the OVERFLOW
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                return *this;
            }

            Cstring operator = (long number) {                   // for assigning long to Cstring, like: a = 1234;
                snprintf (this->__c_str__, N + 2, "%l", number);
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                return *this;
            }

            Cstring operator = (unsigned long number) {          // for assigning unsigned long to Cstring, like: a = 1234;
                snprintf (this->__c_str__, N + 2, "%lu", number);
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                return *this;
            }

            Cstring operator = (float number) {                  // for assigning float to Cstring, like: a = 1234.5;
                snprintf (this->__c_str__, N + 2, "%f", number);
                #ifdef __LOCALE_HPP__
                    // replace decimal and thousand separators with '.' and ','
                    for (int i = 0; this->__c_str__ [i]; i++) {
                        if (this->__c_str__ [i] == __locale_decimalSeparator__)
                            this->__c_str__ [i] = '.';
                        else if (this->__c_str__ [i] == __locale_thousandSeparator__)
                            this->__c_str__ [i] = ',';
                    }
                #endif                
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVEFLOW
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                return *this;
            }

            Cstring operator = (double number) {                 // for assigning double to Cstring, like: a = 1234.5;
                snprintf (this->__c_str__, N + 2, "%lf", number);
                #ifdef __LOCALE_HPP__
                    // replace decimal and thousand separators with '.' and ','
                    for (int i = 0; this->__c_str__ [i]; i++) {
                        if (this->__c_str__ [i] == __locale_decimalSeparator__)
                            this->__c_str__ [i] = '.';
                        else if (this->__c_str__ [i] == __locale_thousandSeparator__)
                            this->__c_str__ [i] = ',';
                    }
                #endif                
                if (this->__c_str__ [N]) {
                    this->__errorFlags__ = err_overflow;                      // err_overflow
                    this->__c_str__ [N] = 0;                                  // mark the end of the string regardles OVERFLOW
                } else {
                    this->__errorFlags__ = 0;                                 // clear error after new assignment
                }
                return *this;
            }

            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                Cstring operator = (time_t t) {                       // for assigning double to Cstring, like: a = time (NULL);
                    struct tm st;
                    localtime_r (&t, &st);
                    char buf [80];
                    #ifdef __LOCALE_HPP__
                        strftime (buf, sizeof (buf), __locale_time__, &st);
                    #else
                        strftime (buf, sizeof (buf), "%Y/%m/%d %T", &st);
                    #endif

                    strncpy (this->__c_str__, buf, N + 1);
                    if (this->__c_str__ [N]) {
                        __errorFlags__ = err_overflow;                          // err_overflow
                        this->__c_str__ [N] = 0;                                // mark the end of the string regardles OVERFLOW
                    }

                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ = err_overflow;                    // err_overflow
                        this->__c_str__ [N] = 0;                                // mark the end of the string regardles OVERFLOW
                    } else {
                        this->__errorFlags__ = 0;                               // clear error after new assignment
                    }
                    return *this;
                }
            #endif


            // += operator
            Cstring operator += (const char *other) {          // concatenate C string to Cstring, like: a += "abc";
                if (other && !(__errorFlags__ & err_overflow)) {              // check if NULL char * pointer to overcome from possible programmer's errors and that overwlow flag has not been set yet
                    strncat (this->__c_str__, other, N + 1 - strlen (this->__c_str__));
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                        #ifdef __LOCALE_HPP__
                            if (__use_utf8__)
                                this->__rTrimUnfinishedUtf8Character__ ();
                        #endif
                    } 
                }
                return *this;
            }

            Cstring operator += (const Cstring& other) {      // concatenate one Cstring to anoterh, like: a += b;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    strncat (this->__c_str__, other.__c_str__, N + 1 - strlen (this->__c_str__));
                    this->__errorFlags__ |= other.__errorFlags__;             // add all errors from other string
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                        #ifdef __LOCALE_HPP__
                            if (__use_utf8__)
                                this->__rTrimUnfinishedUtf8Character__ ();
                        #endif
                    } 
                }
                return *this;
            }

            template<size_t M>
            Cstring operator += (Cstring<M>& other) {      // concatenate one Cstring to another of different size, like: a += b;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    strncat (this->__c_str__, other.c_str (), N + 1 - strlen (this->__c_str__));
                    this->__errorFlags__ |= other.errorFlags ();              // add all errors from other string
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                        #ifdef __LOCALE_HPP__
                            if (__use_utf8__)
                                this->__rTrimUnfinishedUtf8Character__ ();
                        #endif
                    }
                } 
                return *this;
            }

            Cstring operator += (const char& other) {          // concatenate a charactr to Cstring, like: a += 'b';
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    if (l < N) { 
                        this->__c_str__ [l] = other; 
                        this->__c_str__ [l + 1] = 0; 
                    } else {
                        __errorFlags__ |= err_overflow;                           // add err_overflow flag to possibe already existing error flags
                        #ifdef __LOCALE_HPP__
                            if (__use_utf8__)
                                this->__rTrimUnfinishedUtf8Character__ ();
                        #endif
                    }
                }
                return *this;
            }   

            Cstring operator += (int number) {                 // concatenate an int to Cstring, like: a += 12;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    snprintf (this->__c_str__ + l, N + 2 - l, "%i", number);
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                    } 
                }
                return *this;
            }

            Cstring operator += (unsigned int number) {        // concatenate an int to Cstring, like: a += 12;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    snprintf (this->__c_str__ + l, N + 2 - l, "%u", number);
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                    }
                } 
                return *this;
            }   

            Cstring operator += (long number) {                // concatenate a long to Cstring, like: a += 12;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    snprintf (this->__c_str__ + l, N + 2 - l, "%l", number);
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                    }
                } 
                return *this;
            }   

            Cstring operator += (unsigned long number) {        // concatenate an unsigned long to Cstring, like: a += 12;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    snprintf (this->__c_str__ + l, N + 2 - l, "%lu", number);
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                    }
                } 
                return *this;
            }   

            Cstring operator += (float number) {                // concatenate a flaot to Cstring, like: a += 12;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    snprintf (this->__c_str__ + l, N + 2 - l, "%f", number);
                    #ifdef __LOCALE_HPP__
                        // replace decimal and thousand separators with '.' and ','
                        for (int i = l; this->__c_str__ [i]; i++) {
                            if (this->__c_str__ [i] == __locale_decimalSeparator__)
                                this->__c_str__ [i] = '.';
                            else if (this->__c_str__ [i] == __locale_thousandSeparator__)
                                this->__c_str__ [i] = ',';
                        }
                    #endif                
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                    }
                } 
                return *this;
            }   

            Cstring operator += (double number) {                // concatenate a double to Cstring, like: a += 12;
                if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                    size_t l = strlen (this->__c_str__);
                    snprintf (this->__c_str__ + l, N + 2 - l, "%lf", number);
                    #ifdef __LOCALE_HPP__
                        // replace decimal and thousand separators with '.' and ','
                        for (int i = l; this->__c_str__ [i]; i++) {
                            if (this->__c_str__ [i] == __locale_decimalSeparator__)
                                this->__c_str__ [i] = '.';
                            else if (this->__c_str__ [i] == __locale_thousandSeparator__)
                                this->__c_str__ [i] = ',';
                        }
                    #endif                
                    if (this->__c_str__ [N]) {
                        this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                        this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                    }
                } 
                return *this;
            }   

            #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno
                Cstring operator += (time_t t) {                    // concatenate a double to Cstring, like: a += time (NULL);
                    if (!(__errorFlags__ & err_overflow)) {                       // if overwlow flag has not been set yet
                        size_t l = strlen (this->__c_str__);
                        struct tm st;
                        localtime_r (&t, &st);
                        char buf [80];
                        #ifdef __LOCALE_HPP__
                            strftime (buf, sizeof (buf), __locale_time__, &st);
                        #else
                            strftime (buf, sizeof (buf), "%Y/%m/%d %T", &st);
                        #endif

                        strncat (this->__c_str__, buf, N + 1 - strlen (this->__c_str__));
                        if (this->__c_str__ [N]) {
                            this->__errorFlags__ |= err_overflow;                 // add err_overflow flag to possibe already existing error flags
                            this->__c_str__ [N] = 0;                              // mark the end of the string regardles OVERFLOW
                            #ifdef __LOCALE_HPP__
                                if (__use_utf8__)
                                    this->__rTrimUnfinishedUtf8Character__ ();
                            #endif
                        } 
                    } 
                    return *this;
                }
            #endif   


            // + operator
            Cstring operator + (const char *other) {             // for adding C string to Cstring, like: a + "abc";
                Cstring<N> tmp = *this; // copy of this, including error information
                tmp += other;
                return tmp;
            }
        
            Cstring operator + (const Cstring& other) {       // for concatenating one Cstring with anoterh, like: a + b;
                Cstring<N> tmp = *this; // copy of this, including error information
                tmp += other;
                return tmp;
            }

            template<size_t M>
            Cstring operator + (Cstring<M>& other) {          // concatenate one Cstring to another of different size, like: a + b;
                Cstring<N> tmp = *this; // copy of this, including error information
                tmp += other;
                return tmp;
            }

            Cstring operator + (const char& other) {           // for adding a character to Cstring, like: a + 'b';
                Cstring<N> tmp = *this; // copy of this, including error information
                tmp += other;
                return tmp;
            } 

            // can't use + operator for integers, this would make impossible to use for example Cstring + int to calculate the pointer to the location 

        
            // logical operators: ==, !=, <, <=, >, >=, ignore all possible errors

            inline bool operator == (const char *other) __attribute__((always_inline))        { return !strcmp (this->__c_str__, other); }              // Cstring : C string   
            inline bool operator == (char *other) __attribute__((always_inline))              { return !strcmp (this->__c_str__, other); }              // Cstring : C string   
            inline bool operator == (const Cstring& other) __attribute__((always_inline))     { return !strcmp (this->__c_str__, other.__c_str__); }    // Cstring : Cstring
            inline bool operator == (Cstring& other) __attribute__((always_inline))           { return !strcmp (this->__c_str__, other.__c_str__); }    // Cstring : Cstring

            inline bool operator != (const char *other) __attribute__((always_inline))        { return strcmp (this->__c_str__, other); }               // Cstring : C string
            inline bool operator != (char *other) __attribute__((always_inline))              { return strcmp (this->__c_str__, other); }               // Cstring : C string
            inline bool operator != (const Cstring& other) __attribute__((always_inline))     { return strcmp (this->__c_str__, other.__c_str__); }     // Cstring : Cstring
            inline bool operator != (Cstring& other) __attribute__((always_inline))           { return strcmp (this->__c_str__, other.__c_str__); }     // Cstring : Cstring

            #ifndef __LOCALE_HPP__
                inline bool operator <  (const char *other) __attribute__((always_inline))        { return strcmp (this->__c_str__, other) < 0; }           // Cstring : C string
                inline bool operator <  (char *other) __attribute__((always_inline))              { return strcmp (this->__c_str__, other) < 0; }           // Cstring : C string
                inline bool operator <  (const Cstring& other) __attribute__((always_inline))     { return strcmp (this->__c_str__, other.__c_str__) < 0; } // Cstring : Cstring
                inline bool operator <  (Cstring& other) __attribute__((always_inline))           { return strcmp (this->__c_str__, other.__c_str__) < 0; } // Cstring : Cstring

                inline bool operator <= (const char *other) __attribute__((always_inline))        { return strcmp (this->__c_str__, other) <= 0; }          // Cstring : C string
                inline bool operator <= (char *other) __attribute__((always_inline))              { return strcmp (this->__c_str__, other) <= 0; }          // Cstring : C string
                inline bool operator <= (const Cstring& other) __attribute__((always_inline))     { return strcmp (this->__c_str__, other.__c_str__) <= 0; }// Cstring : Cstring
                inline bool operator <= (Cstring& other) __attribute__((always_inline))           { return strcmp (this->__c_str__, other.__c_str__) <= 0; }// Cstring : Cstring

                inline bool operator >  (const char *other) __attribute__((always_inline))        { return strcmp (this->__c_str__, other) > 0; }           // Cstring : C string    
                inline bool operator >  (char *other) __attribute__((always_inline))              { return strcmp (this->__c_str__, other) > 0; }           // Cstring : C string    
                inline bool operator >  (const Cstring& other) __attribute__((always_inline))     { return strcmp (this->__c_str__, other.__c_str__) > 0; } // Cstring : Cstring
                inline bool operator >  (Cstring& other) __attribute__((always_inline))           { return strcmp (this->__c_str__, other.__c_str__) > 0; } // Cstring : Cstring

                inline bool operator >= (const char *other) __attribute__((always_inline))        { return strcmp (this->__c_str__, other) >= 0; }          // Cstring : C string    
                inline bool operator >= (char *other) __attribute__((always_inline))              { return strcmp (this->__c_str__, other) >= 0; }          // Cstring : C string    
                inline bool operator >= (const Cstring& other) __attribute__((always_inline))     { return strcmp (this->__c_str__, other.__c_str__) >= 0; }// Cstring : Cstring
                inline bool operator >= (Cstring& other) __attribute__((always_inline))           { return strcmp (this->__c_str__, other.__c_str__) >= 0; }// Cstring : Cstring
            #else
                inline bool operator <  (const char *other) __attribute__((always_inline))        { return __strUtf8cmp__ (this->__c_str__, other) < 0; }           // Cstring : C string
                inline bool operator <  (char *other) __attribute__((always_inline))              { return __strUtf8cmp__ (this->__c_str__, other) < 0; }           // Cstring : C string
                inline bool operator <  (const Cstring& other) __attribute__((always_inline))     { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) < 0; } // Cstring : Cstring
                inline bool operator <  (Cstring& other) __attribute__((always_inline))           { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) < 0; } // Cstring : Cstring

                inline bool operator <= (const char *other) __attribute__((always_inline))        { return __strUtf8cmp__ (this->__c_str__, other) <= 0; }          // Cstring : C string
                inline bool operator <= (char *other) __attribute__((always_inline))              { return __strUtf8cmp__ (this->__c_str__, other) <= 0; }          // Cstring : C string
                inline bool operator <= (const Cstring& other) __attribute__((always_inline))     { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) <= 0; }// Cstring : Cstring
                inline bool operator <= (Cstring& other) __attribute__((always_inline))           { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) <= 0; }// Cstring : Cstring

                inline bool operator >  (const char *other) __attribute__((always_inline))        { return __strUtf8cmp__ (this->__c_str__, other) > 0; }           // Cstring : C string    
                inline bool operator >  (char *other) __attribute__((always_inline))              { return __strUtf8cmp__ (this->__c_str__, other) > 0; }           // Cstring : C string    
                inline bool operator >  (const Cstring& other) __attribute__((always_inline))     { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) > 0; } // Cstring : Cstring
                inline bool operator >  (Cstring& other) __attribute__((always_inline))           { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) > 0; } // Cstring : Cstring

                inline bool operator >= (const char *other) __attribute__((always_inline))        { return __strUtf8cmp__ (this->__c_str__, other) >= 0; }          // Cstring : C string    
                inline bool operator >= (char *other) __attribute__((always_inline))              { return __strUtf8cmp__ (this->__c_str__, other) >= 0; }          // Cstring : C string    
                inline bool operator >= (const Cstring& other) __attribute__((always_inline))     { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) >= 0; }// Cstring : Cstring
                inline bool operator >= (Cstring& other) __attribute__((always_inline))           { return __strUtf8cmp__ (this->__c_str__, other.__c_str__) >= 0; }// Cstring : Cstring
            #endif


            // [] operator
            inline char &operator [] (size_t i) __attribute__((always_inline)) { return __c_str__ [i]; }
            inline char &operator [] (int i) __attribute__((always_inline)) { return __c_str__ [i]; }
            // inline char &operator [] (unsigned int i) __attribute__((always_inline)) { return __c_str__ [i]; }
            inline char &operator [] (long i) __attribute__((always_inline)) { return __c_str__ [i]; }
            inline char &operator [] (unsigned long i) __attribute__((always_inline)) { return __c_str__ [i]; }


            // some std::string-like member functions
            inline const char *c_str () __attribute__((always_inline)) { return (const char *) __c_str__; } 
        
            inline size_t length () __attribute__((always_inline)) { return strlen (__c_str__); } 

            size_t characters () { // returns the number of characters in the string, considering UTF-8 encoding
                #ifdef __LOCALE_HPP__
                    if (__use_utf8__) {
                        // count UTF-8 characters, the first UTF-8 byte starts with 0xxxxxxx for ASCII, 110xxxxx for 2 byte character, 1110xxxx for 3 byte character and 11110xxx for 4 byte character, all the following bytes start with 10xxxxxx
                        size_t count = 0;
                        for (size_t i = 0; __c_str__ [i]; ) {
                            char c = __c_str__ [i];
                            if ((c & 0x80) == 0) // 1-byte character
                                i += 1;
                            else if ((c & 0xE0) == 0xC0) // 2-byte character
                                i += 2;
                            else if ((c & 0xF0) == 0xE0) // 3-byte character
                                i += 3;
                            else if ((c & 0xF8) == 0xF0) // 4-byte character
                                i += 4;
                            else // invalid UTF-8 character
                                i += 1;

                            count ++;
                        }
                        return count;
                    } 
                #endif

                // ASCII
                return length ();
            }
            
            inline size_t max_size () __attribute__((always_inline)) { return N; } 

            Cstring<N> substr (size_t pos = 0, size_t len = N + 1) {
                Cstring<N> r;
                r.__errorFlags__ = this->__errorFlags__;                      // inherit all errors from original string
                if (pos >= strlen (this->__c_str__)) {
                    r.__errorFlags__ |= err_out_of_range;
                } else {
                    strncpy (r.__c_str__, this->__c_str__ + pos, len);        // can't err_overflow 
                }
                return r;
            }

            static const size_t npos = (size_t) 0xFFFFFFFFFFFFFFFF;

            size_t find (const char *str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str);
                if (p)  return p - __c_str__;
                return npos;
            }

            size_t find (Cstring str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str.__c_str__);
                if (p)  return p - __c_str__;
                return npos;
            }

            size_t rfind (char *str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str);
                char *q = NULL;
                while (p) { q = p; p = strstr (p + 1, str); }
                if (q) return q - __c_str__;
                return npos;
            }            

            size_t rfind (Cstring str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str.__c_str__);
                char *q = NULL;
                while (p) { q = p; p = strstr (p + 1, str); }
                if (q) return q - __c_str__;
                return npos;
            }            

            void erase (size_t pos = 0) {
                if (pos > N) pos = N;
                __c_str__ [pos] = 0;
            }
        
            // some Arduino String-like member functions
            Cstring substring (size_t from = 0, size_t to = N - 1) {
                Cstring<N> r;
                r.__errorFlags__ = this->__errorFlags__;                      // inherit all errors from original string
                if (from >= strlen (this->__c_str__) || to < from) {
                    r.__errorFlags__ |= err_out_of_range;
                } else {
                    strncpy (r.__c_str__, this->__c_str__ + from, to - from); // can't err_overflow 
                }
                return r;
            }

            int indexOf (const char *str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str);
                if (p)  return p - __c_str__;
                return -1;
            }

            int indexOf (Cstring str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str.__c_str__);
                if (p)  return p - __c_str__;
                return -1;
            }

            int lastIndexOf (char *str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str);
                char *q = NULL;
                while (p) { q = p; p = strstr (p + 1, str); }
                if (q) return q - __c_str__;
                return -1;
            }

            int lastIndexOf (Cstring str, size_t pos = 0) {
                char *p = strstr (__c_str__ + pos, str.__c_str__);
                char *q = NULL;
                while (p) { q = p; p = strstr (p + 1, str); }
                if (q) return q - __c_str__;
                return -1;
            }

            bool endsWith (const char *str) { 
              size_t lStr = strlen (str);
              size_t lThis = strlen (__c_str__);
              if (lStr > lThis) return false;
              return !strcmp (str, __c_str__ + (lThis - lStr));
            }

            void remove (size_t pos = 0) {
                if (pos > N) pos = N;
                #ifdef __LOCALE_HPP__
                    if (__use_utf8__)
                        this->__rTrimUnfinishedUtf8Character__ ();
                    else
                #endif
                    __c_str__ [pos] = 0;
            }

            void trim () {
                lTrim ();
                rTrim ();
            }
        
        
            // add other functions that may be useful 
            void toupper () {
                #ifndef __LOCALE_HPP__ // use ASCII codes
                    char *pc = __c_str__;
                    while (*pc)
                        if (*pc >= 'a' && *pc <= 'z')
                            *pc = (*pc - ('a' - 'A'));
                #else // use locale, due to UTF-8 encodint the result string may (theoretically) be longer than the original one
                    Cstring<N> tmp = __c_str__;
                    const char *p = tmp.__c_str__;
                    __c_str__ [0] = 0; // empty internal buffer without clearing error flags
                    while (*p)
                        operator += (__locale_toupper__ (&p));
                #endif
            }

            void tolower () {
                #ifndef __LOCALE_HPP__ // use ASCII codes
                    char *pc = __c_str__;
                    while (*pc)
                        if (*pc >= 'A' && *pc <= 'Z')
                            *pc = (*pc + ('a' - 'A'));
                #else // use locale, due to UTF-8 encodint the result string may (theoretically) be longer than the original one
                    Cstring<N> tmp = __c_str__;
                    const char *p = tmp.__c_str__;
                    __c_str__ [0] = 0; // empty internal buffer without clearing error flags
                    while (*p)
                        operator += (__locale_tolower__ (&p));
                #endif
            }

            void lTrim () {
                size_t i = 0;
                while (__c_str__ [i ++] == ' ');
                if (i) strcpy (__c_str__, __c_str__ + i - 1);
            }    
            
            void rTrim () {
                size_t i = strlen (__c_str__);
                while (__c_str__ [-- i] == ' ');
                __c_str__ [i + 1] = 0;
            }

            void rPad (size_t toLength, char withChar) {
                if (toLength > N) {
                  toLength = N;
                  __errorFlags__ |= err_overflow;                                 // error? overflow?
                }
                size_t l = strlen (__c_str__);
                while (l < toLength) __c_str__ [l ++] = withChar;
                __c_str__ [l] = 0;
            }

        private:

            #ifdef __LOCALE_HPP__
                void __rTrimUnfinishedUtf8Character__ () {                            // trim (unfinished) UTF-8 byte from the right
                    // the first UTF-8 byte starts with 0xxxxxxx for ASCII, 110xxxxx for 2 byte character, 1110xxxx for 3 byte character and 11110xxx for 4 byte character, all the following bytes start with 10xxxxxx
                    int i;
                    for (i = N - 1; i >= 0; i --) {
                        if ((__c_str__ [i] & 0b11000000) == 0b10000000) {
                            __c_str__ [i] = 0; // clear UTF-8 byte 
                        } else {
                            break;
                        }
                    }
                    if (i >= 0 && (__c_str__ [i] & 0b11000000) == 0b11000000) {
                        __c_str__ [i] = 0; // clear the first UTF-8 byte as well
                    }
                }

                int __strUtf8cmp__ (const char *str1, const char*str2) {              // compare two strings in locale sort order
                    while (true) {
                        if (*str1) {
                            if (*str2) {
                                // both strings still have characters
                                int o1 = __locale_charOrder__ (&str1); // also increases the pointer
                                int o2 = __locale_charOrder__ (&str2); // also increases the pointer
                                if (o1 > o2) {
                                    return 1;
                                } else if (o1 < o2) {
                                    return -1;
                                }
                                // both characters are the same, continue with the next ones
                            } else {
                                // str1 has a character but str2 already ended
                                return 1;
                            }
                        } else {
                            if (*str2) {
                                // str2 has a character but str1 already ended
                                return -1;
                            } else {
                                // both strings ended at the same time, they are the same
                                return 0;
                            }
                        }
                    }
                }

            #endif

    };

#endif
