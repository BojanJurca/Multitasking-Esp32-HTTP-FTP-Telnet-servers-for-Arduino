/*
 *  Quick start for C++ package for Arduino
 *
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 * 
 *  May 22, 2025, Bojan Jurca
 */


/* place all the internal memory structures into PSRAM if the bord has one
    #define LIST_MEMORY_TYPE          PSRAM_MEM
    #define VECTOR_QUEUE_MEMORY_TYPE  PSRAM_MEM
    #define MAP_MEMORY_TYPE           PSRAM_MEM
    bool psramused = psramInit ();
*/


// #include "locale.hpp"       // include locale.hpp for support of local UTF-8 characters 
#include "console.hpp"      // cin and cout object to write to/read from Serial in standard C++ style
#include "Cstring.hpp"      // C arrays of characters with C++ syntax and error handling
#include "list.hpp"         // single linked lists with error handling that also works on AVR boards
#include "vector.hpp"       // vectors with error handling that also works on AVR boards
#include "queue.hpp"        // queues with error handling that also works on AVR boards
#include "Map.hpp"          // maps with error handling that also works on AVR boards
#include "algorithm.hpp"    // find, heap sort for vectors, ... merge sort for lists
#include "complex.hpp"      // complex numbers for Arduino
#include "fft.h"            // demonstration of the use of complex numbers 


void setup () {

    cinit (true);                                             // 3 optional arguments: bool waitForSerial = false, unsigned int serialSpeed = 115200 (or 9600, distinguishes 32 nd AVR boards), unsigned int waitAfterSerial = 1000


    cout << "\n----- testing console (cin and cout) -----\n";


        #ifdef __LOCALE_HPP__ // if locale. hpp is included
            setlocale (lc_all, "sl_SI.UTF-8");
        #endif


        cout << "Please enter a float: ";
        float f;
        cin >> f;
        cout << "you entered " << f << ", please note that you can use setlocale function to input/output floating point numbers in your local format" << endl;


    cout << "\n----- testing Cstring (C arrays of chars with C++ operators and error handling) -----\n";


        Cstring<25> s;                                                  // create a Cstring of max 25 characters on the stack
        cstring t;                                                      // create a Cstring with (default) max 300 characters
        s = "thirty tree";                                              // initialize the Cstring
        s = s + ", ...";                                                // calculate with Cstring
        cout << s << endl;
        for (int i = 0; i < 100; i ++)
            s += ", ...";
        cout << s << endl;    


        signed char e = s.errorFlags ();
        if (e) { // check details
            cout << "Error flags: " << e << endl;

            if (e & err_overflow)
                cout << "err_overflow" << endl;
            if (e & err_out_of_range)
                cout << "err_out_of_range" << endl;

            s.clearErrorFlags ();
        }

        // UTF-8 support
        cstring utf8string = "abc čšž ≈√∫ 🐱😂🚀";
        for (auto cp : utf8string) {
            cout << "\"" << *(utf8char *) cp << "\"";   // output utf8 character

            if (*(utf8char *) cp == (utf8char *) "😂")  // compare utf8 characters
                cout << " - face with tears of joy\n";
            else
                cout << endl;
        }


    #ifndef ARDUINO_ARCH_AVR // Assuming Arduino Mega or Uno

        cout << "\n----- testing locale -----\n";

        cout << "Current time is " << time (NULL) << ", please note that you can use setlocale function to input/output time in your local format" << endl;
    #endif


    cout << "\n----- quick start with (single linked) lists -----\n";


        list<int> li ( { -1, -2, -3 } );
        li.clear ();

        list<String> ls ( { String ("banana"), String ("apple"), String ("orange") } );

        ls.push_back ("grapefruit");                                    // add element "grapefruit"
        ls.push_back ("kiwi");
        ls.pop_front ();                                                // remove the first element
        ls.push_front ("grapes");    

        cout << "The first element = " << ls.front () << endl;          // the first element in the list
        cout << "The last element = " << ls.back () << endl;            // the last element in the list
        auto fl = find (ls.begin (), ls.end (), "grapes");              // find an element in the list
        if (fl != ls.end ())
            cout << "Found " << *fl << endl;    

        cout << "There are " << ls.size () << " elements in the list" << endl;
        for (auto e: ls)                                                // scan all list elements
            cout << e << endl;

        cout << ls << endl;                                             // display all list elements at once            

        e = ls.errorFlags ();                                           // check if list is in error state
        if (e) {                                          
            cout << "list error, flags: " << e << endl; 
                                                                        
            if (e & err_bad_alloc)    cout << "err_bad_alloc\n";        // you may want to check individual error flags (there are only 2 possible types of error flags for lists)
            if (e & err_out_of_range) cout << "err_out_of_range\n";

            ls.clearErrorFlags ();   
        }

        auto ml = max_element (ls.begin (), ls.end ());                 // finding min, max elements
        if (ml != ls.end ())                                            // if not empty
            cout << "max element in list = " << *ml << endl;

        cout << "Sorting, please note that you can use setlocale function to sort in your local format:\n";
        sort (ls.begin (), ls.end ());                                  // sorting
        for (auto e: ls)
            cout << "    " << e << endl;

        ls.clear ();


    cout << "\n----- quick start with vectors -----\n";


        vector<int> vi ( { -1, -2, -3 } );
        vector<String> vs ( { String ("banana"), String ("apple"), String ("orange") } ); // or (for non AVR boards): vs = { "bananna", "apple", "orange" };
        vs.clear ();

        vi.push_back (-20);                                              // add element -20
        vi.pop_front ();                                                 // remove the first element
        vi.pop_back ();                                                  // remove the last element
        vi.push_back (-30);

        cout << "The element at position 2 = " << vi [2] << endl;        // access an element by its position in the vector
        cout << "The first element = " << vi.front () << endl;           // the first element in the vector
        cout << "The last element = " << vi.back () << endl;             // the last element in the vector
        auto fv = find (vi.begin (), vi.end (), 20);                     // find an element in the vector
        if (fv != vi.end ()) {
            cout << "Found " << *fv << " ... ";
            vi.erase (fv);                                               // delete it
            cout << "and deleted" << endl;
        }

        cout << "There are " << vi.size () << " elements in the vector" << endl;
        for (auto e: vi)                                                 // scan all vector elements
            cout << e << endl;

        cout << vi << endl;                                              // display all vector elements at once

        /* signed char */ e = vi.errorFlags ();                          // check if vector is in error state
        if (e) {                                          
            cout << "vector error, flags: " << e << endl; 
                                                                        
            if (e & err_bad_alloc)    cout << "err_bad_alloc\n";         // you may want to check individual error flags (there are only 2 possible types of error flags for vectors)
            if (e & err_out_of_range) cout << "err_out_of_range\n";

            vi.clearErrorFlags ();
        }

        auto mv = min_element (vi.begin (), vi.end ());                  // finding min, max elements
        if (mv != vi.end ())                                             // if not empty
            cout << "min element in vector = " << *mv << endl;

        cout << "Sorting, please note that you can use setlocale function to sort in your local format:\n";
        sort (vi.begin (), vi.end ());                                   // sorting
        for (auto e: vi)
            cout << "    " << e << endl;

        vi.clear ();


    cout << "\n----- quick start with queues -----\n";


        queue<int> q ( {222, 444, 333} );
        q.push (555);                                                   // add some elements
        q.push (888);
        q.push (999);
        q.pop ();                                                       // remove the first element

        cout << "The element at position 1 = " << q [1] << endl;        // access an element by its position in the queue
        cout << "The first element = " << q.front () << endl;           // the first element in the queue
        cout << "The last element = " << q.back () << endl;             // the last element in the queue    

        cout << "There are " << q.size () << " elements in the queue" << endl;
        for (auto e: q)                                                 // scan all queue elements
            cout << e << endl;

        cout << q << endl;                                              // display all queue elements at once        

        /* signed char */ e = q.errorFlags ();                          // check if queue is in error state
        if (e) {                                          
            cout << "queue error, flags: " << e << endl; 
                                                                        
            if (e & err_bad_alloc)    cout << "err_bad_alloc\n";        // you may want to check individual error flags (there are only 2 possible types of error flags for queues)
            if (e & err_out_of_range) cout << "err_out_of_range\n";

            q.clearErrorFlags ();
        }

        auto mq = min_element (q.begin (), q.end ());                   // finding min, max elements
        if (mq != q.end ())                                             // if not empty
            cout << "min element in queue = " << *mq << endl;

        q.clear ();


    cout << "\n----- quick start with maps -----\n";


        Map<int, String> mp ( { {11, "eleven"}, {12, "twelve"} });  // or (for non AVR boards): mp = { {11, "eleven"}, {12, "twelve"} }

        mp [1] = "one";                                                 // assign value of "one" to key 1
        mp [2] = "two";
        mp [3] = "tree";
        cout << "The value of key 1 is " << mp [1] << endl;             // access the value if the key is known

        mp.insert (4, "four");                                          // another way of inserting a pair
        mp.insert ( {5, "five"} );                                      // another way of inserting a pair

        auto findIterator = mp.find (3);
        if (findIterator != mp.end ())
            cout << "Found " << findIterator->first << " with value " << findIterator->second << endl;

        mp.erase (3);                                                   // delete it
        if (mp.errorFlags () & err_not_found)
            cout << "the pair with key " << 3 << " not found" << endl;
        else
            cout << "deleted the pair with key " << 3 << endl;

        cout << "There are " << mp.size () << " pairs in the map" << endl;
        for (auto pair: mp)                                             // scan all key-value pairs in the map
            cout << pair.first << "-" << pair.second << endl;

        cout << mp << endl;                                             // display all map elements at once        

        /* signed char */ e = mp.errorFlags ();                         // check if map is in error state
        if (e) {                                          
            cout << "map error, flags: " << e << endl; 
                                                    
            if (e & err_bad_alloc)    cout << "err_bad_alloc\n";        // you may want to check individual error flags (there are only 3 possible types of error flags for maps)
            if (e & err_out_of_range) cout << "err_out_of_range\n";
            if (e & err_not_unique) cout << "err_not_unique\n";

            mp.clearErrorFlags ();
        }

        auto minIterator = mp.begin ();                                 // finding min, max keys
        auto endIterator = mp.end ();
        if (minIterator != endIterator) {
            cout << "min key = " << minIterator->first << endl;
            -- endIterator;
            cout << "max key = " << endIterator->first << endl;
        }

        mp.clear ();


    cout << "\n----- quick start with algorithms on arrays -----\n";


        int a [10] = { 3, 6, 4, 3, 3, 5, 8, 0, 9, 5 };
        int size = sizeof (a) / sizeof (a [0]);

        int *fa = find (a, a + size, 8);                                // find an element in the array
        if (fa != a + size)
            cout << "8 found" << endl;

        cout << "Sorting, please note that you can use setlocale function to sort in your local format:\n";
        sort (a, a + size);                                             // sort the array
        for (int i = 0; i < size; i ++)
            cout << "    " << a [i] << endl;


    cout << "\n----- quick start with complex numbers -----\n";

        complex<float> z1 = { 1, 2 };
        complex<float> z2 = { 3, 4 };
        cout << z1 + z2 << endl;

        // FFT example
        cout << "\nFFT - frequency example\n";
        complex<float> signal [16];
        for (int i = 0; i < 16; i++)
            signal [i] = { sin (i) + cos (2 * i), 0 };
        complex<float> frequency [16];
        fft (frequency, signal);
        for (int i = 0; i < 16 / 2; i++)
            cout << "magnitude [" << i << "] = " << abs (frequency [i]) << endl;

}


void loop () {

}

