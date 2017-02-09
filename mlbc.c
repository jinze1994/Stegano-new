#include "mlbc.h"
#include "matrix.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

static void ChartoBin(const uint8_t* src, size_t src_len, uint8_t* dst, size_t* dst_len) {
	for (int i = (int)src_len-1; i >= 0; i--)
		for (int j = 7; j >= 0; j--)
			dst[i * 8 + j] = (src[i] >> j) & 1;
	if (dst_len)
		*dst_len = src_len * 8;
}

static void printEmbedInfo(size_t rge_len, size_t mess_len,
		size_t len_bit, size_t rge_bit, size_t mess_bit,
		size_t data_bit, size_t mlbc_cnt, size_t data_N_bit_len) {

	printf("rge_len: %lu\tmess_len: %lu\n",
			rge_len, mess_len);
	printf("len_bit: %lu\trge_bit: %lu\tmess_bit: %lu\n",
			len_bit, rge_bit, mess_bit);
	printf("data_K_bit: %lu\tMLBC_cnt: %lu\tdata_N_bit: %lu\n",
			data_bit, mlbc_cnt, data_N_bit_len);
}

static void minimizeWetBits(struct Matrix* stream, struct Matrix* origMessage) {
}

static void encodeMessage(const uint8_t in_buf[K], uint8_t out_buf[N], const uint8_t streamA[N]) {

	struct Matrix* stream = newMatrixA(1, N, streamA);
	struct Matrix* message = newMatrixA(1, K, in_buf);
	struct Matrix* encodedMessage = matrixMul(message, G1);

	minimizeWetBits(stream, encodedMessage);

	destroyMatrix(message);
	destroyMatrix(stream);
	destroyMatrix(encodedMessage);

	stream = stream;
	memcpy(out_buf, in_buf, K);
	memset(out_buf + K, 0, N - K);
}

static void decodeMessage(const uint8_t in_buf[N], uint8_t out_buf[K]) {
	memcpy(out_buf, in_buf, K);
	for (int i = K; i < N; i++)
		assert(in_buf[i] == 0);
}

int encodeLongMessage(const uint8_t* message, uint8_t message_len,
		const uint8_t* stream, size_t stream_len,
		struct rgen* rge,
		uint8_t** dataToHidePtr, size_t *data_len_ptr) {

	size_t rge_len = K - (message_len % K);
	size_t data_len = 8*K + rge_len*8 + message_len*8;
	assert(data_len % K == 0);
	printEmbedInfo(rge_len, message_len,
			8 * K, rge_len * 8, message_len * 8,
			data_len, data_len/K, data_len/K*N);

	if (data_len / K * N >= stream_len || data_len >= 300 * 8) return 10;

	uint8_t* dataToHide = malloc(data_len / K * N);
	if (dataToHide == NULL) return 20;

	for (int i = 0; i < 8; i++)
		for (int j = 0; j < K; j++)
			dataToHide[i*K+j] = (message_len >> i) & 1;

	uint8_t rge_data[rge_len];
	rgen_produce_nbytes(rge, rge_len, rge_data);
	ChartoBin(rge_data, rge_len, dataToHide + 8*K, NULL);

	ChartoBin(message, message_len, dataToHide + 8*K + rge_len*8, NULL);

	data_len /= K;
	for (int i = (int)data_len-1; i >= 0; i--) {
		uint8_t in_buf[K], out_buf[N];
		memcpy(in_buf, dataToHide + i*K, K);
		encodeMessage(in_buf, out_buf, stream + i*N);
		memcpy(dataToHide + i*N, out_buf, N);
	}
	data_len *= N;

	*dataToHidePtr = dataToHide;
	*data_len_ptr = data_len;
	return 0;
}

int decodeLongMessage(const uint8_t* stream, size_t stream_len,
		struct rgen* rge, uint8_t* message) {

	*message = 0;
	int message_len = 0;
	for (int i = 0; i < 8; i++) {
		uint8_t out_buf[K];
		decodeMessage(stream + i * N, out_buf);
		size_t cnt1 = 0;
		for (int j = 0; j < K; j++)
			if (out_buf[j])
				cnt1++;
		uint8_t r = cnt1 > K / 2 ? 1 : 0;
		message_len |= r << i;
	}
	stream += 8 * N;

	int rge_len = K - (message_len % K);
	size_t data_len = 8*K + rge_len*8 + message_len*8;
	if (stream_len <= data_len / K * N) return 40;

	uint8_t* buf = malloc((rge_len + message_len) * 8);
	if (buf == NULL) return 20;

	int rest_mlbc_cnt = (rge_len + message_len) * 8 / K;
	for (int i = 0; i < rest_mlbc_cnt; i++)
		decodeMessage(stream + i * N, buf + i * K);

	// Bin2Chars
	for (int i = 0; i < rge_len + message_len; i++) {
		uint8_t ch = 0;
		for (int j = 0; j < 8; j++)
			ch |= (buf[i * 8 + j] << j);
		buf[i] = ch;
	}

	uint8_t rge_data[rge_len];
	rgen_produce_nbytes(rge, rge_len, rge_data);

	if (memcmp(rge_data, buf, rge_len)) {
		free(buf);
		return 40;
	}

	memcpy(message, buf + rge_len, message_len);
	message[message_len] = 0;

	free(buf);
	printEmbedInfo(rge_len, message_len,
			8 * K, rge_len * 8, message_len * 8,
			data_len, data_len/K, data_len/K*N);
	return 0;
}
