/*
**
** FUNCTIONAL DESCRIPTION:
** 
**      Given a stripped image, create a believable ARC (sort of a.out) image
**      header for it so that it can be loaded via the WNT ARC console.
**
** CALLING ENVIRONMENT: 
**
**	user mode
** 
** AUTHOR: David A Rusling, david.rusling@reo.mts.dec.com
**
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 *  Macros
 */
#define TRUE 1
#define FALSE 0

/****************************************************************
 * Definition of an ARC image header.                           *
 ****************************************************************/
/*
 *
 */
#define int16 unsigned short
#define int32 unsigned long
#define int64 unsigned long long

#define IMAGE_HEADER_SIZE 0x200
#define FMAGIC 0x0184

typedef struct {
    int16 f_magic;
    int16 f_nscns;
    int32 f_timdat;
    int32 f_symptr;
    int32 f_nsyms;
    int16 f_opthdr;
    int16 f_flags;
} FHDR;

#define AMAGIC 0x0107

typedef struct {
    int16 a_magic;
    int32 a_vstamp;
    int32 a_tsize;
    int32 a_dsize;
    int32 a_bsize;
    int32 a_entry;
    int32 a_tbase;
    int32 a_dbase;
    int32 a_bbase;
    int32 a_gmask;
    int32 a_fmask;
    int32 a_gpvalue;
} AHDR;


#define _error(string1)\
     {	\
	fprintf(stderr, "ERROR: %s\n", string1);	\
	exit(1);    \
     }

#define _SEEN(o) seen[o-'a']
#define _OPTION(o) (_SEEN(o) == TRUE)


/*
 *  Global data
 */
#define SEEN_SIZE 100
char seen[SEEN_SIZE];			/* upper and lower case */

/*
 *  Forward routine descriptions.
 */
extern int main(int argc, char **argv);
extern void usage();
extern void build_arc_header(FILE *ofile, int64 base, int64 entry, int size );
extern int put_value(FILE *ofile, int64 value, int width);
extern int put_something(FILE *ofile, void *what, int size);
extern void put_padding(FILE *ofile, unsigned long padding);
extern void output_image(FILE *ifile, FILE *ofile, int size);
extern int64 fsize(FILE *fp);


/*
 *  External routines.
 */

int main(int argc, char **argv)
{
    char *image = NULL;
    char *out = NULL;

    FILE *ifile;
    FILE *ofile;

    int64 entry = 0;
    int64 base = 0;

    int size;

    char *arg, option;
    int i;
    unsigned long int padding, offset, first;
/*
 * Initialise
 */

    for (i = 0; i < SEEN_SIZE; i++)
	seen[i] = FALSE;

/*
 * Parse arguments, but Skip argv[0].
 */
    for (i = 1; i < argc; i++) {
	arg = argv[i];
	if (*arg == '-') {
/*
 * This is a -xyz style options list.  Work out the options specified.
 */
	    arg++;			/* skip the '-' */
	    while (option = *arg++) {	/* until we reach the '0' string
					 * terminator */
		option = tolower(option);
		switch (option) {
		    case 'h': 
			usage();
			exit(1);
		    case 'v': 		/* verbose, allow upper and lower case
					 */
			_SEEN(option) = TRUE;
			break;
		    case 'e': 		/* entry point given. */
			_SEEN(option) = TRUE;
			sscanf(arg, "%X", &entry);
			arg = arg + strlen(arg);
			break;
		    case 'b': 		/* text base given. */
			_SEEN(option) = TRUE;
			sscanf(arg, "%X", &base);
			arg = arg + strlen(arg);
			break;
		    default: 
			usage();
			exit(0);
			break;
		}
	    }
	} else {
/*
 *  Filename.
 */
	    if (image == NULL)
		image = arg;
	    else {
		if (out == NULL) 
		    out = arg;
		else
		    _error("too many filenames given");
	    }
	}
    }
/*
 *  Announce ourselves.
 */
    fprintf(stderr, "arc, ARC image builder [V1.0]\n");
/*
 *  Check the arguments passed.
 */
    /* check for an image file */
    if (image == NULL)
	_error("no filenames given");
    /* check for an entry point */
    if ((entry == 0) && (base == 0))
	_error("no image base or entrypoint given\n");

/*
 *  If there is no entrypoint specified, use the base and vice versa.
 */
    if (entry == 0) entry = base;
    if (base == 0) base = entry;
/*
 *  Tell the world what the arguments are.
 */
    if _OPTION('v') {
	fprintf(stderr, "\timage file is %s\n", image);
	fprintf(stderr, "\toutput file is %s\n", out == NULL ? "stdout" : out);
	fprintf(stderr, "\tentry point at 0x%p\n", entry);
	fprintf(stderr, "\ttext base at 0x%p\n", base);
    }

/*
 *  Open the files.
 */
    ifile = fopen(image, "rb");
    if (ifile == NULL) 
	_error("failed to open input file");

    if (out == NULL)
	ofile = stdout;
    else {
	ofile = fopen(out, "wb");
	if (ofile == NULL) {
	    fclose(ifile);
	    _error("failed to open output file");
	}
    }

/*
 *  How big is the input file?
 */
    size = fsize(ifile);

/*
 *  Output the ARC image header.
 */
    build_arc_header(ofile, base, entry, size);

/*
 *  Now output the image itself.
 */
    output_image(ifile, ofile, size);

/*
 *  Close up shop and exit.
 */
    fclose(ifile);
    if (out != NULL) fclose(ofile);

    return EXIT_SUCCESS;

}					/* end of main() */

/****************************************************************
 * Build and output the ARC image header                        *
 ****************************************************************/
void build_arc_header(FILE *ofile, int64 base, int64 entry, int size)
{
    FHDR fh;
    AHDR ah;
    int count;

    count = 0;

/* 
 * --------------------------------------------------------------
 * The Alpha NT image looks like the following:
 *
 * File Header     (20 bytes)
 * Optional header (size given in the file header, but typically 
 *                  this is 56 bytes)
 * N Section headers, where N is given in the file header 
 *                 (40 bytes each).
 *
 * --------------------------------------------------------------
 *
 * Build the file header and output it.  
 *
 * --------------------------------------------------------------
 * Size  Usage
 * --------------------------------------------------------------
 * 16    Machine, 0x0184 = Alpha
 * 16    Number of image sections
 * 32    Time and date stamp
 * 32    Symbol table pointer
 * 32    Number of symbols
 * 16    Size of optional header (56 bytes)
 * 16    Characteristics
 *       Bit   Meaning
 *       0     No relocation information
 *       1     File is an executable
 *       2     No line number information
 *       3     No local symbols
 *       6     16 bit word machine
 *       7     bytes of machine word are reversed
 *       8     32 bit word machine
 *       9     Debugging info stripped
 *       14    file is a dll
 *       15    Bytes of machine word are reversed.
 *
 */

    count += put_value(ofile, 0x0184,     2);            /* alpha           */
    count += put_value(ofile, 1,          2);            /* nscns           */
    count += put_value(ofile, 0x67d12b7b, 4);            /* timdat          */
    count += put_value(ofile, 0         , 4);            /* symptr          */
    count += put_value(ofile, 0         , 4);            /* nsyms           */
    count += put_value(ofile, 0x38      , 2);            /* opthdr size     */
    count += put_value(ofile, 0x918f    , 2);            /* characteristics:
							    no relocation information
							    executable file
							    no line information
							    no local symbolic information
							    Bytes of machine word reversed
							    32 bit word machine (huh!)
							    System file
							  */
    
/* 
 * Build the optional header and output it.   It is 56 bytes long.
 *
 * --------------------------------------------------------------
 * Size  Usage
 * --------------------------------------------------------------
 * 16    Magic (0x0107) 
 *  8    Major linker version
 *  8    Minor linker version
 * 32    Tsize (text)
 * 32    Dsize (data)
 * 32    Bsize (bss)
 * 32    entry point
 * 32    Tbase
 * 32    Dbase
 * 32    Image base (I don't understand this one).
 * 32    section alignment
 * 32    file alignment
 * 32    OS major/minor
 * 32    Image major/minor
 * 32    Subsystem major/minor
 * 32    Reserved/spacing
 *
 * --------------------------------------------------------------

 */
    count += put_value(ofile, AMAGIC,       2);            /* magic      */
    count += put_value(ofile, 0x2b02,       2);            /* vstamp     */
    count += put_value(ofile, size  ,       4);            /* tsize      */
    count += put_value(ofile, 0     ,       4);            /* dsize      */
    count += put_value(ofile, 0     ,       4);            /* bsize      */
    count += put_value(ofile, entry ,       4);            /* entry      */
    count += put_value(ofile, base  ,       4);            /* tbase      */
    count += put_value(ofile, 0     ,       4);            /* dbase      */
    count += put_value(ofile, base + size,  4);            /* ibase      */
    count += put_value(ofile, 0     ,       4);            /* align      */
    count += put_value(ofile, 0     ,       4);            /* align      */
    count += put_value(ofile, 0     ,       4);            /* OS         */
    count += put_value(ofile, 0     ,       4);            /* Image      */
    count += put_value(ofile, 0     ,       4);            /* Subsys     */
    count += put_value(ofile, 0     ,       4);            /* reserved   */

/*
 *  Now build the image section header(s) and output them/it.
 *
 * --------------------------------------------------------------
 * Size  Usage
 * --------------------------------------------------------------
 *
 * 64    ascii character string containing the name of the section
 * 32    fill 
 * 32    Physical address or virtual size 
 * 32    Raw data for the section
 * 32    Pointer to the relocation information
 * 32    Pointer to the line information
 * 16    Number of relocation entries
 * 16    Number of line entries
 * 32    Characteristics:
 *                0000 0040 : contains initialised data
 *                0000 0020 : contains code (text)
 *                2000 0000 : executable
 *                4000 0000 : Read access
 *                8000 0000 : Write access
 */

    count += put_something
	              (ofile, ".text   ",     8);        /* name           */
    count += put_value(ofile, 0,              4);        /* fill           */
    count += put_value(ofile, base,           4);        /* PAddr          */
    count += put_value(ofile, size,           4);        /* Size           */
    count += put_value(ofile, 0x200,          4);        /* Raw data       */
    count += put_value(ofile, 0,              4);        /* Reloc          */
    count += put_value(ofile, 0,              4);        /* Lines          */
    count += put_value(ofile, 0,              2);        /* # Reloc        */
    count += put_value(ofile, 0,              2);        /* # lines        */
    count += put_value(ofile, 0xE0000020,     4);        /* charactisitics */

/*
 *  We might have to pad to the end of the image.
 */
    if (count < IMAGE_HEADER_SIZE) 
	put_padding(ofile, IMAGE_HEADER_SIZE - count);
}
/****************************************************************
 * Put out some part of the image                               *
 ****************************************************************/
int put_value(FILE *ofile, int64 value, int width)
{
    int size;
    int i;
    char c;

    size = width;
    if (size == 4) {
	put_value(ofile, value & 0xffff, 2);
	put_value(ofile, (value >> 16) & 0xffff, 2);
    } else {
	while (width--) {
	    c = value & 0xff;
	    value = value >> 8;
	    fputc(c, ofile);
	}
    }
    
    return size;
}
/****************************************************************
 * Put out some part of the image                               *
 ****************************************************************/
int put_something(FILE *ofile, void *what, int size)
{
    char *ptr;
    int count;

    count = size;
    ptr = (char *)what;
    while (size--)
	fputc(*ptr++, ofile);
    return count;
}
/****************************************************************
 * Put out some padding                                         *
 ****************************************************************/
void put_padding(FILE *ofile, unsigned long padding)
{
    while (padding--)
	fputc(0x00, ofile);
}

/****************************************************************
 * Output the image to the output file                          *
 ****************************************************************/
void output_image(FILE *ifile, FILE *ofile, int size) 
{
    int c;

    while (size--) {
	c = getc(ifile);
	fputc(c, ofile);
    }
}

/****************************************************************
 * How do you use this thing?                                   *
 ****************************************************************/
void usage()
{
    fprintf(stderr, "arc - builds WNT ARC images\n\n");
    fprintf(stderr, "> arc [-v] -bNNNNNNNN [-eNNNNNNNN] <input-file> [<output-file>]\n");
    fprintf(stderr, "\nwhere:\n");
    fprintf(stderr,
      "\t-eNNNNNNNN: the entrypoint of an image\n");
    fprintf(stderr,
      "\t-bNNNNNNNN: the base of an image\n");
    fprintf(stderr, "\nFlags: -hv\n");
    fprintf(stderr, "\t-h: print this help text\n");
    fprintf(stderr, "\t-v: verbose\n");
    fprintf(stderr, "Example\n\n");
    fprintf(stderr, "\tarc -v -e200000 miniboot miniboot.arc\n\n");
}

int64 fsize(FILE *fp)
{
    int c;
    int64 size;

    size = 0;
    fseek(fp, 0, 0);
    while ((c = getc(fp)) != EOF)
	 size++;
    fseek(fp, 0, 0);
    return size;
}
