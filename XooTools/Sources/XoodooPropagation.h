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

#ifndef _XOODOOPROPAGATION_H_
#define _XOODOOPROPAGATION_H_

#include <cstdint>
#include <string>
#include "XoodooAffineBases.h"
#include "XoodooDCLC.h"

using namespace std;

class Trail;

/** This class provides the necessary tools to compute the propagation of
  * either differences or linear patterns through the rounds of Xoodoo.
  * To provide methods that work similarly for linear (LC) and differential cryptanalysis (DC),
  * an instance of this class is specialized in either DC or LC.
  * The convention of the direction of propagation is as follows:
  * - "direct" means in the direction that use θ or θ transposed
  *   (same direction as the rounds for DC, inverse rounds for LC);
  * - "reverse" means the opposite of "direct"
  *   (hence same direction as the inverse rounds for DC, rounds for LC).
  *
  * In this context, the words "before" and "after" refer to the "direct" direction.
  */
class XoodooPropagation {
public:
    /** The output row patterns in an affine space representation.
      */
    vector<AffineSpaceOfColumns> affinePerInput;
    /** This is a link to the 'parent' XoodooDCLC class.
      */
    const XoodooDCLC& parent;
    /** This attribute contains a string to help build appropriate file names: "DC" or "LC".
      */
    const string name;
protected:
    /** The λ mode contains the lambdaMode attribute to pass to XoodooDCLC
      * to compute the linear part in the "direct" direction.
      */
    XoodooDCLC::LambdaMode lambdaMode;
    /** The λ mode contains the lambdaMode attribute to pass to XoodooDCLC
      * to compute the linear part in the "reverse" direction.
      */
    XoodooDCLC::LambdaMode reverseLambdaMode;
private:
    /** This vector tells whether a pattern x at the input of χ is compatible
      * with output y. This can be found in chiCompatibilityTable[x+8*y].
      * See also isChiCompatible().
      */
    vector<bool> chiCompatibilityTable;
public:
    /** This type allows one to specify the type of propagation: differential (DC) or linear (LC). */
    enum DCorLC { DC = 0, LC };
    /** This constructor initializes the different attributes as a function of the
      * Xoodoo instance referenced by @a aParent and
      * whether the instance handles DC or LC (@a aDCorLC).
      * @param   aParent    A reference to the Xoodoo instance as a XoodooDCLC object.
      * @param   aDCorLC    The propagation type.
      */
    XoodooPropagation(const XoodooDCLC& aParent, XoodooPropagation::DCorLC aDCorLC);
    /** This method returns the propagation type (DC or LC) handled by the instance.
      * @return The propagation type as a DCorLC value.
      */
    DCorLC getPropagationType() const;
    /** This function displays the possible patterns and their weights.
      * @param  out The stream to display to.
      */
    void display(ostream& out) const;
    /** This method returns the propagation weight of a state.
      * @param   state  The value of a state given as a vector of slices.
      * @return The propagation weight of the given state.
      */
    unsigned int getWeight(const XoodooState& state) const;
    /** This method returns true iff the input column pattern is compatible with the output column pattern.
      * @param   beforeChi  The column value at the input of χ.
      * @param   afterChi   The column value at the output of χ.
      * @return It returns true iff the given values are compatible through χ.
      */
    inline bool isChiCompatible(const ColumnValue& beforeChi, const ColumnValue& afterChi) const
    {
        return chiCompatibilityTable[beforeChi+(1<<Xoodoo::sizeY)*afterChi];
    }
    /** This method returns true iff the given state before χ is compatible with the given state after χ.
      * @param   beforeChi  The state value at the input of χ.
      * @param   afterChi   The state value at the output of χ.
      * @return It returns true iff the given values are compatible through χ.
      */
    bool isChiCompatible(const XoodooState& beforeChi, const XoodooState& afterChi) const;
    /** This method returns true iff two states can be chained, i.e., if the first state
      * is compatible through χ and λ with the second state.
      * @param   first  The first state before χ.
      * @param   second The second trail, before the next χ.
      * @return It returns true iff the given states can be chained.
      */
    bool isRoundCompatible(const XoodooState& first, const XoodooState& second) const;
    /** This method builds an affine set of states corresponding to the propagation
      * of a given input state through χ and λ.
      * The affine space produced thus covers the propagation through a whole round.
      * The parities in the AffineSpaceOfStates object are those before θ.
      * @param   state  The state before χ to propagate, given as a vector of slices.
      * @return The affine space as a AffineSpaceOfStates object.
      */
    AffineSpaceOfStates buildStateBase(const XoodooState& state, bool reverse) const;
    /** This method applies λ in the "direct" direction:
      * - DC: ρEast then θ then ρWest;
      * - LC: ρWest<sup>-1</sup> then θ<sup>T</sup> then ρEast<sup>-1</sup>.
      * @param   state  The state to process.
      */
    void directLambda(XoodooState& state) const;
    /** This method applies λ in the "reverse" direction:
      * - DC: ρWest<sup>-1</sup> then θ<sup>-1</sup> then ρEast<sup>-1</sup>.
      * - LC: ρEast then θ<sup>-1T</sup> then ρWest.
      * @param   state  The state to process.
      */
    void reverseLambda(XoodooState& state) const;
    /** This method applies the ρ before θ in the "direct" direction:
      * - DC: ρEast.
      * - LC: ρWest<sup>-1</sup>.
      */
    void directEarlyRho(int x, int y, int z, int& X, int& Y, int& Z) const;
    /** This method applies the ρ before θ in the "reverse" direction:
      * - DC: ρEast<sup>-1</sup>.
      * - LC: ρWest.
      */
    void reverseEarlyRho(int x, int y, int z, int& X, int& Y, int& Z) const;
    /** This method applies the ρ before θ in the "direct" direction:
      * - DC: ρWest.
      * - LC: ρEast<sup>-1</sup>.
      */
    void directLateRho(int x, int y, int z, int& X, int& Y, int& Z) const;
    /** This method applies the ρ before θ in the "direct" direction:
      * - DC: ρWest<sup>-1</sup>.
      * - LC: ρEast.
      */
    void reverseLateRho(int x, int y, int z, int& X, int& Y, int& Z) const;
    void displayStatesInRound(ostream& fout, const XoodooState& stateAfterChi) const;
    /** This method checks the consistency of the trail given as parameter.
      * If an inconsistency is found, an exception is thrown and details
      * about the inconsistency are displayed in the error console (cerr).
      * The aspects tested are:
      * - the propagation weights declared in the trail match the propagation weights of the specified differences;
      * - between two rounds, the specified differences are compatible.
      * .
      * @param  trail   The trail to test the consistence of.
      */
    void checkTrail(const Trail& trail) const;
    /** This methods reads all the trails in a file, checks their consistency
      * and then produces a report in the given output stream.
      * See also Trail::produceHumanReadableFile().
      * @param   fileNameIn The name of the file containing the trails.
      * @param   fout       The output stream to send the report to.
      * @param   maxWeight  The maximum weight to display trails.
      *                     If 0, the maximum weight of trails to display is
      *                     computed automatically so that a reasonable number
      *                     of trails are displayed in the report.
      * @return The number of trails read and checked.
      */
    uint64_t displayTrailsAndCheck(const string& fileNameIn, ostream& fout, unsigned int maxWeight = 0) const;
    /** This function reads all the trails in a file, checks their consistency
      * and then produces a report.
      * The report is output in a file with the same file name plus ".txt".
      * See also XoodooPropagation::displayTrailsAndCheck().
      * @param   DCorLC     The propagation context of the trails,
      *                     as a reference to a XoodooPropagation object.
      * @param   fileName   The name of the file containing the trails.
      * @param   verbose    If true, the function will display the name of
      *                     the file written to cout.
      * @param   maxWeight  As in XoodooPropagation::displayTrailsAndCheck().
      * @return The number of trails read and checked.
      */
    uint64_t produceHumanReadableFile(const string& fileName, bool verbose = true, unsigned int maxWeight = 0) const;
    /** This method builds a file name by prepending "DC" or "LC" as a prefix
      * and appending a given suffix to the name produced by
      * XoodooDCLC::getName().
      * @param   suffix The given suffix.
      * @return The constructed file name.
      */
    string buildFileName(const string& suffix) const;
    /** This method builds a file name by prepending "DC" or "LC" and a given prefix
      * and appending a given suffix to the name produced by
      * XoodooDCLC::getName().
      * @param   prefix The given prefix.
      * @param   suffix The given suffix.
      * @return The constructed file name.
      */
    string buildFileName(const string& prefix, const string& suffix) const;
private:
    /** This method initializes affinePerInput.
      */
    void initializeAffine();
    /** This method initializes chiCompatibilityTable.
      */
    void initializeChiCompatibilityTable();
};

#endif
