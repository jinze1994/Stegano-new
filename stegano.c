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

static int readfrom(struct jpeg_decompress_struct* cinfo,
		jvirt_barray_ptr* coeff_array, uint32_t component_id,
		uint32_t* coeffsPos,
		uint8_t* message) {

	size_t message_len = 0;
	for (int i = 0; i < 8; i++) {
		JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[i], false);
		if ((*coef) & 1)
			message_len |= 1 << i;
	}
	if (message_len >= 256)
		return 40;

	memset(message, 0, (message_len+1) * sizeof(uint8_t));
	for (size_t i = 0; i < message_len; i++)
		for (size_t j = 0; j < 8; j++) {
			size_t k = (i+1) * 8 + j;
			JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[k], false);
			if (*coef & 1)
				message[i] |= 1 << j;
		}

	uint8_t sha1_in[SHA_DIGEST_LENGTH];
	memset(sha1_in, 0, sizeof(sha1_in));
	for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++)
		for (size_t j = 0; j < 8; j++) {
			size_t k = (i+1+message_len) * 8 + j;
			JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[k], false);
			if (*coef & 1)
				sha1_in[i] |= 1 << j;
		}

	uint8_t sha1[SHA_DIGEST_LENGTH];
	SHA1(message, message_len, sha1);

	if (memcmp(sha1, sha1_in, sizeof(sha1_in)))
		return 40;

	return 0;
}

static int writeback(struct jpeg_decompress_struct* cinfo_in, 
		jvirt_barray_ptr* coeff_array,
		FILE* outfile) {

	struct jpeg_compress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_compress(&cinfo);
		return 10;
	}

	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	jpeg_copy_critical_parameters(cinfo_in, &cinfo);
	jpeg_write_coefficients(&cinfo, coeff_array);

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	return 0;
}

struct Cleanup {
	struct jpeg_decompress_struct* cinfo;
	uint32_t* coeffsPos;
	uint8_t* stream;
};

static int destroyCleanUp(struct Cleanup* clu, int rv) {
	if (clu->stream)
		free(clu->stream);
	if (clu->coeffsPos)
		free(clu->coeffsPos);
	if (clu->cinfo)
		jpeg_destroy_decompress(clu->cinfo);
	return rv;
}

int steganoEncode(FILE* infile, FILE* outfile,
	const char* message, const char* password) {

	jvirt_barray_ptr* luma_coeff_array = NULL;
	size_t w, h, blocks;

	struct jpeg_decompress_struct cinfo_in;
	struct Cleanup clu = {&cinfo_in, NULL, NULL};
	{
		struct my_error_mgr jerr;
		cinfo_in.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		if (setjmp(jerr.setjmp_buffer))
			return destroyCleanUp(&clu, 10);
	
		jpeg_create_decompress(&cinfo_in);
		jpeg_stdio_src(&cinfo_in, infile);
		(void) jpeg_read_header(&cinfo_in, TRUE);

		if (!checkJPEGScale(&cinfo_in))
			return destroyCleanUp(&clu, 50);

		luma_coeff_array = jpeg_read_coefficients(&cinfo_in);
		w = cinfo_in.image_width, h = cinfo_in.image_height;
		blocks = w * h / 64;
	}

	uint32_t *coeffsPos; size_t coeffs_len;
	clu.coeffsPos = coeffsPos = getValidCoeffs(blocks, &coeffs_len);
	if (coeffsPos == NULL)
		return destroyCleanUp(&clu, 20);

	{
		struct rgen rge;
		rgen_init(&rge, password);
		rgen_shuffle(&rge, coeffsPos, coeffs_len);
		rgen_free(&rge);
	}

	uint8_t* stream;
	clu.stream = stream = coeffsToStuckBitStream(&cinfo_in,
			luma_coeff_array, 0,
			coeffsPos, coeffs_len);
	if (stream == NULL)
		return destroyCleanUp(&clu, 20);

	uint8_t dataToHide[300 * 8]; size_t data_len;
	size_t message_len = strlen(message);
	if (message_len >= 256)
		return destroyCleanUp(&clu, 10);

	dataToHide[0] = (uint8_t)message_len;
	memcpy(dataToHide+1, message, message_len);

	uint8_t sha1[SHA_DIGEST_LENGTH];
	SHA1((const unsigned char*)message, message_len, sha1);
	memcpy(dataToHide+1+message_len, sha1, SHA_DIGEST_LENGTH);
	data_len = 1 + message_len + SHA_DIGEST_LENGTH;
	if (data_len > coeffs_len)
		return destroyCleanUp(&clu, 10);

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

	int rv = writeback(&cinfo_in, luma_coeff_array, outfile);

	// cleanup:
	return destroyCleanUp(&clu, rv);
}

int steganoDecode(FILE* infile, const char* password, char* message) {

	jvirt_barray_ptr* luma_coeff_array = NULL;
	size_t w, h, blocks;

	struct jpeg_decompress_struct cinfo_in;
	struct Cleanup clu = {&cinfo_in, NULL, NULL};
	{
		struct my_error_mgr jerr;
		cinfo_in.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		if (setjmp(jerr.setjmp_buffer))
			return destroyCleanUp(&clu, 10);

		jpeg_create_decompress(&cinfo_in);
		jpeg_stdio_src(&cinfo_in, infile);
		(void) jpeg_read_header(&cinfo_in, TRUE);

		if (!checkJPEGScale(&cinfo_in))
			return destroyCleanUp(&clu, 50);

		luma_coeff_array = jpeg_read_coefficients(&cinfo_in);
		w = cinfo_in.image_width, h = cinfo_in.image_height;
		blocks = w * h / 64;
	}

	uint32_t *coeffsPos; size_t coeffs_len;
	clu.coeffsPos = coeffsPos = getValidCoeffs(blocks, &coeffs_len);
	if (coeffsPos == NULL)
		return destroyCleanUp(&clu, 20);

	{
		struct rgen rge;
		rgen_init(&rge, password);
		rgen_shuffle(&rge, coeffsPos, coeffs_len);
		rgen_free(&rge);
	}

	uint8_t* stream;
	clu.stream = stream = coeffsToStuckBitStream(&cinfo_in,
			luma_coeff_array, 0,
			coeffsPos, coeffs_len);
	if (stream == NULL)
		return destroyCleanUp(&clu, 20);

	int rv = readfrom(&cinfo_in,
			luma_coeff_array, 0,
			coeffsPos,
			(uint8_t*)message);

	return destroyCleanUp(&clu, rv);
}
