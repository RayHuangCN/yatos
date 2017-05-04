#include "string.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define	OUTBUFSIZE	4096
#define	INBUFSIZE	4096


static char g_pcOutBuf[OUTBUFSIZE];
static char g_pcInBuf[INBUFSIZE];


int printf(const char *__restrict fmt, ...)
{
	int i;
	int len;
	va_list args;

	va_start(args, fmt);
	len = vsprintf(g_pcOutBuf,fmt,args);
	va_end(args);
	write(STDOUT_FILENO, g_pcOutBuf, strlen(g_pcOutBuf));
	return len;
}



int __isoc99_scanf(const char * __restrict fmt, ...)
{
	int i = 0;
	unsigned char c;
	va_list args;
	read(STDIN_FILENO, g_pcInBuf, INBUFSIZE);
	va_start(args,fmt);
	i = vsscanf(g_pcInBuf,fmt,args);
	va_end(args);
	return i;
}







