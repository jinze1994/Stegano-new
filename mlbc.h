#ifndef MLBC_H
#define MLBC_H

#include <stdlib.h>
#include <stdint.h>
#include "rgen.h"

#define N (27)
#define K (3)
#define L (11)
#define R (N-K-L)

void initMLBC();
void destroyMLBC();

int encodeLongMessage(const uint8_t* message, uint8_t message_len,
		const uint8_t* stream, size_t stream_len,
		struct rgen* rge,
		uint8_t** dataToHidePtr, size_t *data_len_ptr);

int decodeLongMessage(const uint8_t* stream, size_t stream_len,
		struct rgen* rge, uint8_t* message);

#endif
