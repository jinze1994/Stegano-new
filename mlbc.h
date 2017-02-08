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

int encodeLongMessage(const uint8_t* message, uint8_t message_len,
		const uint8_t* stream, size_t stream_len,
		uint8_t** dataToHidePtr, size_t *data_len_ptr);

#endif
