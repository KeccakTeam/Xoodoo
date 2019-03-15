/*
Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/


#define JOIN0(a, b)                     a ## b
#define JOIN(a, b)                      JOIN0(a, b)

#define testXoodyakHashOne              JOIN(prefix, _testHashOne)
#define testXoodyakHash                 JOIN(prefix, _testHash)

#define Xoodyak_Instance                JOIN(prefix, _Instance)
#define Xoodyak_Initialize              JOIN(prefix, _Initialize)
#define Xoodyak_Absorb                  JOIN(prefix, _Absorb)
#define Xoodyak_Squeeze                 JOIN(prefix, _Squeeze)

#define	MAX_NUMBER_MESSAGES	3u
#define	MAX_MESSAGE_LEN		(3u*48u+1u)
#define	MAX_HASH_LEN		(3u*48u+1u)
#define	TYPICAL_HASH_LEN	32u

static void testXoodyakHashOne( Xoodyak &global, FILE *f, size_t messageLen, size_t hashLen, unsigned int numberOfMessages )
{
	Xoodyak				instance = Xoodyak(BitString(), BitString(), BitString());
	uint8_t				hashBuffer[MAX_HASH_LEN];
	uint8_t				messageBuffer[MAX_MESSAGE_LEN];
	unsigned int		i;

	for (i = 0; i < numberOfMessages; ++i)	{
        generateSimpleRawMaterial(messageBuffer, messageLen, (uint8_t)(messageLen+hashLen+0x12), 3);
	    #ifdef OUTPUT
	    fprintf( f, "Absorb" );
	    displayByteString(f, "> M", messageBuffer, messageLen);
	    #endif
	    instance.Absorb(BitString(messageBuffer, 8 * messageLen));
	}
    BitString hashBufferString = instance.Squeeze(hashLen);
    if (hashBufferString.size() != 0) std::copy(hashBufferString.array(), hashBufferString.array() + (hashBufferString.size() + 7) / 8, hashBuffer);
    #ifdef OUTPUT
    fprintf( f, "Squeeze" );
    displayByteString(f, "> H", hashBuffer, hashLen);
    #endif
    global.Absorb(BitString(hashBuffer, 8 * hashLen));

}

static void testXoodyakHash( const char *file, const uint8_t *expected )
{
	Xoodyak             global = Xoodyak(BitString(), BitString(), BitString());
	uint8_t				checksum[TYPICAL_HASH_LEN];
    FILE                *f = NULL;
	size_t              messageLen;
	size_t              hashLen;
	unsigned int        numberOfMessages;

    #ifdef OUTPUT
    f = fopen(file, "w");
    assert(f != NULL, "Could not open file");
    PRINTS("Xoodyak hash ");
    #endif

	hashLen = TYPICAL_HASH_LEN;
	for (numberOfMessages = 1u; numberOfMessages < MAX_NUMBER_MESSAGES; ++numberOfMessages) {
		for (messageLen = 0u; messageLen < MAX_MESSAGE_LEN; ++messageLen) {
			testXoodyakHashOne( global, f, messageLen, hashLen, numberOfMessages );
		}
	}

	messageLen = MAX_MESSAGE_LEN;
	numberOfMessages = 1u;
	for (hashLen = 1u; hashLen < MAX_HASH_LEN; ++hashLen) {
		testXoodyakHashOne( global, f, messageLen, hashLen, numberOfMessages );
	}

    BitString checksumString = global.Squeeze(sizeof(checksum));
    if (checksumString.size() != 0) std::copy(checksumString.array(), checksumString.array() + (checksumString.size() + 7) / 8, checksum);
    #ifdef OUTPUT
    displayByteString(f, "+++ Global checksum", checksum, sizeof(checksum));
    fclose(f);
    #endif
    assert(!memcmp(expected, checksum, sizeof(checksum)), "The global checksum is incorrect.");

    #ifdef OUTPUT
    PRINTS("v\n");
    #endif

}

#undef Xoodyak_Instance
#undef Xoodyak_Initialize
#undef Xoodyak_Absorb
#undef Xoodyak_Squeeze

#undef testXoodyakHashOne
#undef testXoodyakHash
