#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

struct Matrix {
	uint32_t r, c;
	uint8_t* buf;
};

void initMatrix(struct Matrix* m, uint32_t r, uint32_t c, const uint8_t* array);

void destroyMatrix(struct Matrix* m);

void matrixAdd(struct Matrix* a, struct Matrix* b);

bool matrixEqual(struct Matrix* a, struct Matrix* b);

void matrixMul(struct Matrix* a, struct Matrix* b, struct Matrix* c);

void matrixPrint(struct Matrix* a);

#endif
