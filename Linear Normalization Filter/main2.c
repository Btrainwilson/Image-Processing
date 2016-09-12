#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "answer05.h"

void print_usage(char * argv0)
{
    printf("\n"
	   "   Usage: %s <in-filename> <out-filename>\n"
	   "\n"
	   "      Reads bmp image file <in-filename> and then:\n"
	   "      (1) Converts it to grayscale\n"
	   "      (2) Inverts pixel intensity\n"
	   "      (3) Writes inverted image to bmp file <out-filename>\n"
	   "\n",
	   basename(argv0));
}


int main(int argc, char * * argv)
{
    int ret = EXIT_SUCCESS; // unless otherwise noted

    // Parse input arguments
    if(argc != 3) {
	print_usage(argv[0]);
	if(argc == 2 && strcmp(argv[1], "--help") == 0)
	    return EXIT_SUCCESS;
	return EXIT_FAILURE;
    }

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    // Read the file
    Image * im = Image_load(in_filename);
    if(im == NULL) {
	fprintf(stderr, "Error: failed to read '%s'\n", in_filename);
	return EXIT_FAILURE;
    }

    // Invert pixel intensity
	linearNormalization(im->width, im->height, im->data);

    // Write out a new file
    if(!Image_save(out_filename, im)) {
	fprintf(stderr, "Error attempting to write '%s'\n", out_filename);
	ret = EXIT_FAILURE;
    }

    Image_free(im);

    return ret;
}

