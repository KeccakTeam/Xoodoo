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

#include <algorithm>
#include <iostream>
#include <math.h>
#include <set>
#include "types.h"
#include "XoodooAffineBases.h"

using namespace std;


// -------------------------------------------------------------
//
// AffineSpaceOfColumns
//
// -------------------------------------------------------------

void AffineSpaceOfColumns::display(ostream& fout) const
{
    fout.fill('0'); fout.width(2); fout << hex << (int)offset;
    fout << " + <";
    for(unsigned int i=0; i<generators.size(); i++) {
        fout.fill('0'); fout.width(2); fout << hex << (int)generators[i];
        if (i < (generators.size()-1))
            fout << ", ";
    }
    fout << ">" << endl;
}

void AffineSpaceOfColumns::getAllColumnValues(vector<ColumnValue>& list) const
{
    list.clear();
    list.push_back(offset);
    if (generators.size() >= 1) {
        list.push_back(offset ^ generators[0]);
    }
    if (generators.size() >= 2) {
        list.push_back(offset ^ generators[1]);
        list.push_back(offset ^ generators[0] ^ generators[1]);
    }
    if (generators.size() > 2)
        throw Exception("This function assumes that there are at most two generators.");
}

// -------------------------------------------------------------
//
// AffineSpaceOfStates
//
// -------------------------------------------------------------

AffineSpaceOfStates::AffineSpaceOfStates(const XoodooDCLC& anInstance, vector<XoodooState>& aGenerators, vector<XoodooPlane>& aGeneratorParities, const XoodooState& aOffset, const XoodooPlane& aOffsetParity)
    : instance(anInstance), offset(aOffset), offsetParity(aOffsetParity)
{
    setGenerators(aGenerators, aGeneratorParities);
}

void AffineSpaceOfStates::setGenerators(vector<XoodooState>& aGenerators, vector<XoodooPlane>& aGeneratorParities)
{
    // Copy the generators into originalGenerators
    originalGenerators = aGenerators;
    originalParities = aGeneratorParities;

    // Upper-triangularize the parities
    for(unsigned int x=0; x<instance.getSizeX(); x++)
    for(unsigned int z=0; z<instance.getSizeZ(); z++) {
        // Look for a generator with a parity 1 at position (x,z)
        bool found = false;
        XoodooState foundState(instance);
        XoodooPlane foundParity(instance);
        for(unsigned int i=0; i<aGenerators.size(); i++) {
            if (aGeneratorParities[i].getBit(x, z) != 0) {
                foundState = aGenerators[i];
                offsetGenerators.push_back(foundState);
                foundParity = aGeneratorParities[i];
                offsetParities.push_back(foundParity);
                found = true;
                break;
            }
        }
        // If found, cancel all parities at position (x,z) for the other aGenerators
        if (found) {
            for(unsigned int i=0; i<aGenerators.size(); i++) {
                if ((aGeneratorParities[i].getBit(x, z)) != 0) {
                    aGenerators[i] ^= foundState;
                    aGeneratorParities[i] ^= foundParity;
                }
            }
        }
    }
    // The remaining aGenerators have zero parity
    for(unsigned int i=0; i<aGenerators.size(); i++) {
        if (!aGenerators[i].isZero())
            kernelGenerators.push_back(aGenerators[i]);
    }
}

void AffineSpaceOfStates::display(ostream& fout) const
{
    fout << "Offset = " << endl;
    fout << offset;
    fout << "with parity: " << endl;
    fout << offsetParity << endl;

    if (originalGenerators.size() == 0)
        fout << "No generators" << endl;
    else {
        fout << dec << originalGenerators.size() << " generators:" << endl;
        for(unsigned int i=0; i<originalGenerators.size(); i++) {
            fout << originalGenerators[i];
            fout << "with parity: " << endl;
            fout << originalParities[i] << endl;
        }
        if (offsetGenerators.size() == 0)
            fout << "No parity-offset generators" << endl;
        else {
            fout << dec << offsetGenerators.size() << " parity-offset generators:" << endl;
            for(unsigned int i=0; i<offsetGenerators.size(); i++) {
                fout << offsetGenerators[i];
                fout << "with parity: " << endl;
                fout << offsetParities[i] << endl;
            }
        }
        if (kernelGenerators.size() == 0)
            fout << "No parity-kernel generators" << endl;
        else {
            fout << dec << kernelGenerators.size() << " parity-kernel generators:" << endl;
            for(unsigned int i=0; i<kernelGenerators.size(); i++)
                fout << kernelGenerators[i] << endl;
        }
    }
}

bool oneAndZeroesBefore(const XoodooPlane& parity, unsigned int x, unsigned int z)
{
    for(unsigned int ix=0; ix<x; ix++)
        if (parity.getLane(ix) != 0)
            return false;
    LaneValue maskZ = ((LaneValue)1 << (z+1))-1;
    LaneValue selectZ = (LaneValue)1 << z;
    bool result = (parity.getLane(x) & maskZ) == selectZ;
    return result;
}

bool AffineSpaceOfStates::getOffsetWithGivenParity(const XoodooPlane& parity, XoodooState& output) const
{
    output = offset;
    XoodooPlane outputParity(offsetParity);
    XoodooPlane correctionParity(parity);
    correctionParity ^= offsetParity;

    unsigned int i = 0;

    for(unsigned int x=0; x<instance.getSizeX(); x++)
    for(unsigned int z=0; z<instance.getSizeZ(); z++) {
        if (correctionParity.getBit(x, z) != 0) {
            while((i<offsetParities.size()) && (!oneAndZeroesBefore(offsetParities[i], x, z)))
                i++;
            if (i<offsetParities.size()) {
                output ^= offsetGenerators[i];
                correctionParity ^= offsetParities[i];
            }
            else
                return false;
        }
    }
    return correctionParity.isZero();
}

XoodooAffineSpaceIterator AffineSpaceOfStates::getIteratorWithGivenParity(const XoodooPlane& parity) const
{
    XoodooState offset(instance);

    if (getOffsetWithGivenParity(parity, offset))
        return XoodooAffineSpaceIterator(kernelGenerators, offset);
    else
        return XoodooAffineSpaceIterator(offset);
}

XoodooAffineSpaceIterator AffineSpaceOfStates::getIteratorInKernel() const
{
    XoodooPlane parity(instance);
    XoodooState offset(instance);

    if (getOffsetWithGivenParity(parity, offset))
        return XoodooAffineSpaceIterator(kernelGenerators, offset);
    else
        return XoodooAffineSpaceIterator(offset);
}

XoodooAffineSpaceIterator AffineSpaceOfStates::getIterator() const
{
    return XoodooAffineSpaceIterator(originalGenerators, offset);
}
