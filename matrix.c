#include "matrix.h"
#include "habit.h"

void initMatrix(struct Matrix* m, uint32_t r, uint32_t c, const uint8_t* array) {
	m->r = r, m->c = c;
	m->buf = malloc(r * c);
	if (array)
		memcpy(m->buf, array, r * c);
	else
		memset(m->buf, 0, r * c);
}

void destroyMatrix(struct Matrix* m) {
	free(m->buf);
}

void matrixAdd(struct Matrix* a, struct Matrix* b) {
	Assert(a->r == b->r && a->c == b->c, "Matrix add error\n");
	for (uint32_t i = 0; i < a->r; i++)
		for (uint32_t j = 0; j < a->c; j++)
			a->buf[i * a->c + j] ^= b->buf[i * b->c + j];
}

bool matrixEqual(struct Matrix* a, struct Matrix* b) {
	Assert(a->r == b->r && a->c == b->c, "Matrix equal error\n");
	return !memcmp(a->buf, b->buf, a->r * a->c);
}

void matrixMul(struct Matrix* a, struct Matrix* b, struct Matrix* c) {
	Assert(a->c == b->r, "Matrix mul error (%d,%d)*(%d,%d)\n", a->r, a->c, b->r, b->c);
	Assert(c->r == a->r && b->c == c->c, "Matrix mul c error\n");
	for (uint32_t i = 0; i < a->r; i++)
		for (uint32_t j = 0; j < b->c; j++)
			for (uint32_t k = 0; k < a->c; k++)
				c->buf[i * c->c + j] ^= a->buf[i * a->c + k] & b->buf[k * b->c + j];
}

void matrixPrint(struct Matrix* a) {
	for (int i = 0; i < a->r; i++) {
		for (int j = 0; j < a->c; j++)
			printf("%d ", a->buf[i * a->c + j]);
		putchar('\n');
	}
}
