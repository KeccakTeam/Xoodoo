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

#ifndef _XOODOOTRAILS_H_
#define _XOODOOTRAILS_H_

#include <fstream>
#include <iostream>
#include "types.h"
#include "Xoodoo.h"
#include "XoodooDCLC.h"
#include "XoodooPropagation.h"

class TrailException : public Exception {
public:
    TrailException() : Exception() {}
    TrailException(const std::string& aReason) : Exception(aReason) {}
};

/** This class implements a container for a differential or linear trail.
  * A trail makes sense only in the context of a XoodooPropagation object.
  * The main attribute of the class is the sequence of state values before χ.
  */
class Trail {
public:
    const XoodooPropagation& DCorLC;
    /** This attribute tells whether the first state (states[0]) is specified.
      *  If false, the trail is actually a trail core. In this case, weights[0]
      * is the weight of λ<sup>-1</sup>(states[1]).
      */
    bool firstStateSpecified;
    /** This attribute contains the list of states round after round, before χ.
      */
    vector<XoodooState> states;
    /** This attribute tells whether the state after the last χ is specified.
      *  If true, the state after the last χ is specified in stateAfterLastChi.
      */
    bool stateAfterLastChiSpecified;
    /**
      *  If stateAfterLastChiSpecified==true, this attribute contains the state after the last χ.
      */
    XoodooState stateAfterLastChi;
    /** This attribute contains the propagation weights of the states
      * in @a states. So, @a weights has the same size
      * as @a states.
      */
    vector<unsigned int> weights;
    /** This attribute contains the sum of the weights[i].
      */
    unsigned int totalWeight;
public:
    /** This constructor creates an empty trail.
    */
    Trail(const XoodooPropagation& aDCorLC);
    /** This constructor loads a trail from an input stream.
      * @param   fin    The input stream to read the trail from.
      */
    Trail(const XoodooPropagation& aDCorLC, istream& fin);
    /** This constructor initializes a trail by copying the
      * trail given in parameter.
      * @param   other  The original trail to copy.
      */
    Trail(const Trail& other)
        : DCorLC(other.DCorLC),
        firstStateSpecified(other.firstStateSpecified),
        states(other.states),
        stateAfterLastChiSpecified(other.stateAfterLastChiSpecified),
        stateAfterLastChi(other.stateAfterLastChi),
        weights(other.weights),
        totalWeight(other.totalWeight) {}
    Trail& operator=(const Trail& other);
    /** This method returns the number of rounds the trail represents.
      *  @return    The number of rounds.
      */
    unsigned int getNumberOfRounds() const;
    /** This method empties the trail.
     */
    void clear();
    /** For a trail core, this sets the weight before the first state, i.e.,
      * the weight of λ<sup>-1</sup>(states[1]).
      * @param   weight The weight of λ<sup>-1</sup>(states[1]).
      */
    void setBeforeFirstStateWeight(unsigned int weight);
    /** This method appends a state to the end of @a states,
      * with its corresponding propagation weight.
      * @param   state  The state to add.
      * @param   weight The propagation weight.
      */
    void append(const XoodooState& state, unsigned int weight);
    /** This method appends another trail to the current trail.
      * @param   otherTrail The trail to append.
      */
    void append(const Trail& otherTrail);
    /** This method inserts a state at the beginning of @a states,
      * with its corresponding propagation weight.
      * @param   state  The state to add.
      * @param   weight The propagation weight.
      */
    void prepend(const XoodooState& state, unsigned int weight);
    /** This method displays the trail for in a human-readable form.
      * @param   DCorLC The propagation context of the trail,
      *                 as a reference to a XoodooPropagation object.
      * @param  fout    The stream to display to.
      */
    void display(ostream& fout) const;
    /** This methods loads the trail from a stream (e.g., file).
      * @param   fin    The input stream to read the trail from.
      */
    void load(istream& fin);
    /** This methods outputs the trail to save it in, e.g., a file.
      * @param  fout    The stream to save the trail to.
      */
    void save(ostream& fout) const;
    /** This method translates the entire trail by a given offset.
      *
      * @param  dx  The translation offset along x.
      * @param  dz  The translation offset along z.
      */
    void translateXZ(int dx, int dz);
    /** This method make the trail canoncial w.r.t. translations along x and z.
      */
    void makeCanonical();
};

#endif
