/*
**
** FUNCTIONAL DESCRIPTION:
** 
**      Given *any* input file, output it as a static asm character array
**      definition.
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
#define int64 unsigned long

extern int main(int argc, char **argv);
extern void usage();
extern void output_image(FILE *ifile, FILE *ofile, int size);
extern int64 fsize(FILE *fp);


/*
 *  External routines.
 */

int main(int argc, char **argv)
{
    char *image = NULL;
    char *out = NULL;
    char *label = "flash_image";
    unsigned int alignment = 3;

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
		    case 'v': 		/* verbose */
		    case 'n':		/* don't include size */
			_SEEN(option) = TRUE;
			break;

		    case 'a':		/* alignment */
			if (*arg)
			    alignment = atoi (arg);
			else if (++i < argc)
			    alignment = atoi (argv[i]);
			else
			    _error ("option `-a' requires an argument");
			if (alignment > 32)
			    _error ("invalid alignment");
			goto next_arg;

		    case 'l':		/* label */
			if (*arg)
			    label = arg;
			else if (++i < argc)
			    label = argv[i];
			else
			    _error ("option `-l' requires an argument");
			goto next_arg;


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
next_arg:
    }
/*
 *  Announce ourselves.
 */
    fprintf(stderr, "data [V2.0]\n");
/*
 *  Check the arguments passed.
 */
    /* check for an image file */
    if (image == NULL)
	_error("no image file given");

/*
 *  Tell the world what the arguments are.
 */
    if _OPTION('v') {
	fprintf(stderr, "\timage file is %s\n", image);
	fprintf(stderr, "\toutput file is %s\n", out == NULL ? "stdout" : out);
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

    size = fsize(ifile);
    fprintf(ofile, "\t.data\n");
    fprintf(ofile, "\t.align %d\n", alignment);
    if (!_OPTION('n')) {
	fprintf(ofile, "\t.globl %s_size\n", label);
	fprintf(ofile, "%s_size:\n", label);
	fprintf(ofile, "\t.long %d\n", size);
    }
    fprintf(ofile, "\t.globl %s\n", label);
    fprintf(ofile, "%s:\n", label);
    output_image(ifile, ofile, size);
    fprintf(ofile, "\n");
/*
 *  Close up shop and exit.
 */
    fclose(ifile);
    if (out != NULL) fclose(ofile);

    return EXIT_SUCCESS;

}					/* end of main() */

/****************************************************************
 * Output the image to the output file                          *
 ****************************************************************/
void output_image(FILE *ifile, FILE *ofile, int size) 
{
    int n, byte;
    unsigned char ch;
    unsigned long long q;

    /*
     * This assumes little-endian byte-order.
     */
    n = 0; q = 0;
    while (size--) {
	byte = n % 8;
	ch = getc(ifile);
	q |= (unsigned long long) ch << (8 * byte);
	if (byte == 7) {
#ifdef GNU_LIBC
	    if (n % 32 == 7) {
		fprintf(ofile, "\n\t.quad 0x%016llx", q);
	    } else {
		fprintf(ofile, ",0x%016llx", q);
	    }
#else
	    if (n % 32 == 7) {
		fprintf(ofile, "\n\t.quad 0x%016lx", q);
	    } else {
		fprintf(ofile, ",0x%016lx", q);
	    }
#endif
	    q = 0;
	}
	++n;
    }
    if (n % 8 != 0) {
#ifdef GNU_LIBC
	if (n % 32 == 7) {
	    fprintf(ofile, "\n\t.quad 0x%016llx", q);
	} else {
	    fprintf(ofile, ",0x%016llx", q);
	}
#else
	if (n % 32 == 7) {
	    fprintf(ofile, "\n\t.quad 0x%016lx", q);
	} else {
	    fprintf(ofile, ",0x%016lx", q);
	}
#endif
    }
}

/****************************************************************
 * How do you use this thing?                                   *
 ****************************************************************/
void usage()
{
    fprintf(stderr, "data\n\n");
    fprintf(stderr, "> data [-hvn] [-a align] [-l label] input-file output-file\n");
    fprintf(stderr, "\nwhere:\n");
    fprintf(stderr, "\nFlags: -hvn\n");
    fprintf(stderr, "\t-h: print this help text\n");
    fprintf(stderr, "\t-v: verbose\n");
    fprintf(stderr, "\t-n: don't include size information\n");
    fprintf(stderr, "Example\n\n");
    fprintf(stderr, "\tdata -v miniloader flash.h\n\n");
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
