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

#ifndef _XOODOOTRAILEXTENSION_H_
#define _XOODOOTRAILEXTENSION_H_

#include "XoodooTrails.h"

void extendTrailAll(ostream& fout, const Trail& trail, bool backwardExtension, unsigned int maxWeight, unsigned int& minWeightFound);
void extendTrail(ostream& fout, const Trail& trail, bool backwardExtension, unsigned int nrRounds, unsigned int maxTotalWeight, unsigned int& minWeightFound, bool verbose = false);

#endif
