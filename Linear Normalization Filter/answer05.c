//Preprocessor Directives and Helper Function Declarations
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <libgen.h>
#include "answer05.h"

#define BMP_MAGIC_NUMBER 0x4d42
#define ECE264_IMAGE_MAGIC_NUMBER 0x21343632
#define FALSE 0
#define TRUE 1
#define DEFAULT_DPI_X 3780
#define DEFAULT_DPI_Y 3780



//End Preprocessor

/**
 * Loads a bmp (windows bitmap) image, returning an Image structure.
 * Will return NULL if there is any error.
 *
 */


#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint16_t type;                      // Magic identifier
    uint32_t size;                      // File size in bytes
    uint16_t reserved1;                 // Not used
    uint16_t reserved2;                 // Not used
    uint32_t offset;                    // Offset to image data in bytes
    uint32_t header_size;               // Header size in bytes
    int32_t  width;                     // Width of the image
    int32_t  height;                    // Height of image
    uint16_t planes;                    // Number of color planes
    uint16_t bits;                      // Bits per pixel
    uint32_t compression;               // Compression type
    uint32_t imagesize;                 // Image size in bytes
    int32_t  xresolution;               // Pixels per meter
    int32_t  yresolution;               // Pixels per meter
    uint32_t ncolors;                  // Number of colors  
    uint32_t importantcolors;          // Important colors
} BMP_Header;
#pragma pack(pop)

static int BMP_checkValid(BMP_Header * header);
static void BMP_printHeader(BMP_Header * header);
int BMP_Valid(ImageHeader * BMP_header);
void BMP_printHeader2(ImageHeader * header);
char * strip_string(char * base, char * remove);

Image * Image_loadbmp(const char * filename)
{
	 FILE * fp = NULL;
    size_t read;
    BMP_Header header;
    Image * tmp_im = NULL, * im = NULL;
    size_t n_bytes = 0;
    int err = FALSE;

    if(!err) { // Open filename
	fp = fopen(filename, "rb");
	if(!fp) {
	    fprintf(stderr, "Failed to open file '%s'\n", filename);
	    err = TRUE;
	}
    }

    if(!err) { // Read the header
	read = fread(&header, sizeof(BMP_Header), 1, fp);
	if(read != 1) {
	    fprintf(stderr, "Failed to read header from '%s'\n", filename);
	    err = TRUE;
	}
    }

    if(!err) { // Print the header
	BMP_printHeader(&header);
    }

    if(!err) { // We're only interested in a subset of valid bmp files
	if(!BMP_checkValid(&header)) {
	    fprintf(stderr, "Invalid header in '%s'\n", filename);
	    err = TRUE;
	}
    }

    if(!err) { // Allocate Image struct
	tmp_im =(Image *) malloc(sizeof(Image));			
	if(tmp_im == NULL) {
	    fprintf(stderr, "Failed to allocate im structure\n");
	    err = TRUE;
	} 
    }

    if(!err) { // Init the Image struct
	tmp_im->width = header.width;
	tmp_im->height = header.height;

	// Handle the comment
	char * filename_cpy = strdup(filename);	// we want to call basename
	char * file_basename = basename(filename_cpy); // requires editable str
	const char * prefix = "Original BMP file: ";
	n_bytes = sizeof(char) * (strlen(prefix) + strlen(file_basename) + 1);
	tmp_im->comment = (char *)malloc (n_bytes);
	if(tmp_im->comment == NULL) {
	    fprintf(stderr, "Failed to allocate %zd bytes for comment\n",
		    n_bytes);
	    err = TRUE;
	} else {
	    sprintf(tmp_im->comment, "%s%s", prefix, file_basename);
	}
	free(filename_cpy); // this also takes care of file_basename

	// Handle image data
	n_bytes = sizeof(uint8_t) * header.width * header.height;
	tmp_im->data = (uint8_t *) malloc(n_bytes);
	if(tmp_im->data == NULL) {
	    fprintf(stderr, "Failed to allocate %zd bytes for image data\n",
		    n_bytes);
	    err = TRUE;
	}
    }

    if(!err) { // Seek the start of the pixel data
	if(fseek(fp, header.offset, SEEK_SET) != 0) {
	    fprintf(stderr, "Failed to seek %d, the data of the image data\n",
		    header.offset);
	    err = TRUE;
	}
    }

    if(!err) { // Read pixel data
	size_t bytes_per_row = ((header.bits * header.width + 31)/32) * 4;
	n_bytes = bytes_per_row * header.height;
	uint8_t * rawbmp = (uint8_t *)malloc(n_bytes);
	if(rawbmp == NULL) {
	    fprintf(stderr, "Could not allocate %zd bytes of image data\n",
		    n_bytes);
	    err = TRUE;
	} else {
	    read = fread(rawbmp, sizeof(uint8_t), n_bytes, fp);
	    if(n_bytes != read) {
		fprintf(stderr, "Only read %zd of %zd bytes of image data\n", 
			read, n_bytes);
		err = TRUE;
	    } else {
		// Must convert RGB to grayscale
		uint8_t * write_ptr = tmp_im->data;
		uint8_t * read_ptr;
		int intensity;
		int row, col; // row and column
		for(row = 0; row < header.height; ++row) {
		    read_ptr = &rawbmp[row * bytes_per_row];
		    for(col = 0; col < header.width; ++col) {
			intensity  = *read_ptr++; // blue
			intensity += *read_ptr++; // green
			intensity += *read_ptr++; // red	
			*write_ptr++ = intensity / 3; // now grayscale
		    }
		}
	    }
	}
	free(rawbmp);
    }

    if(!err) { // We should be at the end of the file now
	uint8_t byte;
	read = fread(&byte, sizeof(uint8_t), 1, fp);
	if(read != 0) {
	    fprintf(stderr, "Stray bytes at the end of bmp file '%s'\n",
		    filename);
	    err = TRUE;
	}
    }

    if(!err) { 
	im = tmp_im;  // bmp will be returned
	tmp_im = NULL; // and not cleaned up
    }

    // Cleanup
    if(tmp_im != NULL) {
	free(tmp_im->comment); // Remember, you can always free(NULL)
	free(tmp_im->data);
	free(tmp_im);
    }
    if(fp) {
	fclose(fp);
    }

    return im;
}

/**
 * Saves an Image structure to a file. Returns TRUE if success, or
 * FALSE if there is any error.
 */
int Image_savebmp(const char * filename, Image * image)
{
	    int err = FALSE; 
    FILE * fp = NULL;
    uint8_t * buffer = NULL;    
    size_t written = 0;

    // Attempt to open file for writing
    fp = fopen(filename, "wb");
    if(fp == NULL) {
	fprintf(stderr, "Failed to open '%s' for writing\n", filename);
	return FALSE; // No file ==> out of here.
    }

    // Number of bytes stored on each row
    size_t bytes_per_row = ((24 * image->width + 31)/32) * 4;

    // Prepare the header
    BMP_Header header;
    header.type = BMP_MAGIC_NUMBER;
    header.size = sizeof(BMP_Header) + bytes_per_row * image->height;
    header.reserved1 = 0;
    header.reserved2 = 0;
    header.offset = sizeof(BMP_Header);
    header.header_size = sizeof(BMP_Header) - 14;
    header.width = image->width;
    header.height = image->height;
    header.planes = 1;
    header.bits = 24; // BGR
    header.compression = 0; // no compression
    header.imagesize = bytes_per_row * image->height;
    header.xresolution = DEFAULT_DPI_X;
    header.yresolution = DEFAULT_DPI_Y;
    header.ncolors = 1 << header.bits;
    header.importantcolors = 0; // i.e., every color is important

    if(!err) {  // Write the header
	written = fwrite(&header, sizeof(BMP_Header), 1, fp);
	if(written != 1) {
	    fprintf(stderr, 
		    "Error: only wrote %zd of %zd of file header to '%s'\n",
		    written, sizeof(BMP_Header), filename);
	    err = TRUE;	
	}
    }

    if(!err) { // Before writing, we'll need a write buffer
	buffer = (uint8_t * ) malloc(bytes_per_row);
	if(buffer == NULL) {
	    fprintf(stderr, "Error: failed to allocate write buffer\n");
	    err = TRUE;
	} else {
	    // not strictly necessary, we output file will be tidier.
	    memset(buffer, 0, bytes_per_row); 
	}
    }

    if(!err) { // Write pixels	
	uint8_t * read_ptr = image->data;	
	int row, col; // row and column
	for(row = 0; row < header.height && !err; ++row) {
	    uint8_t * write_ptr = buffer;
	    for(col = 0; col < header.width; ++col) {
		*write_ptr++ = *read_ptr; // blue
		*write_ptr++ = *read_ptr; // green
		*write_ptr++ = *read_ptr; // red
		read_ptr++; // advance to the next pixel
	    }
	    // Write line to file
	    written = fwrite(buffer, sizeof(uint8_t), bytes_per_row, fp);
	    if(written != bytes_per_row) {
		fprintf(stderr, "Failed to write pixel data to file\n");
		err = TRUE;
	    }
	}
    }
    
    // Cleanup
    free(buffer);
    if(fp)
	fclose(fp);

    return !err;


}

/**
 * Loads an ECE264 image file, returning an Image structure.
 * Will return NULL if there is any error.
 */
Image * Image_load(const char * filename)
{
	size_t num_bytes = 0;
	size_t read_hold;
	int error = false;
	FILE * f_image = NULL;
    Image * temp_image = NULL;
    Image * image = NULL;
    ImageHeader header;
    
    if(!error) //opens file
    {
		f_image = fopen(filename, "rb");
	    if(!f_image) 
	    {
	       fprintf(stderr, "ERROR: Failed to open file (%s)\n", filename);
  	    error = true;
     	}
    }
    
    if (!error) //Reads file header
    {
       read_hold = fread (&header,sizeof (ImageHeader),1,f_image);
		if (read_hold != 1)
		{
			fprintf (stderr,"ERROR:Failed to read header of (%s)\n",filename);
			error = true;
		}
    }
	    if(!error) { // Print the header
	BMP_printHeader2(&header);
    }
	if(!error)
	{
		if(!BMP_Valid(&header))	//Check if header is valid
		{
		fprintf(stderr, "ERROR: Invalid header in '%s'\n", filename);
	    error = true;
		}
	}
	if(!error)//Allocate memory for image
	{
		temp_image = (Image * ) malloc(sizeof(Image));
		if(temp_image == NULL)
		{
			fprintf(stderr, "ERROR: unable to allocate memory for: '%s'\n", filename);
		error = true;
		}
	}
	if(!error)
	{
	temp_image->width = header.width;
	temp_image->height = header.height;
	temp_image->comment = (char *)malloc (header.comment_len);
	num_bytes = sizeof(uint8_t) * header.width * header.height;
	temp_image->data = (uint8_t *) malloc(num_bytes);

	if(temp_image->data == NULL) //Checks if allocated 
		{
	    	fprintf(stderr, "Failed to allocate %zd bytes for image data\n",num_bytes);
	    	error = true;
		}
	}

	if (!error)//read comment
	{
		fseek(f_image, sizeof(ImageHeader), SEEK_SET);//Set pointer
		if (fread (temp_image->comment,sizeof (char),header.comment_len,f_image) != header.comment_len)
		{
			fprintf (stderr,"ERROR: Failed to read comment\n");
			error = true;
		}
		
	}
	if(!error)//Set pointer to after comment null byte
	{
		if(fseek(f_image, sizeof(ImageHeader) + header.comment_len, SEEK_SET) != 0) 
		{
	    	fprintf(stderr, "ERROR: Failed to Seek_Set Data");
	    	error = true;
		}
	}

	if(!error)//Read pixel data
	{ 
		size_t data_bytes = header.width * header.height;
		int check_read = fread (temp_image->data,sizeof (uint8_t),data_bytes,f_image);
		if(temp_image->data== NULL) {
			fprintf(stderr, "Could not allocate %zd bytes of image data\n",
				num_bytes);
			error = true;
		} 
	else {
	    if(data_bytes != check_read) {
			fprintf(stderr, "Only read %zd of %zd bytes of image data\n", check_read, data_bytes);
			error = true;
	    } 
	}
	
    }

    if(!error) 
	{
	uint8_t test_byte;
	read_hold = fread(&byte, sizeof(test_byte), 1, f_image);
	if(read_hold != 0) {
	    fprintf(stderr, "Leftover bytes unread in '%s'\n",
		    filename);
	    error = true;
	}
    }

    if(!error) 
	{ 
	image = temp_image;  // bmp will be returned
	temp_image = NULL; // and not cleaned up
    }
    
    if(temp_image != NULL) {
	free(temp_image->comment); // Remember, you can always free(NULL)
	free(temp_image->data);
	free(temp_image);
    }
    if(f_image) {
	fclose(f_image);
    }

    return image;
}

/**
 * Save an image to the passed filename, in ECE264 format.
 * Return TRUE if this succeeds, or FALSE if there is any error.
 *
 */
int Image_save(const char * filename, Image * image)
{
	
    int error = false; 
	FILE * f_image = NULL;
	size_t bytes_per_row = ((8 * image->width + 31) / 32) * 4;
	size_t write_record = 0;
	char * str1 = "-unnormalized";
	char * str_comment = strip_string (image->comment,str1);

	ImageHeader header;
	header.magic_number = ECE264_IMAGE_MAGIC_NUMBER;
	header.width = image->width;
	header.height = image->height;
	header.comment_len = strlen(str_comment);

	f_image = fopen(filename, "wb");

    if(f_image == NULL) {
	fprintf(stderr, "Error: Could not open: '%s'\n", filename);
	return false;
    }
   
    if(!error) //Write header
	{
		write_record = fwrite(&header, sizeof(ImageHeader), 1, f_image);
		if(write_record != 1)
		{
			fprintf(stderr, 
		    "Error: only wrote %zd of %zd of file header to '%s'\n",
		    write_record, sizeof(ImageHeader), filename);
			error = true;	
		}
    }
	if (!error)//Write comment
	{
		write_record = fwrite (str_comment,strlen (str_comment),1,f_image);
		if (write_record != 1)
		{
			fprintf(stderr, "Error: Unable to write %zd bytes for comment : %s in file %s\n", write_record, image->comment, filename);
			error = true;
		}
	}
    

    if(!error) //write data 
	{ 
		uint8_t * data_ptr = image->data;	
	    fseek (f_image,1,SEEK_CUR);//Offset for data
	    write_record = fwrite(data_ptr, sizeof(uint8_t), image->width * image->height, f_image);
	    if(write_record != image->width * image->height )
		{
			fprintf(stderr, "Failed to write pixel data to file\n");
			error = true;
	    }
	}
    free (str_comment);
    
    if(f_image)
	fclose(f_image);

    return !error;
}

/**
 * Free memory for an image structure
 */
void Image_free(Image * image)
{
	free (image->comment);
	free (image->data);
	free (image);
}

/**
 * Performs linear normalization, see README
 */
void linearNormalization(int width, int height, uint8_t * intensity)
{
	int pixel_count = width * height;
	int curr_max = 0;
	int curr_min = 255;
	int i = 0;
	int bef;
	int aft;
	for(i = 0; i < pixel_count; i++)
	{
		if(intensity[i] > curr_max)
		{
			fprintf (stderr,"max:%d. at %d\n",intensity [i],i);
			curr_max = intensity[i];
		}
		if(intensity[i] < curr_min)
		{
			fprintf (stderr,"min:%d at %d\n",intensity [i],i);
			curr_min = intensity[i];
		}
	//	fprintf(stderr,"min:%d max:%d",curr_min,curr_max);
	}
//	fprintf(stderr,"min:%d max:%d",curr_min,curr_max);
	for(i = 0; i < pixel_count;i++)
	{
		
	//	fprintf (stderr,"%d",intensity [i]);
		intensity[i] = (intensity[i] - curr_min) * 255.0 / (curr_max - curr_min);
	//	fprintf (stderr,"      %d\n",intensity [i]);
	
	
	//fprintf (stderr,"%d != %d\n",bef,aft);
	}
}


//Helper function definitions
void BMP_printHeader2(ImageHeader * header)
{
    printf("Printing BMP header information:\n");
    printf("  file type (should be %x): %x\n", ECE264_IMAGE_MAGIC_NUMBER, header->magic_number);
    printf("  width: %d\n", header->width);
    printf("  height: %d\n", header->height); 
	printf("  comment length: %d\n",header->comment_len);
}


int BMP_Valid(ImageHeader * BMP_header)
{
	int result = true;
	if((BMP_header->magic_number) != ECE264_IMAGE_MAGIC_NUMBER)
		result = false;
	
	return result;
}

static int BMP_checkValid(BMP_Header * header)
{
    // Make sure this is a BMP file
    if (header->type != BMP_MAGIC_NUMBER) return FALSE;

    // Make sure we are getting 24 bits per pixel
    if (header->bits != 24) return FALSE;

    // Make sure there is only one image plane
    if (header->planes != 1) return FALSE;

    // Make sure there is no compression
    if (header->compression != 0) return FALSE;

    // We're winners!!!
    return TRUE;
}

// ------------------------------------------------------------- BMP_printHeader

static void BMP_printHeader(BMP_Header * header)
{
    printf("Printing BMP header information:\n");
    printf("  file type (should be %x): %x\n", BMP_MAGIC_NUMBER, header->type);
    printf("  file size: %d\n", header->size);
    printf("  offset to image data: %d\n", header->offset);
    printf("  header size: %d\n", header->header_size);
    printf("  width: %d\n", header->width);
    printf("  height: %d\n", header->height);
    printf("  planes: %d\n", header->planes);
    printf("  bits: %d\n", header->bits);    
}
char * strip_string (char * base, char * remove)
{
	int base_length = strlen (base);
	int rem_length = strlen (remove);
	char * new_set = (char *)malloc (base_length - rem_length + 1);
	int i;
	int j = 0;
	int k = 0;
	for (i = 0; i < base_length; i++)
	{
		if (base [i] == remove [j])
		j++;
		else {
			new_set [k] = base [i];
			k++;
		}
	}
	new_set [k+1] = '\0';
	return new_set;
}