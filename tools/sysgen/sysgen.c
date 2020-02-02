/*
**
** FUNCTIONAL DESCRIPTION:
** 
**	Create a system image from a HWRPB, the PALcode and
**	the system image (Debug Monitor, O/S or Firmware).
**
** CALLING ENVIRONMENT: 
**
**	user mode
** 
** AUTHOR: David A Rusling
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

#define ROMMAX	0x100000
#define PALMAX	0x020000
#define RPBMAX	0x020000
#define PGSIZE	0x002000

#define int16 unsigned short
#define int32 unsigned long
#define int64 unsigned long long

typedef struct {
    int16 f_magic;
    int16 f_nscns;
    int32 f_timdat;
    int64 f_symptr;
    int32 f_nsyms;
    int16 f_opthdr;
    int16 f_flags;
} FHDR;

#define NFHDR	(2+2+4+8+4+2+2)
#define FMAGIC	0603

typedef struct {
    int16 a_magic;
    int16 a_vstamp;
    int32 a_pad0;
    int64 a_tsize;
    int64 a_dsize;
    int64 a_bsize;
    int64 a_entry;
    int64 a_tbase;
    int64 a_dbase;
    int64 a_bbase;
    int32 a_gmask;
    int32 a_fmask;
    int64 a_gpvalue;
} AHDR;

#define NAHDR	(2+2+4+8+8+8+8+8+8+8+4+4+8)
#define AMAGIC	0407
#define ASTAMP	23

typedef struct {
    char s_name[8];
    int64 s_paddr;
    int64 s_vaddr;
    int64 s_size;
    int64 s_scnptr;
    int64 s_relptr;
    int64 s_lnnoptr;
    int16 s_nreloc;
    int16 s_nlnno;
    int32 s_flags;
} SHDR;

#define NSHDR	(8+8+8+8+8+8+8+2+2+4)
#define SROUND	16

#define _error(string1,string2)\
     {	\
	fprintf(stderr, "%s %s\n", string1, string2);	\
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
 *  Describe the files that we have as input.
 */
#define MAX_FILES 	10

#define coff		1
#define stripped 	2
#define output		3
#define unknown		4

typedef struct fileinfo {
    char *file;
    int type;
    FILE *fp;
    int64 entry;			/* entry point for this file */
    int64 newentry;			/* entry point specified by user */
    int64 tbase;
    int64 dbase;
    int64 dsize;
    int64 bsize;
    int64 tsize;
    int64 size;				/* total bytes read from this file */
    int valid : 1;
    int entrypoint_given : 1;
} fileinfo_t;
fileinfo_t files[MAX_FILES];

unsigned long count = 0;
#define _fputc(a,b) {fputc((a),(b)); count++;}

FILE *output_fp;
/*
 *  Forward routine descriptions.
 */
extern int main(int argc, char **argv);
extern void usage();
extern void print_filetype(int type);
extern FILE *parse_filehead(int index);
extern void output_file(int index);
extern void put_padding(unsigned long padding);
extern int64 fsize(FILE *fp);
/*
 *  External routines.
 */

int main(int argc, char **argv)
{
    char *arg, option;
    int i, findex = 0;
    long int entry = 0;
    unsigned long int padding, offset, first;
/*
 * Initialise
 */

    for (i = 0; i < SEEN_SIZE; i++)
	seen[i] = FALSE;

    for (i = 0; i < MAX_FILES; i++)
	 files[i].valid = FALSE;

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
		    case 'c': 
			_SEEN(option) = TRUE;
			files[findex].type = coff;
			break;
		    case 's': 
			_SEEN(option) = TRUE;
			files[findex].type = stripped;
			break;
		    case 'o': 
			_SEEN(option) = TRUE;
			files[findex].type = output;
			break;
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
			files[findex].newentry = entry;
			files[findex].entrypoint_given = TRUE;
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
 *  Filename, keep these in an array and make sure that we do not see too
 *  many of them.
 */
	    if (findex < MAX_FILES) {
		files[findex].valid = TRUE;
		files[findex++].file = arg;
	    } else
		_error("ERROR:", "too many filenames given");
	}
    }
/*
 *  Announce ourselves.
 */
    fprintf(stderr, "sysgen, system builder [V3.1]\n");
/*
 *  Check for no files supplied.
 */
    if (findex == 0)
	_error("ERROR: no filenames given\n", "");
/*
 *  For each file, open it and work out where everything is.
 *  The side effect of this is to open the files for input.
 */
    for (i = 0; i < MAX_FILES; i++) {
	if ((files[i].type != output) && (files[i].valid))
	    files[i].fp = parse_filehead(i);
    }
/*
 *  Now, if one has been specified, open the output file.
 */
    output_fp = stdout;
    for (i = 0; i < MAX_FILES; i++) {
	if ((files[i].type == output) && (files[i].valid)) {
	    if ((output_fp = 
	      fopen(files[i].file, "wb")) == NULL) {
		_error("ERROR: failed to open output file ", 
		  files[i].file);
	    }
	}
    }
/*
 *  If the user asked for details, then give him/her details.
 */
    if (_OPTION('v')) {
	fprintf(stderr, "Files are:\n");
	for (i = 0; i < MAX_FILES; i++) {
	    if (files[i].valid) {
		fprintf(stderr, "\t%s: (", files[i].file);
		print_filetype(files[i].type);
		fprintf(stderr, "), entry = 0x%08X,", files[i].entry);
		fprintf(stderr, "%d text, %d data\n", 
		  files[i].tsize, 
		  files[i].dsize);
	    }
	}
	fprintf(stderr, "\n");
    }
/*
 *  Now concatenate each object file together to form the
 *  final image.  Between each file there is padding.  
 *  Note that we treat the first file differently just in
 *  case it's not based at zero.
 */
    padding = 0;
    offset = 0;
    first = TRUE;

    for (i = 0; i < MAX_FILES; i++) {
	if ((files[i].type != output) && (files[i].valid)) {
/*
 *  We have a valid file (image).  If it is not the first image
 *  work out how much  padding to put out before it.  We assume that 
 *  the entrypoint is at the base of the image and that if
 *  no entrypoint has been supplied, then we just concatenate
 *  the files.
 */
	    if (first) {
		offset = (unsigned int)files[i].entry;
		padding = 0;
		first = FALSE;
	    } else {
		if (files[i].entrypoint_given) {
		    if ((unsigned int)files[i].entry < offset) {
			fprintf(stderr, "entry = 0x%X, offset = 0x%X\n", 
			  files[i].entry, offset);
			_error("ERROR: image overlap", files[i].file);
		    }
		    padding = (unsigned int)files[i].entry - offset;
		} else {
		    padding = files[i].entry - offset;
		}
	    }
/*
 *  Tell the world what we're doing.
 */
	    fprintf(stderr, "%08x %08x %08x\t%s pad, base/entry, size\n", 
	      padding, files[i].entry, 
	       files[i].size, files[i].file);

	    put_padding(padding);
	    output_file(i);
/*
 *  Now move offset on.  The new offset is comprised of the
 *  old offset, the file size and the padding.  (curewitz)
 */
	    offset += files[i].size + padding;
	}
    }
/*
 *  exit normally.
 */
    fclose(output_fp);
   
    return EXIT_SUCCESS;

}					/* end of main() */

/****************************************************************
 * Put out some padding                                         *
 ****************************************************************/
void put_padding(unsigned long padding)
{
    while (padding--)
	_fputc(0x00, output_fp);
}
/****************************************************************
 * Parse a file header (of whatever type)                       *
 ****************************************************************/
/*
 * Parse the header of the file given (via the files array).
 * and put the details in the files array.
 */
FILE *parse_filehead(int index)
{
    FILE *fp = NULL;

/*
 *  Initialise the information in the files vector.
 */

    files[index].tbase = 0;
    files[index].tsize = 0;
    files[index].dbase = 0;
    files[index].dsize = 0;
    files[index].bsize = 0;
    files[index].size = 0;
    files[index].entry = 0;

    switch (files[index].type) {
/*
 *  File that has had any headers stripped and now contains just
 *  the image itself.
 */
	case stripped: 
	    {
		if ((fp = fopen(files[index].file, "rb")) == NULL)
		    _error("ERROR: failed to open file ", 
		      files[index].file);
		if (files[index].entrypoint_given) {
		    files[index].entry = files[index].newentry;
		} else
		    _error("ERROR:",
		      "no entrypoint given for a stripped file");
		files[index].size = fsize(fp);
		break;
	    }
	case coff: 
	    {
		FHDR *fhdr;
		AHDR *ahdr;

		fhdr = (FHDR *) malloc(sizeof(FHDR));
		ahdr = (AHDR *) malloc(sizeof(AHDR));
		if ((fhdr == NULL) || (ahdr == NULL))
		    _error("ERROR:", "failed to allocate memory");
		fp = fopen(files[index].file, "rb");
		if (fp == (FILE *) NULL)
		    _error("ERROR: failed to open file", 
		      files[index].file);
		if (fread((char *) fhdr, NFHDR, 1, fp) != 1 || 
		  fhdr->f_magic != FMAGIC)
		    _error("ERROR: bad file header", 
		      files[index].file);
		if (fread((char *) ahdr, NAHDR, 1, fp) != 1 || 
		  ahdr->a_magic != AMAGIC)
		    _error("ERROR: bad a.out header", 
		      files[index].file);
		if (ahdr->a_vstamp < ASTAMP)
		    _error("ERROR: ancient a.out version", 
		      files[index].file);
		files[index].tbase =  
		  ((NFHDR + NAHDR + fhdr->f_nscns * NSHDR
		  + (SROUND - 1)) & ~(SROUND - 1));
		files[index].tsize = ahdr->a_tsize;
		files[index].dbase = files[index].tbase +
		  files[index].tsize;
		files[index].dsize = ahdr->a_dsize;
		files[index].bsize = ahdr->a_bsize;
		if (files[index].entrypoint_given) {
		    files[index].entry = files[index].newentry;
		} else {
		    files[index].entry = ahdr->a_entry;
		}
		files[index].size = 
		  files[index].tsize + files[index].dsize;
		break;
	    }
	default: 
	    _error("ERROR: unknown file type", files[index].file);
    }
    return fp;
}

/****************************************************************
 * Output a file (of whatever type)                             *
 ****************************************************************/
void output_file(int index)
{
    int i;
    int c;
    int status;

    if (files[index].type != stripped) {
	status = fseek(files[index].fp, files[index].tbase, 0);
	if (status != 0)
	    _error("ERROR: fseek() failure on file ", 
	      files[index].file);
	for (i = 0; i < files[index].tsize; ++i) {
	    if ((c = getc(files[index].fp)) == EOF)
		abort();
	    _fputc(c, output_fp);
	}

	/* Data segment */
	status = fseek(files[index].fp, files[index].dbase, 0);
	if (status != 0)
	    _error("ERROR: fseek() failure on file ", 
	      files[index].file);
	for (i = 0; i < files[index].dsize; ++i) {
	    if ((c = getc(files[index].fp)) == EOF)
		abort();
	    _fputc(c, output_fp);
	}

    } else {
	while ((c = getc(files[index].fp)) != EOF)
	    _fputc(c, output_fp);
    }

    fclose(files[index].fp);
}
/****************************************************************
 * What sort of file is it?                                     *
 ****************************************************************/
void print_filetype(int type)
{
    switch (type) {
	case coff: 
	    fprintf(stderr, "coff");
	    break;
	case stripped: 
	    fprintf(stderr, "stripped");
	    break;
	case unknown: 
	    fprintf(stderr, "unknown");
	    break;
	case output: 
	    fprintf(stderr, "output");
	    break;
	default: 
	    fprintf(stderr, "programming error! (%d)", type);
    }
}

/****************************************************************
 * How do you use this thing?                                   *
 ****************************************************************/
void usage()
{
    fprintf(stderr, "sysgen - generates system images\n\n");
    fprintf(stderr, "> sysgen [flags] [[fileflags] file] ...");
    fprintf(stderr, " [[fileflags] file]\n\n");
    fprintf(stderr, "\tWhere [fileflags] is one of -o, -e, -a, -c or -s:\n");
    fprintf(stderr, "\t\t-c: OSF/1 coff object file\n");
    fprintf(stderr, "\t\t-s: stripped file (no header)\n");
    fprintf(stderr,
      "\t\t-eNNN: override/supply the entrypoint/base of an image\n");
    fprintf(stderr, "\t\t-o: file is the output file\n");
    fprintf(stderr, "\tThe files default to type -a\n\n");
    fprintf(stderr, "\tFlags: -hv\n");
    fprintf(stderr, "\t\t-h: print this help text\n");
    fprintf(stderr, "\t\t-v: verbose\n");
    fprintf(stderr, "Example\n\n");
    fprintf(stderr, "\tsysgen -v -a dbmpal -a -e40000 dbmhwrpb ");
    fprintf(stderr, "-s eb66_rom.nh -o eb66_rom.img\n\n");
    fprintf(stderr, "NOTE: the filenames may appear in any order.\n");
    fprintf(stderr,
      "      Stripped files must have an entrypoint/base specified\n\n");
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
