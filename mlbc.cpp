#include "mlbc.h"

int steganoEncode(FILE* infile, FILE* outfile,
	const char* message, const char* password) {

	struct jpeg_decompress_struct cinfo_in;
	jvirt_barray_ptr* luma_coeff_array;
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
		luma_coeff_array = jpeg_read_coefficients(&cinfo_in);
	}

	jpeg_destroy_decompress(&cinfo_in);
	return 0;
}
