#include "stegano.h"
#include "rgen.h"
#include <openssl/sha.h>

static bool checkJPEGScale(struct jpeg_decompress_struct* cinfo) {
	if (cinfo->comp_info[0].downsampled_width % DCTSIZE)
		return false;
	if (cinfo->comp_info[0].downsampled_width > 960)
		return false;
	if (cinfo->comp_info[0].downsampled_height % DCTSIZE)
		return false;
	if (cinfo->comp_info[0].downsampled_height > 720)
		return false;

	return true;
}

static uint32_t* getValidCoeffs(size_t blocks, size_t* n) {
	uint32_t* coeffsPos = (uint32_t*) malloc(blocks * sizeof(uint32_t));
	if (coeffsPos == NULL) return NULL;

	for (size_t i = 0; i < blocks; i++)
		coeffsPos[i] = i * DCTSIZE2 + 1;

	*n = blocks;
	return coeffsPos;
}

static inline JCOEFPTR fetchCoefByPos(struct jpeg_decompress_struct* cinfo,
	jvirt_barray_ptr* coeff_array, uint32_t component_id,
	uint32_t coeffsPos,
	bool modified) {

	uint32_t block_id = coeffsPos / DCTSIZE2, k = coeffsPos % DCTSIZE2;
	uint32_t block_x = block_id / cinfo->comp_info[component_id].width_in_blocks;
	uint32_t block_y = block_id % cinfo->comp_info[component_id].width_in_blocks;
	JBLOCKARRAY B = (cinfo->mem -> access_virt_barray)((j_common_ptr)cinfo,
			coeff_array[component_id],
			block_x, 1, modified);
	JCOEFPTR dctblck = B[0][block_y];
	JCOEFPTR coef = &dctblck[k];
	return coef;
}

static uint8_t* coeffsToStuckBitStream(struct jpeg_decompress_struct* cinfo,
	jvirt_barray_ptr* coeff_array, uint32_t component_id,
	uint32_t* coeffsPos, size_t coeffs_len) {

	uint8_t* stream = malloc(coeffs_len * sizeof(uint8_t));
	if (stream == NULL) return NULL;

	for (size_t i = 0; i < coeffs_len; i++) {
		JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[i], false);
		if (*coef == 0)
			stream[i] = 0;
		else
			stream[i] = 1;
	}
	return stream;
}

static size_t makeChange(struct jpeg_decompress_struct* cinfo,
	jvirt_barray_ptr* coeff_array, uint32_t component_id,
	uint32_t* coeffsPos,
	uint8_t* dataToHide, size_t data_len) {

	size_t used_bit = 0;

	for (size_t i = 0; i < data_len; i++) {
		JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[i], true);
		uint8_t lsb = (*coef) & 1;
		if (lsb != dataToHide[i])
			*coef ^= 1, used_bit++;
	}
	return used_bit;
}

int steganoEncode(FILE* infile, FILE* outfile,
	const char* message, const char* password) {

	jvirt_barray_ptr* luma_coeff_array = NULL;
	size_t w, h, blocks;

	struct jpeg_decompress_struct cinfo_in;
	{
		struct my_error_mgr jerr;
		cinfo_in.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		if (setjmp(jerr.setjmp_buffer)) {
			jpeg_destroy_decompress(&cinfo_in);
			return 10;
		}
	
		jpeg_create_decompress(&cinfo_in);
		jpeg_stdio_src(&cinfo_in, infile);
		(void) jpeg_read_header(&cinfo_in, TRUE);

		if (!checkJPEGScale(&cinfo_in)) {
			jpeg_destroy_decompress(&cinfo_in);
			return 50;
		}

		luma_coeff_array = jpeg_read_coefficients(&cinfo_in);
		w = cinfo_in.image_width, h = cinfo_in.image_height;
		blocks = w * h / 64;
	}

	uint32_t *coeffsPos; size_t coeffs_len;
	coeffsPos = getValidCoeffs(blocks, &coeffs_len);
	if (coeffsPos == NULL) {
		jpeg_destroy_decompress(&cinfo_in);
		return 20;
	}

	{
		struct rgen rge;
		rgen_init(&rge, password);
		rgen_shuffle(&rge, coeffsPos, coeffs_len);
		rgen_free(&rge);
	}

	uint8_t* stream = coeffsToStuckBitStream(&cinfo_in, 
			luma_coeff_array, 0,
			coeffsPos, coeffs_len);
	if (stream == NULL) {
		free(coeffsPos);
		jpeg_destroy_decompress(&cinfo_in);
		return 20;
	}

	uint8_t dataToHide[300 * 8]; size_t data_len;
	size_t message_len = strlen(message);
	if (message_len > 256) {
		free(stream);
		free(coeffsPos);
		jpeg_destroy_decompress(&cinfo_in);
		return 10;
	}
	dataToHide[0] = (uint8_t)message_len;
	memcpy(dataToHide+1, message, message_len);

	uint8_t sha1[SHA_DIGEST_LENGTH];
	SHA1((const unsigned char*)message, message_len, sha1);
	memcpy(dataToHide+1+message_len, sha1, SHA_DIGEST_LENGTH);
	data_len = 1 + message_len + SHA_DIGEST_LENGTH;
	if (data_len > coeffs_len) {
		free(stream);
		free(coeffsPos);
		jpeg_destroy_decompress(&cinfo_in);
		return 10;
	}

	for (int i = (int)data_len-1; i >= 0; i--)
		for (int j = 7; j >= 0; j--)
			dataToHide[i * 8 + j] = (dataToHide[i] >> j) & 1;
	data_len *= 8;

	size_t used_bit = makeChange(&cinfo_in, luma_coeff_array, 0,
			coeffsPos,
			dataToHide, data_len);
	printf("data_bype_len:\t%lu\n", data_len/8);
	printf("data_bit_len:\t%lu\n", data_len);
	printf("coeffs_len:\t%lu\n", coeffs_len);
	printf("used_bit:\t%lu\n", used_bit);
	printf("%lf%%\n", used_bit * 100.0 / coeffs_len);

	free(stream);
	free(coeffsPos);
	jpeg_destroy_decompress(&cinfo_in);
	return 0;
}
