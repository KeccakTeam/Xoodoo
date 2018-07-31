/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include "Xoofff.h"

/* XoodooCompressionRollingFunction */
BitString XoodooCompressionRollingFunction::operator()(const BitString &k, unsigned int i) const
{
	BitString kp = k;
	XoodooState A(kp.array());

	for (unsigned int j = 0; j < i; j++)
	{
		A[0][0] = A[0][0] ^ shiftLane(A[0][0], 13) ^ cyclicShiftLane(A[1][0], 3);
		XoodooPlane B = cyclicShiftPlane(A[0], 3, 0);

		A[0] = A[1];
		A[1] = A[2];
		A[2] = B;
	}

	A.write(kp.array());

	return kp;
}

/* XoodooExpansionRollingFunction */
BitString XoodooExpansionRollingFunction::operator()(const BitString &k, unsigned int i) const
{
	BitString kp = k;
	XoodooState A(kp.array());

	for (unsigned int j = 0; j < i; j++)
	{
		A[0][0] = (A[1][0] & A[2][0]) ^ cyclicShiftLane(A[0][0], 5) ^ cyclicShiftLane(A[1][0], 13) ^ 7;
		XoodooPlane B = cyclicShiftPlane(A[0], 3, 0);

		A[0] = A[1];
		A[1] = A[2];
		A[2] = B;
	}

	A.write(kp.array());

	return kp;
}

/* Xoofff instantiation parameters */
namespace XooParams
{
	IterableTransformation<Xoodoo>   p_b(384, 6);
	IterableTransformation<Xoodoo>   p_c(384, 6);
	IterableTransformation<Xoodoo>   p_d(384, 6);
	IterableTransformation<Xoodoo>   p_e(384, 6);
	IterableTransformation<Identity> p_identity(384);

	XoodooCompressionRollingFunction roll_c;
	XoodooExpansionRollingFunction   roll_e;

	unsigned int                     param_SANE_t = 128;
	unsigned int                     param_SANE_l = 8;

	unsigned int                     param_SANSE_t = 256;

	unsigned int                     param_WBC_l = 8;

	unsigned int                     param_WBC_AE_t = 128;
	unsigned int                     param_WBC_AE_l = 8;
};

Farfalle make_Short_Xoofff()
{
	return Farfalle(XooParams::p_b, XooParams::p_c, XooParams::p_identity, XooParams::p_e, XooParams::roll_c, XooParams::roll_e);
}

Farfalle make_Xoofff()
{
	return Farfalle(XooParams::p_b, XooParams::p_c, XooParams::p_d, XooParams::p_e, XooParams::roll_c, XooParams::roll_e);
}

/* Xoofff */
Xoofff::Xoofff()
	: Farfalle(make_Xoofff())
{
}

/* Xoofff-SANE */
XoofffSANE::XoofffSANE(const BitString &K, const BitString &N, BitString &T, bool sender)
	: FarfalleSANE(make_Xoofff(), XooParams::param_SANE_t, XooParams::param_SANE_l, K, N, T, sender)
{
}

/* Xoofff-SANSE */
XoofffSANSE::XoofffSANSE(const BitString &K)
	: FarfalleSANSE(make_Xoofff(), XooParams::param_SANSE_t, K)
{
}

/* Xoofff-WBC */
XoofffWBC::XoofffWBC()
	: FarfalleWBC(make_Short_Xoofff(), make_Xoofff(), XooParams::param_WBC_l)
{
}

/* Xoofff-WBC-AE */
XoofffWBCAE::XoofffWBCAE()
	: FarfalleWBCAE(make_Short_Xoofff(), make_Xoofff(), XooParams::param_WBC_AE_t, XooParams::param_WBC_AE_l)
{
}
