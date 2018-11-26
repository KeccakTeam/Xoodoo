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

#ifndef _TREE_
#define _TREE_

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stack>
#include <vector>

using namespace std;

template<class Unit>
class UnitList : public std::vector<Unit> {};

template<class Unit>
ostream& operator<<(ostream& a, const UnitList<Unit>& vec)
{
    for(typename UnitList<Unit>::const_iterator i=vec.begin(); i != vec.end(); ++i)
        a << (*i) << " ";
    a << endl;
    return a;
}

/**
* This exception is launched when a set of units reaches the end.
*/
class EndOfSet {};

class GenericTreeIteratorStatistics {
public:
    uint64_t subtreesConsidered;
    uint64_t subtreesNotWellFormed;
    uint64_t subtreesTooCostly;
    uint64_t subtreesNotCanonical;
    uint64_t nodesConsidered;
    uint64_t nodesNotWellFormed;
    uint64_t nodesTooCostly;
    uint64_t nodesNotCanonical;
    uint64_t nodesOutput;
public:
    GenericTreeIteratorStatistics()
        : subtreesConsidered(0), subtreesNotWellFormed(0), subtreesTooCostly(0), subtreesNotCanonical(0),
            nodesConsidered(0), nodesNotWellFormed(0), nodesTooCostly(0), nodesNotCanonical(0),
            nodesOutput(0) {}
    friend ostream& operator<<(ostream& a, const GenericTreeIteratorStatistics& s);
};

class EmptyProgressDisplay {
public:
    EmptyProgressDisplay() {}
    template<class Unit, class CachedRepresentation>
    void subtreeConsidered(const UnitList<Unit>& parentUnitList, const Unit& newUnit, const CachedRepresentation& cache, const GenericTreeIteratorStatistics& stats)
    {
        (void)parentUnitList;
        (void)newUnit;
        (void)cache;
        (void)stats;
    }
};

/**
* Generic iterator to traverse a tree.
*
* From the class UnitSet, this class expects the following methods:
*
*   - Unit UnitSet::getFirstChildUnit(const UnitList<Unit>& unitList, CachedRepresentation& cache) const
*       This methods must return the first unit that comes after unitList.back() according to the ordering. If no such unit exists, it shall throw an EndOfSet exception.
*
*   - void UnitSet::iterateUnit(const UnitList<Unit>& unitList, Unit& current, CachedRepresentation& cache) const
*       This method must return in @a current the next value of the unit according to the ordering. If no such unit exists, it shall throw an EndOfSet exception.
*
*   - bool UnitSet::isSubtreeWellFormed(const UnitList<Unit>& parentUnitList, const Unit& newUnit, const CachedRepresentation& cache) const
*       This method must return false if the subtree defined by (@a parentUnitList | @a newUnit) and all its descendants is not well formed and should be excluded from the search. Otherwise, it must return true.
*
*   -  bool UnitSet::isNodeWellFormed(const UnitList<Unit>& unitList, const CachedRepresentation& cache) const
*       This method must return true if and only if the node defined by @a unitList is well-formed and may be output by the iterator. This method will not be called, and may return true even if the node is not well formed, if isSubtreeWellFormed() already returned false on the same node.
*
*   - bool UnitSet::isSubtreeCanonical(const UnitList<Unit>& parentUnitList, const Unit& newUnit, const CachedRepresentation& cache) const
*       This method must return false if the subtree defined by (@a parentUnitList | @a newUnit) and all its descendants is not canonical and should be excluded from the search. Otherwise, it must return true.
*
*   -  bool UnitSet::isNodeCanonical(const UnitList<Unit>& unitList, const CachedRepresentation& cache) const
*       This method must return true if and only if the node defined by @a unitList is canonical and may be output by the iterator.  This method will not be called, and may return true even if the node is not canonical, if isSubtreeCanonical() already returned false on the same node.
*
*
* From the class CachedRepresentation, this class expects the following methods:
*
*   - void CachedRepresentation::push(const Unit& newUnit)
*       This method must synchronize the cached representation of the current node due to the addition of the new unit given as parameter.
*
*   - void CachedRepresentation::pop(const Unit& lastUnit)
*       This method must synchronize the cached representation of the current node due to the removal of the last unit given as parameter.
*
*
* From the class OutputRepresentation, this class expects the following method:
*
*   - void OutputRepresentation::set(const UnitList<Unit>& unitList, const CachedRepresentation& cache)
*       This method must set its attributes to represent the node given as unit list, possibly using its cached representation.
*
*
* From the class CostFunction, this class expects the following method:
*
*   - unsigned int CostFunction::getSubtreeLowerBound(const UnitList<Unit>& parentUnitList, const Unit& newUnit, CachedRepresentation& cache) const
*       This method must return a lower bound of the cost function on the subtree defined by (@a parentUnitList | @a newUnit) and all its descendants. If the cost is above the maximum cost, the entire subtree will be excluded from the search.
*
*   - unsigned int CostFunction::getNodeCost(const UnitList<Unit>& unitList, CachedRepresentation& cache) const
*       This method must return the cost of the node given as unit list, possibly using its cached representation. If the cost is above the maximum cost, this node will not be output by the iterator.
*/
template<class Unit, class UnitSet, class Context, class CachedRepresentation, class OutputRepresentation, class CostFunction, class ProgressDisplay = EmptyProgressDisplay>
class GenericTreeIterator {
public:
    /** Statistics on the search. */
    GenericTreeIteratorStatistics statistics;
    /** The progress display. */
    ProgressDisplay progressDisplay;
protected:
    /** The set of units */
    const UnitSet& unitSet;
    /** The unit-list representing the current node. */
    UnitList<Unit> unitList;
    /** The cache representation of the current node. */
    CachedRepresentation cache;
    /** The output representation of the current node. */
    OutputRepresentation out;
    /** The cost function. */
    const CostFunction& costFunction;
    /** The maximum cost allowed when traversing the tree. */
    unsigned int maxCost;

    /** Attribute that indicates whether the iterator has reached the end. */
    bool end;
    /** Attribute that indicates whether the iterator has been initialized. */
    bool initialized;
    /** Attribute that indicates whether the tree is empty. */
    bool empty;
    /** Number of the current iteration. */
    uint64_t index;

public:

    /** The constructor.
    * @param  aUnitSet      The set of units.
    * @param  aContext      The context needed to initialize the cache and output representations.
    * @param  aCostFunction The cost function.
    * @param  aMaxCost      The maximum cost.
    */
    GenericTreeIterator(const UnitSet& aUnitSet, const Context& aContext, const CostFunction& aCostFunction, unsigned int aMaxCost)
        : unitSet(aUnitSet), cache(aContext), out(aContext), costFunction(aCostFunction), maxCost(aMaxCost)
    {
        empty = true;
        end = false;
        initialized = false;
        index = 0;
    }

    /** This method indicates whether the iterator has reached the end of the tree.
    *  @return True if there are no more nodes to consider.
    */
    bool isEnd()
    {
        if (!initialized) iteratorInitialize();
        return end;
    }

    /** This method indicates whether the tree is empty.
    *  @return True if the tree is empty.
    */
    bool isEmpty()
    {
        if (!initialized) iteratorInitialize();
        return empty;
    }

    /** This method returns the index of the current node considered.
    * @return  The current node index.
    */
    uint64_t getIndex() const
    {
        return index;
    }

    /** The '++' operator moves the iterator to the next node in the tree.
    */
    void operator++()
    {
        if (!initialized) {
            iteratorInitialize();
        }
        else {
            if (!end) {
                ++index;
                if (!iteratorNext())
                    end = true;
            }
        }
    }

    /** The '*' operator gives a constant reference to the current node.
    *  @return A constant reference to the current node.
    */
    const OutputRepresentation& operator*()
    {
        out.set(unitList, cache);
        return out;
    }

    const UnitList<Unit>& getCurrentUnitList() const
    {
        return unitList;
    }

private:

    /** Method to initialize the iterator. It goes to the first acceptable node,
     * or sets end = empty = true if none exists.
     * The iterator uses the tree traversal on connected nodes, then filters out the nodes that are not acceptable.
     */
    void iteratorInitialize()
    {
        index = 0;
        initialized = true;
        if (treeInitialize()) {
            while(!canAcceptNode()) {
                if (!treeNext()) {
                    end = empty = true;
                    return;
                }
            }
            end = empty = false;
        }
        else {
            end = empty = true;
        }
    }

    /** Method to go to the next acceptable node, or returns false if none exists.
     * The iterator uses the tree traversal on connected nodes, then filters out the nodes that are not acceptable.
     */
    bool iteratorNext()
    {
        do {
            if (!treeNext()) {
                end = true;
                return false;
            }
        } while(!canAcceptNode());
        return true;
    }

    /** Method to determine whether the current node is acceptable.
     * It first tests if the node is well-formed (using the unit set).
     * Then, it checks if the cost is not above the maximum (using the cost function).
     * Finally, it checks if the node is canonical (using the unit set).
     */
    bool canAcceptNode()
    {
        statistics.nodesConsidered++;
        if (!unitSet.isNodeWellFormed(unitList, cache)) {
            statistics.nodesNotWellFormed++;
            return false;
        }
        if (costFunction.getNodeCost(unitList, cache) > maxCost) {
            statistics.nodesTooCostly++;
            return false;
        }
        if (!unitSet.isNodeCanonical(unitList, cache)) {
            statistics.nodesNotCanonical++;
            return false;
        }
        statistics.nodesOutput++;
        return true;
    }

    /** This method initializes the tree traversal.
     * @return true if a node exists, false otherwise.
    */
    bool treeInitialize()
    {
        return true;
    }

    /** This method goes to the next node in the tree traversal.
    * @return true if the next node exists, false otherwise.
    */
    bool treeNext()
    {
        if (toChild())
            return true;
        do{
            if (toSibling())
                return true;
            if (unitList.empty())
                return false;
        } while (true);
    }

    /** This method moves to the first child of the current node.
    * A child is obtained adding a new unit to the current node.
    * @return true if a child is found, false otherwise.
    */
    bool toChild()
    {
        try {
            Unit newUnit = unitSet.getFirstChildUnit(unitList, cache);
            while(!canEnterSubtree(newUnit)) {
                unitSet.iterateUnit(unitList, newUnit,cache);
            }
            push(newUnit);
            return true;
        }
        catch (EndOfSet) {
            return false;
        }
    }

    /** This method moves to the next sibling of the current node.
    * Siblings are obtained iterating the higher unit of the current node.
    * @return true if a sibling is found, false otherwise, in which case, it goes to the parent.
    */
    bool toSibling()
    {
        try {
            if (unitList.empty())
                return false;
            else {
                Unit lastUnit = unitList.back();
                pop();
                do {
                        unitSet.iterateUnit(unitList, lastUnit,cache);
                } while(!canEnterSubtree(lastUnit));
                push(lastUnit);
                return true;
            }
        }
        catch (EndOfSet) {
            return false;
        }
    }

    /**
    * This method pushes a new unit to the unit list and updates the cached representation.
    * @param newUnit the unit to be pushed.
    */
    void push(const Unit& newUnit)
    {
        unitList.push_back(newUnit);
        cache.push(newUnit);
    }

    /**
    * This method pops the highest unit from the unit list and updates the cached representation.
    */
    void pop()
    {
        cache.pop(unitList.back());
        unitList.pop_back();
    }

    /** This method checks if the subtree formed by the addition has to be traversed.
     * It first tests if the nodes in the subtree are well-formed (using the unit set).
     * Then, it checks if the lower bound on the cost is not above the maximum (using the cost function).
     * Finally, it checks if the nodes in the subtree are canonical (using the unit set).
    */
    bool canEnterSubtree(const Unit& newUnit)
    {
        progressDisplay.subtreeConsidered(unitList, newUnit, cache, statistics);
        statistics.subtreesConsidered++;
        if (!unitSet.isSubtreeWellFormed(unitList, newUnit, cache)) {
            statistics.subtreesNotWellFormed++;
            return false;
        }
        if (costFunction.getSubtreeLowerBound(unitList, newUnit, cache) > maxCost) {
            statistics.subtreesTooCostly++;
            return false;
        }
        if (!unitSet.isSubtreeCanonical(unitList, newUnit, cache)) {
            statistics.subtreesNotCanonical++;
            return false;
        }
        return true;
    }

};

template<class T>
bool operator<(const UnitList<T>& first, const UnitList<T>& second)
{
    for(unsigned int i=0; (i<first.size()) && (i<second.size()); i++) {
        if (first[i] < second[i])
            return true;
        else if (second[i] < first[i])
            return false;
    }
    return false;
}

template<class SymmetryClass, class Unit>
bool isCanonical(const SymmetryClass& symmetryClass, const UnitList<Unit>& unitList)
{
    for(typename UnitList<Unit>::const_iterator i=unitList.begin(); i != unitList.end(); ++i) {
        UnitList<Unit> translated(unitList);
        for(typename UnitList<Unit>::iterator j=translated.begin(); j != translated.end(); ++j)
            symmetryClass.translateTo((*i), (*j));
        std::sort(translated.begin(), translated.end());
        if (translated < unitList)
            return false;
    }
    return true;
}

template<class SymmetryClass, class Unit>
void makeCanonical(const SymmetryClass& symmetryClass, UnitList<Unit>& unitList)
{
    UnitList<Unit> best(unitList);
    for(typename UnitList<Unit>::const_iterator i=unitList.begin(); i != unitList.end(); ++i) {
        UnitList<Unit> translated(unitList);
        for(typename UnitList<Unit>::iterator j=translated.begin(); j != translated.end(); ++j)
            symmetryClass.translateTo((*i), (*j));
        std::sort(translated.begin(), translated.end());
        if (translated < best)
            best = translated;
    }
    unitList = best;
}

#endif
