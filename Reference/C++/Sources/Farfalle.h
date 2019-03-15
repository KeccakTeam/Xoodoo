/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _FARFALLE_H_
#define _FARFALLE_H_

#include <iostream>
#include <memory>

#include "bitstring.h"
#include "transformations.h"
#include "types.h"

/**
 * Class implementing a rolling function
 */
class BaseRollingFunction
{
	public:
		virtual BitString operator()(const BitString &k, unsigned int i) const = 0;
};

class IdentityRollingFunction : public BaseRollingFunction
{
	public:
		BitString operator()(const BitString &k, unsigned int i) const;
};

/**
 * Class implementing the Farfalle construction
 */
class Farfalle
{
	protected:
		BaseIterableTransformation &p_b;
		BaseIterableTransformation &p_c;
		BaseIterableTransformation &p_d;
		BaseIterableTransformation &p_e;
		BaseRollingFunction        &roll_c;
		BaseRollingFunction        &roll_e;

	public:
		Farfalle(BaseIterableTransformation &p_b, BaseIterableTransformation &p_c, BaseIterableTransformation &p_d, BaseIterableTransformation &p_e, BaseRollingFunction &roll_c, BaseRollingFunction &roll_e);
		BitString     operator()(const BitString &K, const BitStrings &Mseq, unsigned int n, unsigned int q = 0) const;
		unsigned int  width() const;
};

/**
 * Class implementing Farfalle-SANE
 */
class FarfalleSANE
{
	private:
		Farfalle           F;
		const unsigned int t;
		const unsigned int l;
		BitString          K;
		BitStrings         history;
		unsigned int       offset;
		unsigned int       e;

	public:
		FarfalleSANE(const Farfalle &F, unsigned int t, unsigned int l, const BitString &K, const BitString &N, BitString &T, bool sender);
		std::pair<BitString, BitString>  wrap(const BitString &A, const BitString &P);
		BitString                        unwrap(const BitString &A, const BitString &C, const BitString &T);
};

/**
 * Class implementing Farfalle-SANSE
 */
class FarfalleSANSE
{
	private:
		Farfalle           F;
		const unsigned int t;
		BitString          K;
		BitStrings         history;
		unsigned int       e;

	public:
		FarfalleSANSE(const Farfalle &F, unsigned int t, const BitString &K);
		std::pair<BitString, BitString>  wrap(const BitString &A, const BitString &P);
		BitString                        unwrap(const BitString &A, const BitString &C, const BitString &T);
};

/**
 * Class implementing Farfalle-WBC
 */
class FarfalleWBC
{
	protected:
		Farfalle           H;
		Farfalle           G;
		const unsigned int l;

		unsigned int split(unsigned int n) const;

	public:
		FarfalleWBC(const Farfalle &H, const Farfalle &G, unsigned int l);
		BitString  encipher(const BitString &K, const BitString &W, const BitString &P) const;
		BitString  decipher(const BitString &K, const BitString &W, const BitString &C) const;
};

/**
 * Class implementing Farfalle-WBC-AE
 */
class FarfalleWBCAE : public FarfalleWBC
{
	private:
		const unsigned int t;

	public:
		FarfalleWBCAE(const Farfalle &H, const Farfalle &G, unsigned int t, unsigned int l);
		BitString  wrap(const BitString &K, const BitString &A, const BitString &P) const;
		BitString  unwrap(const BitString &K, const BitString &A, const BitString &C) const;
};

#endif
