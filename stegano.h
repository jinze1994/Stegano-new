#ifndef STEGANO_H
#define STEGANO_H

#include "habit.h"
#include "jpeg.h"

int steganoEncode(FILE* infile, FILE* outfile, const char* message, const char* password);

int steganoDecode(FILE* infile, const char* password, char* message);

#endif
