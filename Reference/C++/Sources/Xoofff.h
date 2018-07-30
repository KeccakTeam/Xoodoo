/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _XOOFFF_H_
#define _XOOFFF_H_

#include <iostream>
#include <memory>

#include "bitstring.h"
#include "Farfalle.h"
#include "transformations.h"
#include "types.h"
#include "Xoodoo.h"

class XoodooCompressionRollingFunction : public BaseRollingFunction
{
	public:
		BitString operator()(const BitString &k, unsigned int i) const;
};

class XoodooExpansionRollingFunction : public BaseRollingFunction
{
	public:
		BitString operator()(const BitString &k, unsigned int i) const;
};

class Xoofff : public Farfalle
{
	public:
		Xoofff();
};

class XoofffSANE : public FarfalleSANE
{
	public:
		XoofffSANE(const BitString &K, const BitString &N, BitString &T, bool sender);
};

class XoofffSANSE : public FarfalleSANSE
{
	public:
		XoofffSANSE(const BitString &K);
};

class XoofffWBC : public FarfalleWBC
{
	public:
		XoofffWBC();
};

class XoofffWBCAE : public FarfalleWBCAE
{
	public:
		XoofffWBCAE();
};

#endif
