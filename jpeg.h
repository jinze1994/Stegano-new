#ifndef JPEG_H
#define JPEG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <jpeglib.h>

struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

const char * stegano_describe(int code);

void my_error_exit (j_common_ptr cinfo);

int jpegChangeQuantity(FILE* infile, FILE* repfile, int quantity);

void fetchJpegQuant(struct jpeg_decompress_struct* cinfo,
		uint16_t quant_tbl[NUM_QUANT_TBLS][DCTSIZE][DCTSIZE],
		uint16_t* num_quant_tbls,
		int* quantfactor);

int printJpegQuantity(FILE* infile);

#endif
