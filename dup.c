#include "dup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void initMLBC() {}
void destroyMLBC() {}

#define K (11)
#define rge_len (2)

uint8_t* encodeMessage(const uint8_t* message, size_t message_len,
		uint8_t* dataToHide) {
	for (size_t i = 0; i < message_len; i++)
		for (int j = 0; j < 8; j++)
			for (int k = 0; k < K; k++)
				*dataToHide++ = (message[i] >> j) & 1;
	return dataToHide;
}

int encodeLongMessage(const uint8_t* message, uint8_t message_len,
		const uint8_t* stream, size_t stream_len,
		struct rgen* rge,
		uint8_t** dataToHidePtr, size_t *data_len_ptr) {

	stream = stream, stream_len = stream_len;

	size_t data_len = (1 + rge_len + message_len) * 8 * K;
	if (data_len >= 256 * 8 * K) return 10;

	uint8_t* dataToHide = malloc(data_len);
	if (dataToHide == NULL) return 20;

	uint8_t* p = encodeMessage(&message_len, 1, dataToHide);

	uint8_t rge_data[rge_len];
	rgen_produce_nbytes(rge, rge_len, rge_data);
	p = encodeMessage(rge_data, rge_len, p);

	p = encodeMessage(message, message_len, p);
	assert(p == dataToHide + data_len);

	*dataToHidePtr = dataToHide;
	*data_len_ptr = data_len;
	printf("data_bit: %lu\n", data_len);
	return 0;
}

const uint8_t* decodeMessage(const uint8_t* stream, size_t decode_len,
		uint8_t* message) {
	memset(message, 0, decode_len);
	for (size_t i = 0; i < decode_len; i++)
		for (int j = 0; j < 8; j++) {
			int cnt = 0;
			for (int k = 0; k < K; k++)
				if (*stream++)
					cnt++;
			if (cnt > K / 2)
				message[i] |= 1 << j;
		}
	return stream;
}

int decodeLongMessage(const uint8_t* stream, size_t stream_len,
		struct rgen* rge, uint8_t* message) {
	stream_len = stream_len;

	uint8_t message_len;
	const uint8_t* p = decodeMessage(stream, 1, &message_len);

	uint8_t rge_data[rge_len];
	rgen_produce_nbytes(rge, rge_len, rge_data);
	uint8_t rge_data2[rge_len];
	p = decodeMessage(p, rge_len, rge_data2);
	if (memcmp(rge_data, rge_data2, rge_len)) return 40;

	decodeMessage(p, message_len, message);
	message[message_len] = 0;
	return 0;
}
