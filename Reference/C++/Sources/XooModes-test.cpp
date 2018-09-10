/*
Implementation by Seth Hoffert, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
https://keccak.team/xoodoo.html

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include "Keccak.h"
#include "XooModes-test.h"
#include "Xoofff.h"

/* #define OUTPUT */
/* #define VERBOSE_SANSE */
/* #define VERBOSE_SANE */
/* #define VERBOSE_WBC */
/* #define VERBOSE_WBCAE */

typedef unsigned char           BitSequence;
typedef size_t                  BitLength;

#define SnP_width_sponge        1600
#define XnP_widthInBytes        48
#define XnP_width               (XnP_widthInBytes*8)
#define dataByteSize            (2*16*XnP_widthInBytes+XnP_widthInBytes)
#define ADByteSize              (2*16*XnP_widthInBytes+XnP_widthInBytes)
#define keyByteSize             (1*XnP_widthInBytes)
#define nonceByteSize           (2*XnP_widthInBytes)
#define WByteSize               (2*XnP_widthInBytes)
#define dataBitSize             (dataByteSize*8)
#define ADBitSize               (ADByteSize*8)
#define keyBitSize              (keyByteSize*8)
#define nonceBitSize            (nonceByteSize*8)
#define WBitSize                (WByteSize*8)
#define tagLenSANSE             32
#define tagLenSANE              16
#define expansionLenWBCAE       16

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

#if defined(OUTPUT)
static void outputHex(const unsigned char *data, unsigned char length)
{
    unsigned int i;
    for(i=0; i<length; i++)
        printf("%02x ", (int)data[i]);
    printf("\n\n");
}
#endif

/* ------------------------------------------------------------------------- */

static void performTestXoofffSANSE_OneInput(BitLength keyLen, BitLength dataLen, BitLength ADLen, Keccak &rSpongeChecksum)
{
    BitSequence input[dataByteSize];
    BitSequence inputPrime[dataByteSize];
    BitSequence output[dataByteSize];
    BitSequence AD[ADByteSize];
    BitSequence key[keyByteSize];
    unsigned char tag[tagLenSANSE];
    unsigned int seed;
    unsigned int session;

    randomize(key, keyByteSize);
    randomize(input, dataByteSize);
    randomize(inputPrime, dataByteSize);
    randomize(output, dataByteSize);
    randomize(AD, ADByteSize);
    randomize(tag, tagLenSANSE);

    seed = keyLen + dataLen + ADLen;
    seed ^= seed >> 3;
    generateSimpleRawMaterial(key, (keyLen + 7) / 8, 0x4321 - seed, 0x89 + seed);
    if (keyLen & 7)
        key[(keyLen + 7) / 8 - 1] &= (1 << (keyLen & 7)) - 1;
    generateSimpleRawMaterial(input, (dataLen + 7) / 8, 0x6523 - seed, 0x43 + seed);
    if (dataLen & 7)
        input[(dataLen + 7) / 8 - 1] &= (1 << (dataLen & 7)) - 1;
    generateSimpleRawMaterial(AD, (ADLen + 7) / 8, 0x1A29 - seed, 0xC3 + seed);
    if (ADLen & 7)
        AD[(ADLen + 7) / 8 - 1] &= (1 << (ADLen & 7)) - 1;

    #ifdef VERBOSE_SANSE
    printf( "keyLen %5u, dataLen %5u, ADLen %5u (in bits)\n", (unsigned int)keyLen, (unsigned int)dataLen, (unsigned int)ADLen);
    #endif

	XoofffSANSE xpEnc(BitString(key, keyLen));
	XoofffSANSE xpDec(BitString(key, keyLen));

    #ifdef VERBOSE_SANSE
    {
        unsigned int i;
        BitLength len;

        printf("Key of %d bits:", (int)keyLen);
        len = (keyLen + 7) / 8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)key[i]);
        if (len > 16)
            printf(" ...");
        printf("\n");

        printf("Input of %d bits:", (int)dataLen);
        len = (dataLen + 7) /8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)input[i]);
        if (len > 16)
            printf(" ...");
        printf("\n");

        printf("AD of %d bits:", (int)ADLen);
        len = (ADLen + 7) / 8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)AD[i]);
        if (len > 16)
            printf(" ...");
        printf("\n\n");
        fflush(stdout);
    }
    #endif

	for (session = 3; session != 0; --session) {
		const std::pair<BitString, BitString> ct = xpEnc.wrap(BitString(AD, ADLen), BitString(input, dataLen));
		if (ct.first.size() != 0) std::copy(ct.first.array(), ct.first.array() + (ct.first.size() + 7) / 8, output);
		if (ct.second.size() != 0) std::copy(ct.second.array(), ct.second.array() + (ct.second.size() + 7) / 8, tag);

		const BitString p_prime = xpDec.unwrap(BitString(AD, ADLen), ct.first, ct.second);
		if (p_prime.size() != 0) std::copy(p_prime.array(), p_prime.array() + (p_prime.size() + 7) / 8, inputPrime);

        assert(!memcmp(input,inputPrime,(dataLen + 7) / 8));
		rSpongeChecksum.absorb(output, 8 * ((dataLen + 7) / 8));
		rSpongeChecksum.absorb(tag, 8 * tagLenSANSE);
        #ifdef VERBOSE_SANSE
        {
            unsigned int i;
            unsigned int len;

            printf("Output of %d bits:", (int)dataLen);
            len = (dataLen + 7) / 8;
            for(i=0; (i<len) && (i<8); i++)
                printf(" %02x", (int)output[i]);
            if (len > 16)
                printf(" ...");
            if (i < (len - 8))
                i = len - 8;
            for( /* empty */; i<len; i++)
                printf(" %02x", (int)output[i]);
            printf("\n");

            printf("Tag of %d bytes:", (int)tagLenSANSE);
            for(i=0; i<tagLenSANSE; i++)
                printf(" %02x", (int)tag[i]);
            printf("\n");
            fflush(stdout);
            if (session == 1)
                printf("\n");
        }
        #endif
    }
}

static void performTestXoofffSANSE(unsigned char *checksum)
{
    BitLength dataLen, ADLen, keyLen;

    /* Accumulated test vector */
	Keccak spongeChecksum(SnP_width_sponge, 0);

    #ifdef OUTPUT
    printf("k ");
    #endif
    dataLen = 128*8;
    ADLen = 64*8;
    for(keyLen=0; keyLen<keyBitSize; keyLen = (keyLen < 2*XnP_width) ? (keyLen+1) : (keyLen+8)) {
        performTestXoofffSANSE_OneInput(keyLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("d ");
    #endif
    ADLen = 64*8;
    keyLen = 16*8;
    for(dataLen=0; dataLen<=dataBitSize; dataLen = (dataLen < 2*XnP_width) ? (dataLen+1) : (dataLen+8)) {
        performTestXoofffSANSE_OneInput(keyLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("a ");
    #endif
    dataLen = 128*8;
    keyLen = 16*8;
    for(ADLen=0; ADLen<=ADBitSize; ADLen = (ADLen < 2*XnP_width) ? (ADLen+1) : (ADLen+8)) {
        performTestXoofffSANSE_OneInput(keyLen, dataLen, ADLen, spongeChecksum);
    }
    
	spongeChecksum.squeeze(checksum, 8 * checksumByteSize);

    #ifdef VERBOSE_SANSE
    {
        unsigned int i;
        printf("Xoofff-SANSE\n" );
        printf("Checksum: ");
        for(i=0; i<checksumByteSize; i++)
            printf("\\x%02x", (int)checksum[i]);
        printf("\n\n");
    }
    #endif
}

void selfTestXoofffSANSE(const char *expected)
{
    unsigned char checksum[checksumByteSize];

    #if defined(OUTPUT)
    printf("Testing Xoofff-SANSE ");
    fflush(stdout);
    #endif
    performTestXoofffSANSE(checksum);
    assert(memcmp(expected, checksum, checksumByteSize) == 0);
    #if defined(OUTPUT)
    printf(" - OK.\n");
    #endif
}

#ifdef OUTPUT
void writeTestXoofffSANSE_One(FILE *f)
{
    unsigned char checksum[checksumByteSize];
    unsigned int offset;

    printf("Writing Xoofff-SANSE ");
    performTestXoofffSANSE(checksum);
    fprintf(f, "    selfTestXoofffSANSE(\"");
    for(offset=0; offset<checksumByteSize; offset++)
        fprintf(f, "\\x%02x", checksum[offset]);
    fprintf(f, "\");\n");
    printf("\n");
}

void writeTestXoofffSANSE(const char *filename)
{
    FILE *f = fopen(filename, "w");
    assert(f != NULL);
    writeTestXoofffSANSE_One(f);
    fclose(f);
}
#endif

/* ------------------------------------------------------------------------- */

static void performTestXoofffSANE_OneInput(BitLength keyLen, BitLength nonceLen, BitLength dataLen, BitLength ADLen, Keccak &rSpongeChecksum)
{
    BitSequence input[dataByteSize];
    BitSequence inputPrime[dataByteSize];
    BitSequence output[dataByteSize];
    BitSequence AD[ADByteSize];
    BitSequence key[keyByteSize];
    BitSequence nonce[nonceByteSize];
    unsigned char tag[tagLenSANE];
    unsigned char tagInit[tagLenSANE];
    unsigned int seed;
    unsigned int session;

    randomize(key, keyByteSize);
    randomize(nonce, nonceByteSize);
    randomize(input, dataByteSize);
    randomize(inputPrime, dataByteSize);
    randomize(output, dataByteSize);
    randomize(AD, ADByteSize);
    randomize(tag, tagLenSANE);

    seed = keyLen + nonceLen + dataLen + ADLen;
    seed ^= seed >> 3;
    generateSimpleRawMaterial(key, (keyLen + 7) / 8, 0x4371 - seed, 0x59 + seed);
    if (keyLen & 7)
        key[keyLen / 8] &= (1 << (keyLen & 7)) - 1;
    generateSimpleRawMaterial(nonce, (nonceLen + 7) / 8, 0x1327 - seed, 0x84 + seed);
    if (nonceLen & 7)
        nonce[nonceLen / 8] &= (1 << (nonceLen & 7)) - 1;
    generateSimpleRawMaterial(input, (dataLen + 7) / 8, 0x4861 - seed, 0xb1 + seed);
    if (dataLen & 7)
        input[dataLen / 8] &= (1 << (dataLen & 7)) - 1;
    generateSimpleRawMaterial(AD, (ADLen + 7) / 8, 0x243B - seed, 0x17 + seed);
    if (ADLen & 7)
        AD[ADLen / 8] &= (1 << (ADLen & 7)) - 1;

    #ifdef VERBOSE_SANE
    printf( "keyLen %5u, nonceLen %5u, dataLen %5u, ADLen %5u (in bits)\n", (unsigned int)keyLen, (unsigned int)nonceLen, (unsigned int)dataLen, (unsigned int)ADLen);
    #endif

	BitString bits_tagInit;
	XoofffSANE xpEnc(BitString(key, keyLen), BitString(nonce, nonceLen), bits_tagInit, true);
	if (bits_tagInit.size() != 0) std::copy(bits_tagInit.array(), bits_tagInit.array() + (bits_tagInit.size() + 7) / 8, tagInit);

	XoofffSANE xpDec(BitString(key, keyLen), BitString(nonce, nonceLen), bits_tagInit, false);

	rSpongeChecksum.absorb(tagInit, 8 * tagLenSANE);

    #ifdef VERBOSE_SANE
    {
        unsigned int i;
        unsigned int len;

        printf("Key of %d bits:", (int)keyLen);
        len = (keyLen + 7) / 8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)key[i]);
        if (len > 16)
            printf(" ...");
        printf("\n");

        printf("Nonce of %d bits:", (int)nonceLen);
        len = (nonceLen + 7) / 8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)nonce[i]);
        if (len > 16)
            printf(" ...");
        printf("\n");

        printf("Input of %d bits:", (int)dataLen);
        len = (dataLen + 7) / 8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)input[i]);
        if (len > 16)
            printf(" ...");
        printf("\n");

        printf("AD of %d bits:", (int)ADLen);
        len = (ADLen + 7) / 8;
        for(i=0; (i<len) && (i<16); i++)
            printf(" %02x", (int)AD[i]);
        if (len > 16)
            printf(" ...");
        printf("\n");
    }
    #endif

    for (session = 3; session != 0; --session) {
		const std::pair<BitString, BitString> ct = xpEnc.wrap(BitString(AD, ADLen), BitString(input, dataLen));
		if (ct.first.size() != 0) std::copy(ct.first.array(), ct.first.array() + (ct.first.size() + 7) / 8, output);
		if (ct.second.size() != 0) std::copy(ct.second.array(), ct.second.array() + (ct.second.size() + 7) / 8, tag);

		const BitString p_prime = xpDec.unwrap(BitString(AD, ADLen), ct.first, ct.second);
		if (p_prime.size() != 0) std::copy(p_prime.array(), p_prime.array() + (p_prime.size() + 7) / 8, inputPrime);

        assert(!memcmp(input,inputPrime,(dataLen + 7) / 8));
		rSpongeChecksum.absorb(output, 8 * ((dataLen + 7) / 8));
		rSpongeChecksum.absorb(tag, 8 * tagLenSANE);
        #ifdef VERBOSE_SANE
        {
            unsigned int i;
            unsigned int len;

            printf("Output of %d bits:", (int)dataLen);
            len = (dataLen + 7) / 8;
            for(i=0; (i<len) && (i<8); i++)
                printf(" %02x", (int)output[i]);
            if (len > 16)
                printf(" ...");
            if (i < (len - 8))
                i = len - 8;
            for( /* empty */; i<len; i++)
                printf(" %02x", (int)output[i]);
            printf("\n");

            printf("Tag of %d bytes:", (int)tagLenSANE);
            for(i=0; i<tagLenSANE; i++)
                printf(" %02x", (int)tag[i]);
            printf("\n");
            fflush(stdout);
            if (session == 1)
                printf("\n");
        }
        #endif
    }

}


static void performTestXoofffSANE(unsigned char *checksum)
{
    BitLength dataLen, ADLen, keyLen, nonceLen;

    /* Accumulated test vector */
	Keccak spongeChecksum(SnP_width_sponge, 0);

    #ifdef OUTPUT
    printf("k ");
    #endif
    dataLen = 128*8;
    ADLen = 64*8;
    nonceLen = 24*8;
    for(keyLen=0; keyLen<keyBitSize; keyLen = (keyLen < 2*XnP_width) ? (keyLen+1) : (keyLen+8)) {
        performTestXoofffSANE_OneInput(keyLen, nonceLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("n ");
    #endif
    dataLen = 128*8;
    ADLen = 64*8;
    keyLen = 16*8;
    for(nonceLen=0; nonceLen<=nonceBitSize; nonceLen = (nonceLen < 2*XnP_width) ? (nonceLen+1) : (nonceLen+8)) {
        performTestXoofffSANE_OneInput(keyLen, nonceLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("d ");
    #endif
    ADLen = 64*8;
    keyLen = 16*8;
    nonceLen = 24*8;
    for(dataLen=0; dataLen<=dataBitSize; dataLen = (dataLen < 2*XnP_width) ? (dataLen+1) : (dataLen+8)) {
        performTestXoofffSANE_OneInput(keyLen, nonceLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("a ");
    #endif
    dataLen = 128*8;
    keyLen = 16*8;
    nonceLen = 24*8;
    for(ADLen=0; ADLen<=ADBitSize; ADLen = (ADLen < 2*XnP_width) ? (ADLen+1) : (ADLen+8)) {
        performTestXoofffSANE_OneInput(keyLen, nonceLen, dataLen, ADLen, spongeChecksum);
    }
    
	spongeChecksum.squeeze(checksum, 8 * checksumByteSize);

    #ifdef VERBOSE_SANE
    {
        unsigned int i;
        printf("Xoofff-SANE\n" );
        printf("Checksum: ");
        for(i=0; i<checksumByteSize; i++)
            printf("\\x%02x", (int)checksum[i]);
        printf("\n\n");
    }
    #endif
}

void selfTestXoofffSANE(const char *expected)
{
    unsigned char checksum[checksumByteSize];

    #if defined(OUTPUT)
    printf("Testing Xoofff-SANE ");
    fflush(stdout);
    #endif
    performTestXoofffSANE(checksum);
    assert(memcmp(expected, checksum, checksumByteSize) == 0);
    #if defined(OUTPUT)
    printf(" - OK.\n");
    #endif
}

#ifdef OUTPUT
void writeTestXoofffSANE_One(FILE *f)
{
    unsigned char checksum[checksumByteSize];
    unsigned int offset;

    printf("Writing Xoofff-SANE ");
    performTestXoofffSANE(checksum);
    fprintf(f, "    selfTestXoofffSANE(\"");
    for(offset=0; offset<checksumByteSize; offset++)
        fprintf(f, "\\x%02x", checksum[offset]);
    fprintf(f, "\");\n");
    printf("\n");
}

void writeTestXoofffSANE(const char *filename)
{
    FILE *f = fopen(filename, "w");
    assert(f != NULL);
    writeTestXoofffSANE_One(f);
    fclose(f);
}
#endif

/* ------------------------------------------------------------------------- */

static void performTestXoofffWBC_OneInput(BitLength keyLen, BitLength dataLen, BitLength WLen, Keccak &rSpongeChecksum)
{
    BitSequence input[dataByteSize];
    BitSequence inputPrime[dataByteSize];
    BitSequence output[dataByteSize];
    BitSequence key[keyByteSize];
    BitSequence W[WByteSize];
    unsigned int seed;

    randomize(key, keyByteSize);
    randomize(W, WByteSize);
    randomize(input, dataByteSize);
    randomize(inputPrime, dataByteSize);
    randomize(output, dataByteSize);

    seed = keyLen + WLen + dataLen;
    seed ^= seed >> 3;
    generateSimpleRawMaterial(key, (keyLen + 7) / 8, 0x43C1 - seed, 0xB9 + seed);
    if (keyLen & 7)
        key[keyLen / 8] &= (1 << (keyLen & 7)) - 1;
    generateSimpleRawMaterial(W, (WLen + 7) / 8, 0x1727 - seed, 0x34 + seed);
    if (WLen & 7)
        W[WLen / 8] &= (1 << (WLen & 7)) - 1;
    generateSimpleRawMaterial(input, (dataLen + 7) / 8, 0x4165 - seed, 0xA9 + seed);
    if (dataLen & 7)
        input[dataLen / 8] &= (1 << (dataLen & 7)) - 1;

    #ifdef VERBOSE_WBC
    printf( "keyLen %5u, WLen %5u, dataLen %5u (in bits)\n", (unsigned int)keyLen, (unsigned int)WLen, (unsigned int)dataLen);
    #endif

	XoofffWBC xpw;

	const BitString bits_output = xpw.encipher(BitString(key, keyLen), BitString(W, WLen), BitString(input, dataLen));
	if (bits_output.size() != 0) std::copy(bits_output.array(), bits_output.array() + (bits_output.size() + 7) / 8, output);

	const BitString p_prime = xpw.decipher(BitString(key, keyLen), BitString(W, WLen), bits_output);
	if (p_prime.size() != 0) std::copy(p_prime.array(), p_prime.array() + (p_prime.size() + 7) / 8, inputPrime);

    assert(!memcmp(input,inputPrime,(dataLen + 7) / 8));

	rSpongeChecksum.absorb(output, 8 * ((dataLen + 7) / 8));

    #ifdef VERBOSE_WBC
    {
        unsigned int i;
        unsigned int dataByteLen;

        printf("Key of %d bits:", (int)keyLen);
        keyLen += 7;
        keyLen /= 8;
        for(i=0; (i<keyLen) && (i<16); i++)
            printf(" %02x", (int)key[i]);
        if (keyLen > 16)
            printf(" ...");
        printf("\n");

        printf("Tweak of %d bits:", (int)WLen);
        WLen += 7;
        WLen /= 8;
        for(i=0; (i<WLen) && (i<16); i++)
            printf(" %02x", (int)W[i]);
        if (WLen > 16)
            printf(" ...");
        printf("\n");

        printf("Input of %d bits:", (int)dataLen);
        dataByteLen = (dataLen + 7) / 8;
        for(i=0; (i<dataByteLen) && (i<16); i++)
            printf(" %02x", (int)input[i]);
        if (dataByteLen > 16)
            printf(" ...");
        printf("\n");

        printf("Output of %d bits:", (int)dataLen);
        for(i=0; (i<dataByteLen) && (i<8); i++)
            printf(" %02x", (int)output[i]);
        if (dataByteLen > 16)
            printf(" ...");
        if (i < (dataByteLen - 8))
            i = dataByteLen - 8;
        for( /* empty */; i<dataByteLen; i++)
            printf(" %02x", (int)output[i]);
        printf("\n\n");
        fflush(stdout);
    }
    #endif

}


static void performTestXoofffWBC(unsigned char *checksum)
{
    BitLength dataLen, WLen, keyLen;

    /* Accumulated test vector */
	Keccak spongeChecksum(SnP_width_sponge, 0);

    #ifdef OUTPUT
    printf("k ");
    #endif
    dataLen = 128*8;
    WLen = 64*8;
    for(keyLen=0; keyLen<keyBitSize; keyLen = (keyLen < 2*XnP_width) ? (keyLen+1) : (keyLen+8)) {
        performTestXoofffWBC_OneInput(keyLen, dataLen, WLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("d ");
    #endif
    WLen = 64*8;
    keyLen = 16*8;
    for(dataLen=0; dataLen<=dataBitSize; dataLen = (dataLen < 2*XnP_width) ? (dataLen+1) : (dataLen+7)) {
        performTestXoofffWBC_OneInput(keyLen, dataLen, WLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("w ");
    #endif
    dataLen = 128*8;
    keyLen = 16*8;
    for(WLen=0; WLen<=WBitSize; WLen = (WLen < 2*XnP_width) ? (WLen+1) : (WLen+8)) {
        performTestXoofffWBC_OneInput(keyLen, dataLen, WLen, spongeChecksum);
    }
    
	spongeChecksum.squeeze(checksum, 8 * checksumByteSize);

    #ifdef VERBOSE_WBC
    {
        unsigned int i;
        printf("Xoofff-WBC\n" );
        printf("Checksum: ");
        for(i=0; i<checksumByteSize; i++)
            printf("\\x%02x", (int)checksum[i]);
        printf("\n\n");
    }
    #endif
}

void selfTestXoofffWBC(const char *expected)
{
    unsigned char checksum[checksumByteSize];

    #if defined(OUTPUT)
    printf("Testing Xoofff-WBC ");
    fflush(stdout);
    #endif
    performTestXoofffWBC(checksum);
    assert(memcmp(expected, checksum, checksumByteSize) == 0);
    #if defined(OUTPUT)
    printf(" - OK.\n");
    #endif
}

#ifdef OUTPUT
void writeTestXoofffWBC_One(FILE *f)
{
    unsigned char checksum[checksumByteSize];
    unsigned int offset;

    printf("Writing Xoofff-WBC ");
    performTestXoofffWBC(checksum);
    fprintf(f, "    selfTestXoofffWBC(\"");
    for(offset=0; offset<checksumByteSize; offset++)
        fprintf(f, "\\x%02x", checksum[offset]);
    fprintf(f, "\");\n");
    printf("\n");
}

void writeTestXoofffWBC(const char *filename)
{
    FILE *f = fopen(filename, "w");
    assert(f != NULL);

    #if 0
    {
        BitLength n, nl, prevnl;

        prevnl = 0xFFFFFFFF;
        for (n = 0; n <= 8*64*1024; ++n )
        {
            nl = XoofffWBC_Split(n);
            if (nl != prevnl)
            {
                printf("n %6u, nl %6u, nr %6u", n, nl, n - nl);
                if (n >= 2*1536)
                    printf(", 2^x %6u", nl / 1536);
                printf("\n");
                prevnl = nl;
            }
        }
    }
    #endif

    writeTestXoofffWBC_One(f);
    fclose(f);
}
#endif

/* ------------------------------------------------------------------------- */

static void performTestXoofffWBCAE_OneInput(BitLength keyLen, BitLength dataLen, BitLength ADLen, Keccak &rSpongeChecksum)
{
    BitSequence input[dataByteSize];
    BitSequence inputPrime[dataByteSize];
    BitSequence output[dataByteSize+expansionLenWBCAE];
    BitSequence key[keyByteSize];
    BitSequence AD[ADByteSize];
    unsigned int seed;

    randomize(key, keyByteSize);
    randomize(AD, ADByteSize);
    randomize(input, dataByteSize);
    randomize(inputPrime, dataByteSize);
    randomize(output, dataByteSize);

    seed = keyLen + ADLen + dataLen;
    seed ^= seed >> 3;
    generateSimpleRawMaterial(key, (keyLen + 7) / 8, 0x91FC - seed, 0x5A + seed);
    if (keyLen & 7)
        key[keyLen / 8] &= (1 << (keyLen & 7)) - 1;
    generateSimpleRawMaterial(AD, (ADLen + 7) / 8, 0x8181 - seed, 0x9B + seed);
    if (ADLen & 7)
        AD[ADLen / 8] &= (1 << (ADLen & 7)) - 1;
    generateSimpleRawMaterial(input, (dataLen + 7) / 8, 0x1BF0 - seed, 0xC6 + seed);
    if (dataLen & 7)
        input[dataLen / 8] &= (1 << (dataLen & 7)) - 1;

    #ifdef VERBOSE_WBCAE
    printf( "keyLen %5u, ADLen %5u, dataLen %5u (in bits)\n", (unsigned int)keyLen, (unsigned int)ADLen, (unsigned int)dataLen);
    #endif

	XoofffWBCAE xpw;

	const BitString bits_output = xpw.wrap(BitString(key, keyLen), BitString(AD, ADLen), BitString(input, dataLen));
	if (bits_output.size() != 0) std::copy(bits_output.array(), bits_output.array() + (bits_output.size() + 7) / 8, output);

	const BitString p_prime = xpw.unwrap(BitString(key, keyLen), BitString(AD, ADLen), bits_output);
	if (p_prime.size() != 0) std::copy(p_prime.array(), p_prime.array() + (p_prime.size() + 7) / 8, inputPrime);

    assert(!memcmp(input,inputPrime,(dataLen + 7) / 8));

	rSpongeChecksum.absorb(output, 8 * ((dataLen + 8 * expansionLenWBCAE + 7) / 8));

    #ifdef VERBOSE_WBCAE
    {
        unsigned int i;
        unsigned int dataByteLen;
        BitLength outputLen = dataLen + 8 * expansionLenWBCAE;
        unsigned int outputByteLen;

        printf("Key of %d bits:", (int)keyLen);
        keyLen += 7;
        keyLen /= 8;
        for(i=0; (i<keyLen) && (i<16); i++)
            printf(" %02x", (int)key[i]);
        if (keyLen > 16)
            printf(" ...");
        printf("\n");

        printf("AD of %d bits:", (int)ADLen);
        ADLen += 7;
        ADLen /= 8;
        for(i=0; (i<ADLen) && (i<16); i++)
            printf(" %02x", (int)AD[i]);
        if (ADLen > 16)
            printf(" ...");
        printf("\n");

        printf("Input of %d bits:", (int)dataLen);
        dataByteLen = (dataLen + 7) / 8;
        for(i=0; (i<dataByteLen) && (i<16); i++)
            printf(" %02x", (int)input[i]);
        if (dataByteLen > 16)
            printf(" ...");
        printf("\n");

        printf("Output of %d bits:", (int)outputLen);
        outputByteLen = (outputLen + 7) / 8;
        for(i=0; (i<outputByteLen) && (i<8); i++)
            printf(" %02x", (int)output[i]);
        if (outputByteLen > 16)
            printf(" ...");
        if (i < (outputByteLen - 8))
            i = outputByteLen - 8;
        for( /* empty */; i<outputByteLen; i++)
            printf(" %02x", (int)output[i]);
        printf("\n\n");
        fflush(stdout);
    }
    #endif

}


static void performTestXoofffWBCAE(unsigned char *checksum)
{
    BitLength dataLen, ADLen, keyLen;

    /* Accumulated test vector */
	Keccak spongeChecksum(SnP_width_sponge, 0);

    #ifdef OUTPUT
    printf("k ");
    #endif
    dataLen = 128*8;
    ADLen = 64*8;
    for(keyLen=0; keyLen<keyBitSize; keyLen = (keyLen < 2*XnP_width) ? (keyLen+1) : (keyLen+8)) {
        performTestXoofffWBCAE_OneInput(keyLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("d ");
    #endif
    ADLen = 64*8;
    keyLen = 16*8;
    for(dataLen=0; dataLen<=dataBitSize-8*expansionLenWBCAE; dataLen = (dataLen < 2*XnP_width) ? (dataLen+1) : (dataLen+7)) {
        performTestXoofffWBCAE_OneInput(keyLen, dataLen, ADLen, spongeChecksum);
    }
    
    #ifdef OUTPUT
    printf("a ");
    #endif
    dataLen = 128*8;
    keyLen = 16*8;
    for(ADLen=0; ADLen<=ADBitSize; ADLen = (ADLen < 2*XnP_width) ? (ADLen+1) : (ADLen+8)) {
        performTestXoofffWBCAE_OneInput(keyLen, dataLen, ADLen, spongeChecksum);
    }
    
	spongeChecksum.squeeze(checksum, 8 * checksumByteSize);

    #ifdef VERBOSE_WBCAE
    {
        unsigned int i;
        printf("Xoofff-WBC-AE\n" );
        printf("Checksum: ");
        for(i=0; i<checksumByteSize; i++)
            printf("\\x%02x", (int)checksum[i]);
        printf("\n\n");
    }
    #endif
}

void selfTestXoofffWBCAE(const char *expected)
{
    unsigned char checksum[checksumByteSize];

#if defined(OUTPUT)
    printf("Testing Xoofff-WBC-AE ");
    fflush(stdout);
#endif
    performTestXoofffWBCAE(checksum);
    assert(memcmp(expected, checksum, checksumByteSize) == 0);
#if defined(OUTPUT)
    printf(" - OK.\n");
#endif
}

#ifdef OUTPUT
void writeTestXoofffWBCAE_One(FILE *f)
{
    unsigned char checksum[checksumByteSize];
    unsigned int offset;

    printf("Writing Xoofff-WBC-AE ");
    performTestXoofffWBCAE(checksum);
    fprintf(f, "    selfTestXoofffWBCAE(\"");
    for(offset=0; offset<checksumByteSize; offset++)
        fprintf(f, "\\x%02x", checksum[offset]);
    fprintf(f, "\");\n");
    printf("\n");
}

void writeTestXoofffWBCAE(const char *filename)
{
    FILE *f = fopen(filename, "w");
    assert(f != NULL);

    #if 0
    {
        BitLength n, nl, prevnl;
        prevnl = 0xFFFFFFFF;
        for (n = 0; n <= 8*64*1024; ++n )
        {
            nl = XoofffWBCAE_Split(n);
            if (nl != prevnl)
            {
                printf("n %6u, nl %6u, nr %6u", n, nl, n - nl);
                if (n >= 2*1536)
                    printf(", 2^x %6u", nl / 1536);
                printf("\n");
                prevnl = nl;
            }
        }
    }
    #endif

    writeTestXoofffWBCAE_One(f);
    fclose(f);
}
#endif

/* ------------------------------------------------------------------------- */
void testXooModes(void)
{
#ifndef KeccakP1600_excluded
#ifdef OUTPUT
//    printXooTestVectors();
    writeTestXoofffSANSE("Xoofff-SANSE.txt");
    writeTestXoofffSANE("Xoofff-SANE.txt");
    writeTestXoofffWBC("Xoofff-WBC.txt");
    writeTestXoofffWBCAE("Xoofff-WBC-AE.txt");
#endif

    selfTestXoofffSANSE("\x06\xed\xf9\xa6\x70\xb3\xfe\x83\x34\x2c\xb4\x18\x75\x0d\xf2\xcc");
    selfTestXoofffSANE("\xf7\xf5\xb8\x84\x08\x96\xf7\xa8\xb5\xfa\x83\x7f\xa0\x90\x0a\x05");
    selfTestXoofffWBC("\x96\x09\x5c\xeb\x82\xa4\x7c\x94\xfc\x90\x42\xd8\xb0\xe3\xc8\xe1");
    selfTestXoofffWBCAE("\x45\x56\x9c\x96\x78\x20\x4b\xd4\xfb\xc0\xfe\xcb\x59\x6c\x85\x56");
#endif
}
