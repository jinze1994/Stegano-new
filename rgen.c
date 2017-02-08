#include "rgen.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

void rgen_init(struct rgen * obj, const char * str) {
	size_t len = strlen(str);
	unsigned char str_rz[len*2 + 1];
	str_rz[len*2] = 0; /* Stop */
	size_t ksi;
	unsigned char df=1;
	for(ksi = 0; ksi < len; ksi++) {
		str_rz[ksi*2]=str[ksi];
		str_rz[ksi*2+1] = df;
		/* 1->2, 2->3, ..., 254->255, 255->1 */
		if(255 == df)
			df = 1;
		else
			df += 1;
	}
	unsigned char sha1digest[SHA_DIGEST_LENGTH];
	SHA1(str_rz, len*2+1, sha1digest);
	BF_set_key(& obj -> key, SHA_DIGEST_LENGTH, sha1digest);
	/* Memsetting just in case */
	memset(str_rz, 0, len*2+1);
	memset(sha1digest, 0, SHA_DIGEST_LENGTH);
	/* And, of cause */
	obj -> N = 0;
	obj -> bytes_in_queue = 0;
}

void rgen_free(struct rgen * O) {
	memset(& O -> key, 0, sizeof(BF_KEY));
	O -> N = 0;
}

/**
 * Produces 8 bytes of pseudorandom data
 * @param out - place to put data to, must have space for 8 bytes
 */
static void rgen_produce(struct rgen * obj, unsigned char * out) {
	unsigned char in[RGEN_BLOCK];
	int j;
	for(j = 0; j < RGEN_BLOCK; j++)
		in[j] = (obj->N >> (8*j)) & 0xff;
	BF_ecb_encrypt(in, out, &obj->key, BF_ENCRYPT);
	/* Next block will be produced by ciphering increased 64bit integer N */
	obj->N ++;
}

void rgen_produce_nbytes(struct rgen * obj, unsigned int bytes, unsigned char * out) {
	if(bytes == 0)
		return; /* Nothing to do */
	if(bytes <= obj -> bytes_in_queue) {
		/* We have enough bytes in queue */
		memcpy(out,
			obj -> queue + (RGEN_BLOCK-obj->bytes_in_queue),
			bytes);
		obj -> bytes_in_queue -= bytes;
		return;
	}
	/* need more bytes than in buffer */
	unsigned int written = 0;
	if(obj -> bytes_in_queue) {
		memcpy(out,
				obj -> queue + (RGEN_BLOCK-obj->bytes_in_queue),
				obj -> bytes_in_queue);
		written += obj -> bytes_in_queue;
	}
	/* Now queue is empty */
	unsigned int nblocks = (bytes-written)/RGEN_BLOCK;
	/* We need to write this number of full blocks */
	unsigned int ksi;
	for(ksi = 0; ksi < nblocks; ksi++) {
		rgen_produce(obj, out + written);
		written += RGEN_BLOCK;
	}
	/* One last, unfull block left */
	rgen_produce(obj, obj -> queue);
	obj -> bytes_in_queue = RGEN_BLOCK;
	memcpy(out + written, obj -> queue, bytes - written);
	obj -> bytes_in_queue -= bytes-written; /* these bytes used */
	/* written += bytes - written -> TADA, we are ready */
}

/**
 * For given non-zero unsigned integer finds a bit such that the integer has
 * no more higher bits different from zero, but the returned bit is not a zero
 * in given number.
 * For zero returns zero.
 * e.g. 00000000 00000000 00000000 00000000 00000000 00000000 00010010 00010000
 * shall produce 12.
 *
 * @param i - integer to study
 * @return number with one bit set to 1, which describes the higher non-zero bit
 * of the studied number.
 */
static uint8_t find_upbit(uint64_t i){
	/* We are using /2 division algorythm */
	uint8_t upper, lower;
	upper = 63; /* searching from lower to upper including ends */
	lower = 0; /* initial search diapason */
	/* at entering search we assume that there is some difference between
	upper and lower */
	do {
		uint8_t middle = (upper-lower)/2 + lower;
		uint64_t mask = ~0;
		mask = mask << (middle + 1);
		if(mask & i)
			lower = middle + 1; /* something upper than middle bit */
		else
			upper = middle; /* nothing above middle */
	} while(lower != upper);
	return upper;
}

uint64_t rgen_uniform(struct rgen * obj, uint64_t a, uint64_t b) {
	if(a==b)
		return a;
	uint64_t diff = b - a;
	/* Let's generate a number form 0 up to diff */
	uint8_t bit = find_upbit(diff);
	uint8_t need_bits = bit+1; /* We need so many bits from random source */
	uint8_t need_bytes = need_bits / 8;
	if(need_bits % 8)
		need_bytes += 1;

	unsigned char bytes[8];
	uint64_t rval;
	do {
		rgen_produce_nbytes(obj, need_bytes, bytes);
		uint8_t ksi;
		rval = 0;
		for(ksi = 0; ksi < need_bytes; ksi++)
			rval |= (uint64_t)bytes[ksi] << 8*ksi;

		/* applying mask */
		uint64_t mask = ~ (0xffffffffffffffffLL << need_bits);
		rval &= mask;
	} while(! (rval <= diff));/* generating numbers until our number is not in diapason */
	return rval + a;
}

/* Make sure 'int' is smaller than 64-bit (futuristic)*/
#if UINT_MAX > 0xffffffffffffffff
	#error "unsigned integer is bigger than 64 bits, rgen_uniform operate on 64-bit integers: original assumptions fail"
#endif
void rgen_shuffle(struct rgen * obj, uint32_t* values, unsigned int N) {
	unsigned int ksi;
	for(ksi = N-1; ksi > 0; ksi--) {
		/* for all positions from max down to second from beginning
		 * inclusive
		 **/
		/* ksi is an index, above which elements of permutation have
		 * allready been chosen and will be not modified
		 **/
		unsigned int choice = (unsigned int
			/*we are sure that's OK, see #if above this function*/
			)rgen_uniform(obj, 0, ksi);
		unsigned int e1, e2; /* Things to exchange */
		e1 = choice;
		e2 = ksi;
		{/* exchange e1 and e2 */
			unsigned int tmp;
			tmp = values[e1];
			values[e1] = values[e2];
			values[e2] = tmp;
		}
	}
}
