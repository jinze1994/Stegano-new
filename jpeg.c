#include "jpeg.h"

const char * stegano_describe(int code) {
	switch(code) {
		case 0:
			return "OK";
		case 2:
			return "Jpeglib fail";
		case 3:
			return "Can't read data from RANDOM_SOURCE";
		case 10:
			return "Data too long";
		case 20:
			return "Out of memory";
		case 30:
			return "Error writing file copy";
		case 40:
			return "Only garbage found";
		case 50:
			return "Jpeg scale error";
	}
	return "Unknown error";
}

void my_error_exit (j_common_ptr cinfo) {
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	struct my_error_mgr* myerr = (struct my_error_mgr*) cinfo->err;

   (*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

int jpegChangeQuantity(FILE* infile, FILE* repfile, int quantity) {
	struct jpeg_decompress_struct cinfo_in;
	struct jpeg_compress_struct cinfo_out;

	struct my_error_mgr jerr;
	cinfo_in.err = jpeg_std_error(&jerr.pub);
	cinfo_out.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo_in);
		jpeg_destroy_compress(&cinfo_out);
		return 10;
	}
	
	jpeg_create_decompress(&cinfo_in);
	jpeg_create_compress(&cinfo_out);
	jpeg_stdio_src(&cinfo_in, infile);
	jpeg_stdio_dest(&cinfo_out, repfile);
	jpeg_read_header(&cinfo_in, TRUE);

	cinfo_out.image_width = cinfo_in.image_width;
	cinfo_out.image_height = cinfo_in.image_height;
	cinfo_out.input_components = cinfo_in.num_components;
	cinfo_out.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo_out);
	jpeg_set_quality(&cinfo_out, quantity, TRUE);
	
	jpeg_start_decompress(&cinfo_in);
	jpeg_start_compress(&cinfo_out, TRUE);
	JSAMPROW row_pointer[1];
	row_pointer[0] = malloc(cinfo_in.image_width * cinfo_in.num_components);
	while (cinfo_in.output_scanline < cinfo_in.output_height) {
		jpeg_read_scanlines(&cinfo_in, row_pointer, 1);
		jpeg_write_scanlines(&cinfo_out, row_pointer, 1);
	}
	free(row_pointer[0]);

	/*clean-up*/
	jpeg_finish_decompress(&cinfo_in);
	jpeg_finish_compress(&cinfo_out);
	jpeg_destroy_decompress(&cinfo_in);
	jpeg_destroy_compress(&cinfo_out);

	return 0;
}

static void initAllQualTabls(uint16_t AllQualTabls[100][128]) {
	static const uint16_t std_luminance_quant_tbl[64] = {
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99
	};
	static const uint16_t std_chrominance_quant_tbl[64] = {
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99
	};
	long temp;
	int scale_factor = 0;

	for (int quality = 1; quality <= 100; quality++) {
		if (quality <= 0) quality = 1;
		else if (quality > 100) quality = 100;
		else if (quality < 50) scale_factor=5000 / quality;
		else scale_factor= 200 - quality*2;

		for (int j = 0;j < 2; j++)
			for (int i = 0; i < 64; i++)
				if (j == 0) {
					temp = ((long) std_luminance_quant_tbl[i]*scale_factor+ 50L)/100L;
					if (temp <= 0L) temp = 1L;
					else if (temp > 32767L) temp = 32767L; /* max quantizer needed for 12 bits */
					else if (temp > 255) temp = 255L;
					AllQualTabls[quality-1][i] = (UINT16) temp;
				} else if (j == 1) {
					temp = ((long) std_chrominance_quant_tbl[i]*scale_factor+ 50L)/100L;
					if (temp <= 0L) temp = 1L;
					if (temp > 32767L) temp = 32767L; /* max quantizer needed for 12 bits */
					if (temp > 255) temp = 255L;
					AllQualTabls[quality-1][64+i] = (UINT16) temp;
				}
	}
}

static int getQualFactor(uint16_t QualTabl[128]) {
	static uint16_t AllQualTabls[100][128];
	static char inited = 0;
	if (!inited) {
		initAllQualTabls(AllQualTabls);
		inited = 1;
	}
	int final = -2, count = 0;
	for(int tmp = 99; tmp >= 0; tmp--)
		for(int j = 0; j < 128; j++)
			if ( QualTabl[j] == AllQualTabls[tmp][j] && j == 127) {
				count++;
				if (tmp > final) final = tmp;
			} else if(QualTabl[j] != AllQualTabls[tmp][j])
				break;
	return final+1;
}

void fetchJpegQuant(struct jpeg_decompress_struct* cinfo,
		uint16_t quant_tbl[NUM_QUANT_TBLS][DCTSIZE][DCTSIZE],
		uint16_t* num_quant_tbls,
		int* quantfactor) {

	memset(quant_tbl, 0, NUM_QUANT_TBLS * DCTSIZE2 * sizeof(uint16_t));
	int i;
	for (i = 0; i < NUM_QUANT_TBLS; i++)
		if (cinfo->quant_tbl_ptrs[i] != NULL)
			memcpy(quant_tbl[i], cinfo->quant_tbl_ptrs[i]->quantval, DCTSIZE2 * sizeof(uint16_t));
		else
			break;
	*num_quant_tbls = i;
	*quantfactor = getQualFactor((uint16_t*)(quant_tbl));
}

int printJpegQuantity(FILE* infile) {
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return 10;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void) jpeg_read_header(&cinfo, TRUE);

	uint16_t quant_tbl[NUM_QUANT_TBLS][DCTSIZE][DCTSIZE];	/* Quantity table of the image */
	uint16_t num_quant_tbls;
	int quantfactor;	/* Quantity factor of the image */
	fetchJpegQuant(&cinfo, quant_tbl, &num_quant_tbls, &quantfactor);

	jpeg_destroy_decompress(&cinfo);

	printf("Quantity Factor: %d\tNumber of QuantTbl: %d\n", quantfactor, num_quant_tbls);
	printf("Quant Table:\n");
	for (int i = 0; i < DCTSIZE; i++)
		for (int k = 0; k < num_quant_tbls; k++) {
			for (int j = 0; j < DCTSIZE; j++)
				printf("%2d ", (int)(quant_tbl[k][i][j]));
			if (k == num_quant_tbls - 1)
				printf("\n");
			else
				printf("\t");
		}

	return 0;
}
