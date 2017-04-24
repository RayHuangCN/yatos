
#ifndef _PRINTF_H
#define _PRINTF_H
#include <printk/string.h>
#include <printk/stdio.h>
extern int printk(const char *fmt, ...);
extern int scank(const char * fmt, ...);
extern void putc(char c);

#endif /* _PRINTF_H */
