/*
 *  locale.hpp for Arduino
 *
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 *
 *  This file is more or less just an example. Implement your own locale settings here.
 *
 *  October 31, 2024, Bojan Jurca
 *
 */


#ifndef __LOCALE_HPP__
    #define __LOCALE_HPP__

    // ----- TO DO: implement your own locale settings (an example is below) -----

    enum localeCategory_t {
        lc_collate  = 0b00000001, // LC_COLLATE is used for string comparison and sorting. It defines how strings are compared and sorted according to the rules of the specified locale.
        lc_ctype    = 0b00000010, // LC_CTYPE is crucial for character classification and conversion functions. It governs what’s considered a letter, digit, punctuation mark, etc., and how characters convert between uppercase and lowercase.
        lc_monetary = 0b00000100, // LC_MONETARY is used to format monetary values according to the rules of a specified locale.
        lc_numeric  = 0b00001000, // LC_NUMERIC controls the formatting of numbers, specifically the decimal point and thousands separator, according to the rules of a specified locale.
        lc_time     = 0b00010000, // LC_TIME controls the formatting of dates and times according to the locale's rules.
        lc_messages = 0b00100000, // LC_MESSAGES handles the localization of system messages and prompts. It ensures that messages, warnings, and errors are displayed in the appropriate language and format for the user's locale.
        lc_all      = 0b00111111
    };

    bool setlocale (localeCategory_t category, const char *locale);


    // ----- this is the default - ASCII locale -----

    struct utf8char { 
        char __c_str__ [5]; // UTF-8 encoding requires max 4 bytes to represent a character
        inline const char *c_str () __attribute__((always_inline)) { return (const char *) __c_str__; } 
        inline operator char *() __attribute__((always_inline)) { return __c_str__; }
    };

    // assing each charcter its own number that will be used for sorting - in case of ASCII character just return its ASCII code and increase the pointer
    int __charOrder_ASCII__ (const char **pc) {
        int i = **pc;
        (*pc) ++;
        return i;
    }

    // returns upper case ASCII code as UTF-8 character and increase the pointer
    utf8char __toupper_ASCII__ (const char **pc) {
        char c = **pc;
        (*pc) ++;
        if (c >= 'a' && c <= 'z')
            return {(char) (c - ('a' - 'A')), 0};
        else
            return {c, 0};
    }

    // returns lower case ASCII code as UTF-8 character and increase the pointer
    utf8char __tolower_ASCII__ (const char **pc) {
        char c = **pc;
        (*pc) ++;
        if (c >= 'A' && c <= 'Z')
            return {(char) (c + ('a' - 'A')), 0};
        else
            return {c, 0};
    }


    int (*__locale_charOrder__) (const char **) = __charOrder_ASCII__; // by default

    utf8char (*__locale_toupper__) (const char **) = __toupper_ASCII__; // by default
    utf8char (*__locale_tolower__) (const char **) = __tolower_ASCII__; // by default

    char __locale_decimalSeparator__ = '.';
    char __locale_thousandSeparator__ = ',';

    const char *__locale_time__ = "%Y/%m/%d %T";

    bool __use_utf8__ = false;


    // ----- example for locale "sl_SI.UTF-8", you may freely delete this part if not needed -----
    /*

                //  Slovenian alphabet, extended with some frequently used foreign letters:
                //  A B C Č (Ć) D (Đ) E F G H I J K L M N O P (Q) R S Š T U V (W) (X) (Y) Z Ž
                //        |  |     |                                  |                     |
                //     2 byte UTF-8 characters               2 byte UTF-8 character    2 byte UTF-8 character


                // assing each UTF-8 charcter in "sl_SI.UTF-8" locale its own number that will be used for sorting
                int __charOrder_sl_SI_UTF_8__ (const char **pc) {
                    // count UTF-8 characters, the first UTF-8 byte starts with 0xxxxxxx for ASCII, 110xxxxx for 2 byte character, 1110xxxx for 3 byte character and 11110xxx for 4 byte character, all the following bytes start with 10xxxxxx

                    // is it a 2 byteUTF-8 characters in "sl_SI.UTF-8" locale?
                    char c1 = **pc;
                    if ((c1 & 0xE0) == 0xC0) { // 2-byte character                        
                        char c2 = *(*pc + 1); 

                        if (c1 == (char) 196 && c2 == (char) 140) {
                            (*pc) += 2;
                            return 67 * 3 + 1; // C + 1 = Č
                        } else if (c1 == (char) 196 && c2 == (char) 134) {
                            (*pc) += 2;
                            return 67 * 3 + 2; // C + 2 = Ć
                        } else if (c1 == (char) 196 && c2 == (char) 144) {
                            (*pc) += 2;
                            return 68 * 3 + 1; // D + 1 = Đ
                        } else if (c1 == (char) 197 && c2 == (char) 160) {
                            (*pc) += 2;
                            return 83 * 3 + 1; // S + 1 = Š
                        } else if (c1 == (char) 197 && c2 == (char) 189) {
                            (*pc) += 2;
                            return 90 * 3 + 1; // Z + 1 = Ž
                        } else if (c1 == (char) 196 && c2 == (char) 141) {
                            (*pc) += 2;
                            return 99 * 3 + 1; // c + 1 = č
                        } else if (c1 == (char) 196 && c2 == (char) 135) {
                            (*pc) += 2;
                            return 99 * 3 + 2; // c + 2 = ć
                        } else if (c1 == (char) 196 && c2 == (char) 145) {
                            (*pc) += 2;
                            return 100 * 3 + 1; // d + 1 = đ
                        } else if (c1 == (char) 197 && c2 == (char) 161) {
                            (*pc) += 2;
                            return 115 * 3 + 1; // s + 1 = š
                        } else if (c1 == (char) 197 && c2 == (char) 190) {
                            (*pc) += 2;
                            return 122 * 3 + 1; // z + 1 = ž
                        }
                    }

                    // if none of 2 byte UTF-8 characters in "sl_SI.UTF-8" locale, treat each character as a single byte character
                    int i = 3 * c1; // returning triple ASCII value will ensure 2 free number between ASCII characters, this is just what we need
                    (*pc) ++;
                    return i;
                }

                utf8char __toupper_sl_SI_UTF_8__ (const char **pc) {
                    // count UTF-8 characters, the first UTF-8 byte starts with 0xxxxxxx for ASCII, 110xxxxx for 2 byte character, 1110xxxx for 3 byte character and 11110xxx for 4 byte character, all the following bytes start with 10xxxxxx

                    // is it a 2 byteUTF-8 characters in "sl_SI.UTF-8" locale?
                    char c1 = **pc;
                    if ((c1 & 0xE0) == 0xC0) { // 2-byte character
                        
                        char c2 = *(*pc + 1); 
                        if (c1 == (char) 196 && c2 == (char) 140) {
                            (*pc) += 2;
                            return {(char) 196, (char) 140, (char) 0}; // Č is alread in upper case
                        } else if (c1 == (char) 196 && c2 == (char) 134) {
                            (*pc) += 2;
                            return {(char) 196, (char) 134, (char) 0}; // Ć is alread in upper case
                        } else if (c1 == (char) 196 && c2 == (char) 144) {
                            (*pc) += 2;
                            return {(char) 196, (char) 144, (char) 0}; // Đ is alread in upper case
                        } else if (c1 == (char) 197 && c2 == (char) 160) {
                            (*pc) += 2;
                            return {(char) 197, (char) 160, (char) 0}; // Š is alread in upper case
                        } else if (c1 == (char) 197 && c2 == (char) 189) {
                            (*pc) += 2;
                            return {(char) 197, (char) 189, (char) 0}; // Ž is alread in upper case
                        } else if (c1 == (char) 196 && c2 == (char) 141) {
                            (*pc) += 2;
                            return {(char) 196, (char) 140, (char) 0}; // č -> Č
                        } else if (c1 == (char) 196 && c2 == (char) 135) {
                            (*pc) += 2;
                            return {(char) 196, (char) 134, (char) 0}; // ć -> Ć
                        } else if (c1 == (char) 196 && c2 == (char) 145) {
                            (*pc) += 2;
                            return {(char) 196, (char) 144, (char) 0}; // đ -> Đ
                        } else if (c1 == (char) 197 && c2 == (char) 161) {
                            (*pc) += 2;
                            return {(char) 197, (char) 160, (char) 0}; // š -> Š
                        } else if (c1 == (char) 197 && c2 == (char) 190) {
                            (*pc) += 2;
                            return {(char) 197, (char) 189, (char) 0}; // ž -> Ž
                        }
                    }
    
                    return __toupper_ASCII__ (pc);
                }

                utf8char __tolower_sl_SI_UTF_8__ (const char **pc) {
                    // count UTF-8 characters, the first UTF-8 byte starts with 0xxxxxxx for ASCII, 110xxxxx for 2 byte character, 1110xxxx for 3 byte character and 11110xxx for 4 byte character, all the following bytes start with 10xxxxxx

                    // is it a 2 byteUTF-8 characters in "sl_SI.UTF-8" locale?
                    char c1 = **pc;
                    if ((c1 & 0xE0) == 0xC0) { // 2-byte character
                        
                        char c2 = *(*pc + 1); 
                        if (c1 == (char) 196 && c2 == (char) 140) {
                            (*pc) += 2;
                            return {(char) 196, (char) 141, (char) 0}; // Č -> č
                        } else if (c1 == (char) 196 && c2 == (char) 134) {
                            (*pc) += 2;
                            return {(char) 196, (char) 135, (char) 0}; // Ć -> ć
                        } else if (c1 == (char) 196 && c2 == (char) 144) {
                            (*pc) += 2;
                            return {(char) 196, (char) 145, (char) 0}; // Đ -> đ
                        } else if (c1 == (char) 197 && c2 == (char) 160) {
                            (*pc) += 2;
                            return {(char) 197, (char) 161, (char) 0}; // Š -> š
                        } else if (c1 == (char) 197 && c2 == (char) 189) {
                            (*pc) += 2;
                            return {(char) 197, (char) 190, (char) 0}; // Ž -> ž
                        } else if (c1 == (char) 196 && c2 == (char) 141) {
                            (*pc) += 2;
                            return {(char) 196, (char) 141, (char) 0}; // č is already in lower case
                        } else if (c1 == (char) 196 && c2 == (char) 135) {
                            (*pc) += 2;
                            return {(char) 196, (char) 135, (char) 0}; // ć is already in lower case
                        } else if (c1 == (char) 196 && c2 == (char) 145) {
                            (*pc) += 2;
                            return {(char) 196, (char) 145, (char) 0}; // đ is already in lower case
                        } else if (c1 == (char) 197 && c2 == (char) 161) {
                            (*pc) += 2;
                            return {(char) 197, (char) 161, (char) 0}; // š is alread in lowercase
                        } else if (c1 == (char) 197 && c2 == (char) 190) {
                            (*pc) += 2;
                            return {(char) 197, (char) 190, (char) 0}; // ž -> Ž
                        }
                    }
    
                    return __tolower_ASCII__ (pc);
                }
    */





    bool setlocale (localeCategory_t category, const char *locale) {

        if (locale == NULL) {
            __use_utf8__ = false;

            if (category & lc_collate) {
                __locale_charOrder__ = __charOrder_ASCII__;
            }

            if (category & lc_ctype) {
                __locale_toupper__ = __toupper_ASCII__;
                __locale_tolower__ = __tolower_ASCII__;
            }

            if (category & lc_numeric) {
                __locale_decimalSeparator__ = '.';
                __locale_thousandSeparator__ = ',';
            }

            if (category & lc_time) {
                __locale_time__ = "%Y/%m/%d %T";
            }

            return true;
        }

        /* ----- example for locale "sl_SI.UTF-8" -----
        if (!strcmp (locale, "sl_SI.UTF-8")) {
            __use_utf8__ = true;

            if (category & lc_collate) {
                __locale_charOrder__ = __charOrder_sl_SI_UTF_8__;
            }

            if (category & lc_ctype) {
                __locale_toupper__ = __toupper_sl_SI_UTF_8__;
                __locale_tolower__ = __tolower_sl_SI_UTF_8__;
            }

            if (category & lc_numeric) {
                __locale_decimalSeparator__ = ',';
                __locale_thousandSeparator__ = '.';
            }

            if (category & lc_time) {
                __locale_time__ = "%d.%m.%Y %H:%M:%S";
            }

            return true;
        }
        */

        return false; // locale not set
    }

#endif
