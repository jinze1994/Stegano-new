#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

struct Matrix {
	uint32_t r, c;
	uint8_t* buf;
};

struct Matrix* newMatrix(uint32_t r, uint32_t c);

void destroyMatrix(struct Matrix* m);

struct Matrix* matrixDup(struct Matrix* m);

void matrixAdd(struct Matrix* a, struct Matrix* b);

bool matrixEqual(struct Matrix* a, struct Matrix* b);

struct Matrix* matrixMul(struct Matrix* a, struct Matrix* b);

#endif
