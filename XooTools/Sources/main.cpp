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

#include <cstdlib>
#include <fstream>
#include "Xoodoo3RoundTrailCoreGeneration.h"
#include "XoodooPropagation.h"
#include "XoodooTrailExtension.h"
#include "XoodooTrails.h"

void produceHumanReadableFile(const string& fileName)
{
    try {
        XoodooDCLC xoodoo;
        XoodooPropagation DCorLC(xoodoo, ((fileName.size() >= 1) && (fileName[0] == 'L')) ? XoodooPropagation::LC : XoodooPropagation::DC);
        DCorLC.produceHumanReadableFile(fileName);
    }
    catch(Exception e) {
        cerr << e.reason << endl;
    }
}

void extendTrails(const string& inFileName, XoodooPropagation::DCorLC propagationType, bool backwardExtension, int nrRounds, int maxWeight)
{
    try {
        const bool verbose =true;
        unsigned int minWeight = 9999;
        XoodooDCLC xoodoo;
        cout << "*** " << xoodoo << endl;
        XoodooPropagation DCorLC(xoodoo, propagationType);
        ifstream fin(inFileName);
        string outFileName = inFileName + (backwardExtension ? "-revext" : "-ext");
        ofstream fout(outFileName);
        while(!(fin.eof())) {
            try {
                Trail trail(DCorLC, fin);
                DCorLC.checkTrail(trail);
                extendTrail(fout, trail, backwardExtension, nrRounds, maxWeight, minWeight, verbose);
            }
            catch(TrailException) {
                cout << "!" << flush;
            }
        }
        cout << endl;
        DCorLC.produceHumanReadableFile(outFileName);
    }
    catch(Exception e) {
        cerr << e.reason << endl;
    }
}

void generateAll3RoundTrailCores()
{
    const int T3 = 44; // or 50, but it takes several days
    generate3RoundTrailCores(XoodooPropagation::DC, false, T3);
    generate3RoundTrailCores(XoodooPropagation::DC, true,  T3);
    generate3RoundTrailCores(XoodooPropagation::LC, false, T3);
    generate3RoundTrailCores(XoodooPropagation::LC, true,  T3);

    // sort -u DC*CDir DC*CRev > DC-Xoodoo-3rounds
    // sort -u LC*CDir LC*CRev > LC-Xoodoo-3rounds
    // produceHumanReadableFile("DC-Xoodoo-3rounds");
    // produceHumanReadableFile("LC-Xoodoo-3rounds");
}

void extendTo6RoundTrailCores()
{
    extendTrails("DC-Xoodoo-3rounds", XoodooPropagation::DC, false, 6, 102);
    extendTrails("DC-Xoodoo-3rounds", XoodooPropagation::DC, true,  6, 102);
    extendTrails("LC-Xoodoo-3rounds", XoodooPropagation::LC, false, 6, 102);
    extendTrails("LC-Xoodoo-3rounds", XoodooPropagation::LC, true,  6, 102);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    // generateAll3RoundTrailCores();
    // extendTo6RoundTrailCores();
    return EXIT_SUCCESS;
}
