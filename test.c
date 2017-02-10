#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "matrix.h"

#define N (7)
#define K (2)
#define L (1)
#define R (N-K-L)

static const uint8_t G1A[K][N] = {
	{1, 0, 0, 1, 1, 1, 1},
	{0, 1, 0, 1, 1, 0, 0},
};

static const uint8_t G0A[L][N] = {
	{1, 1, 1, 1, 0, 1, 0},
};

static const uint8_t HtA[N][R] = {
	{1, 1, 1, 1},
	{1, 1, 0, 0},
	{1, 0, 0, 1},
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1},
};

static const uint8_t JtA[N][K] = {
	{1, 0},
	{0, 1},
	{1, 1},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};

static struct Matrix G1;
static struct Matrix G0;
static struct Matrix Ht;
static struct Matrix Jt;

void initMLBC() {
	initMatrix(&G1, K, N, (uint8_t*)G1A);
	initMatrix(&G0, L, N, (uint8_t*)G0A);
	initMatrix(&Ht, N, R, (uint8_t*)HtA);
	initMatrix(&Jt, N, K, (uint8_t*)JtA);
}

void destroyMLBC() {
	destroyMatrix(&G1);
	destroyMatrix(&G0);
	destroyMatrix(&Ht);
	destroyMatrix(&Jt);
}

static void ChartoBin(const uint8_t* src, size_t src_len, uint8_t* dst, size_t* dst_len) {
	for (int i = (int)src_len-1; i >= 0; i--)
		for (int j = 7; j >= 0; j--)
			dst[i * 8 + j] = (src[i] >> j) & 1;
	if (dst_len)
		*dst_len = src_len * 8;
}

#if L >= 64
	#error "L must <= 64"
#endif
static void minimizeWetBits(struct Matrix* origMessage, const uint8_t stream[N], uint8_t out_buf[N]) {
	uint8_t v[64];
	memset(v, 0, sizeof(v));

	size_t minStuck = INT_MAX;
	struct Matrix mv; initMatrix(&mv, 1, L, v);
	struct Matrix x;  initMatrix(&x, 1, N, NULL);
	for (uint64_t vint = 0; vint < 1<<L; vint++) {
		ChartoBin((uint8_t*)&vint, L/8+(L%8?1:0), v, NULL);
		matrixMul(&mv, &G0, &x);
		matrixAdd(&x, origMessage);

		size_t stuckBitsMissed = 0;
		for (int i = 0; i < N; i++)
			if (stream[i] == 0 && x.buf[i] != stream[i])
				stuckBitsMissed++;

		if (stuckBitsMissed < minStuck) {
			minStuck = stuckBitsMissed;
			memcpy(out_buf, x.buf, N);
		}

	}
	destroyMatrix(&mv);
	destroyMatrix(&x);
}

static void solveConstraintsMinimally(struct Matrix* z, struct Matrix* S) {
	size_t minHamm = INT_MAX;
	uint8_t res[N];
	memset(res, 0, sizeof(res));

	uint8_t zz[64];
	memset(zz, 0, sizeof(zz));
	struct Matrix tmp; initMatrix(&tmp, 1, R, NULL);
	for (uint64_t zint = 0; zint < 1<<N; zint++) {
		ChartoBin((uint8_t*)&zint, N/8+(N%8?1:0), zz, NULL);
		memcpy(z->buf, zz, N);
		matrixMul(z, &Ht, &tmp);

		size_t hamm = 0;
		for (int i = 0; i < R; i++)
			if (tmp.buf[i])
				hamm++;

		if (matrixEqual(&tmp, S) && hamm < minHamm) {
			minHamm = hamm;
			memcpy(res, z->buf, N);
		}
	}
	memcpy(z->buf, res, N);
	destroyMatrix(&tmp);
}

static void encodeMessage(const uint8_t in_buf[K], uint8_t out_buf[N], const uint8_t stream[N]) {

	struct Matrix message; initMatrix(&message, 1, K, in_buf);
	struct Matrix encodedMessage; initMatrix(&encodedMessage, 1, N, NULL);
	matrixMul(&message, &G1, &encodedMessage);

	minimizeWetBits(&encodedMessage, stream, out_buf);

	destroyMatrix(&message);
	destroyMatrix(&encodedMessage);
}

static void decodeMessage(const uint8_t in_buf[N], uint8_t out_buf[K]) {

	struct Matrix y; initMatrix(&y, 1, N, in_buf);
	struct Matrix S; initMatrix(&S, 1, R, NULL);
	matrixMul(&y, &Ht, &S);

	struct Matrix z; initMatrix(&z, 1, N, NULL);
	solveConstraintsMinimally(&z, &S);

	matrixAdd(&y, &z);
	struct Matrix xNewJt; initMatrix(&xNewJt, 1, K, NULL);
	matrixMul(&y, &Jt, &xNewJt);
	memcpy(out_buf, xNewJt.buf, K);

	destroyMatrix(&y);
	destroyMatrix(&S);
	destroyMatrix(&z);
	destroyMatrix(&xNewJt);
}

int main() {
	initMLBC();
	uint8_t in_buf[N] = {0, 0, 1, 0, 1, 1, 0}, out_buf[K];
	decodeMessage(in_buf, out_buf);
	for(int i = 0; i < K; i++)
		printf("%d ", out_buf[i]);
	putchar('\n');
	destroyMLBC();
}
