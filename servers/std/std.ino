/*
 *  Quick start for C++ package for Arduino (ESP boards)
 *
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 * 
 *  November 26, 2024, Bojan Jurca
 */


#include "locale.hpp"
#include "Cstring.hpp"
#include "list.hpp"
#include "vector.hpp"
#include "queue.hpp"
#include "Map.hpp"
#include "algorithm.hpp"
#include "console.hpp"

#include <list> // std::list
#include <vector> // std::vector
#include <map> // std::map
#include <algorithm> // std::find, std::sort


/* place all the internal memory structures into PSRAM if the bord has one
    #define LIST_MEMORY_TYPE          PSRAM_MEM
    #define VECTOR_QUEUE_MEMORY_TYPE  PSRAM_MEM
    #define MAP_MEMORY_TYPE           PSRAM_MEM
    bool psramused = psramInit ();
*/


void setup () {

    cinit (true);
    
    unsigned long start;
    unsigned long end;
    unsigned long startFreeHeap;
    unsigned long endFreeHeap;
    int N = 100;
    int i;
    int lasti;
    String lasts;

    // instances to be tested
    list<int> li;
    std::list<int> stdli;
    list<String> ls;

    vector<int> vi;
    std::vector<int> stdvi;
    vector<String> vs;

    Map<int, char> mi; // maps int keys  to char valzes, which is unrealistic, but we would like to eliminate value effect while focusing on keys
    std::map<int, char> stdmi;

    int *arr;


    cout << "Plese enter N ... ";
    cin >> N;
    cout << N << endl;


    cout << "\n----- testing list<int> of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 1; i <= N; i++)
            li.push_back (lasti = random (1000));
        endFreeHeap = ESP.getFreeHeap ();
        cout << "list<int>: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (li) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (li) - N * sizeof (int) << " overhed)\n";

        // find time
        start =  micros ();
        for (int i = 0; i < 1000; i++)
          auto fli = find (li.begin (), li.end (), i);
        end = micros ();
        cout << "find: " << (end - start) / 1000 << " µs" << endl;

        // push_back time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            li.push_back (random (1000));
        end = micros ();
        for (int i = 0; i < 10; i++)
            li.pop_front ();
        cout << "push_back: " << (end - start) / 10 << " µs\n";

        // sort
        start =  micros ();
        sort (li.begin (), li.end ());
        end = micros ();
        cout << "sort: " << end - start << " µs merge-sorting: " << li.size () << " elements\n";

        // any error so far?
        cout << "Error flags: " << li.errorFlags () << endl;

        // free the memory used
        li.clear ();



    cout << "\n----- testing std::list<int> of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 1; i <= N; i++)
            stdli.push_back (lasti = random (1000));
        endFreeHeap = ESP.getFreeHeap ();
        cout << "std::list<int>: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (stdli) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (stdli) - N * sizeof (int) << " overhed)\n";

        // find time
        start =  micros ();
        for (int i = 0; i < 1000; i++)
            auto fstdli = std::find (stdli.begin (), stdli.end (), i);
        end = micros ();
        cout << "std::find: " << (end - start) / 1000 << " µs" << endl;

        // push_back time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            stdli.push_back (random (1000));
        end = micros ();
        for (int i = 0; i < 10; i++)
            stdli.pop_front ();
        cout << "std::push_back: " << (end - start) / 10 << " µs\n";

        // sorting is not supported
        cout << "size: " << stdli.size () << " elements\n";

        // any error so far?
        cout << "If the controller didn't restart it is OK" << endl;

        // free the memory used
        stdli.clear ();



    cout << "\n----- testing list<String> of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 0; i < N; i++)
            ls.push_back (lasts = String (random (1000)) + "                      ");
        endFreeHeap = ESP.getFreeHeap ();
        cout << "used memory (25 characters in each String): " << (startFreeHeap - endFreeHeap) + sizeof (ls) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (ls) - N * sizeof (String) << " overhed)\n";

        // find time
        start =  micros ();
        auto fls = find (ls.begin (), ls.end (), lasts);
        end = micros ();
        cout << "find: " << end - start << " µs" << endl;

        // push_back time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            ls.push_back (String (random (1000)));
        end = micros ();
        for (int i = 0; i < 10; i++)
            ls.pop_front ();
        cout << "push_back: " << (end - start) / 10 << " µs\n";

        //  sort time
        start =  micros ();
        sort (ls.begin (), ls.end ());
        end = micros ();
        cout << "sort: " << end - start << " µs heap-sorting: " << ls.size () << " elements\n";

        // any error so far?
        cout << "Error flags: " << ls.errorFlags () << endl;

        // free the memory used
        ls.clear ();



    cout << "\n----- testing vector<int> of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 1; i <= N; i++)
            vi.push_back (lasti = random (1000));
        endFreeHeap = ESP.getFreeHeap ();
        cout << "vector<int>: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (vi) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (vi) - N * sizeof (int) << " overhed)\n";

        // find time
        start =  micros ();
        for (int i = 0; i < 1000; i++)
            auto fvi = find (vi.begin (), vi.end (), i);
        end = micros ();
        cout << "find: " << (end - start) / 1000 << " µs" << endl;

        // push_back time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            vi.push_back (random (1000));
        end = micros ();
        for (int i = 0; i < 10; i++)
            vi.pop_front ();
        cout << "push_back (without prior reservation of memory): " << (end - start) / 10 << " µs\n";

        // sort time
        start =  micros ();
        sort (vi.begin (), vi.end ());
        end = micros ();
        cout << "sort: " << end - start << " µs heap-sorting: " << vi.size () << " elements\n";

        // any error so far?
        cout << "Error flags: " << vi.errorFlags () << endl;

        // free the memory used
        vi.clear ();



    cout << "\n----- testing std::vector<int> of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 1; i <= N; i++)
            stdvi.push_back (lasti = random (1000));
        endFreeHeap = ESP.getFreeHeap ();
        cout << "std::vector<int>: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (stdvi) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (stdvi) - N * sizeof (int) << " overhed)\n";

        // find time
        start =  micros ();
        for (int i = 0; i < 1000; i++)
            auto fstdvi = std::find (stdvi.begin (), stdvi.end (), i);
        end = micros ();
        cout << "std::find: " << (end - start) / 1000 << " µs" << endl;

        // push_back time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            stdvi.push_back (random (1000));
        end = micros ();
        for (int i = 0; i < 10; i++)
            stdvi.pop_back ();
        cout << "std::push_back (without prior reservation of memory): " << (end - start) / 10 << " µs\n";

        // sort time
        start =  micros ();
        sort (stdvi.begin (), stdvi.end ());
        end = micros ();
        cout << "sort: " << end - start << " µs\n";

        // std::sort time
        start =  micros ();
        std::sort (stdvi.begin (), stdvi.end ());
        end = micros ();
        cout << "std::sort: " << end - start << " µs intro-sorting: " << stdvi.size () << " elements\n";

        // any error so far?
        cout << "If the controller didn't restart it is OK" << endl;

        // free the memory used
        stdvi.clear ();



    cout << "\n----- testing vector<String> of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 0; i < N; i++)
            vs.push_back (lasts = String (random (1000)) + "                      ");
        endFreeHeap = ESP.getFreeHeap ();
        cout << "used memory (25 characters in each String): " << (startFreeHeap - endFreeHeap) + sizeof (vs) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (vs) - N * (sizeof (String) + 25) << " overhed)\n";

        // find time
        start =  micros ();
        auto fvs = find (vs.begin (), vs.end (), lasts);
        end = micros ();
        cout << "find: " << end - start << " µs" << endl;

        // push_back time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            vs.push_back (String (random (1000)));
        end = micros ();
        for (int i = 0; i < 10; i++)
            vs.pop_front ();
        cout << "push_back: " << (end - start) / 10 << " µs\n";

        //  sort time
        start =  micros ();
        sort (vs.begin (), vs.end ());
        end = micros ();
        cout << "sort: " << end - start << " µs heap-sorting: " << vs.size () << " elements\n";

        // any error so far?
        cout << "Error flags: " << vs.errorFlags () << endl;

        // free the memory used
        vs.clear ();

    cout << "\n----- testing int array of " << N << " elements -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        arr = (int *) malloc (N * sizeof (int));
        if (N > 0 && arr) {

            for (int i = 0; i < N; i++)
                lasti = arr [i] = random (1000);
            endFreeHeap = ESP.getFreeHeap ();
            cout << "int array: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (*arr) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (*arr) - N * sizeof (int) << " overhed)\n";

            // find time
            start =  micros ();
            for (int i = 0; i < 1000; i++)
                auto fai = find (arr, arr + N, i);
            end = micros ();
            cout << "find: " << (end - start) / 1000 << " µs" << endl;

            // push_back time
            start =  micros ();
            arr [0] = 0;
            end = micros ();
            cout << "assign: " << (end - start) << " µs\n";

            // sort time
            start =  micros ();
            sort (arr, arr + N);
            end = micros ();
            cout << "sort: " << end - start << " µs heap-sorting: " << N << " elements\n";

            // std::sort time
            start =  micros ();
            std::sort (arr, arr + N);
            end = micros ();
            cout << "std::sort: " << end - start << " µs intro-sorting: " << N << " elements\n";

            // any error so far?
            cout << "Nothing unpredictible could have happened, so no error at all" << endl;

            // free the memory used
            free (arr);
        }



    cout << "\n----- testing map<int, char> of " << N << " pairs -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 1; i <= N; i++)
            mi [lasti = random (1000)] = 'c';
        endFreeHeap = ESP.getFreeHeap ();
        cout << "map<int, char>: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (mi) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (mi) - N * (sizeof (int) + sizeof (char)) << " overhed)\n";

        // find time
        start =  micros ();
        for (int i = 0; i < 1000; i++)
            auto fmi = mi.find (i);
        end = micros ();
        cout << "find: " << (end - start) / 1000 << " µs" << endl;

        // insert time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            mi [1000 + i] = 'c';
        end = micros ();
        cout << "insert: " << (end - start) / 10 << " µs\n";

        // any error so far?
        cout << "Error flags: " << mi.errorFlags () << endl;

        // free the memory used
        mi.clear ();



    cout << "\n----- testing std::map<int, char> of " << N << " pairs -----\n";

        // memory used
        startFreeHeap = ESP.getFreeHeap ();
        for (int i = 1; i <= N; i++)
            stdmi [lasti = random (1000)] = 'c';
        endFreeHeap = ESP.getFreeHeap ();
        cout << "std::map<int, char>: used memory: " << (startFreeHeap - endFreeHeap) + sizeof (stdmi) << " bytes (" << (startFreeHeap - endFreeHeap) + sizeof (stdmi) - N * (sizeof (int) + sizeof (char)) << " overhed)\n";

        // find time
        start =  micros ();
        for (int i = 0; i < 1000; i++)
            auto fstdmi = stdmi.find (i);
        end = micros ();
        cout << "find: " << (end - start) / 1000 << " µs" << endl;

        // insert time
        start =  micros ();
        for (int i = 0; i < 10; i++)
            stdmi [1000 + i] = 'c';
        end = micros ();
        cout << "insert: " << (end - start) / 10 << " µs\n";

        // any error so far?
        cout << "If the controller didn't restart it is OK" << endl;

        // free the memory used
        stdmi.clear ();


    cout << "\n\nFinishing setup ()\n";
}

void loop () {
    delay (10000);
    ESP.restart ();
}

