#ifndef PTY_H_
#define PTY_H_

#include <termios.h>

struct winsize
{
  unsigned short int ws_row;	/* Rows, in characters.  */
  unsigned short int ws_col;	/* Columns, in characters.  */

  /* These are not actually used.  */
  unsigned short int ws_xpixel;	/* Horizontal pixels.  */
  unsigned short int ws_ypixel;	/* Vertical pixels.  */
};

 /* Create pseudo tty master slave pair with NAME and set terminal
   attributes according to TERMP and WINP and return handles for both
   ends in AMASTER and ASLAVE.  */
int openpty (int *__amaster, int *__aslave, char *__name,
		    const struct termios *__termp,
		    const struct winsize *__winp);

#endif /* _PTY_H_ */
