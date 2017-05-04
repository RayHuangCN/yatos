#include "printk/vsprintf.h"
#include "printk/string.h"
#include "yatos/printk.h"
extern void putc( char c);

#define	OUTBUFSIZE	1024
#define	INBUFSIZE	1024


static char g_pcOutBuf[OUTBUFSIZE];
static char g_pcInBuf[INBUFSIZE];


int printk(const char *fmt, ...)
{
	int i;
	int len;
	va_list args;

	va_start(args, fmt);
	len = vsprintf(g_pcOutBuf,fmt,args);
	va_end(args);
	for (i = 0; i < strlen(g_pcOutBuf); i++)
	{
		putc(g_pcOutBuf[i]);
	}
	return len;
}
