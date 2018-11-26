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

#include <cstdint>
#include <fstream>
#include "progress.h"
#include "Tree.h"
#include "Xoodoo.h"
#include "XoodooDCLC.h"
#include "XoodooPropagation.h"
#include "XoodooTrailExtension.h"
#include "XoodooTrails.h"
#include "Xoodoo2RoundTrailCoreGeneration.h"

using namespace std;

void generate3RoundTrailCores(XoodooPropagation::DCorLC propagationType, bool backwardExtension, int T3)
{
    try {
        const int delta = 2;
        const int weightedWeight2R = backwardExtension ? T3-2-delta : T3+delta;
        uint64_t trailCount = 0;
        unsigned int minWeight = 9999;
        ColoredBitSymmetryClass xoodoo;
        cout << "*** " << xoodoo << endl;
        XoodooPropagation DCorLC(xoodoo, propagationType);
        string fileName = DCorLC.buildFileName(backwardExtension ? "CRev" : "CDir");
        ColoredBitSet bitSet(xoodoo);
        CoreGenerationCache cache(DCorLC);
        CoreGenerationCostFunction cost(backwardExtension ? 1 : 2, backwardExtension ? 2 : 1);
        GenericTreeIterator<ColoredBit, ColoredBitSet, XoodooPropagation, CoreGenerationCache, TwoRoundTrailCoreFromColoredBits, CoreGenerationCostFunction, GenericProgressDisplay>
            tree(bitSet, DCorLC, cost, weightedWeight2R);
        {
            ofstream fout(fileName);

            while(!tree.isEnd()) {
                const TwoRoundTrailCoreFromColoredBits& core = (*tree);
                extendTrailAll(fout, core, backwardExtension, T3, minWeight);
                ++tree;
            }
        }
        cout << tree.statistics;
        cout << endl << endl;
        trailCount += DCorLC.produceHumanReadableFile(fileName);
        cout << "Minimum weight 3-round trail core found: " << dec << minWeight << endl;
        cout << "A total of " << dec << trailCount << " trails found." << endl;
    }
    catch(Exception e) {
        cerr << e.reason << endl;
    }
}
