extern unsigned long milo_global_flags;

/* To avoid potential collision with flags from the MILO reboot block,
 * we'll start these from 16.  Of course, these flags are completely
 * different and always kept separately, but... just in case. */

#define SERIAL_OUTPUT_FLAG	16UL
#define VERBOSE_FLAG		32UL
