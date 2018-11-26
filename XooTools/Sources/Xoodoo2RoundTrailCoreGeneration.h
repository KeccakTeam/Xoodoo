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

#ifndef _XOODOO2ROUNDTRAILCOREGENERATION_H_
#define _XOODOO2ROUNDTRAILCOREGENERATION_H_

class ColoredBit {
public:
    typedef enum { Loop=0, Run=1, Orbital=2 } Color;
    Color color;
    int x, y, z;
    int rank, subrank;
    typedef enum { Before=-1, Both=0, After=1 } Side;
    Side side;
public:
    ColoredBit(int aColor, int aX, int aY, int aZ, int aRank, int aSubrank, int aSide)
        : color(Color(aColor)), x(aX), y(aY), z(aZ), rank(aRank), subrank(aSubrank), side(Side(aSide)) {}
    bool operator<(const ColoredBit& other) const
    {
        if (color < other.color)
            return true;
        else if (color == other.color) {
            if (x < other.x)
                return true;
            else if (x == other.x) {
                if (z < other.z)
                    return true;
                else if (z == other.z) {
                    if (rank < other.rank)
                        return true;
                    else if (rank == other.rank) {
                        if (subrank < other.subrank)
                            return true;
                        else if (subrank == other.subrank) {
                            if (y < other.y)
                                return true;
                            else if (y == other.y) {
                                if (side < other.side)
                                    return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
    bool operator==(const ColoredBit& other) const
    {
        return (color == other.color)
            && (x == other.x) && (y == other.y) && (z == other.z)
            && (rank == other.rank) && (subrank == other.subrank)
            && (side == other.side);
    }
    friend ostream& operator<<(ostream& a, const ColoredBit& bit)
    {
        a << "(" << dec << bit.x << ", " << bit.y << ", " << bit.z << " : ";
        if (bit.color == ColoredBit::Orbital)
            a << "Orb#" << bit.rank;
        else if (bit.color == ColoredBit::Loop)
            a << "Loop";
        else { /* if (bit.color == ColoredBit::Run) */
            if (bit.subrank < 0)
                a << "Aff#0/" << bit.subrank;
            else if (bit.subrank == 0)
                a << "Odd#" << bit.rank;
            else if (bit.subrank > 0)
                a << "Aff#" << bit.rank << "/" << bit.subrank;
            if (bit.side == ColoredBit::Before)
                a << " <<";
            else if (bit.side == ColoredBit::After)
                a << " >>";
        }
        a << ")";
        return a;
    }
};

class ExpandedColoredBit : public ColoredBit {
public:
    int Sx, Sy, Sz;
    int Ex, Ey, Ez;
    int Lx, Ly, Lz;
    ExpandedColoredBit(const ColoredBit& bit, const XoodooPropagation& DCorLC)
        : ColoredBit(bit)
    {
        computeEffectiveCoordinates(DCorLC);
    }
private:
    void computeEffectiveCoordinates(const XoodooPropagation& DCorLC)
    {
        Sx = x;
        Sy = y;
        Sz = z;
        const int t1 = DCorLC.parent.getParameters().t1;
        const int t2 = DCorLC.parent.getParameters().t2;
        const int t3 = DCorLC.parent.getParameters().t3;
        if (color == Run) {
            if (DCorLC.getPropagationType() == XoodooPropagation::DC) {
                if (subrank < 0) {
                    Sx += t3;
                    Sz += t1;
                }
                else if (subrank == 0) {
                    Sz += rank * (t2 - t1);
                }
                else /*if (subrank > 0)*/ {
                    Sx += t3;
                    Sz += rank * (t2 - t1) + t2;
                }
            }
            else {
                if (subrank < 0) {
                    Sx -= t3;
                    Sz -= t1;
                }
                else if (subrank == 0) {
                    Sz -= rank * (t2 - t1);
                }
                else /*if (subrank > 0)*/ {
                    Sx -= t3;
                    Sz -= rank * (t2 - t1) + t2;
                }
            }
            DCorLC.parent.reduceX(Sx);
            DCorLC.parent.reduceZ(Sz);
        }
        DCorLC.reverseEarlyRho(Sx, Sy, Sz, Ex, Ey, Ez);
        DCorLC.directLateRho(Sx, Sy, Sz, Lx, Ly, Lz);
    }
    friend ostream& operator<<(ostream& a, const ExpandedColoredBit& bit)
    {
        a << "(" << dec << bit.x << ", " << bit.y << ", " << bit.z << " : ";
        if (bit.color == ColoredBit::Orbital)
            a << "Orb#" << bit.rank;
        else if (bit.color == ColoredBit::Loop)
            a << "Loop";
        else { /* if (bit.color == ColoredBit::Run) */
            if (bit.subrank < 0)
                a << "Aff#0/" << bit.subrank;
            else if (bit.subrank == 0)
                a << "Odd#" << bit.rank;
            else if (bit.subrank > 0)
                a << "Aff#" << bit.rank << "/" << bit.subrank;
            if (bit.side == ColoredBit::Before)
                a << " <<";
            else if (bit.side == ColoredBit::After)
                a << " >>";
            a << " at [" << dec << bit.Sx << ", " << bit.Sy << ", " << bit.Sz << "]";
        }
        a << ")";
        return a;
    }
};

class ColoredBitSymmetryClass : public XoodooDCLC {
public:
    ColoredBitSymmetryClass() = default;
    ColoredBitSymmetryClass(unsigned int aSizeX, unsigned int aSizeZ, const XoodooParameters& aParameters)
        : XoodooDCLC(aSizeX, aSizeZ,  aParameters) {}
    void translateTo(const ColoredBit& origin, ColoredBit& bit) const
    {
        translateXZ(bit.x, bit.z, -origin.x, -origin.z);
    }
};

class CoreGenerationCache {
public:
    const XoodooPropagation& DCorLC;
    XoodooState stateEarly;
    int activeColumnsEarly;
    XoodooState stateLate;
    int activeColumnsLate;

    XoodooState stableBitsEarly, stableBitsLate;
    int stableActiveColumnsEarly, stableActiveColumnsLate;

    XoodooPlane columnsAffected;
    XoodooPlane columnsOddAndPreviousNeighbours;
    XoodooPlane columnsOddNextNeighbours;
    XoodooPlane columnsOddZero;
    XoodooPlane columnsOddNonZero;

    vector<ExpandedColoredBit> listOfOddZero;
    vector<ExpandedColoredBit> listOfAffectedZero;

    vector<bool> sheetTakenByLoop;

    int t1, t2, deltat;
public:
    CoreGenerationCache(const XoodooPropagation& aDCorLC)
        : DCorLC(aDCorLC),
            stateEarly(aDCorLC.parent), activeColumnsEarly(0),
            stateLate(aDCorLC.parent), activeColumnsLate(0),
            stableBitsEarly(aDCorLC.parent), stableBitsLate(aDCorLC.parent),
            stableActiveColumnsEarly(0), stableActiveColumnsLate(0),
            columnsAffected(aDCorLC.parent),
            columnsOddAndPreviousNeighbours(aDCorLC.parent),
            columnsOddNextNeighbours(aDCorLC.parent),
            columnsOddZero(aDCorLC.parent),
            columnsOddNonZero(aDCorLC.parent),
            sheetTakenByLoop(aDCorLC.parent.getSizeX(), false),
            t1(aDCorLC.parent.getParameters().t1),
            t2(aDCorLC.parent.getParameters().t2),
            deltat((DCorLC.getPropagationType() == XoodooPropagation::DC) ? t2-t1 : t1-t2)
    {
    }
protected:
    void setOrUnsetBit(bool set, bool stable, const ExpandedColoredBit& bit)
    {
        (void)set;
        if (bit.side != ColoredBit::After) {
            if (stateEarly.getColumn(bit.Ex, bit.Ez) == 0)
                activeColumnsEarly++;
            stateEarly.invertBit(bit.Ex, bit.Ey, bit.Ez);
            if (stateEarly.getColumn(bit.Ex, bit.Ez) == 0)
                activeColumnsEarly--;

            if (stable) {
                if (stableBitsEarly.getColumn(bit.Ex, bit.Ez) == 0)
                    stableActiveColumnsEarly++;
                stableBitsEarly.invertBit(bit.Ex, bit.Ey, bit.Ez);
                if (stableBitsEarly.getColumn(bit.Ex, bit.Ez) == 0)
                    stableActiveColumnsEarly--;
            }
        }
        if (bit.side != ColoredBit::Before) {
            if (stateLate.getColumn(bit.Lx, bit.Lz) == 0)
                activeColumnsLate++;
            stateLate.invertBit(bit.Lx, bit.Ly, bit.Lz);
            if (stateLate.getColumn(bit.Lx, bit.Lz) == 0)
                activeColumnsLate--;

            if (stable) {
                if (stableBitsLate.getColumn(bit.Lx, bit.Lz) == 0)
                    stableActiveColumnsLate++;
                stableBitsLate.invertBit(bit.Lx, bit.Ly, bit.Lz);
                if (stableBitsLate.getColumn(bit.Lx, bit.Lz) == 0)
                    stableActiveColumnsLate--;
            }
        }
    }
    bool isStable(const ColoredBit& bit) const
    {
        if ((bit.color == ColoredBit::Loop) || (bit.color == ColoredBit::Run))
            return bit.y != 0;
        else
            return true;
    }
public:
    void push(const ColoredBit& unit)
    {
        ExpandedColoredBit bit(unit, DCorLC);
        bool stable = isStable(bit);
        setOrUnsetBit(true, stable, bit);

        if ((bit.color == ColoredBit::Loop) && (bit.z == 0))
            sheetTakenByLoop[bit.x] = true;
        if ((bit.color == ColoredBit::Loop) || ((bit.color == ColoredBit::Run) && (bit.subrank == 0))) {
            if (bit.y == 0) {
                listOfOddZero.push_back(bit);
                columnsOddZero.setBitToOne(bit.Sx, bit.Sz);
            }
            else {
                columnsOddNonZero.setBitToOne(bit.Sx, bit.Sz);
            }
        }
        if ((bit.color == ColoredBit::Run) && (bit.subrank == 0)) {
            if (bit.rank == 0) {
                int Z = bit.Sz - deltat;
                DCorLC.parent.reduceZ(Z);
                columnsOddAndPreviousNeighbours.setBitToOne(bit.Sx, Z);
            }
            columnsOddAndPreviousNeighbours.setBitToOne(bit.Sx, bit.Sz);
            {
                int Z = bit.Sz + deltat;
                DCorLC.parent.reduceZ(Z);
                columnsOddNextNeighbours.setBitToOne(bit.Sx, Z);
            }
        }
        else if ((bit.color == ColoredBit::Run) && ((bit.subrank == -3) || (bit.subrank == 1))) {
            columnsAffected.setBitToOne(bit.Sx, bit.Sz);
            if (bit.y == 0)
                listOfAffectedZero.push_back(bit);
        }
    }
    void pop(const ColoredBit& unit)
    {
        ExpandedColoredBit bit(unit, DCorLC);
        bool stable = isStable(bit);
        setOrUnsetBit(false, stable, bit);

        if ((bit.color == ColoredBit::Loop) && (bit.z == 0))
            sheetTakenByLoop[bit.x] = false;
        if ((bit.color == ColoredBit::Loop) || ((bit.color == ColoredBit::Run) && (bit.subrank == 0))) {
            if (bit.y == 0) {
                listOfOddZero.pop_back();
                columnsOddZero.setBitToZero(bit.Sx, bit.Sz);
            }
            else {
                columnsOddNonZero.setBitToZero(bit.Sx, bit.Sz);
            }
        }
        if ((bit.color == ColoredBit::Run) && (bit.subrank == 0)) {
            if (bit.rank == 0) {
                int Z = bit.Sz - deltat;
                DCorLC.parent.reduceZ(Z);
                columnsOddAndPreviousNeighbours.setBitToZero(bit.Sx, Z);
            }
            columnsOddAndPreviousNeighbours.setBitToZero(bit.Sx, bit.Sz);
            {
                int Z = bit.Sz + deltat;
                DCorLC.parent.reduceZ(Z);
                columnsOddNextNeighbours.setBitToZero(bit.Sx, Z);
            }
        }
        else if ((bit.color == ColoredBit::Run) && ((bit.subrank == -3) || (bit.subrank == 1))) {
            columnsAffected.setBitToZero(bit.Sx, bit.Sz);
            if (bit.y == 0)
                listOfAffectedZero.pop_back();
        }
    }
};

class TwoRoundTrailCoreFromColoredBits : public Trail {
public:
    TwoRoundTrailCoreFromColoredBits(const XoodooPropagation& DCorLC)
        : Trail(DCorLC) {}
    void set(const UnitList<ColoredBit>& unitList, const CoreGenerationCache& cache)
    {
        (void)unitList;
        clear();
        setBeforeFirstStateWeight(DCorLC.getWeight(cache.stateEarly));
        append(cache.stateLate, DCorLC.getWeight(cache.stateLate));
    }
};

class ColoredBitSet {
protected:
    const ColoredBitSymmetryClass& instance;
    bool inKernel;
    bool outOfKernel;
    bool bareOnly;
public:
    ColoredBitSet(const ColoredBitSymmetryClass& anInstance, bool aInKernel = true, bool aOutOfKernel = true, bool aBareOnly = false)
        : instance(anInstance), inKernel(aInKernel), outOfKernel(aOutOfKernel), bareOnly(aBareOnly)
    {}
    ColoredBit getFirstChildUnit(const UnitList<ColoredBit>& unitList, const CoreGenerationCache& cache) const
    {
        (void)cache;
        if (unitList.size() == 0) {
            if (outOfKernel)
                return ColoredBit(ColoredBit::Loop, 0, 0, 0, 0, 0, ColoredBit::Both);
            else if (inKernel)
                return ColoredBit(ColoredBit::Orbital, 0, 0, 0, 0, 0, ColoredBit::Both);
            else
                throw EndOfSet();
        }
        else {
            const ColoredBit& parent = unitList.back();
            if (parent.color == ColoredBit::Loop) {
                ColoredBit current(parent);
                current.z++;
                current.y = 0;
                if (current.z == (int)instance.getSizeZ()) {
                    current.z = 0;
                    current.x++;
                }
                if (current.x < (int)instance.getSizeX())
                    return current;
                else
                    return ColoredBit(ColoredBit::Run, 0, 0, 0, 0, -3, ColoredBit::Before);
            }
            else if (parent.color == ColoredBit::Run) {
                if (parent.subrank == -3)
                    return ColoredBit(ColoredBit::Run, parent.x, 1, parent.z, 0, -2, ColoredBit::Before);
                else if (parent.subrank == -2) {
                    ColoredBit::Side side = ColoredBit::Side(unitList[unitList.size()-2].side * parent.side);
                    return ColoredBit(ColoredBit::Run, parent.x, 2, parent.z, 0, -1, side);
                }
                else if (parent.subrank == -1)
                    return ColoredBit(ColoredBit::Run, parent.x, 0, parent.z, 0, 0, ColoredBit::Both);
                else if (parent.subrank == 0)
                    return ColoredBit(ColoredBit::Run, parent.x, 0, parent.z, parent.rank, 1, ColoredBit::Before);
                else if (parent.subrank == 1)
                    return ColoredBit(ColoredBit::Run, parent.x, 1, parent.z, parent.rank, 2, ColoredBit::Before);
                else if (parent.subrank == 2) {
                    ColoredBit::Side side = ColoredBit::Side(unitList[unitList.size()-2].side * parent.side);
                    return ColoredBit(ColoredBit::Run, parent.x, 2, parent.z, parent.rank, 3, side);
                }
                else /*if (parent.subrank == 3)*/ {
                    ColoredBit current(ColoredBit::Run, parent.x, 0, parent.z+1, 0, -3, ColoredBit::Before);
                    if (current.z == (int)instance.getSizeZ()) {
                        current.z = 0;
                        current.x++;
                    }
                    if (current.x < (int)instance.getSizeX())
                        return current;
                    else if (bareOnly)
                        throw EndOfSet();
                    else
                        return ColoredBit(ColoredBit::Orbital, 0, 0, 0, 0, 0, ColoredBit::Both);
                }
            }
            else /*if (parent.color == ColoredBit::Orbital)*/ {
                ColoredBit current(parent);
                if (current.rank == 0) {
                    current.y++;
                    current.rank++;
                    if (current.y == 3)
                        throw EndOfSet();
                }
                else /*if (current.rank == 1)*/ {
                    current.rank = 0;
                    current.y = 0;
                    current.z++;
                    if (current.z == (int)instance.getSizeZ()) {
                        current.z = 0;
                        current.x++;
                    }
                    if (current.x == (int)instance.getSizeX())
                        throw EndOfSet();
                }
                return current;
            }
        }
    }
    void iterateUnit(const UnitList<ColoredBit>& unitList, ColoredBit& current, const CoreGenerationCache& cache) const
    {
        (void)unitList;
        (void)current;
        (void)cache;
        if (current.color == ColoredBit::Loop) {
            current.y++;
            if (current.y == 3) {
                if (current.z == 0) {
                    current.y = 0;
                    current.x++;
                    if (current.x == (int)instance.getSizeX())
                        current = ColoredBit(ColoredBit::Run, 0, 0, 0, 0, -3, ColoredBit::Before);
                }
                else
                    throw EndOfSet();
            }
        }
        else if (current.color == ColoredBit::Run) {
            if (current.subrank == -3) {
                if (current.side == ColoredBit::Before)
                    current.side = ColoredBit::After;
                else {
                    current.z++;
                    if (current.z == (int)instance.getSizeZ()) {
                        current.z = 0;
                        current.x++;
                    }
                    if (current.x == (int)instance.getSizeX()) {
                        if (((unitList.size() == 0) && (inKernel)) || ((unitList.size() > 0) && (!bareOnly)))
                            current = ColoredBit(ColoredBit::Orbital, 0, 0, 0, 0, 0, ColoredBit::Both);
                        else
                            throw EndOfSet();
                    }
                }
            }
            else if (current.subrank == -2) {
                if (current.side == ColoredBit::Before)
                    current.side = ColoredBit::After;
                else
                    throw EndOfSet();
            }
            else if (current.subrank == -1)
                throw EndOfSet();
            else if (current.subrank == 0) {
                current.y++;
                if (current.y == 3)
                    throw EndOfSet();
            }
            else if (current.subrank == 1) {
                if (current.side == ColoredBit::Before)
                    current.side = ColoredBit::After;
                else {
                    current.side = ColoredBit::Both;
                    current.y = 0;
                    current.subrank = 0;
                    current.rank++;
                }
            }
            else if (current.subrank == 2) {
                if (current.side == ColoredBit::Before)
                    current.side = ColoredBit::After;
                else
                    throw EndOfSet();
            }
            else /*if (current.subrank == 3)*/ {
                throw EndOfSet();
            }
        }
        else /*if (current.color == ColoredBit::Orbital)*/ {
            if (current.rank == 0) {
                current.y++;
                if (current.y == 2) {
                    current.y = 0;
                    current.z++;
                }
                if (current.z == (int)instance.getSizeZ()) {
                    current.z = 0;
                    current.x++;
                }
                if (current.x == (int)instance.getSizeX())
                    throw EndOfSet();
            }
            else /*if (current.rank == 1)*/ {
                current.y++;
                if (current.y == 3)
                    throw EndOfSet();
            }
        }
    }
    bool isSubtreeWellFormed(const UnitList<ColoredBit>& parentUnitList, const ColoredBit& newBit, const CoreGenerationCache& cache) const
    {
        (void)parentUnitList;
        (void)newBit;
        (void)cache;
        if ((newBit.color == ColoredBit::Run) && (newBit.subrank == -3)) {
            ExpandedColoredBit bit(newBit, cache.DCorLC);
            return (cache.sheetTakenByLoop[bit.x] == false)
                && (cache.columnsOddAndPreviousNeighbours.getBit(bit.x, bit.z) == 0)
                && (cache.columnsOddNextNeighbours.getBit(bit.x, bit.z) == 0)
                && (cache.columnsAffected.getBit(bit.Sx, bit.Sz) == 0)
                && (cache.columnsOddNonZero.getBit(bit.Sx, bit.Sz) == 0);
        }
        else if ((newBit.color == ColoredBit::Run) && (newBit.subrank == 1)) {
            ExpandedColoredBit bit(newBit, cache.DCorLC);
            return (cache.columnsAffected.getBit(bit.Sx, bit.Sz) == 0) && (cache.columnsOddNonZero.getBit(bit.Sx, bit.Sz) == 0);
        }
        else if ((newBit.color == ColoredBit::Run) && (newBit.subrank == 0)) {
            ExpandedColoredBit bit(newBit, cache.DCorLC);
            return (cache.columnsOddAndPreviousNeighbours.getBit(bit.Sx, bit.Sz) == 0) && ((newBit.y == 0) || (cache.columnsAffected.getBit(bit.Sx, bit.Sz) == 0));
        }
        else if ((newBit.color == ColoredBit::Orbital) && (newBit.rank == 0)) {
            ExpandedColoredBit bit(newBit, cache.DCorLC);
            return (cache.columnsAffected.getBit(bit.Sx, bit.Sz) == 0)
                && (cache.columnsOddNonZero.getBit(bit.Sx, bit.Sz) == 0)
                && ((bit.y > 0) || (cache.columnsOddZero.getBit(bit.Sx, bit.Sz) == 0));
        }
        else
            return true;
    }
    bool isNodeWellFormed(const UnitList<ColoredBit>& unitList, const CoreGenerationCache& cache) const
    {
        (void)unitList;
        (void)cache;
        if (unitList.empty())
            return false;
        else {
            const ColoredBit& top = unitList.back();
            if (top.color == ColoredBit::Loop)
                return top.z == (int)instance.getSizeZ()-1;
            else if (top.color == ColoredBit::Run)
                return top.subrank == 3;
            else /*if (top.color == ColoredBit::Orbital)*/
                return top.rank == 1;
        }
    }
    bool isSubtreeCanonical(const UnitList<ColoredBit>& parentUnitList, const ColoredBit& newBit, const CoreGenerationCache& cache) const
    {
        (void)cache;
        {
            UnitList<ColoredBit> unitList(parentUnitList);
            unitList.push_back(newBit);
            return isCanonical(instance, unitList);
        }
    }
    bool isNodeCanonical(const UnitList<ColoredBit>& unitList, const CoreGenerationCache& cache) const
    {
        (void)cache;
        (void)unitList;
        return true;
    }
};

class CoreGenerationCostFunction {
public:
    int factorEarly, factorLate;
    CoreGenerationCostFunction(int early, int late) : factorEarly(early), factorLate(late) {}
    unsigned int getNodeCost(const UnitList<ColoredBit>& unitList, const CoreGenerationCache& cache) const
    {
        (void)unitList;
        return factorEarly*cache.activeColumnsEarly*2 + factorLate*cache.activeColumnsLate*2;
    }
    unsigned int getSubtreeLowerBound(const UnitList<ColoredBit>& parentUnitList, const ColoredBit& newBit, const CoreGenerationCache& cache) const
    {
        (void)parentUnitList;
        if (newBit.color == ColoredBit::Loop) {
            XoodooState stableBitsEarly(cache.stableBitsEarly);
            XoodooState stableBitsLate(cache.stableBitsLate);
            int contributionOfUnstableBits = 0;
            for(auto bit : cache.listOfOddZero)
                contributionOfUnstableBits += getContributionOfUnstableBit(bit, stableBitsEarly, stableBitsLate);
            for(auto bit : cache.listOfAffectedZero)
                contributionOfUnstableBits += getContributionOfUnstableBit(bit, stableBitsEarly, stableBitsLate);
            {
                ExpandedColoredBit bit(newBit, cache.DCorLC);
                contributionOfUnstableBits += getContributionOfUnstableBit(bit, stableBitsEarly, stableBitsLate);
            }
            for(int z=newBit.z+1; z<(int)cache.DCorLC.parent.getSizeZ(); z++)
                contributionOfUnstableBits +=
                    getContributionOfFutureLoopBit(newBit.x, z,
                        stableBitsEarly, stableBitsLate, cache);
            return contributionOfUnstableBits + factorEarly*cache.stableActiveColumnsEarly*2 + factorLate*cache.stableActiveColumnsLate*2;
        }
        else if (newBit.color == ColoredBit::Run) {
            XoodooState stableBitsEarly(cache.stableBitsEarly);
            XoodooState stableBitsLate(cache.stableBitsLate);
            int contributionOfUnstableBits = 0;
            for(auto bit : cache.listOfOddZero)
                contributionOfUnstableBits += getContributionOfUnstableBit(bit, stableBitsEarly, stableBitsLate);
            for(auto bit : cache.listOfAffectedZero)
                contributionOfUnstableBits += getContributionOfUnstableBit(bit, stableBitsEarly, stableBitsLate);
            {
                ExpandedColoredBit bit(newBit, cache.DCorLC);
                contributionOfUnstableBits += getContributionOfUnstableBit(bit, stableBitsEarly, stableBitsLate);
            }
            return contributionOfUnstableBits + factorEarly*cache.stableActiveColumnsEarly*2 + factorLate*cache.stableActiveColumnsLate*2;
        }
        else /*if (newBit.color == ColoredBit::Orbital)*/ {
            unsigned int costNewBit = 0;
            if (factorEarly > 0) {
                int Ex, Ey, Ez;
                cache.DCorLC.reverseEarlyRho(newBit.x, newBit.y, newBit.z, Ex, Ey, Ez);
                if (cache.stateEarly.getColumn(Ex, Ez) == 0)
                    costNewBit += factorEarly*2;
            }
            if (factorLate > 0) {
                int Lx, Ly, Lz;
                cache.DCorLC.directLateRho(newBit.x, newBit.y, newBit.z, Lx, Ly, Lz);
                if (cache.stateLate.getColumn(Lx, Lz) == 0)
                    costNewBit += factorLate*2;
            }
            return costNewBit + factorEarly*cache.activeColumnsEarly*2 + factorLate*cache.activeColumnsLate*2;
        }
    }
private:
    int getContributionOfUnstableBit(const ExpandedColoredBit& bit, XoodooState& stableBitsEarly, XoodooState& stableBitsLate) const
    {
        int early = (stableBitsEarly.getColumn(bit.Ex, bit.Ez) == 0) ? 2 : 0;
        int late = (stableBitsLate.getColumn(bit.Lx, bit.Lz) == 0) ? 2 : 0;
        stableBitsEarly.setBitToOne(bit.Ex, bit.Ey, bit.Ez);
        stableBitsLate.setBitToOne(bit.Lx, bit.Ly, bit.Lz);
        return min(factorEarly*early, factorLate*late);
    }
    int getContributionOfFutureLoopBit(int x, int z, XoodooState& stableBitsEarly, XoodooState& stableBitsLate, const CoreGenerationCache& cache) const
    {
        int minCost = 0;
        vector<ExpandedColoredBit> bitY;
        for(int y=0; y<3; y++)
            bitY.push_back(
                ExpandedColoredBit(
                    ColoredBit(ColoredBit::Loop, x, y, z, 0, 0, ColoredBit::Both), cache.DCorLC));
        for(int y=0; y<3; y++) { // Unaffected odd cases
            const ExpandedColoredBit& bit = bitY[y];
            int early = (stableBitsEarly.getColumn(bit.Ex, bit.Ez) == 0) ? 2 : 0;
            int late = (stableBitsLate.getColumn(bit.Lx, bit.Lz) == 0) ? 2 : 0;
            int cost = factorEarly*early + factorLate*late;
            if ((y == 0) || (cost < minCost))
                minCost = cost;
        }
        for(int y=0; y<3; y++) { // Affected odd cases with one early active bit, two late
            int early = (stableBitsEarly.getColumn(bitY[y].Ex, bitY[y].Ez) == 0) ? 2 : 0;
            int late = (stableBitsLate.getColumn(bitY[(y+1)%3].Lx, bitY[(y+1)%3].Lz) == 0) ? 2 : 0;
            late += (stableBitsLate.getColumn(bitY[(y+2)%3].Lx, bitY[(y+2)%3].Lz) == 0) ? 2 : 0;
            int cost = factorEarly*early + factorLate*late;
            if (cost < minCost) minCost = cost;
        }
        { // Affected odd cases with three active bits early, zero late
            int early = (stableBitsEarly.getColumn(bitY[0].Ex, bitY[0].Ez) == 0) ? 2 : 0;
            early += (stableBitsEarly.getColumn(bitY[1].Ex, bitY[1].Ez) == 0) ? 2 : 0;
            early += (stableBitsEarly.getColumn(bitY[2].Ex, bitY[2].Ez) == 0) ? 2 : 0;
            int cost = factorEarly*early;
            if (cost < minCost) minCost = cost;
        }
        for(int y=0; y<3; y++) {
            const ExpandedColoredBit& bit = bitY[y];
            stableBitsEarly.setBitToOne(bit.Ex, bit.Ey, bit.Ez);
            stableBitsLate.setBitToOne(bit.Lx, bit.Ly, bit.Lz);
        }
        return minCost;
    }
};

#endif
