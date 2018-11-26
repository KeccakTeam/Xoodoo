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

#ifndef _XOODOODCLC_H_
#define _XOODOODCLC_H_

#include "Xoodoo.h"

class XoodooPlane;

/** This class is an extension of Xoodoo with additional functionality
  * aimed at differential and linear cryptanalysis.
  */
class XoodooDCLC : public Xoodoo {
public:
    /** In this context, λ represents the linear operations in Xoodoo
      * between two applications of χ.
      * The λ mode indicates whether to perform these operations, their transpose,
      * their inverse or the transpose of their inverse.
      * - Straight: λ(a) means ρWest(θ(ρEast(a)));
      * - Inverse: λ(a) means ρEast<sup>-1</sup>(θ<sup>-1</sup>(ρEast<sup>-1</sup>(a)));
      * - Transpose: λ(a) means ρEast<sup>-1</sup>(θ<sup>T</sup>(ρEast<sup>-1</sup>(a)));
      * - Dual: λ(a) means ρWest(θ<sup>-1T</sup>(ρEast(a))).
      */
    enum LambdaMode {
        Straight = 0,
        Inverse,
        Transpose,
        Dual,
        EndOfLambdaModes
    };
    XoodooDCLC() = default;
    /** This constructor has the same parameters as Xoodoo::Xoodoo.
      */
    XoodooDCLC(unsigned int aSizeX, unsigned int aSizeZ, const XoodooParameters& aParameters);
    /** This method retuns a string describing the instance. */
    string getDescription() const;
    /** Apply the χ function on a single column value.
      * @param   a      The input column value.
      * @return The output column value.
      */
    ColumnValue chiOnColumn(ColumnValue a) const;
    /** Apply the χ<sup>-1</sup> function on a single column value.
      * @param   a      The input column value.
      * @return The output column value.
      */
    ColumnValue inverseChiOnColumn(ColumnValue a) const;
    /** This method applies the λ function (see LambdaMode).
      * The mode argument gives the λ mode (i.e., inverse/transpose) to use.
      * @param   state  The state to apply λ to.
      * @param   mode   The λ mode.
      */
    void lambda(XoodooState& state, LambdaMode mode) const;
    void lambdaBeforeTheta(XoodooState& state, LambdaMode mode) const;
    void lambdaTheta(XoodooState& state, LambdaMode mode) const;
    void lambdaAfterTheta(XoodooState& state, LambdaMode mode) const;
    void lambdaThetaAndAfter(XoodooState& state, LambdaMode mode) const;
    /** Get the parity of a state before or after θ.
     */
    void getParity(const XoodooState& state, XoodooPlane& parity) const;
};

class XoodooPlane : public XoodooLanes {
public:
    const Xoodoo& instance;
    const unsigned int sizeZ;
public:
    XoodooPlane(const Xoodoo& anInstance);
    XoodooPlane& operator=(const XoodooPlane& other);
    inline LaneValue getLane(unsigned int x) const
    {
        return lanes[x];
    }
    /** This method returns the value of a given bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  z   The z coordinate.
      * @return The bit value.
      */
    inline int getBit(unsigned int x, unsigned int z) const
    {
        return (lanes[x] >> z) & 1;
    }
    /** This method sets to 0 a particular bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  z   The z coordinate.
      */
    inline void setBitToZero(unsigned int x, unsigned int z)
    {
        lanes[x] &= ~((LaneValue)1 << z);
    }
    /** This method sets to 1 a particular bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  z   The z coordinate.
      */
    inline void setBitToOne(unsigned int x, unsigned int z)
    {
        lanes[x] |= ((LaneValue)1 << z);
    }
    /** This method inverts a particular bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  z   The z coordinate.
      */
    inline void invertBit(unsigned int x, unsigned int z)
    {
        lanes[x] ^= ((LaneValue)1 << z);
    }
    string getDisplayString(unsigned int x) const;
    void display(ostream& fout) const;
    friend ostream& operator<<(ostream& a, const XoodooPlane& state);
};

#endif
