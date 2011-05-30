#ifndef __REG_H
#define __REG_H

#define red(str) "\e[01;31m"#str"\e\[0m"
#define green(str) "\e[01;32m"#str"\e\[0m"
#define yellow(str) "\e[01;33m"#str"\e\[0m"
#define navyblue(str) "\e[01;34m"#str"\e\[0m"
#define purple(str) "\e[01;35m"#str"\e\[0m"
#define cambrigeblue(str) "\e[01;36m"#str"\e\[0m"
#define grey(str) "\e[01;30m"#str"\e\[0m"

#define MAX_INT ((~(0U)) >> 1)

extern void errexit(char *);
extern void fileopen(char *);
extern struct regnode *interpret(void);

#endif
