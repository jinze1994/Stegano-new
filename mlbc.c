#include "mlbc.h"
#include "matrix.h"
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

static const uint8_t G1A[K][N] = {
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,1},
	{0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,1,0,0,0,0},
	{0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,1,1,1,1,0,0,1},
};

static const uint8_t G0A[L][N] = {
	{1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,1,1,0,1,1,0,0,0},
	{1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,0},
	{0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,1,1,1,1},
	{0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,1,0,0,0,1,0,1,1,0,0,1},
	{0,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,1,1,1,1,0,0,0,0,0},
	{1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1},
	{0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,1,1,0,0,0},
	{0,1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,1,0,0,1,1,0},
	{0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,0,0,0,1,0,0,1,0},
	{1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,1,0,0,1,1},
	{1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,0,1,0,0,0,1,0,1},
};

static const uint8_t HtA[N][R] = {
	{0,1,1,1,1,1,1,1,0,0,0,0,1},
	{1,1,0,1,1,0,0,0,1,0,0,0,0},
	{1,0,0,1,0,1,1,1,1,1,0,0,1},
	{1,0,1,0,1,1,1,0,1,0,0,0,0},
	{0,0,1,0,0,0,0,0,0,1,0,0,1},
	{1,1,0,0,1,0,1,0,1,1,1,1,1},
	{0,1,1,0,0,0,1,0,1,1,0,0,1},
	{0,1,1,0,0,0,0,0,0,1,0,0,1},
	{1,0,1,0,0,1,0,0,1,0,0,0,0},
	{0,0,1,0,0,0,1,0,1,1,0,0,0},
	{0,1,0,1,1,1,0,1,1,0,1,1,0},
	{1,1,0,1,1,0,0,0,1,0,0,1,0},
	{1,0,1,0,0,1,0,0,0,0,0,1,0},
	{1,0,0,1,0,0,1,0,1,1,1,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,1,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,1,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,1,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,1,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,1,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,1,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,1,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,1,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,1},
};

static const uint8_t JtA[N][K] = {
	{1,0,0},
	{0,1,0},
	{0,0,1},
	{1,1,1},
	{1,0,0},
	{0,1,0},
	{0,0,0},
	{0,1,1},
	{1,1,0},
	{0,0,0},
	{0,1,0},
	{0,0,0},
	{1,1,0},
	{1,0,1},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
};

static const struct Matrix* G1;
static const struct Matrix* G0;
static const struct Matrix* Ht;
static const struct Matrix* Jt;

void initMLBC() {
	G1 = newMatrixA(K, N, (const uint8_t*)G1A);
	G0 = newMatrixA(L, N, (const uint8_t*)G0A);
	Ht = newMatrixA(N, R, (const uint8_t*)HtA);
	Jt = newMatrixA(N, K, (const uint8_t*)JtA);
}

void destroyMLBC() {
	destroyMatrix((struct Matrix*)G1);
	destroyMatrix((struct Matrix*)G0);
	destroyMatrix((struct Matrix*)Ht);
	destroyMatrix((struct Matrix*)Jt);
}

void toBin(const uint8_t* src, size_t src_len, uint8_t* dst, size_t* dst_len) {
	for (int i = (int)src_len-1; i >= 0; i--)
		for (int j = 7; j >= 0; j--)
			dst[i * 8 + j] = (src[i] >> j) & 1;
	if (dst_len)
		*dst_len = src_len * 8;
}

static void encodeMessage(const uint8_t in_buf[K], uint8_t out_buf[N], const uint8_t stream[N]) {

	stream = stream;
	memcpy(out_buf, in_buf, K);
	memset(out_buf + K, 0, N - K);
}

static void printEmbedInfo(size_t message_len, size_t sha1_len,
		size_t len_bit, size_t message_bit, size_t sha1_bit,
		size_t data_bit, size_t mlbc_cnt, size_t data_N_bit_len) {

	printf("message_len: %lu\tsha1_len: %lu\n",
			message_len, sha1_len);
	printf("len_bit: %lu\tmess_bit: %lu\tsha1_bit: %lu\n",
			len_bit, message_bit, sha1_bit);
	printf("data_K_bit: %lu\tMLBC_cnt: %lu\tdata_N_bit: %lu\n",
			data_bit, mlbc_cnt, data_N_bit_len);
}

int encodeLongMessage(const uint8_t* message, uint8_t message_len,
		const uint8_t* stream, size_t stream_len,
		uint8_t** dataToHidePtr, size_t *data_len_ptr) {

	size_t sha1_len = 2;
	size_t data_len = 8*K + message_len*8 + sha1_len*8;
	while (data_len % K)
		data_len++;

	if (data_len / K * N >= stream_len || data_len >= 300 * 8) return 10;

	uint8_t* dataToHide = malloc(data_len / K * N);
	if (dataToHide == NULL) return 20;

	for (int i = 0; i < 8; i++)
		for (int j = 0; j < K; j++)
			dataToHide[i*K+j] = (message_len >> i) & 1;
	toBin(message, message_len, dataToHide + 8*K, NULL);

	uint8_t sha1[SHA_DIGEST_LENGTH];
	SHA1((const unsigned char*)message, message_len, sha1);
	toBin(sha1, sha1_len, dataToHide + 8*K + message_len*8, NULL);

	data_len = 8*K + message_len*8 + sha1_len*8;
	while (data_len % K)
		dataToHide[data_len++] = 0;
	data_len /= K;

	for (int i = (int)data_len-1; i >= 0; i--) {
		uint8_t in_buf[K], out_buf[N];
		memcpy(in_buf, dataToHide + i*K, K);
		encodeMessage(in_buf, out_buf, stream + i*N);
		memcpy(dataToHide + i*N, out_buf, N);
	}
	data_len *= N;

	printEmbedInfo(message_len, sha1_len,
			8 * K, message_len * 8, sha1_len * 8,
			data_len/N*K, data_len/N, data_len);

	*dataToHidePtr = dataToHide;
	*data_len_ptr = data_len;
	return 0;
}
