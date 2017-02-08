#include "stegano.h"
#include "rgen.h"

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
	uint32_t* coeffs = (uint32_t*) malloc(blocks * sizeof(uint32_t));
	if (coeffs == NULL) return NULL;

	for (size_t i = 0; i < blocks; i++)
		coeffs[i] = i * DCTSIZE2 + 1;

	*n = blocks;
	return coeffs;
}

static uint8_t* coeffsToStuckBitStream(struct jpeg_decompress_struct* cinfo,
	jvirt_barray_ptr* coeff_array, uint32_t component_id,
	uint32_t* coeffs, size_t coeffs_len) {

	uint8_t* stream = malloc(coeffs_len * sizeof(uint8_t));
	if (stream == NULL) return NULL;

	for (size_t i = 0; i < coeffs_len; i++) {
		uint32_t block_id = coeffs[i] / DCTSIZE2, k = coeffs[i] % DCTSIZE2;
		uint32_t block_x = block_id / cinfo->comp_info[component_id].width_in_blocks;
		uint32_t block_y = block_id % cinfo->comp_info[component_id].width_in_blocks;
		JBLOCKARRAY B = (cinfo->mem -> access_virt_barray)((j_common_ptr)cinfo,
				coeff_array[component_id],
				block_x, 1, FALSE);
		JCOEFPTR dctblck = B[0][block_y];
		JCOEF coef = dctblck[k];

		if (coef == 0)
			stream[i] = 0;
		else
			stream[i] = 1;
	}
	return stream;
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

	uint32_t *coeffs; size_t coeffs_len;
	coeffs = getValidCoeffs(blocks, &coeffs_len);
	if (coeffs == NULL) {
		jpeg_destroy_decompress(&cinfo_in);
		return 20;
	}

	{
		struct rgen rge;
		rgen_init(&rge, password);
		rgen_shuffle(&rge, coeffs, coeffs_len);
		rgen_free(&rge);
	}

	uint8_t* stream = coeffsToStuckBitStream(&cinfo_in, 
			luma_coeff_array, 0,
			coeffs, coeffs_len);
	if (stream == NULL) {
		free(coeffs);
		jpeg_destroy_decompress(&cinfo_in);
		return 20;
	}

	free(stream);
	free(coeffs);
	jpeg_destroy_decompress(&cinfo_in);
	return 0;
}
