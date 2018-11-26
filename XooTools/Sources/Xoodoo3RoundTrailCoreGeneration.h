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

#ifndef _XOODOO3ROUNDTRAILCOREGENERATION_H_
#define _XOODOO3ROUNDTRAILCOREGENERATION_H_

#include "XoodooPropagation.h"

void generate3RoundTrailCores(XoodooPropagation::DCorLC propagationType, bool backwardExtension, int T3);

#endif
