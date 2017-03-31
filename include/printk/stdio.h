
#ifndef _STDIO_H
#define _STDIO_H

#include "types.h"

#ifndef _VALIST
#define _VALIST
typedef char *va_list;
#endif /* _VALIST */
#include <board/uart.h>
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern int snprintf(char * buf, size_t size, const char *fmt, ...);
extern int vsprintf(char *buf, const char *fmt, va_list args);
extern int sprintf(char * buf, const char *fmt, ...);
extern int vsscanf(const char * buf, const char * fmt, va_list args);
extern int sscanf(const char * buf, const char * fmt, ...);

extern void putc(  char c);
extern  char getc(void);

int printk(const char *fmt, ...);
int scank(const char * fmt, ...);

#endif /* _STDIO_H */
