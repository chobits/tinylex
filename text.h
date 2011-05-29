#ifndef __TEXT_H
#define __TEXT_H

extern char *text_getpos(void);
extern char text_getchar(void);
extern int text_getline(char **line);
extern char *text_lookahead(int len);

#endif	/* text.h */
