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

#ifndef _XOODOO_H_
#define _XOODOO_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "transformations.h"

using namespace std;

typedef uint64_t LaneValue;

class XoodooState;

class XoodooParameters {
public:
    /** The parameter e0. */
    int e0;
    /** The parameter e1. */
    int e1;
    /** The parameter t1. */
    int t1;
    /** The parameter t2. */
    int t2;
    /** The parameter t3. */
    int t3;
    /** The parameter w1. */
    int w1;
public:
    XoodooParameters(int aE0, int aE1, int aT1, int aT2, int aW1)
        : e0(aE0), e1(aE1), t1(aT1), t2(aT2), t3(1), w1(aW1) {}
};

/**
  * Class implementing the Xoodoo permutation, as well as its inverse.
  */
class Xoodoo : public Permutation {
protected:
    /** The size along x. */
    unsigned int sizeX;
public:
    /** The size along y (fixed to 3). */
    static const unsigned int sizeY = 3;
protected:
    /** The size along z. */
    unsigned int sizeZ;
    /** The other perameters. */
    XoodooParameters p;
    /** The order of Theta. */
	unsigned int thetaOrder;
public:
    /**
      * The constructor.
      * Xoodoo is Xoodoo[4×3×32, e0=2, e1=8, t1=5, t2=14, t3=1, w1=11].
      */
    Xoodoo();
    /**
      * The constructor. The sizes and rotation constants are given as parameters.
      */
    Xoodoo(unsigned int aSizeX, unsigned int aSizeZ, const XoodooParameters& aParameters);
    /**
      * Method that returns the number of bits of its domain and range.
      */
    unsigned int getWidth() const;
    inline unsigned int getSizeX() const { return sizeX; }
	inline unsigned int getSizeZ() const { return sizeZ; }
	inline unsigned int getThetaOrder() const { return thetaOrder; }
	inline const XoodooParameters& getParameters() const { return p; }
    /**
      * Method that applies a Xoodoo round onto the parameter
      * @a state.
      */
    void round(XoodooState& state, LaneValue rc) const;
    /**
      * Method that applies the Xoodoo permutation onto the parameter
      * @a state.
      */
    void permute(XoodooState& state, unsigned int numberOfRounds) const;
    /**
      * Method that applies the Xoodoo permutation onto the parameter
      * @a state.
      */
    void operator()(uint8_t * state) const;
    /**
      * Method that applies the inverse of the Xoodoo permutation onto
      * the parameter @a state.
      */
    void inverse(uint8_t * state) const;
    /**
      * Method that returns a string describing the instance of the Xoodoo
      * permutation.
      */
    string getDescription() const;
    /**
      * Method that returns a short string that uniquely identifies the
      * Xoodoo instance.
      */
    virtual string getName() const;
    /**
      * Method that builds a file name by prepending a prefix and appending
      * a suffix to getName().
      */
    string buildFileName(const string& prefix, const string& suffix) const;
    /**
      * Method to translate (rotate) the lane value by a given amount.
      *
      * @param[in]  a   The lane value to translate.
      * @param[in]  dz  The amount of translation.
      * @return The translate lane value.
      */
    inline LaneValue translateZ(const LaneValue& a, int dz) const
    {
        reduceZ(dz);
        if (dz == 0)
            return a;
        else
            return ((a << dz) | (a >> (sizeZ-dz))) & (((LaneValue)1 << sizeZ) - 1);
    }
    /**
      * Method that applies the ρEast to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
    void rhoEast(XoodooState& state) const;
    /**
      * Method that applies the ρEast coordinate transformation to a bit position (x,y,z).
      *
      * @param[in]  x   The input coordinate x.
      * @param[in]  y   The input coordinate y.
      * @param[in]  z   The input coordinate z.
      * @param[out] X   The output coordinate x.
      * @param[out] Y   The output coordinate y.
      * @param[out] Z   The output coordinate z.
      */
    void rhoEast(int x, int y, int z, int& X, int& Y, int& Z) const;
    /**
      * Method that applies the inverse of ρEast to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
    void inverseRhoEast(XoodooState& state) const;
    /**
      * Method that applies the inverse of the ρEast coordinate transformation to a bit position (X,Y,Z).
      *
      * @param[in]  X   The input coordinate x.
      * @param[in]  Y   The input coordinate y.
      * @param[in]  Z   The input coordinate z.
      * @param[out] x   The output coordinate x.
      * @param[out] y   The output coordinate y.
      * @param[out] z   The output coordinate z.
      */
    void inverseRhoEast(int X, int Y, int Z, int& x, int& y, int& z) const;
    /**
      * Method that applies the ρWest to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
    void rhoWest(XoodooState& state) const;
    /**
      * Method that applies the ρWest coordinate transformation to a bit position (x,y,z).
      *
      * @param[in]  x   The input coordinate x.
      * @param[in]  y   The input coordinate y.
      * @param[in]  z   The input coordinate z.
      * @param[out] X   The output coordinate x.
      * @param[out] Y   The output coordinate y.
      * @param[out] Z   The output coordinate z.
      */
    void rhoWest(int x, int y, int z, int& X, int& Y, int& Z) const;
    /**
      * Method that applies the inverse of ρWest to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
    void inverseRhoWest(XoodooState& state) const;
    /**
      * Method that applies the inverse of the ρWest coordinate transformation to a bit position (X,Y,Z).
      *
      * @param[in]  X   The input coordinate x.
      * @param[in]  Y   The input coordinate y.
      * @param[in]  Z   The input coordinate z.
      * @param[out] x   The output coordinate x.
      * @param[out] y   The output coordinate y.
      * @param[out] z   The output coordinate z.
      */
    void inverseRhoWest(int X, int Y, int Z, int& x, int& y, int& z) const;
    /**
      * Method that applies theta to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
	void theta(XoodooState& state) const;
    /**
      * Method that applies theta transposed to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
	void thetaTransposed(XoodooState& state) const;
    /**
      * Method that applies the inverse of theta to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
	void inverseTheta(XoodooState& state) const;
    /**
      * Method that applies the inverse of theta transposed to an input state.
      *
      * @param[in,out]  state   The state to transform.
      */
	void inverseThetaTransposed(XoodooState& state) const;
    /**
      * Method to translate the coordinates (x, z) by a given amount.
      *
      * @param[in,out]  x   The input and output coordinate x.
      * @param[in,out]  z   The input and output coordinate z.
      * @param[in]  dx      The translation offset in x (may be negative).
      * @param[in]  dz      The translation offset in z (may be negative).
      */
    void translateXZ(int& x, int& z, int dx, int dz) const;
    void chi(XoodooState& state) const;
    void inverseChi(XoodooState& state) const;
    void reduceX(int& X) const;
    void reduceY(int& Y) const;
    void reduceZ(int& Z) const;
    void reduceXYZ(int& X, int& Y, int& Z) const;
private:
    unsigned int computeThetaOrder() const;
};

typedef unsigned char ColumnValue;

class XoodooLanes {
protected:
    vector<LaneValue> lanes;
public:
    XoodooLanes() {}
    XoodooLanes(const unsigned int sizeInLanes)
        : lanes(sizeInLanes, 0) {}
    XoodooLanes& operator=(const XoodooLanes& other)
    {
        lanes = other.lanes;
        return *this;
    }
    const vector<LaneValue>& getLanesConst() const { return lanes; }
    vector<LaneValue>& getLanes() { return lanes; }
    void clear();
    void invert();
    XoodooLanes& operator^=(const XoodooLanes& other);
    XoodooLanes& operator&=(const XoodooLanes& other);
    XoodooLanes& operator|=(const XoodooLanes& other);
    bool isZero() const;
    /** This methods loads the lanes from a stream (e.g., file).
      * @param   fin    The input stream to read the lanes from.
      */
    void load(istream& fin);
    /** This methods saves the lanes to a stream (e.g., file).
      * @param  fout    The output stream to write the lanes to.
      */
    void save(ostream& fout) const;
};

class XoodooState : public XoodooLanes {
public:
    const Xoodoo& instance;
    const unsigned int sizeZ;
public:
    XoodooState(const Xoodoo& anInstance);
    XoodooState& operator=(const XoodooState& other);
    inline LaneValue getLane(unsigned int x, unsigned int y) const
    {
        return lanes[y + Xoodoo::sizeY*x];
    }
    /** This method returns the value of a given bit in a state.
      *
      * @param  index   The index of the bit.
      * @return The bit value.
      */
    int getBit(unsigned int index) const;
    /** This method sets the value of a given bit in a state.
      *
      * @param  index   The index of the bit.
      * @param value    The value of the bit.
      */
    void setBit(unsigned int index, int value);
    /** This method returns the value of a given bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  y   The y coordinate.
      * @param  z   The z coordinate.
      * @return The bit value.
      */
    inline int getBit(unsigned int x, unsigned int y, unsigned int z) const
    {
        return (lanes[y + Xoodoo::sizeY*x] >> z) & 1;
    }
    /** This method sets to 0 a particular bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  y   The y coordinate.
      * @param  z   The z coordinate.
      */
    inline void setBitToZero(unsigned int x, unsigned int y, unsigned int z)
    {
        lanes[y + Xoodoo::sizeY*x] &= ~((LaneValue)1 << z);
    }
    /** This method sets to 1 a particular bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  y   The y coordinate.
      * @param  z   The z coordinate.
      */
    inline void setBitToOne(unsigned int x, unsigned int y, unsigned int z)
    {
        lanes[y + Xoodoo::sizeY*x] |= ((LaneValue)1 << z);
    }
    /** This method inverts a particular bit in a state.
      *
      * @param  x   The x coordinate.
      * @param  y   The y coordinate.
      * @param  z   The z coordinate.
      */
    inline void invertBit(unsigned int x, unsigned int y, unsigned int z)
    {
        lanes[y + Xoodoo::sizeY*x] ^= ((LaneValue)1 << z);
    }
    inline ColumnValue getColumn(unsigned int x, unsigned int z) const
    {
        return getBit(x, 0, z) + 2*getBit(x, 1, z) + 4*getBit(x, 2, z);
    }
    inline void setColumn(unsigned int x, unsigned int z, const ColumnValue& value)
    {
        if (value & 1) setBitToOne(x, 0, z); else setBitToZero(x, 0, z);
        if (value & 2) setBitToOne(x, 1, z); else setBitToZero(x, 1, z);
        if (value & 4) setBitToOne(x, 2, z); else setBitToZero(x, 2, z);
    }
    inline void addToColumn(unsigned int x, unsigned int z, const ColumnValue& value)
    {
        if (value & 1) invertBit(x, 0, z);
        if (value & 2) invertBit(x, 1, z);
        if (value & 4) invertBit(x, 2, z);
    }
    /** This method translates the entire state by a given offset.
      *
      * @param  dx  The translation offset along x.
      * @param  dz  The translation offset along z.
      */
    void translateXZ(int dx, int dz);
    string getDisplayString(unsigned int x, unsigned int pad=0) const;
    void display(ostream& fout) const;
    friend ostream& operator<<(ostream& a, const XoodooState& state);
};

void displayStatesInRound(ostream& fout, const XoodooState& NE, const XoodooState& SE, const XoodooState& NW);
void displayStatesInRound(ostream& fout, const XoodooState& NE, const XoodooState& SE, const XoodooState& SW, const XoodooState& NW);

/** This function translates a column value along the y axis and returns the
  * translated value.
  */
inline ColumnValue translateColumn(const ColumnValue& column, const int& dy)
{
    int ddy = ((dy%(int)Xoodoo::sizeY)+(int)Xoodoo::sizeY)%(int)Xoodoo::sizeY;
    if (ddy == 0)
        return column;
    else
        return ((column << ddy) | (column >> (Xoodoo::sizeY-ddy))) & ((1 << Xoodoo::sizeY) - 1);
}

#endif
