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

#include "XoodooDCLC.h"
#include "XoodooPropagation.h"

using namespace std;

const unsigned int nrRows = Xoodoo::sizeY;

XoodooDCLC::XoodooDCLC(unsigned int aSizeX, unsigned int aSizeZ, const XoodooParameters& aParameters)
    : Xoodoo(aSizeX, aSizeZ, aParameters)
{
}

string XoodooDCLC::getDescription() const
{
    return "DC/LC analysis of " + Xoodoo::getDescription();
}

ColumnValue XoodooDCLC::chiOnColumn(ColumnValue a) const
{
    XoodooState state(*this);
    state.setColumn(0, 0, a);
    chi(state);
    return state.getColumn(0, 0);
}

ColumnValue XoodooDCLC::inverseChiOnColumn(ColumnValue a) const
{
    XoodooState state(*this);
    state.setColumn(0, 0, a);
    inverseChi(state);
    return state.getColumn(0, 0);
}

void XoodooDCLC::lambda(XoodooState& state, LambdaMode mode) const
{
    if (mode == Straight) {
        rhoEast(state);
        theta(state);
        rhoWest(state);
    }
    else if (mode == Inverse) {
        inverseRhoWest(state);
        inverseTheta(state);
        inverseRhoEast(state);
    }
    else if (mode == Transpose) {
        inverseRhoWest(state);
        thetaTransposed(state);
        inverseRhoEast(state);
    }
    else if (mode == Dual) {
        rhoEast(state);
        inverseThetaTransposed(state);
        rhoWest(state);
    }
}

void XoodooDCLC::lambdaBeforeTheta(XoodooState& state, LambdaMode mode) const
{
    if (mode == Straight) {
        rhoEast(state);
    }
    else if (mode == Inverse) {
        inverseRhoWest(state);
    }
    else if (mode == Transpose) {
        inverseRhoWest(state);
    }
    else if (mode == Dual) {
        rhoEast(state);
    }
}

void XoodooDCLC::lambdaTheta(XoodooState& state, LambdaMode mode) const
{
    (void)state;
    if (mode == Straight) {
        theta(state);
    }
    else if (mode == Inverse) {
        inverseTheta(state);
    }
    else if (mode == Transpose) {
        thetaTransposed(state);
    }
    else if (mode == Dual) {
        inverseThetaTransposed(state);
    }
}

void XoodooDCLC::lambdaAfterTheta(XoodooState& state, LambdaMode mode) const
{
    if (mode == Straight) {
        rhoWest(state);
    }
    else if (mode == Inverse) {
        inverseRhoEast(state);
    }
    else if (mode == Transpose) {
        inverseRhoEast(state);
    }
    else if (mode == Dual) {
        rhoWest(state);
    }
}

void XoodooDCLC::lambdaThetaAndAfter(XoodooState& state, LambdaMode mode) const
{
    lambdaTheta(state, mode);
    lambdaAfterTheta(state, mode);
}

void XoodooDCLC::getParity(const XoodooState& state, XoodooPlane& parity) const
{
    parity.clear();
    for(unsigned int x=0; x<sizeX; x++)
        parity.getLanes()[x] = state.getLane(x, 0) ^ state.getLane(x, 1) ^ state.getLane(x, 2);
}

XoodooPlane::XoodooPlane(const Xoodoo& anInstance)
    : XoodooLanes(anInstance.getSizeX()), instance(anInstance), sizeZ(anInstance.getSizeZ())
{
}

XoodooPlane& XoodooPlane::operator=(const XoodooPlane& other)
{
    lanes = other.lanes;
    return *this;
}

string XoodooPlane::getDisplayString(unsigned int x) const
{
    const char character[2] = {'.', 'o'};
    string result;
    for(unsigned int z=0; z<instance.getSizeZ(); z++)
        result += character[getBit(x, z)];
    return result;
}

void XoodooPlane::display(ostream& fout) const
{
    for(unsigned int x=0; x<instance.getSizeX(); x++)
        fout << getDisplayString(x) << endl;
}

ostream& operator<<(ostream& a, const XoodooPlane& state)
{
    state.display(a);
    return a;
}
