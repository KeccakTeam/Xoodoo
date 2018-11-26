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

#include <fstream>
#include "Xoodoo.h"
#include "XoodooTrails.h"

using namespace std;

Trail::Trail(const XoodooPropagation& aDCorLC)
    : DCorLC(aDCorLC), firstStateSpecified(true), stateAfterLastChiSpecified(false), stateAfterLastChi(aDCorLC.parent), totalWeight(0)
{
}

Trail::Trail(const XoodooPropagation& aDCorLC, istream& fin)
    : DCorLC(aDCorLC), stateAfterLastChi(aDCorLC.parent)
{
    load(fin);
}

Trail& Trail::operator=(const Trail& other)
{
    firstStateSpecified = other.firstStateSpecified;
    states = other.states;
    stateAfterLastChiSpecified = other.stateAfterLastChiSpecified;
    stateAfterLastChi = other.stateAfterLastChi;
    weights = other.weights;
    totalWeight = other.totalWeight;
    return *this;
}

unsigned int Trail::getNumberOfRounds() const
{
    return states.size();
}

void Trail::clear()
{
    states.clear();
    weights.clear();
    totalWeight = 0;
    firstStateSpecified = true;
    stateAfterLastChiSpecified = false;
}

void Trail::setBeforeFirstStateWeight(unsigned int weight)
{
    if (states.size() == 0) {
        states.push_back(XoodooState(DCorLC.parent));
        firstStateSpecified = false;
    }
    if (weights.size() == 0)
        weights.push_back(weight);
    else {
        totalWeight -= weights[0];
        weights[0] = weight;
    }
    totalWeight += weight;
}

void Trail::append(const XoodooState& state, unsigned int weight)
{
    states.push_back(state);
    weights.push_back(weight);
    totalWeight += weight;
}

void Trail::prepend(const XoodooState& state, unsigned int weight)
{
    if (firstStateSpecified) {
        states.insert(states.begin(), state);
        weights.insert(weights.begin(), weight);
        totalWeight += weight;
    }
    else {
        states.insert(states.begin()+1, state);
        weights.insert(weights.begin(), weight);
        totalWeight += weight;
    }
}

void Trail::display(ostream& fout) const
{
    if (states.size() == 0) {
        fout << "This trail is empty." << endl;
        return;
    }

    fout << dec << states.size() << "-round ";
    if (DCorLC.getPropagationType() == XoodooPropagation::DC)
        fout << "differential ";
    else
        fout << "linear ";
    if (firstStateSpecified) {
        if (stateAfterLastChiSpecified)
            fout << "fully specified trail ";
        else
            fout << "trail prefix ";
    }
    else
        fout << "trail core ";
    fout << "of total weight " << dec << totalWeight << endl;

    unsigned int offsetIndex = (firstStateSpecified ? 0 : 1);
    if (!firstStateSpecified)
        fout << "Round 0 would have weight " << weights[0] << endl;
    for(unsigned int i=offsetIndex; i<states.size(); i++) {
        fout << "Round " << dec << i << " (weight " << weights[i] << "):" << endl;
        XoodooState stateAfterChi = states[i];
        DCorLC.reverseLambda(stateAfterChi);
        DCorLC.displayStatesInRound(fout, stateAfterChi);
    }
    if (stateAfterLastChiSpecified) {
        fout << "After \xCF\x87 of round " << dec << (states.size()-1) << ":" << endl;
        DCorLC.displayStatesInRound(fout, stateAfterLastChi);
    }
}

void Trail::save(ostream& fout) const
{
    fout << ((DCorLC.getPropagationType() == XoodooPropagation::DC) ? "DT" : "LT");
    if (firstStateSpecified) {
        if (!stateAfterLastChiSpecified)
            fout << "p";
    }
    else
        fout << "c";
    if (stateAfterLastChiSpecified) fout << "l";
    fout << " ";
    fout << dec << totalWeight << " ";
    fout << dec << weights.size() << " ";
    for(unsigned int i=0; i<weights.size(); i++)
        fout << dec << weights[i] << " ";
    fout << (states.size() - (firstStateSpecified ? 0 : 1)) << " ";
    for(unsigned int i=(firstStateSpecified ? 0 : 1); i<states.size(); i++)
        states[i].save(fout);
    if (stateAfterLastChiSpecified)
        stateAfterLastChi.save(fout);
    fout << endl;
}

void Trail::load(istream& fin)
{
    firstStateSpecified = true;
    stateAfterLastChiSpecified = false;
    stateAfterLastChi.clear();

    {
        string c;
        fin >> c;
        if (c.size() < 2)
            throw TrailException();
        if ((c[0] == 'D') && (DCorLC.getPropagationType() == XoodooPropagation::LC))
            throw TrailException("Differential trail read in a linear propagation context");
        if ((c[0] == 'L') && (DCorLC.getPropagationType() == XoodooPropagation::DC))
            throw TrailException("Linear trail read in a differential propagation context");
        for(unsigned int i=0; i<c.size(); i++) {
            if (c[i] == 'c') firstStateSpecified = false;
            if (c[i] == 'l') stateAfterLastChiSpecified = true;
        }
    }

    fin >> dec >> totalWeight;

    {
        unsigned int size;
        fin >> dec >> size;
        weights.resize(size);
        for(unsigned int i=0; i<size; i++)
            fin >> dec >> weights[i];
    }

    {
        unsigned int size;
        fin >> dec >> size;
        states.clear();
        if (!firstStateSpecified)
            states.push_back(XoodooState(DCorLC.parent));
        for(unsigned int i=0; i<size; i++) {
            XoodooState state(DCorLC.parent);
            state.load(fin);
            states.push_back(state);
        }
    }

    if (stateAfterLastChiSpecified)
        stateAfterLastChi.load(fin);
}

void Trail::append(const Trail& otherTrail)
{
    for(unsigned int i=0; i<otherTrail.weights.size(); i++)
        append(otherTrail.states[i], otherTrail.weights[i]);
}

void Trail::translateXZ(int dx, int dz)
{
    for(unsigned int i=(firstStateSpecified ? 0 : 1); i<states.size(); i++)
        states[i].translateXZ(dx, dz);
    if (stateAfterLastChiSpecified)
        stateAfterLastChi.translateXZ(dx, dz);
}

/** This method implements an arbitrary ordering between Xoodoo states, for Trail::makeCanonical(). */
static bool isSmaller(const XoodooState& a, const XoodooState& b)
{
    const vector<LaneValue>& lanesA = a.getLanesConst();
    const vector<LaneValue>& lanesB = b.getLanesConst();
    for(unsigned int i=0; (i < lanesA.size()) && (i < lanesB.size()); i++) {
        if (lanesA[i] < lanesB[i])
            return true;
        else if (lanesA[i] > lanesB[i])
            return false;
    }
    return false;
}

/** This method implements an arbitrary ordering between Xoodoo trails, for Trail::makeCanonical(). */
static bool isSmaller(const Trail& a, const Trail& b)
{
    for(unsigned int i=((a.firstStateSpecified && b.firstStateSpecified) ? 0 : 1); (i < a.states.size()) && (i < b.states.size()); i++) {
        if (isSmaller(a.states[i], b.states[i]))
            return true;
        else if (isSmaller(b.states[i], a.states[i]))
            return false;
    }
    if (a.stateAfterLastChiSpecified && b.stateAfterLastChiSpecified) {
        if (isSmaller(a.stateAfterLastChi, b.stateAfterLastChi))
            return true;
        else
            return false;
    }
    else
        return false;
}

void Trail::makeCanonical()
{
    Trail currentMin(*this);
    int dxMin = 0;
    int dzMin = 0;
    for(int dx=0; dx<(int)DCorLC.parent.getSizeX(); dx++)
    for(int dz=0; dz<(int)DCorLC.parent.getSizeZ(); dz++) {
        Trail current(*this);
        current.translateXZ(dx, dz);
        if (isSmaller(current, currentMin)) {
            currentMin = current;
            dxMin = dx;
            dzMin = dz;
        }
    }
    translateXZ(dxMin, dzMin);
}
