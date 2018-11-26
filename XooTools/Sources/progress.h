/*
XooTools, a set of C++ classes for analyzing Xoodoo.

Xoodoo, designed by Joan Daemen, Seth Hoffert, Gilles Van Assche and Ronny Van Keer.
For specifications, please refer to https://eprint.iacr.org/2018/767
For contact information, please visit https://keccak.team/team.html

Implementation by Gilles Van Assche, hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _PROGRESS_H_
#define _PROGRESS_H_

#include <cstdint>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
#include "Tree.h"

using namespace std;

class ProgressMeter {
public:
    vector<string> synopsis;
    vector<uint64_t> index, count;
    unsigned int height;
    uint64_t topIndex;
    time_t previousDisplay;
    unsigned int lastHeightDisplayed;
    unsigned int nrDisplaysSinceFullDisplay;
public:
    ProgressMeter();
    void stack(uint64_t aCount = 0);
    void stack(const string& aSynopsis, uint64_t aCount = 0);
    void unstack();
    void operator++();
    void clear();
protected:
    void display();
    void displayIfNecessary();
};

class GenericProgressDisplay {
public:
        time_t previousDisplay;
public:
    GenericProgressDisplay() : previousDisplay(time(NULL)) {}
    template<class Unit, class CachedRepresentation>
    void subtreeConsidered(const UnitList<Unit>& parentUnitList, const Unit& newUnit, const CachedRepresentation& cache, const GenericTreeIteratorStatistics& stats)
    {
        (void)cache;
        if (difftime(time(NULL), previousDisplay) >= 10.0) {
            cout << "Current subtree: ";
            if (parentUnitList.size() == 0)
                cout << "root" << endl;
            else
                cout << parentUnitList;
            cout << "Child node considered: " << newUnit << endl;
            cout << stats;
            previousDisplay = time(NULL);
        }
    }
};

#endif
