#ifndef MLBC_H
#define MLBC_H

#include <stdlib.h>
#include <stdint.h>

#define N (27)
#define K (3)
#define L (11)
#define R (N-K-L)

void initMLBC();
void destroyMLBC();

void toBin(const uint8_t* src, size_t src_len, uint8_t* dst, size_t* dst_len);

#endif
