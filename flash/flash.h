/*****************************************************************************

       Copyright © 1995, 1996 Digital Equipment Corporation,
                       Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, provided  
that the copyright notice and this permission notice appear in all copies  
of software and supporting documentation, and that the name of Digital not  
be used in advertising or publicity pertaining to distribution of the software 
without specific, written prior permission. Digital grants this permission 
provided that you prominently mark, as not part of the original, any 
modifications made to this software or documentation.

Digital Equipment Corporation disclaims all warranties and/or guarantees  
with regard to this software, including all implied warranties of fitness for 
a particular purpose and merchantability, and makes no representations 
regarding the use of, or the results of the use of, the software and 
documentation in terms of correctness, accuracy, reliability, currentness or
otherwise; and you rely on the software, documentation and results solely at 
your own risk. 

******************************************************************************/

#ifndef MILO_VIDEO_H
#define MILO_VIDEO_H 1

/* 
 *  Milo video support.
 */

typedef struct flash {
	char *name;

	int (*init) (void);
	void (*program) (void);

	void (*list) (void);
	void (*env) (int blockno);
	void (*delete_block) (int blockno);
	void (*delete_image) (char *name);
} flash_t;


extern flash_t *flashdriver;

/*
 *  Mandatory.
 */
#define __flash_init() (flashdriver->init)()
#define __flash_program() (flashdriver->program)()

/*
 * Optional.
 */
#define __flash_list()  \
{ \
  if (flashdriver->list) \
    (flashdriver->list)() ; \
  else \
    printk("Action not implemented for this system\n") ; \
}

#define __flash_env(blockno) \
{ \
  if (flashdriver->env) \
    (flashdriver->env)(blockno) ; \
  else \
    printk("Action not implemented for this system\n") ; \
}

#define __flash_delete_block(blockno) \
{ \
  if (flashdriver->delete_block) \
    (flashdriver->delete_block)(blockno) ; \
  else \
    printk("Action not implemented for this system\n") ; \
}

#define __flash_delete_image(imageno) \
{ \
  if (flashdriver->delete_image) \
    (flashdriver->delete_image)(imageno) ; \
  else \
    printk("Action not implemented for this system\n") ; \
}

#endif
