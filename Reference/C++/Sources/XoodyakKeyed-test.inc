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

#define Xoodyak_TagLength			    16
#define Xoodyak_DataSize			    (3*48+1)
#define Xoodyak_MaxKeySize			    (48-4-1)
#define Xoodyak_MaxNonceSize			16

#define testXoodyakKeyed0ne             JOIN(prefix, _testKeyed0ne)
#define testXoodyakKeyed                JOIN(prefix, _testKeyed)

#define Xoodyak_Instance                JOIN(prefix, _Instance)
#define Xoodyak_Initialize              JOIN(prefix, _Initialize)
#define Xoodyak_Absorb                  JOIN(prefix, _Absorb)
#define Xoodyak_Crypt                   JOIN(prefix, _Crypt)
#define Xoodyak_Squeeze                 JOIN(prefix, _Squeeze)
#define Xoodyak_SqueezeKey				JOIN(prefix, _SqueezeKey)

static void testXoodyakKeyed0ne(	Xoodyak &global, FILE * f, 
									const uint8_t *K, unsigned int Klen, const uint8_t *ID, unsigned int IDlen, const uint8_t *N, unsigned int Nlen,
									uint8_t * AD, size_t ADlen, uint8_t *P, size_t Plen, 
									unsigned int nbrMessagesInSession, unsigned int keyVariant, unsigned int ratchet, unsigned int squeezeKLen)
{
	std::unique_ptr<Xoodyak> encrypt;
	std::unique_ptr<Xoodyak> decrypt;
    uint8_t newKey[Xoodyak_MaxKeySize];
    uint8_t newKeyPrime[Xoodyak_MaxKeySize];
    uint8_t tag[Xoodyak_TagLength];
    uint8_t tagPrime[Xoodyak_TagLength];
    uint8_t PPbuffer[Xoodyak_DataSize];
    uint8_t Cbuffer[Xoodyak_DataSize];

    #ifdef OUTPUT
    fprintf( f, "*** Xoodyak( keyVariant %u, ratchet %u, SqueezeKey %u, nbrMessagesInSession %u ), with:\n", keyVariant, ratchet, squeezeKLen, nbrMessagesInSession );
    displayByteString(f, "> K", K, Klen);
    displayByteString(f, "> ID", ID, IDlen);
    displayByteString(f, "> N", N, Nlen);
    displayByteString(f, "> A (metadata)", AD, (unsigned int)ADlen);
    displayByteString(f, "> I (plaintext)", P, (unsigned int)Plen);
    #endif

	switch (keyVariant)
	{
		case 0: /* Absorb nonce in blocks */
		    encrypt = std::make_unique<Xoodyak>(BitString(K, 8 * Klen), BitString(ID, 8 * IDlen), BitString());
			encrypt->Absorb(BitString(N, 8 * Nlen));
			break;
		case 1: /* Absorb counter in trickling mode */
		    encrypt = std::make_unique<Xoodyak>(BitString(K, 8 * Klen), BitString(ID, 8 * IDlen), BitString(N, 8 * Nlen));
			break;
	}

	switch (keyVariant)
	{
		case 0: /* Absorb nonce in blocks */
		    decrypt = std::make_unique<Xoodyak>(BitString(K, 8 * Klen), BitString(ID, 8 * IDlen), BitString());
			decrypt->Absorb(BitString(N, 8 * Nlen));
			break;
		case 1: /* Absorb counter in trickling mode */
		    decrypt = std::make_unique<Xoodyak>(BitString(K, 8 * Klen), BitString(ID, 8 * IDlen), BitString(N, 8 * Nlen));
			break;
	}

	for ( /* empty */; nbrMessagesInSession != 0; --nbrMessagesInSession) {
		if (squeezeKLen != 0) {
			BitString keyString = encrypt->SqueezeKey(squeezeKLen);
			if (keyString.size() != 0) std::copy(keyString.array(), keyString.array() + (keyString.size() + 7) / 8, newKey);
		}

		encrypt->Absorb(BitString(AD, 8 * ADlen));
		BitString cryptString = encrypt->Encrypt(BitString(P, 8 * Plen));
		if (cryptString.size() != 0) std::copy(cryptString.array(), cryptString.array() + (cryptString.size() + 7) / 8, Cbuffer);
		if (ratchet == 1) { /* ratchet before squeeze */
			encrypt->Ratchet();
		}
		BitString tagString = encrypt->Squeeze(Xoodyak_TagLength);
		if (tagString.size() != 0) std::copy(tagString.array(), tagString.array() + (tagString.size() + 7) / 8, tag);
		if (ratchet == 2) { /* ratchet after squeeze */
			encrypt->Ratchet();
		}

	    #ifdef OUTPUT
	    displayByteString(f, "< O (ciphertext)", Cbuffer, (unsigned int)Plen);
	    displayByteString(f, "< T (tag)", tag, Xoodyak_TagLength);
		if (squeezeKLen != 0) {
		    displayByteString(f, "< K (new key)", newKey, squeezeKLen);
		}
	    fprintf(f, "\n");
	    #endif

		if (squeezeKLen != 0) {
			BitString keyString = decrypt->SqueezeKey(squeezeKLen);
			if (keyString.size() != 0) std::copy(keyString.array(), keyString.array() + (keyString.size() + 7) / 8, newKeyPrime);
		}
		decrypt->Absorb(BitString(AD, 8 * ADlen));
	    cryptString = decrypt->Decrypt(BitString(Cbuffer, 8 * Plen));
		if (cryptString.size() != 0) std::copy(cryptString.array(), cryptString.array() + (cryptString.size() + 7) / 8, PPbuffer);
		if (ratchet == 1) { /* ratchet before squeeze */
			decrypt->Ratchet();
		}
		tagString = decrypt->Squeeze(Xoodyak_TagLength);
		if (tagString.size() != 0) std::copy(tagString.array(), tagString.array() + (tagString.size() + 7) / 8, tagPrime);
		if (ratchet == 2) { /* ratchet after squeeze */
			decrypt->Ratchet();
		}

	    assert(!memcmp( newKey, newKeyPrime, squeezeKLen ), "The new keys do not match.");
	    assert(!memcmp( P, PPbuffer, Plen ), "The plaintexts do not match.");
	    assert(!memcmp( tag, tagPrime, Xoodyak_TagLength ), "The tags do not match.");

		if (squeezeKLen != 0) {
		    global.Absorb(BitString(newKey, 8 * squeezeKLen));
		}
	    global.Absorb(BitString(Cbuffer, 8 * Plen));
	    global.Absorb(BitString(tag, 8 * Xoodyak_TagLength));
	}

}

static int testXoodyakKeyed( const char *file, const uint8_t *expected )
{
    unsigned int Klen;
    unsigned int IDlen;
    unsigned int Nlen;
    unsigned int ratchet;
	unsigned int keyVariant;
	unsigned int nbrMessagesInSession;
    unsigned int Mlen;
    unsigned int Alen;
    unsigned int Mleni;
    unsigned int Aleni;
    unsigned int newKlen;
    uint8_t checksum[32];
    Xoodyak global = Xoodyak(BitString(), BitString(), BitString());
    uint8_t K[Xoodyak_MaxKeySize];
    uint8_t N[Xoodyak_MaxNonceSize];
    uint8_t Pbuffer[Xoodyak_DataSize];
    uint8_t Abuffer[Xoodyak_DataSize];
    uint8_t *ID;
    uint8_t c;
    FILE *f = NULL;

    #ifdef OUTPUT
    f = fopen(file, "w");
    assert(f != NULL, "Could not open file");
    PRINTS("Xoodyak keyed ");
    #endif

	nbrMessagesInSession = 1;
    for( keyVariant = 0; keyVariant < 2; ++keyVariant)
    for( ratchet = 0; ratchet < 3; ++ratchet) /* 0, no ratchet, 1 ratchet before squeeze, 2 ratchet after squeeze */
    for( newKlen = 0; newKlen <= Xoodyak_MaxKeySize; newKlen += 16)
    for( Klen=16; Klen<=Xoodyak_MaxKeySize; Klen++)
    for( Nlen=0; Nlen<=Xoodyak_MaxNonceSize; Nlen += (Klen == 16) ? 1 : Xoodyak_MaxNonceSize)
    {
        generateSimpleRawMaterial(K, Klen, (uint8_t)(Klen+Nlen+0x12), 3),
        generateSimpleRawMaterial(N, Nlen, (uint8_t)(Klen+Nlen+0x45), 6),
        generateSimpleRawMaterial(&c, 1, (uint8_t)(Klen+Nlen+0x78), 11),

		IDlen = ((Klen <= 16) || (keyVariant == 2)) ? 0 : (c % (Klen - (16 - 1)));
		ID = K + Klen - IDlen;
        testXoodyakKeyed0ne(global, f, K, Klen - IDlen, ID, IDlen, N, Nlen, (uint8_t*)"ABC", 3, (uint8_t*)"DEF", 3, nbrMessagesInSession, keyVariant, ratchet, newKlen);
    }

    {
        unsigned int Alengths[5] = { 0, 1, 48-4-1, 48-4, 48-4+1 };

		newKlen = 0;
        Klen = 16;
        Nlen = 16;
		keyVariant = 0;
        for(ratchet = 0; ratchet < 3; ++ratchet)
	    for(nbrMessagesInSession = 1; nbrMessagesInSession <= 3; ++nbrMessagesInSession)
        for(Aleni=0; Aleni<5; Aleni++)
        for(Mlen = 0; Mlen <= Xoodyak_DataSize; Mlen+=(Aleni==0)?1:(ratchet*4+1))
        {
            Alen = Alengths[Aleni];

            generateSimpleRawMaterial(K, Klen, (uint8_t)(0x23+Mlen+Alen), 4),
            generateSimpleRawMaterial(N, Nlen, (uint8_t)(0x56+Mlen+Alen), 7),
            generateSimpleRawMaterial(Abuffer, Alen, (uint8_t)(0xAB+Mlen+Alen), 3),
            generateSimpleRawMaterial(Pbuffer, Mlen, (uint8_t)(0xCD+Mlen+Alen), 4),
	        testXoodyakKeyed0ne(global, f, K, Klen, NULL, 0, N, Nlen, Pbuffer, Mlen, Abuffer, Alen, nbrMessagesInSession, keyVariant, ratchet, newKlen);
        }
    }

    {
        unsigned int Mlengths[5] = { 0, 1, 24-1, 24, 24+1 };

		newKlen = 0;
        Klen = 16;
        Nlen = 16;
		keyVariant = 0;
        for(ratchet = 0; ratchet < 3; ++ratchet)
	    for(nbrMessagesInSession = 1; nbrMessagesInSession <= 3; ++nbrMessagesInSession)
        for(Mleni=0; Mleni<5; Mleni++)
        for(Alen=0; Alen<=Xoodyak_DataSize; Alen+=(Mleni==0)?1:(ratchet*4+1))
        {
            Mlen = Mlengths[Mleni];

            generateSimpleRawMaterial(K, Klen, (uint8_t)(0x34+Mlen+Alen), 5),
            generateSimpleRawMaterial(N, Nlen, (uint8_t)(0x45+Mlen+Alen), 6),
            generateSimpleRawMaterial(Abuffer, Alen, (uint8_t)(0x01+Mlen+Alen), 5),
            generateSimpleRawMaterial(Pbuffer, Mlen, (uint8_t)(0x23+Mlen+Alen), 6),
	        testXoodyakKeyed0ne(global, f, K, Klen, NULL, 0, N, Nlen, Pbuffer, Mlen, Abuffer, Alen, nbrMessagesInSession, keyVariant, ratchet, newKlen);
        }
    }

    {
		BitString checksumString = global.Squeeze(sizeof(checksum));
		if (checksumString.size() != 0) std::copy(checksumString.array(), checksumString.array() + (checksumString.size() + 7) / 8, checksum);
        #ifdef OUTPUT
        displayByteString(f, "+++ Global checksum", checksum, sizeof(checksum));
        fclose(f);
        #endif
        assert(!memcmp(expected, checksum, sizeof(checksum)), "The global checksum is incorrect.");
    }

    #ifdef OUTPUT
    PRINTS("v\n");
    #endif

    return 0;
}

#undef W
#undef Ra
#undef Rs

#undef testXoodyakKeyed0ne
#undef testXoodyakKeyed

#undef Xoodyak_Instance
#undef Xoodyak_Initialize
#undef Xoodyak_Absorb
#undef Xoodyak_Crypt
#undef Xoodyak_Squeeze
#undef Xoodyak_SqueezeKey
