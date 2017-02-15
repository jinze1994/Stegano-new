#ifndef DUP_H
#define DUP_H

#include <stdint.h>
#include <stdlib.h>
#include "rgen.h"

void initMLBC();
void destroyMLBC();

int encodeLongMessage(const uint8_t* message, uint8_t message_len,
		const uint8_t* stream, size_t stream_len,
		struct rgen* rge,
		uint8_t** dataToHidePtr, size_t *data_len_ptr);

int decodeLongMessage(const uint8_t* stream, size_t stream_len,
		struct rgen* rge, uint8_t* message);

#endif
