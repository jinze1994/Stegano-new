#include "matrix.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct Matrix* newMatrix(uint32_t r, uint32_t c) {
	struct Matrix* m = (struct Matrix*) malloc(sizeof(struct Matrix));
	if (m == NULL) return NULL;
	assert(r != 0 && c != 0);
	m->r = r, m->c = c;
	m->buf = calloc(r * c, 1);
	if (m->buf == NULL) {
		free(m);
		return NULL;
	}
	return m;
}

void destroyMatrix(struct Matrix* m) {
	if (m != NULL) {
		free(m->buf);
		free(m);
	}
}

struct Matrix* matrixDup(struct Matrix* m) {
	struct Matrix* a = newMatrix(m->r, m->c);
	memcpy(a->buf, m->buf, m->r * m->c);
	return a;
}

void matrixAdd(struct Matrix* a, struct Matrix* b) {
	assert(a->r == b->r && a->c == b->c);
	for (uint32_t i = 0; i < a->r; i++)
		for (uint32_t j = 0; j < a->c; j++)
			a->buf[i * a->c + j] ^= b->buf[i * b->c + j];
}

bool matrixEqual(struct Matrix* a, struct Matrix* b) {
	assert(a->r == b->r && a->c == b->c);
	return !memcmp(a->buf, b->buf, a->r * a->c);
}

struct Matrix* matrixMul(struct Matrix* a, struct Matrix* b) {
	assert(a->c == b->r);
	struct Matrix* c = newMatrix(a->r, b->c);
	for (uint32_t i = 0; i < a->r; i++)
		for (uint32_t j = 0; j < b->c; j++)
			for (uint32_t k = 0; k < a->c; k++)
				c->buf[i * c->c + j] ^= a->buf[i * a->c + k] & b->buf[k * b->c + j];
	return c;
}

