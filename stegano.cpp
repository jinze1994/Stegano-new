#include "stegano.h"

static int getValidCoeffs(size_t blocks, uint32_t** ptr, size_t* n) {
	uint32_t* coeffs = (uint32_t*) malloc(blocks * sizeof(uint32_t));
	if (coeffs == NULL) return 20;

	for (size_t i = 0; i < blocks; i++)
		coeffs[i] = i * 64 + 1;

	*ptr = coeffs;
	*n = blocks;
	return 0;
}

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

	int rv;
	uint32_t *coeffs; size_t coeffs_len;
	rv = getValidCoeffs(blocks, &coeffs, &coeffs_len);
	if (rv) {
		jpeg_destroy_decompress(&cinfo_in);
		return rv;
	}

	free(coeffs);
	jpeg_destroy_decompress(&cinfo_in);
	return 0;
}
