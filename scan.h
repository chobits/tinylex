#ifndef __SCAN_H
#define __SCAN_H

enum tokentype {
	/* book-keeping tokens */
	tkunknown, tkeof, tkerr,
	/* revered words */
	tkif, tkthen, tkelse, tkend,	/* if then else end */
	tkrepeat, tkuntil,		/* repeat until */
	tkread, tkwrite,		/* read write */
	/* special symbol */
	tkeq, tklt,			/* = < */
	tkadd, tksub, tkmul, tkdiv,	/* + - * / */
	tkassign, tksemi,		/* := ; */
	tklparen, tkrparen,		/* ( ) */
	/* multicharacter tokens */
	tknum, tkid
};

#endif	/* scan.h */
