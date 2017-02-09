#include "stegano.h"
#include "rgen.h"
#include "mlbc.h"
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

	size_t used_bit = 0, stuck_bit = 0;

	for (size_t i = 0; i < data_len; i++) {
		JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[i], true);
		uint8_t lsb = (*coef) & 1;
		if (lsb != dataToHide[i]) {
			*coef ^= 1, used_bit++;
			if (*coef)
				stuck_bit++;
		}
	}
	printf("stuck_bit: %lu\n", stuck_bit);
	return used_bit;
}

static uint8_t* coeffsToStream(struct jpeg_decompress_struct* cinfo,
	jvirt_barray_ptr* coeff_array, uint32_t component_id,
	uint32_t* coeffsPos, size_t coeffs_len) {

	uint8_t* stream = malloc(coeffs_len * sizeof(uint8_t));
	if (stream == NULL) return NULL;

	for (size_t i = 0; i < coeffs_len; i++) {
		JCOEFPTR coef = fetchCoefByPos(cinfo, coeff_array, component_id, coeffsPos[i], false);
		stream[i] = (*coef) & 1;
	}
	return stream;
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
	struct rgen* rge;
	uint8_t* stream;
	uint8_t* dataToHide;
};

static int destroyCleanUp(struct Cleanup* clu, int rv) {
	if (clu->stream)
		free(clu->stream);
	if (clu->coeffsPos)
		free(clu->coeffsPos);
	if (clu->rge)
		rgen_free(clu->rge);
	if (clu->dataToHide)
		free(clu->dataToHide);
	if (clu->cinfo)
		jpeg_destroy_decompress(clu->cinfo);
	return rv;
}

int steganoEncode(FILE* infile, FILE* outfile,
	const char* message, const char* password) {

	jvirt_barray_ptr* luma_coeff_array = NULL;
	size_t blocks;

	struct jpeg_decompress_struct cinfo_in;
	struct Cleanup clu = {&cinfo_in, NULL, NULL, NULL, NULL};
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
		blocks = cinfo_in.image_width * cinfo_in.image_height / 64;
	}

	uint32_t *coeffsPos; size_t coeffs_len;
	clu.coeffsPos = coeffsPos = getValidCoeffs(blocks, &coeffs_len);
	if (coeffsPos == NULL)
		return destroyCleanUp(&clu, 20);

	struct rgen rge;
	rgen_init(&rge, password);
	clu.rge = &rge;
	rgen_shuffle(&rge, coeffsPos, coeffs_len);

	uint8_t* stream;
	clu.stream = stream = coeffsToStuckBitStream(&cinfo_in,
			luma_coeff_array, 0,
			coeffsPos, coeffs_len);
	if (stream == NULL)
		return destroyCleanUp(&clu, 20);

	size_t message_len = strlen(message);
	if (message_len >= 256)
		return destroyCleanUp(&clu, 10);

	uint8_t* dataToHide; size_t data_len;
	int rv = encodeLongMessage((const uint8_t*)message, message_len,
			stream, coeffs_len,
			&rge,
			&dataToHide, &data_len);
	if (rv) return destroyCleanUp(&clu, rv);
	clu.dataToHide = dataToHide;

	size_t modify_bit = makeChange(&cinfo_in, luma_coeff_array, 0,
			coeffsPos,
			dataToHide, data_len);
	printf("avail_bit:%lu\tmodify_bit: %lu\tpercent: %.2lf%%\n",
			coeffs_len, modify_bit, modify_bit * 100.0 / coeffs_len);

	rv = writeback(&cinfo_in, luma_coeff_array, outfile);

	// cleanup:
	return destroyCleanUp(&clu, rv);
}

int steganoDecode(FILE* infile, const char* password, char* message) {

	jvirt_barray_ptr* luma_coeff_array = NULL;
	size_t blocks;

	struct jpeg_decompress_struct cinfo_in;
	struct Cleanup clu = {&cinfo_in, NULL, NULL, NULL, NULL};
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
		blocks = cinfo_in.image_width * cinfo_in.image_height / 64;
	}

	uint32_t *coeffsPos; size_t coeffs_len;
	clu.coeffsPos = coeffsPos = getValidCoeffs(blocks, &coeffs_len);
	if (coeffsPos == NULL)
		return destroyCleanUp(&clu, 20);

	struct rgen rge;
	rgen_init(&rge, password);
	clu.rge = &rge;
	rgen_shuffle(&rge, coeffsPos, coeffs_len);

	uint8_t* stream;
	clu.stream = stream = coeffsToStream(&cinfo_in,
			luma_coeff_array, 0,
			coeffsPos, coeffs_len);
	if (stream == NULL)
		return destroyCleanUp(&clu, 20);

	int rv = decodeLongMessage(stream, coeffs_len, &rge, (uint8_t*)message);

	return destroyCleanUp(&clu, rv);
}
