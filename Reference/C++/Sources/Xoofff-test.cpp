/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include "Keccak.h"
#include "Xoofff.h"
#include "Xoofff-test.h"

/* #define OUTPUT */
/* #define VERBOSE */

typedef unsigned char           BitSequence;
typedef size_t                  BitLength;

#define SnP_width_sponge        1600
#define XnP_widthInBytes        48
#define XnP_width               (XnP_widthInBytes*8)
#define inputByteSize           (2*16*XnP_widthInBytes+XnP_widthInBytes)
#define outputByteSize          (2*16*XnP_widthInBytes+XnP_widthInBytes)
#define keyByteSize             (1*XnP_widthInBytes)
#define inputBitSize            (inputByteSize*8)
#define outputBitSize           (outputByteSize*8)
#define keyBitSize              (keyByteSize*8)
#define checksumByteSize        16

#if (defined(OUTPUT) || defined(VERBOSE) || !defined(EMBEDDED))
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(EMBEDDED)
static void assert(int condition)
{
    if (!condition)
    {
        for ( ; ; ) ;
    }
}
uint8_t random8( void );
#define rand    random8
#else
#include <assert.h>
#endif

static void randomize( unsigned char* data, unsigned int length)
{
	#if !defined(EMBEDDED)
    srand((unsigned int)time(0));
	#endif
    while (length--)
    {
        *data++ = rand();
    }
}

static void generateSimpleRawMaterial(unsigned char* data, unsigned int length, unsigned char seed1, unsigned int seed2)
{
    unsigned int i;

    for(i=0; i<length; i++) {
        unsigned char iRolled;
        unsigned char byte;
        seed2 = seed2 % 8;
        iRolled = ((unsigned char)i << seed2) | ((unsigned char)i >> (8-seed2));
        byte = seed1 + 161*length - iRolled + i;
        data[i] = byte;
    }
}

static void performTestXoofffOneInput(BitLength keyLen, BitLength inputLen, BitLength outputLen, int /*flags*/, Keccak &rSpongeChecksum, unsigned int mode)
{
    BitSequence input[inputByteSize];
    BitSequence output[outputByteSize];
    BitSequence key[keyByteSize];
    unsigned int seed;

    seed = keyLen + outputLen + inputLen;
    seed ^= seed >> 3;
    randomize(key, keyByteSize);
    randomize(input, inputByteSize);
    randomize(output, outputByteSize);
    generateSimpleRawMaterial(input, (inputLen + 7) / 8, seed + 0x13AD, 0x75 - seed );
    generateSimpleRawMaterial(key, (keyLen + 7) / 8, seed + 0x2749, 0x31 - seed );
    if (inputLen & 7)
        input[inputLen / 8] &= (1 << (inputLen & 7)) - 1;
    if (keyLen & 7)
        key[keyLen / 8] &= (1 << (keyLen & 7)) - 1;

    #ifdef VERBOSE
    printf( "keyLen %5u, outputLen %5u, inputLen %5u (in bits)\n", (unsigned int)keyLen, (unsigned int)outputLen, (unsigned int)inputLen);
    #endif

	Xoofff xp;

    if (mode == 0)
    {
        /* Input/Output full size in one call */
		const BitString Z = xp(BitString(key, keyLen), BitString(input, inputLen), outputLen);
		if (Z.size() != 0) std::copy(Z.array(), Z.array() + (outputLen + 7) / 8, output);
    }

	rSpongeChecksum.absorb(output, 8 * ((outputLen + 7) / 8));

    #ifdef VERBOSE
    {
        unsigned int i;

        printf("Key of %d bits:", (int)keyLen);
        keyLen += 7;
        keyLen /= 8;
        for(i=0; (i<keyLen) && (i<16); i++)
            printf(" %02x", (int)key[i]);
        if (keyLen > 16)
            printf(" ...");
        printf("\n");

        printf("Input of %d bits:", (int)inputLen);
        inputLen += 7;
        inputLen /= 8;
        for(i=0; (i<inputLen) && (i<16); i++)
            printf(" %02x", (int)input[i]);
        if (inputLen > 16)
            printf(" ...");
        printf("\n");

        printf("Output of %d bits:", (int)outputLen);
        outputLen += 7;
        outputLen /= 8;
        for(i=0; (i<outputLen) && (i<8); i++)
            printf(" %02x", (int)output[i]);
        if (outputLen > 16)
            printf(" ...");
        if (i < (outputLen - 8))
            i = outputLen - 8;
        for( /* empty */; i<outputLen; i++)
            printf(" %02x", (int)output[i]);
        printf("\n\n");
        fflush(stdout);
    }
    #endif
}

static void performTestXoofff(unsigned char *checksum, unsigned int mode)
{
    BitLength inputLen, outputLen, keyLen;
    int flags;

    /* Accumulated test vector */
	Keccak spongeChecksum(SnP_width_sponge, 0);

    #ifdef OUTPUT
    printf("k ");
    #endif
    outputLen = 128*8;
    inputLen = 64*8;
    flags = 0;
    for(keyLen=0; keyLen<keyBitSize; keyLen = (keyLen < 2*XnP_width) ? (keyLen+1) : (keyLen+8)) {
        performTestXoofffOneInput(keyLen, inputLen, outputLen, flags, spongeChecksum, mode);
    }
    
    #ifdef OUTPUT
    printf("i ");
    #endif
    outputLen = 128*8;
    keyLen = 16*8;
    for(inputLen=0; inputLen<=inputBitSize; inputLen = (inputLen < 2*XnP_width) ? (inputLen+1) : (inputLen+8)) {
        performTestXoofffOneInput(keyLen, inputLen, outputLen, flags, spongeChecksum, mode);
    }
    
    #ifdef OUTPUT
    printf("o ");
    #endif
    inputLen = 64*8;
    keyLen = 16*8;
    for(outputLen=0; outputLen<=outputBitSize; outputLen = (outputLen < 2*XnP_width) ? (outputLen+1) : (outputLen+8)) {
        performTestXoofffOneInput(keyLen, inputLen, outputLen, flags, spongeChecksum, mode);
    }
    
	spongeChecksum.squeeze(checksum, 8 * checksumByteSize);

    #ifdef VERBOSE
    {
        unsigned int i;
        printf("Xoofff\n" );
        printf("Checksum: ");
        for(i=0; i<checksumByteSize; i++)
            printf("\\x%02x", (int)checksum[i]);
        printf("\n\n");
    }
    #endif
}

void selfTestXoofff(const char *expected)
{
    unsigned char checksum[checksumByteSize];
    unsigned int mode;

    for(mode = 0; mode <= 0; ++mode) {
		#ifdef OUTPUT
        printf("Testing Xoofff %u ", mode);
        fflush(stdout);
		#endif
        performTestXoofff(checksum, mode);
        assert(memcmp(expected, checksum, checksumByteSize) == 0);
		#ifdef OUTPUT
        printf(" - OK.\n");
		#endif
    }
}

#ifdef OUTPUT
void writeTestXoofffOne(FILE *f)
{
    unsigned char checksum[checksumByteSize];
    unsigned int offset;

    printf("Writing Xoofff ");
    performTestXoofff(checksum, 0);
    fprintf(f, "    selfTestXoofff(\"");
    for(offset=0; offset<checksumByteSize; offset++)
        fprintf(f, "\\x%02x", checksum[offset]);
    fprintf(f, "\");\n");
    printf("\n");
}

void writeTestXoofff(const char *filename)
{
    FILE *f = fopen(filename, "w");
    assert(f != NULL);
    writeTestXoofffOne(f);
    fclose(f);
}
#endif

#if 0
static void outputHex(const unsigned char *data, unsigned char length)
{
    unsigned int i;
    for(i=0; i<length; i++)
        printf("%02x ", (int)data[i]);
    printf("\n\n");
}

void printXoofffTestVectors()
{
    unsigned char *M, *C;
    unsigned char output[10032];
    unsigned int i, j, l;

    printf("Xoofff(M=empty, C=empty, 32 output bytes):\n");
    Xoofff(0, 0, output, 32, 0, 0);
    outputHex(output, 32);
    printf("Xoofff(M=empty, C=empty, 64 output bytes):\n");
    Xoofff(0, 0, output, 64, 0, 0);
    outputHex(output, 64);
    printf("Xoofff(M=empty, C=empty, 10032 output bytes), last 32 bytes:\n");
    Xoofff(0, 0, output, 10032, 0, 0);
    outputHex(output+10000, 32);
    for(l=1, i=0; i<7; i++, l=l*17) {
        M = malloc(l);
        for(j=0; j<l; j++)
            M[j] = j%251;
        printf("Xoofff(M=pattern 0x00 to 0xFA for 17^%d bytes, C=empty, 32 output bytes):\n", i);
        Xoofff(M, l, output, 32, 0, 0);
        outputHex(output, 32);
        free(M);
    }
    for(l=1, i=0; i<4; i++, l=l*41) {
        unsigned int ll = (1 << i)-1;
        M = malloc(ll);
        memset(M, 0xFF, ll);
        C = malloc(l);
        for(j=0; j<l; j++)
            C[j] = j%251;
        printf("Xoofff(M=%d times byte 0xFF, C=pattern 0x00 to 0xFA for 41^%d bytes, 32 output bytes):\n", ll, i);
        Xoofff(M, ll, output, 32, C, l);
        outputHex(output, 32);
        free(M);
        free(C);
    }
}
#endif

void testXoofff(void)
{

#ifndef KeccakP1600_excluded
#ifdef OUTPUT
//    printXoofffTestVectors();
    writeTestXoofff("Xoofff.txt");
#endif
    selfTestXoofff("\xca\x8e\x19\x14\xb6\xe2\x8f\xeb\x5f\xcb\xd2\x7d\xc2\x39\x2b\xd5");
#endif
}
