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
#include <cstdint>
#include <fstream>
#include <iostream>
#include <math.h>
#include "XoodooDCLC.h"
#include "XoodooPropagation.h"
#include "XoodooTrails.h"

const unsigned int nrRows = Xoodoo::sizeY;

XoodooPropagation::XoodooPropagation(const XoodooDCLC& aParent, XoodooPropagation::DCorLC aDCorLC)
    : parent(aParent),
    name((aDCorLC == DC) ? "DC" : "LC"),
    lambdaMode((aDCorLC == DC) ? XoodooDCLC::Straight : XoodooDCLC::Transpose),
    reverseLambdaMode((aDCorLC == DC) ? XoodooDCLC::Inverse : XoodooDCLC::Dual)
{
    initializeAffine();
    initializeChiCompatibilityTable();
}

void XoodooPropagation::initializeAffine()
{
    for(ColumnValue column=0; column<(1<<nrRows); column++) {
        AffineSpaceOfColumns a;
        if (column == ((1<<nrRows)-1)) {
            a.setOffset(0x7);
            a.addGenerator(0x3);
            a.addGenerator(0x6);
        }
        else {
            for(int i=0; i<(int)nrRows; i++) {
                ColumnValue t = translateColumn(column, i);
                if (t == 0x1) {
                    a.setOffset(translateColumn(0x1, -i));
                    a.addGenerator(translateColumn(0x2, -i));
                    a.addGenerator(translateColumn(0x4, -i));
                    break;
                }
                if (t == 0x3) {
                    a.setOffset(translateColumn(0x1, -i));
                    a.addGenerator(translateColumn(0x3, -i));
                    a.addGenerator(translateColumn(0x4, -i));
                    break;
                }
            }
        }
        affinePerInput.push_back(a);
    }
}

void XoodooPropagation::initializeChiCompatibilityTable()
{
    chiCompatibilityTable.assign((1<<nrRows)*(1<<nrRows), false);
    for(ColumnValue a=0; a<(1<<nrRows); a++) {
        vector<ColumnValue> list;
        affinePerInput[a].getAllColumnValues(list);
        for(unsigned int i=0; i<list.size(); i++)
            chiCompatibilityTable[a+(1<<nrRows)*list[i]] = true;
    }
}

XoodooPropagation::DCorLC XoodooPropagation::getPropagationType() const
{
    if (lambdaMode == XoodooDCLC::Straight)
        return DC;
    else if (lambdaMode == XoodooDCLC::Transpose)
        return LC;
    else
        throw Exception("The lambda mode does not match either DC or LC propagation.");
}

unsigned int XoodooPropagation::getWeight(const XoodooState& state) const
{
    unsigned int weight = 0;
    for(unsigned int x=0; x<parent.getSizeX(); x++) {
        LaneValue activeColumns = state.getLane(x, 0) | state.getLane(x, 1) | state.getLane(x, 2);
        while(activeColumns != 0) {
            if (activeColumns & 1)
                weight += 2;
            activeColumns >>= 1;
        }
    }
    return weight;
}

bool XoodooPropagation::isChiCompatible(const XoodooState& beforeChi, const XoodooState& afterChi) const
{
    for(unsigned int x=0; x<parent.getSizeX(); x++)
    for(unsigned int z=0; z<parent.getSizeZ(); z++) {
        if (!isChiCompatible(beforeChi.getColumn(x, z), afterChi.getColumn(x, z)))
            return false;
    }
    return true;
}

bool XoodooPropagation::isRoundCompatible(const XoodooState& first, const XoodooState& second) const
{
    XoodooState secondBeforeLambda(second);
    reverseLambda(secondBeforeLambda);
    return isChiCompatible(first, secondBeforeLambda);
}

void XoodooPropagation::display(ostream& out) const
{
    if (getPropagationType() == XoodooPropagation::DC)
        out << "DC analysis tables; patterns are differences." << endl;
    else
        out << "LC analysis tables; patterns are linear masks." << endl;

    vector<vector<ColumnValue> > columnValuesPerWeight;
    unsigned int maxWeight = 2;

    columnValuesPerWeight.resize(maxWeight+1);
    for(unsigned int i=0; i<affinePerInput.size(); i++)
        columnValuesPerWeight[affinePerInput[i].getWeight()].push_back(i);

    for(unsigned int i=0; i<columnValuesPerWeight.size(); i++) {
        if (columnValuesPerWeight[i].size() > 0) {
            out << "Weight " << dec << i << ": ";
            for(unsigned int j=0; j<columnValuesPerWeight[i].size(); j++) {
                if (j > 0)
                    out << ", ";
                out << hex << (int)columnValuesPerWeight[i][j];
            }
            out << endl;
        }
    }
}

AffineSpaceOfStates XoodooPropagation::buildStateBase(const XoodooState& state, bool reverse) const
{
    static const bool debug = false;
    XoodooDCLC::LambdaMode actualLambdaMode = reverse ? reverseLambdaMode : lambdaMode;
    XoodooState afterChi(parent), beforeTheta(parent), beforeChi(parent);

    vector<XoodooState> basisBeforeChi;
    vector<XoodooPlane> paritiesBeforeTheta;
    XoodooState o(parent);
    for(unsigned int x=0; x<parent.getSizeX(); x++)
    for(unsigned int z=0; z<parent.getSizeZ(); z++) {
        ColumnValue columnIn = state.getColumn(x, z);
        if (columnIn != 0) {
            const AffineSpaceOfColumns& columnsOut = affinePerInput[columnIn];
            o.addToColumn(x, z, columnsOut.offset);
            for(unsigned int i=0; i<columnsOut.generators.size(); i++) {
                XoodooState v(parent);
                ColumnValue genColumnOut = columnsOut.generators[i];
                v.setColumn(x, z, genColumnOut);
                if (debug) afterChi = v;
                parent.lambdaBeforeTheta(v, actualLambdaMode);
                if (debug) beforeTheta = v;
                XoodooPlane p(parent);
                parent.getParity(v, p);
                parent.lambdaThetaAndAfter(v, actualLambdaMode);
                if (debug) beforeChi = v;
                basisBeforeChi.push_back(v);
                paritiesBeforeTheta.push_back(p);
                if (debug) {
                    cout << "Generator: " << endl;
                    ::displayStatesInRound(cout, afterChi, beforeTheta, beforeChi);
                }
            }
        }
    }

    if (debug) afterChi = o;
    parent.lambdaBeforeTheta(o, actualLambdaMode);
    if (debug) beforeTheta = o;
    XoodooPlane p(parent);
    parent.getParity(o, p);
    parent.lambdaThetaAndAfter(o, actualLambdaMode);
    if (debug) beforeChi = o;
    if (debug) {
        cout << "Offset: " << endl;
        ::displayStatesInRound(cout, afterChi, beforeTheta, beforeChi);
    }

    return AffineSpaceOfStates(parent, basisBeforeChi, paritiesBeforeTheta, o, p);
}

void XoodooPropagation::directLambda(XoodooState& state) const
{
    parent.lambda(state, lambdaMode);
}

void XoodooPropagation::reverseLambda(XoodooState& state) const
{
    parent.lambda(state, reverseLambdaMode);
}

void XoodooPropagation::directEarlyRho(int x, int y, int z, int& X, int& Y, int& Z) const
{
    if (lambdaMode == XoodooDCLC::Straight)
        parent.rhoEast(x, y, z, X, Y, Z);
    else if (lambdaMode == XoodooDCLC::Transpose)
        parent.inverseRhoWest(x, y, z, X, Y, Z);
}

void XoodooPropagation::reverseEarlyRho(int x, int y, int z, int& X, int& Y, int& Z) const
{
    if (lambdaMode == XoodooDCLC::Straight)
        parent.inverseRhoEast(x, y, z, X, Y, Z);
    else if (lambdaMode == XoodooDCLC::Transpose)
        parent.rhoWest(x, y, z, X, Y, Z);
}

void XoodooPropagation::directLateRho(int x, int y, int z, int& X, int& Y, int& Z) const
{
    if (lambdaMode == XoodooDCLC::Straight)
        parent.rhoWest(x, y, z, X, Y, Z);
    else if (lambdaMode == XoodooDCLC::Transpose)
        parent.inverseRhoEast(x, y, z, X, Y, Z);
}

void XoodooPropagation::reverseLateRho(int x, int y, int z, int& X, int& Y, int& Z) const
{
    if (lambdaMode == XoodooDCLC::Straight)
        parent.inverseRhoWest(x, y, z, X, Y, Z);
    else if (lambdaMode == XoodooDCLC::Transpose)
        parent.rhoEast(x, y, z, X, Y, Z);
}

void XoodooPropagation::checkTrail(const Trail& trail) const
{
    // Check weights
    unsigned int totalWeight = 0;
    unsigned int offsetIndex = (trail.firstStateSpecified ? 0 : 1);
    if ((!trail.firstStateSpecified) && (trail.weights.size() >= 1))
        totalWeight += trail.weights[0];
    for(unsigned int i=offsetIndex; i<trail.weights.size(); i++) {
        unsigned int weight = getWeight(trail.states[i]);
        if (weight != trail.weights[i]) {
            trail.display(cerr);
            cerr << "The weight of state at round " << dec << i << " is incorrect; it should be " << weight << "." << endl;
            throw Exception("The weights in the trail are incorrect!");
        }
        totalWeight += weight;
    }
    if (totalWeight != trail.totalWeight) {
        trail.display(cerr);
        cerr << "The total weight of the trail is incorrect; it should be " << totalWeight << "." << endl;
        throw Exception("The total weight in the trail is incorrect!");
    }

    // Check compatibility between consecutive states
    for(unsigned int i=1+offsetIndex; i<trail.states.size(); i++) {
        if (!isRoundCompatible(trail.states[i-1], trail.states[i])) {
            trail.display(cerr);
            cerr << "The state at round " << dec << i-1 << " is incompatible with that at round " << dec << i << "." << endl;
            throw Exception("Incompatible states found in the trail.");
        }
    }
    if (trail.stateAfterLastChiSpecified) {
        if (!isChiCompatible(trail.states.back(), trail.stateAfterLastChi)) {
            trail.display(cerr);
            cerr << "The state after the last \xCF\x87 is incompatible with that of the last round." << endl;
            throw Exception("Incompatible states found in the trail.");
        }
    }
}

uint64_t XoodooPropagation::displayTrailsAndCheck(const string& fileNameIn, ostream& fout, unsigned int maxWeight) const
{
    fout << parent << endl;
    if (getPropagationType() == XoodooPropagation::DC)
        fout << "Differential cryptanalysis" << endl;
    else
        fout << "Linear cryptanalysis" << endl;
    fout << endl;
    vector<uint64_t> countPerWeight, countPerLength;
    uint64_t totalCount = 0;
    unsigned int minWeight = 0;
    {
        ifstream fin(fileNameIn.c_str());
        while(!(fin.eof())) {
            try {
                Trail trail(*this, fin);
                checkTrail(trail);
                if (trail.totalWeight >= countPerWeight.size())
                    countPerWeight.resize(trail.totalWeight+1, 0);
                countPerWeight[trail.totalWeight]++;
                if (trail.states.size() >= countPerLength.size())
                    countPerLength.resize(trail.states.size()+1, 0);
                countPerLength[trail.states.size()]++;
                totalCount++;
            }
            catch(TrailException) {
            }
        }
        if (totalCount == 0) {
            fout << "No trails found in file " << fileNameIn << "!" << endl;
            return totalCount;
        }
        minWeight = 0;
        while((minWeight < countPerWeight.size()) && (countPerWeight[minWeight] == 0))
            minWeight++;
        for(unsigned int i=0; i<countPerLength.size(); i++)
            if (countPerLength[i] > 0)
                fout << dec << countPerLength[i] << " trails of length " << dec << i << " read and checked." << endl;
        fout << "Minimum weight: " << dec << minWeight << endl;
        for(unsigned int i=minWeight; i<countPerWeight.size(); i++)
            if (countPerWeight[i] > 0) {
                fout.width(8); fout.fill(' ');
                fout << dec << countPerWeight[i] << " trails of weight ";
                fout.width(2); fout.fill(' ');
                fout << i << endl;
            }
        fout << endl;
    }
    if (maxWeight == 0) {
        const unsigned int reasonableNumber = 2000;
        maxWeight = minWeight;
        uint64_t countSoFar = countPerWeight[minWeight];
        while((maxWeight < (countPerWeight.size()-1)) && ((countSoFar+countPerWeight[maxWeight+1]) <= reasonableNumber)) {
            maxWeight++;
            countSoFar += countPerWeight[maxWeight];
        }
    }
    fout << "Showing the trails up to weight " << dec << maxWeight << " (in no particular order)." << endl;
    fout << endl;
    {
        ifstream fin(fileNameIn.c_str());
        while(!(fin.eof())) {
            try {
                Trail trail(*this, fin);
                if (trail.totalWeight <= maxWeight) {
                    trail.display(fout);
                    fout << endl;
                }
            }
            catch(TrailException) {
            }
        }
    }
    return totalCount;
}

uint64_t XoodooPropagation::produceHumanReadableFile(const string& fileName, bool verbose, unsigned int maxWeight) const
{
    string fileName2 = fileName+".txt";
    ofstream fout(fileName2.c_str());
    if (verbose)
        cout << "Writing " << fileName2 << flush;
    uint64_t count = displayTrailsAndCheck(fileName, fout, maxWeight);
    if (verbose)
        cout << endl;
    return count;
}

string XoodooPropagation::buildFileName(const string& suffix) const
{
    return parent.buildFileName(name, suffix);
}

string XoodooPropagation::buildFileName(const string& prefix, const string& suffix) const
{
    return parent.buildFileName(name+prefix, suffix);
}

void XoodooPropagation::displayStatesInRound(ostream& fout, const XoodooState& stateAfterChi) const
{
    const unsigned int sizeZ = parent.getSizeZ();
    const unsigned int padZ = (sizeZ < 4) ? 4 : sizeZ;
    XoodooState stateBeforeTheta = stateAfterChi;
    parent.lambdaBeforeTheta(stateBeforeTheta, lambdaMode);
    XoodooPlane parity(parent);
    parent.getParity(stateBeforeTheta, parity);
    bool kernel = parity.isZero();
    XoodooState stateAfterTheta = stateBeforeTheta;
    parent.lambdaTheta(stateAfterTheta, lambdaMode);
    XoodooState stateBeforeChi = stateAfterTheta;
    parent.lambdaAfterTheta(stateBeforeChi, lambdaMode);

    if (lambdaMode == XoodooDCLC::Straight) {
        fout << "NE"; for(unsigned int i=2; i<padZ; i++) fout << " ";
        fout << " \xCF\x81" << "E  ";
        if (kernel) {
            fout << "S(K)"; for(unsigned int i=4; i<padZ; i++) fout << " ";
        }
        else {
            fout << "SE"; for(unsigned int i=2; i<padZ; i++) fout << " ";
            fout << "  \xCE\xB8  ";
            fout << "SW"; for(unsigned int i=2; i<padZ; i++) fout << " ";
        }
        fout << " \xCF\x81W  ";
        fout << "NW"; for(unsigned int i=2; i<padZ; i++) fout << " ";
        fout << endl;
    }
    else if (lambdaMode == XoodooDCLC::Transpose) {
        fout << "NW"; for(unsigned int i=2; i<padZ; i++) fout << " ";
        fout << "\xCF\x81W-1 ";
        if (kernel) {
            fout << "S(K)"; for(unsigned int i=4; i<padZ; i++) fout << " ";
        }
        else {
            fout << "SW"; for(unsigned int i=2; i<padZ; i++) fout << " ";
            fout << " \xCE\xB8T  ";
            fout << "SE"; for(unsigned int i=2; i<padZ; i++) fout << " ";
        }
        fout << "\xCF\x81" << "E-1 ";
        fout << "NE"; for(unsigned int i=2; i<padZ; i++) fout << " ";
        fout << endl;
    }
    for(unsigned int x=0; x<parent.getSizeX(); x++) {
        fout << stateAfterChi.getDisplayString(x, padZ) << "  |  ";
        if (!kernel)
            fout << stateBeforeTheta.getDisplayString(x, padZ) << "  |  ";
        fout << stateAfterTheta.getDisplayString(x, padZ) << "  |  ";
        fout << stateBeforeChi.getDisplayString(x, padZ) << endl;
    }
}
