/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <iostream>
#include <sstream>
#include "Keccak.h"

Keccak::Keccak(unsigned int aRate, unsigned int aCapacity)
    : Sponge(new KeccakF(aRate+aCapacity), new MultiRatePadding(), aRate)
{
}

Keccak::~Keccak()
{
    delete f;
    delete pad;
}

std::string Keccak::getDescription() const
{
    std::stringstream a;
    a << "Keccak[r=" << std::dec << rate << ", c=" << std::dec << capacity << "]";
    return a.str();
}

ReducedRoundKeccak::ReducedRoundKeccak(unsigned int aRate, unsigned int aCapacity, int aStartRoundIndex, unsigned int aNrRounds)
    : Sponge(new KeccakFanyRounds(aRate+aCapacity, aStartRoundIndex, aNrRounds), new MultiRatePadding(), aRate),
    nrRounds(aNrRounds),
    startRoundIndex(aStartRoundIndex)
{
}

ReducedRoundKeccak::~ReducedRoundKeccak()
{
    delete f;
    delete pad;
}

std::string ReducedRoundKeccak::getDescription() const
{
    std::stringstream a;
    a << "Keccak[r=" << std::dec << rate << ", c=" << std::dec << capacity << ", " << std::dec << nrRounds << " ";
    a << ((nrRounds > 1) ? "rounds" : "round");
    a << " from " << std::dec << startRoundIndex << " to " << startRoundIndex+nrRounds-1 << "]";
    return a.str();
}
