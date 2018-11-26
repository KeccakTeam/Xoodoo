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

#include <iostream>
#include <map>
#include <sstream>
#include "progress.h"
#include "Tree.h"
#include "XoodooPropagation.h"
#include "XoodooTrailExtension.h"

class XoodooStateMask : public XoodooState {
public:
    XoodooStateMask(const Xoodoo& anInstance)
        : XoodooState(anInstance) {}
    void setMaskYXZ(unsigned int x, unsigned int y, unsigned int z);
    bool nonEmptyIntersection(const XoodooState& other) const;
};

void XoodooStateMask::setMaskYXZ(unsigned int x, unsigned int y, unsigned int z)
{
    clear();
    for(unsigned int iy=0; iy<Xoodoo::sizeY; iy++)
    for(unsigned int ix=0; ix<instance.getSizeX(); ix++) {
        if ((iy < y) || ((iy == y) && (ix < x)))
            lanes[y + Xoodoo::sizeY*x] = ~(LaneValue)0;
        else if ((iy == y) && (ix == x))
            lanes[y + Xoodoo::sizeY*x] = ((LaneValue)1 << z)-1;
    }
}

bool XoodooStateMask::nonEmptyIntersection(const XoodooState& other) const
{
    for(unsigned int i=0; i<lanes.size(); i++)
        if ((lanes[i] & other.getLanesConst()[i]) != 0)
            return true;
    return false;
}

class Coordinates {
public:
    unsigned int x;
    unsigned int y;
    unsigned int z;
public:
    Coordinates()
        : x(0), y(0), z(0) {}
    Coordinates(unsigned int ax, unsigned int ay, unsigned int az)
        : x(ax), y(ay), z(az) {}
    friend ostream& operator<<(ostream& a, const Coordinates& coordinates);
};

ostream& operator<<(ostream& a, const Coordinates& coordinates)
{
    return a << "(" << dec << coordinates.x << ", " << coordinates.y << ", " << coordinates.z << ")";
}

void upperTriangalizeBasis(const XoodooDCLC& instance, const vector<XoodooState>& originalBasis, vector<XoodooState>& newBasis, vector<Coordinates>& stability)
{
    vector<XoodooState> basis(originalBasis);
    for(unsigned int iy=0; iy<Xoodoo::sizeY; iy++)
    for(unsigned int ix=0; ix<instance.getSizeX(); ix++)
    for(unsigned int iz=0; iz<instance.getSizeZ(); iz++) {
        // Look for a generator with a parity 1 at position (x,y,z)
        bool found = false;
        XoodooState foundState(instance);
        for(unsigned int i=0; i<basis.size(); i++) {
            if (basis[i].getBit(ix, iy, iz) == 1) {
                found = true;
                foundState = basis[i];
                newBasis.push_back(foundState);
                stability.push_back(Coordinates(ix, iy, iz));
                basis.erase(basis.begin()+i);
                break;
            }
        }
        if (basis.size() == 0)
            break;
        if (found) {
            for(unsigned int i=0; i<basis.size(); i++)
                if (basis[i].getBit(ix, iy, iz) == 1)
                    basis[i] ^= foundState;
        }
    }
}

class AffineSpaceIteratorContext {
public:
    const Trail& trail;
    const vector<XoodooState>& basis;
    const XoodooState& offset;
    const vector<Coordinates>& stability;
    bool backwardExtension;
    bool optimized;
public:
    AffineSpaceIteratorContext(const Trail& aTrail, const vector<XoodooState>& aBasis, const XoodooState& anOffset, const vector<Coordinates>& aStability, bool aBackwardExtension)
        : trail(aTrail), basis(aBasis), offset(anOffset), stability(aStability), backwardExtension(aBackwardExtension) {}
    friend ostream& operator<<(ostream& a, const AffineSpaceIteratorContext& context);
};

ostream& operator<<(ostream& a, const AffineSpaceIteratorContext& context)
{
    a << "Trail to extend " << (context.backwardExtension ? "backward" : "forward") << ":" << endl;
    context.trail.display(a);
    a << endl;
    a << "Offset state:" << endl;
    a << context.offset;
    for(unsigned int i=0; i<context.basis.size(); i++) {
        a << "Basis state #" << dec << i;
        if (i < context.stability.size())
            a << " (stable until " << context.stability[i] << ")";
        a << ":" << endl;
        a << context.basis[i];
    }
    return a;
}

typedef unsigned int AffineSpaceBasisIndex;

class AffineSpaceIteratorCache {
public:
    const XoodooDCLC& instance;
    const XoodooPropagation& DCorLC;
    const AffineSpaceIteratorContext& affineSpace;
    XoodooState stateConsidered;
    XoodooStateMask partOfOffsetNeverMoving;
public:
    AffineSpaceIteratorCache(const AffineSpaceIteratorContext& anAffineSpace)
        : instance(anAffineSpace.trail.DCorLC.parent), DCorLC(anAffineSpace.trail.DCorLC),
            affineSpace(anAffineSpace), stateConsidered(anAffineSpace.offset),
            partOfOffsetNeverMoving(anAffineSpace.trail.DCorLC.parent)
    {
        initializePartOfOffsetNeverMoving();
    }
    void push(const AffineSpaceBasisIndex& newUnit);
    void pop(const AffineSpaceBasisIndex& newUnit);
private:
    void initializePartOfOffsetNeverMoving();
};

void AffineSpaceIteratorCache::initializePartOfOffsetNeverMoving()
{
    partOfOffsetNeverMoving.clear();
    for(unsigned int i=0; i<affineSpace.basis.size(); i++)
        partOfOffsetNeverMoving |= affineSpace.basis[i];
    partOfOffsetNeverMoving.invert();
}

void AffineSpaceIteratorCache::push(const AffineSpaceBasisIndex& newUnit)
{
    stateConsidered ^= affineSpace.basis[newUnit];
}

void AffineSpaceIteratorCache::pop(const AffineSpaceBasisIndex& lastUnit)
{
    stateConsidered ^= affineSpace.basis[lastUnit];
}

class AffineSpaceUnitSet {
public:
    const AffineSpaceIteratorContext& affineSpace;
public:
    AffineSpaceUnitSet(const AffineSpaceIteratorContext& anAffineSpace)
        : affineSpace(anAffineSpace) {}
    AffineSpaceBasisIndex getFirstChildUnit(const UnitList<AffineSpaceBasisIndex>& unitList, AffineSpaceIteratorCache& cache) const;
    void iterateUnit(const UnitList<AffineSpaceBasisIndex>& unitList, AffineSpaceBasisIndex& current, AffineSpaceIteratorCache& cache) const;
    bool isSubtreeWellFormed(const UnitList<AffineSpaceBasisIndex>& parentUnitList, const AffineSpaceBasisIndex& newUnit, const AffineSpaceIteratorCache& cache) const;
    bool isNodeWellFormed(const UnitList<AffineSpaceBasisIndex>& unitList, const AffineSpaceIteratorCache& cache) const;
    bool isSubtreeCanonical(const UnitList<AffineSpaceBasisIndex>& parentUnitList, const AffineSpaceBasisIndex& newUnit, const AffineSpaceIteratorCache& cache) const;
    bool isNodeCanonical(const UnitList<AffineSpaceBasisIndex>& unitList, const AffineSpaceIteratorCache& cache) const;
};

AffineSpaceBasisIndex AffineSpaceUnitSet::getFirstChildUnit(const UnitList<AffineSpaceBasisIndex>& unitList, AffineSpaceIteratorCache& cache) const
{
    (void)cache;
    if (unitList.size() == 0) {
        if (affineSpace.basis.size() == 0)
            throw EndOfSet();
        return 0;
    }
    else {
        AffineSpaceBasisIndex next = unitList.back() + 1;
        if (next >= affineSpace.basis.size())
            throw EndOfSet();
        return next;
    }
}

void AffineSpaceUnitSet::iterateUnit(const UnitList<AffineSpaceBasisIndex>& unitList, AffineSpaceBasisIndex& current, AffineSpaceIteratorCache& cache) const
{
    (void)unitList;
    (void)cache;
    current++;
    if (current >= affineSpace.basis.size())
        throw EndOfSet();
}

bool AffineSpaceUnitSet::isSubtreeWellFormed(const UnitList<AffineSpaceBasisIndex>& parentUnitList, const AffineSpaceBasisIndex& newUnit, const AffineSpaceIteratorCache& cache) const
{
    (void)parentUnitList;
    (void)newUnit;
    (void)cache;
    return true;
}

bool AffineSpaceUnitSet::isNodeWellFormed(const UnitList<AffineSpaceBasisIndex>& unitList, const AffineSpaceIteratorCache& cache) const
{
    (void)unitList;
    (void)cache;
    return true;
}

bool AffineSpaceUnitSet::isSubtreeCanonical(const UnitList<AffineSpaceBasisIndex>& parentUnitList, const AffineSpaceBasisIndex& newUnit, const AffineSpaceIteratorCache& cache) const
{
    (void)parentUnitList;
    (void)newUnit;
    (void)cache;
    return true;
}

bool AffineSpaceUnitSet::isNodeCanonical(const UnitList<AffineSpaceBasisIndex>& unitList, const AffineSpaceIteratorCache& cache) const
{
    (void)unitList;
    (void)cache;
    return true;
}

class AffineSpaceExtendedTrail : public Trail {
public:
    AffineSpaceExtendedTrail(const XoodooPropagation& aDCorLC)
        : Trail(aDCorLC) {}
    AffineSpaceExtendedTrail(const AffineSpaceIteratorContext& context)
        : Trail(context.trail.DCorLC) {}
    AffineSpaceExtendedTrail(const Trail& other)
        : Trail(other) {}
    void set(const UnitList<AffineSpaceBasisIndex>& unitList, const AffineSpaceIteratorCache& cache);
};

void AffineSpaceExtendedTrail::set(const UnitList<AffineSpaceBasisIndex>& unitList, const AffineSpaceIteratorCache& cache)
{
    (void)unitList;
    Trail::operator=(cache.affineSpace.trail);
    if (cache.affineSpace.backwardExtension) {
        XoodooState stateBeforeChi(cache.stateConsidered);
        DCorLC.directLambda(stateBeforeChi);
        prepend(stateBeforeChi, DCorLC.getWeight(cache.stateConsidered));
    }
    else
        append(cache.stateConsidered, DCorLC.getWeight(cache.stateConsidered));
}

class AffineSpaceCostFunction {
public:
    unsigned int getSubtreeLowerBound(const UnitList<AffineSpaceBasisIndex>& parentUnitList, const AffineSpaceBasisIndex& newUnit, AffineSpaceIteratorCache& cache) const;
    unsigned int getNodeCost(const UnitList<AffineSpaceBasisIndex>& unitList, AffineSpaceIteratorCache& cache) const;
};

unsigned int AffineSpaceCostFunction::getSubtreeLowerBound(const UnitList<AffineSpaceBasisIndex>& parentUnitList, const AffineSpaceBasisIndex& newUnit, AffineSpaceIteratorCache& cache) const
{
    (void)parentUnitList;
    (void)newUnit;
    unsigned int trailWeight = cache.affineSpace.trail.totalWeight;
    if (cache.affineSpace.optimized) {
        if (cache.affineSpace.stability.size() > newUnit+1) {
            Coordinates currentStability = cache.affineSpace.stability[newUnit+1];
            XoodooStateMask stabilityMask(cache.instance);
            stabilityMask.setMaskYXZ(currentStability.x, currentStability.y, currentStability.z);
            stabilityMask |= cache.partOfOffsetNeverMoving;
            XoodooState maskedState(cache.stateConsidered);
            maskedState ^= cache.affineSpace.basis[newUnit];
            maskedState &= stabilityMask;
            return trailWeight + cache.DCorLC.getWeight(maskedState);
        }
        else if (newUnit == (cache.affineSpace.basis.size()-1)) {
            XoodooState stateConsidered(cache.stateConsidered);
            stateConsidered ^= cache.affineSpace.basis[newUnit];
            return trailWeight + cache.DCorLC.getWeight(stateConsidered);
        }
        else
            return trailWeight;
    }
    else
        return trailWeight;
}

unsigned int AffineSpaceCostFunction::getNodeCost(const UnitList<AffineSpaceBasisIndex>& unitList, AffineSpaceIteratorCache& cache) const
{
    (void)unitList;
    unsigned int trailWeight = cache.affineSpace.trail.totalWeight;
    return trailWeight + cache.DCorLC.getWeight(cache.stateConsidered);
}

void saveCoreCanonically(ostream& fout, const Trail& core)
{
    Trail canonical(core);
    canonical.makeCanonical();
    canonical.save(fout);
}


void extendTrailAll(ostream& fout, const Trail& trail, bool backwardExtension, unsigned int maxWeight, unsigned int& minWeightFound)
{
    XoodooState stateToExtend(trail.DCorLC.parent);
    if (backwardExtension) {
        stateToExtend = trail.firstStateSpecified ? trail.states[0] : trail.states[1];
        trail.DCorLC.reverseLambda(stateToExtend);
    }
    else {
        stateToExtend = trail.states.back();
    }
    AffineSpaceOfStates as = trail.DCorLC.buildStateBase(stateToExtend, backwardExtension);
    vector<XoodooState> triangularBasis;
    vector<Coordinates> stability;
    upperTriangalizeBasis(trail.DCorLC.parent, as.originalGenerators, triangularBasis, stability);
    AffineSpaceIteratorContext context(trail, triangularBasis, as.offset, stability, backwardExtension);
    context.optimized = true;
    AffineSpaceUnitSet unitSet(context);
    AffineSpaceCostFunction cost;
    GenericTreeIterator<AffineSpaceBasisIndex, AffineSpaceUnitSet, AffineSpaceIteratorContext,
        AffineSpaceIteratorCache, AffineSpaceExtendedTrail, AffineSpaceCostFunction, GenericProgressDisplay>
        tree(unitSet, context, cost, maxWeight);
    while(!tree.isEnd()) {
        const AffineSpaceExtendedTrail& core = (*tree);
        saveCoreCanonically(fout, core);
        if (core.totalWeight < minWeightFound) minWeightFound = core.totalWeight;
        ++tree;
    }
}

/** Class that maintains a list of minimum weights for each number of rounds
  * below which there is no need to look for trails.
  * This is typically used during trail extension to avoid looking for trails
  * with weight lower than the lower bound, or to exclude a subspace
  * already covered.
  */
class LowWeightExclusion {
protected:
    /** The explicitly excluded weights per number of rounds. */
    map<unsigned int, int> excludedWeight;
    /** The interpolated minimum weights per number of rounds.
      * Note that minWeight[nrRounds-1] contains the minimum weight
      * for nrRounds rounds.
      */
    vector<int> minWeight;
public:
    /** The constructor. */
    LowWeightExclusion();
    /** This method tells to exclude trails with weight below the given value
      * for the given number of rounds.
      * @param  nrRounds    The number of rounds.
      * @param  weight  The weight below which trails are to be excluded.
      */
    void excludeBelowWeight(unsigned int nrRounds, int weight);
    /** For a given number of rounds, this function returns the minimum weight
      * to consider.
      * @param  nrRounds    The number of rounds.
      * @return The minimum weight to consider.
      */
    int getMinWeight(unsigned int nrRounds);
    friend ostream& operator<<(ostream& out, const LowWeightExclusion& lwe);
protected:
    void computeExcludedLowWeight(unsigned int upToNrRounds);
};

LowWeightExclusion::LowWeightExclusion()
{
}

void LowWeightExclusion::excludeBelowWeight(unsigned int nrRounds, int weight)
{
    excludedWeight[nrRounds] = weight;
    minWeight.clear();
}

int LowWeightExclusion::getMinWeight(unsigned int nrRounds)
{
    if (nrRounds == 0)
        return 0;
    if (nrRounds > minWeight.size())
        computeExcludedLowWeight(nrRounds);
    return minWeight[nrRounds-1];
}

void LowWeightExclusion::computeExcludedLowWeight(unsigned int upToNrRounds)
{
    minWeight.clear();
    for(unsigned int nrRounds=1; nrRounds<=upToNrRounds; nrRounds++) {
        map<unsigned int, int>::iterator i = excludedWeight.find(nrRounds);
        if (i != excludedWeight.end()) {
            minWeight.push_back(i->second);
        }
        else {
            int max = 0;
            for(unsigned int n1=1; n1<=(nrRounds-1); n1++) {
                unsigned int n2 = nrRounds - n1;
                int sum = minWeight[n1-1] + minWeight[n2-1];
                if (sum > max) max = sum;
            }
            minWeight.push_back(max);
        }
    }
}

ostream& operator<<(ostream& out, const LowWeightExclusion& lwe)
{
    for(unsigned int nrRounds=1; nrRounds<=lwe.minWeight.size(); nrRounds++) {
        map<unsigned int, int>::const_iterator i = lwe.excludedWeight.find(nrRounds);
        bool extrapolated = (i == lwe.excludedWeight.end());
        out.width(2); out.fill(' '); out << dec << nrRounds << " rounds: ";
        out.width(3); out.fill(' '); out << dec << lwe.minWeight[nrRounds-1] << " ";
        if (extrapolated)
            out << "+";
        out << endl;
    }
    return out;
}

void recurseExtendTrail(ostream& fout, const Trail& trail, bool backwardExtension, unsigned int nrRounds, unsigned int maxTotalWeight, unsigned int& minWeightFound, LowWeightExclusion& knownBounds, ProgressMeter& progress, bool verbose);

void extendTrail(ostream& fout, const Trail& trail, bool backwardExtension, unsigned int nrRounds, unsigned int maxTotalWeight, unsigned int& minWeightFound, bool verbose)
{
    LowWeightExclusion knownBounds;
    knownBounds.excludeBelowWeight(1, 2);
    knownBounds.excludeBelowWeight(2, 8);
    knownBounds.excludeBelowWeight(3, 36);
    ProgressMeter progress;
    recurseExtendTrail(fout, trail, backwardExtension, nrRounds, maxTotalWeight, minWeightFound, knownBounds, progress, verbose);
}

void recurseExtendTrail(ostream& fout, const Trail& trail, bool backwardExtension, unsigned int nrRounds, unsigned int maxTotalWeight, unsigned int& minWeightFound, LowWeightExclusion& knownBounds, ProgressMeter& progress, bool verbose)
{
    if (verbose) {
        cout << "*** recurseExtendTrail("
            << (backwardExtension ? "backward" : "forward") << ", "
            << dec << nrRounds << " rounds, "
            << "up to weight " << maxTotalWeight
            << ")" << endl;
        trail.display(cout);
    }

    int baseWeight = trail.totalWeight;
    int baseNrRounds  = trail.getNumberOfRounds();
    unsigned int curNrRounds = baseNrRounds + 1;
    int curWeight = trail.weights.back();
    int maxWeightOut = maxTotalWeight - baseWeight
        - knownBounds.getMinWeight(nrRounds-baseNrRounds-1);
    if (maxWeightOut < knownBounds.getMinWeight(1)) {
        if (verbose) {
            cout << "--- leaving because maxWeightOut(=" << dec << maxWeightOut << ") < knownBounds.getMinWeight(1)(=" << dec << knownBounds.getMinWeight(1) << ")";
            cout << endl;
        }
        return;
    }
    int maxWeightNext = maxTotalWeight - knownBounds.getMinWeight(nrRounds-baseNrRounds-1);

    string synopsis;
    {
        stringstream str;
        str << (backwardExtension ? "Backward" : "Forward")
            <<" extending, current weight " << dec << curWeight << " towards round " << dec << curNrRounds;
        str << " (limiting weight to " << dec << maxWeightNext << ")";
        synopsis = str.str();
    }

    XoodooState stateToExtend(trail.DCorLC.parent);
    if (backwardExtension) {
        stateToExtend = trail.firstStateSpecified ? trail.states[0] : trail.states[1];
        trail.DCorLC.reverseLambda(stateToExtend);
    }
    else {
        stateToExtend = trail.states.back();
    }
    AffineSpaceOfStates as = trail.DCorLC.buildStateBase(stateToExtend, backwardExtension);
    vector<XoodooState> triangularBasis;
    vector<Coordinates> stability;
    upperTriangalizeBasis(trail.DCorLC.parent, as.originalGenerators, triangularBasis, stability);
    AffineSpaceIteratorContext context(trail, triangularBasis, as.offset, stability, backwardExtension);
    context.optimized = true;
    AffineSpaceUnitSet unitSet(context);
    AffineSpaceCostFunction cost;
    GenericTreeIterator<AffineSpaceBasisIndex, AffineSpaceUnitSet, AffineSpaceIteratorContext,
        AffineSpaceIteratorCache, AffineSpaceExtendedTrail, AffineSpaceCostFunction, GenericProgressDisplay>
        tree(unitSet, context, cost, maxWeightNext);
    progress.stack(synopsis);
    while(!tree.isEnd()) {
        const AffineSpaceExtendedTrail& core = (*tree);
        if (curNrRounds == nrRounds) {
            saveCoreCanonically(fout, core);
            if (core.totalWeight < minWeightFound) minWeightFound = core.totalWeight;
        }
        else {
            saveCoreCanonically(fout, core); // save even if not the desired length yet
            recurseExtendTrail(fout, core, backwardExtension, nrRounds, maxTotalWeight, minWeightFound, knownBounds, progress, verbose);
        }
        ++tree;
        ++progress;
    }
    progress.unstack();
    if (verbose) {
        cout << "--- leaving";
        cout << endl;
    }
}
