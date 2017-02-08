#ifndef RGEN_H
#define RGEN_H
#include <openssl/blowfish.h>

#include <stdint.h> /* A c99 feature to get uint64_t */
/**
 * This is a pseudorandom number generator. It is based on blowfish
 * block cipher: we take a seed, make a key of it and start producing
 * byte blocks by ciphering 0,1,2,3,.....,(2^64-1).
 *
 * To tell the truth, there are rumours that this is a good secure
 * PRNG.
 *
 * To setup BLOWFISH key a string is used. The string is mixed with
 * 1,2,3...255 bytes one after one and passed via SHA1 before becoming
 * a key: SHA1 is used to protect passphrase from revealing key, bytes
 * are added because I like it and this don't decrease randomness.
 *
 * e.g. qwerty -> key=sha1("q\1w\2e\3r\4t\5y\6\0"), and this is cycled
 * down to 1 if password is more than 255 symbols in length.
 **/

#define RGEN_BLOCK (8)

struct rgen {
	uint64_t N; /* counter */
	BF_KEY key; /* key */
	/* Stream : (1-2-3-4-5-6-7-8) (1-2-3-4-5-6-7-8) (1-2-3-4-5-6-7-8) ....... */
	/* Here thing in brackets is a block of data from cipher */
	unsigned char queue[RGEN_BLOCK];
	uint8_t bytes_in_queue; /* `bytes_in_queue` bytes, last of which
	* stands at last buffer position, are a part of pseudorandom stream
	* and shall be logically prepended before yet unproduced cipher blocks
	*/
};

/**
 * Initialises given rgen with string `str` as a seed.
 * The whole might crash if `str` length is somewhat above
 * stack size / 2 .
 *
 * @param str - null-terminated string to initialise from
 */
void rgen_init(struct rgen * obj, const char * str);

/**
 * Frees object: performs blanking of key and N (somewhat about for
 * security).
 */
void rgen_free(struct rgen * O);

/**
 * Produces requested number of pseudorandom bytes
 * @param bytes - number of bytes to acquire
 * @param out - buffer capable for holding `bytes` bytes of data
 */
void rgen_produce_nbytes(struct rgen * obj, unsigned int bytes, unsigned char * out);

/**
 * Produces uniformely distributed radnom value by consuming
 * internal pseudorandom stream.
 * @param a,b, a<=b - inclusive borders
 * @return pseudorandom value
 */
uint64_t rgen_uniform(struct rgen * obj, uint64_t a, uint64_t b);

/**
 * Generates a random permuation aka shuffle of size N. The permutation
 * is output as a pointer to array of numbers from set {1..N} with no
 * duplicates. The caller shall free the aray.
 * For N=0 returns NULL.
 *
 * This implements Durstenfeld aka Fisher-Yates algorythm.
 * @param N - size of permutation to generate
 * @return pointer to array of N elements or NULL if out of memory
 */
void rgen_shuffle(struct rgen * obj, uint32_t* values, unsigned int N);

#endif
