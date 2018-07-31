/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "types.h"
#include "Xoofff.h"
#include "Xoofff-test.h"
#include "XooModes-test.h"

void testXoodoo(int iterations, std::ostream &os)
{
	UINT32 state[12];
	std::fill(std::begin(state), std::end(state), 0);

	Xoodoo wp(384, 12);
	wp.setLog(LOG_ROUND, os);

	os << "Permutation 1 (starting with a state of all zeros)\n";
	wp(reinterpret_cast<UINT8 *>(&state[0]));
	os << '\n';

	wp.setLog(LOG_FINAL, os);

	for (int i = 2; i <= iterations; i++)
	{
		os << "Permutation " << i << "\n";
		wp(reinterpret_cast<UINT8 *>(&state[0]));
		os << '\n';
	}
}

int main(int argc, char *argv[])
{
	try
	{
		//testXoodoo(384, std::cout);
		testXoofff();
		testXooModes();

		std::cout << std::flush;
	}
	catch (Exception e)
	{
		std::cout << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
