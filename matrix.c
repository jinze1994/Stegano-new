#include "matrix.h"
#include "habit.h"

struct Matrix* newMatrix(uint32_t r, uint32_t c) {
	struct Matrix* m = (struct Matrix*) malloc(sizeof(struct Matrix));
	Assert(m != NULL, "No enough memory\n");
	Assert(r != 0 && c != 0, "New matrix error\n");
	m->r = r, m->c = c;
	m->buf = calloc(r * c, 1);
	Assert(m->buf != NULL, "No enough memory\n");
	return m;
}

struct Matrix* newMatrixA(uint32_t r, uint32_t c, const uint8_t* array) {
	struct Matrix* m = newMatrix(r, c);
	memcpy(m->buf, array, r * c);
	return m;
}

void destroyMatrix(struct Matrix* m) {
	if (m != NULL) {
		free(m->buf);
		free(m);
	}
}

struct Matrix* matrixDup(const struct Matrix* m) {
	struct Matrix* a = newMatrix(m->r, m->c);
	memcpy(a->buf, m->buf, m->r * m->c);
	return a;
}

void matrixAdd(struct Matrix* a, const struct Matrix* b) {
	Assert(a->r == b->r && a->c == b->c, "Matrix add error\n");
	for (uint32_t i = 0; i < a->r; i++)
		for (uint32_t j = 0; j < a->c; j++)
			a->buf[i * a->c + j] ^= b->buf[i * b->c + j];
}

bool matrixEqual(const struct Matrix* a, const struct Matrix* b) {
	Assert(a->r == b->r && a->c == b->c, "Matrix equal error\n");
	return !memcmp(a->buf, b->buf, a->r * a->c);
}

struct Matrix* matrixMul(const struct Matrix* a, const struct Matrix* b) {
	Assert(a->c == b->r, "Matrix mul error\n");
	struct Matrix* c = newMatrix(a->r, b->c);
	for (uint32_t i = 0; i < a->r; i++)
		for (uint32_t j = 0; j < b->c; j++)
			for (uint32_t k = 0; k < a->c; k++)
				c->buf[i * c->c + j] ^= a->buf[i * a->c + k] & b->buf[k * b->c + j];
	return c;
}

