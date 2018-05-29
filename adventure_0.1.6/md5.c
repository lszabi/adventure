/* MD5 hash in C and x86 assembly
 * Copyright (c) 2012 Nayuki Minase
 * Modified by L Szabi 2013
 * http://nayuki.eigenstate.org/page/fast-md5-hash-implementation-in-x86-assembly
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"

// Link this program with an external C or x86 compression function
extern void md5_compress(uint32_t *state, uint32_t *block);

// Full message hasher
void md5_hash(uint8_t *message, uint8_t *hash_str) {
	uint32_t hash[4];
	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;
	int len = strlen((const char *)message);
	int i;
	for (i = 0; i + 64 <= len; i += 64) {
		md5_compress(hash, (uint32_t*)(message + i));
	}
	uint32_t block[16];
	uint8_t *byteBlock = (uint8_t*)block;
	int rem = len - i;
	memcpy(byteBlock, message + i, rem);
	byteBlock[rem] = 0x80;
	rem++;
	if (64 - rem >= 8) {
		memset(byteBlock + rem, 0, 56 - rem);
	} else {
		memset(byteBlock + rem, 0, 64 - rem);
		md5_compress(hash, block);
		memset(block, 0, 56);
	}
	block[14] = len << 3;
	block[15] = len >> 29;
	md5_compress(hash, block);
	for ( i = 0; i < 4; i++ ) {
		hash_str[4 * i] = hash[i] & 0xFF;
		hash_str[4 * i + 1] = ( hash[i] >> 8 ) & 0xFF;
		hash_str[4 * i + 2] = ( hash[i] >> 16 ) & 0xFF;
		hash_str[4 * i + 3] = ( hash[i] >> 24 ) & 0xFF;
	}
}
