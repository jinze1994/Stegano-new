#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

struct Matrix {
	uint32_t r, c;
	uint8_t* buf;
};

struct Matrix* newMatrix(uint32_t r, uint32_t c);

struct Matrix* newMatrixA(uint32_t r, uint32_t c, const uint8_t* array);

void destroyMatrix(struct Matrix* m);

struct Matrix* matrixDup(const struct Matrix* m);

void matrixAdd(struct Matrix* a, const struct Matrix* b);

bool matrixEqual(const struct Matrix* a, const struct Matrix* b);

struct Matrix* matrixMul(const struct Matrix* a, const struct Matrix* b);

#endif
