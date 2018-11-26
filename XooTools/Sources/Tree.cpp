/*
KeccakTools

This code implements the techniques described in FSE2017 paper
"New techniques for trail bounds and application to differential trails in Keccak"
by Silvia Mella, Joan Daemen and Gilles Van Assche.
http://tosc.iacr.org/index.php/ToSC/article/view/597
http://eprint.iacr.org/2017/181

Implementation by Silvia Mella and Gilles Van Assche, hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <iostream>
#include "Tree.h"

ostream& operator<<(ostream& a, const GenericTreeIteratorStatistics& s)
{
    a << "Subtrees considered:          " << dec; a.width(20); a.fill(' '); a << s.subtreesConsidered << endl;
    a << "Subtrees not well formed:     " << dec; a.width(20); a.fill(' '); a << s.subtreesNotWellFormed << endl;
    a << "Subtrees too costly:          " << dec; a.width(20); a.fill(' '); a << s.subtreesTooCostly << endl;
    a << "Subtrees not canonical:       " << dec; a.width(20); a.fill(' '); a << s.subtreesNotCanonical << endl;
    a << "------------------------------" << endl;
    a << "Nodes considered:             " << dec; a.width(20); a.fill(' '); a << s.nodesConsidered << endl;
    a << "Nodes not well formed:        " << dec; a.width(20); a.fill(' '); a << s.nodesNotWellFormed << endl;
    a << "Nodes too costly:             " << dec; a.width(20); a.fill(' '); a << s.nodesTooCostly << endl;
    a << "Nodes not canonical:          " << dec; a.width(20); a.fill(' '); a << s.nodesNotCanonical << endl;
    a << "------------------------------" << endl;
    a << "Nodes actually output:        " << dec; a.width(20); a.fill(' '); a << s.nodesOutput << endl;
    return a;
}
