#ifndef __ASM_ARM_DIV64
#define __ASM_ARM_DIV64

//#include <asm/system.h>

/*
 * The semantics of do_div() are:
 *
 * uint32_t do_div(uint64_t *n, uint32_t base)
 * {
 * 	uint32_t remainder = *n % base;
 * 	*n = *n / base;
 * 	return remainder;
 * }
 *
 * In other words, a 64-bit dividend with a 32-bit divisor producing
 * a 64-bit result and a 32-bit remainder.  To accomplish this optimally
 * we call a special __do_div64 helper with completely non standard
 * calling convention for arguments and results (beware).
 */
typedef char bool;

int ilog2(unsigned long v)
{
	int l = 0;
	while ((1UL << l) < v)
		l++;
	return l;
}

bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

#define do_div(n, base)						\
({								\
	unsigned long __upper, __low, __high, __mod, __base;	\
	__base = (base);					\
	if (__builtin_constant_p(__base) && is_power_of_2(__base)) { \
		__mod = n & (__base - 1);			\
		n >>= ilog2(__base);				\
	} else {						\
		asm("" : "=a" (__low), "=d" (__high) : "A" (n));\
		__upper = __high;				\
		if (__high) {					\
			__upper = __high % (__base);		\
			__high = __high / (__base);		\
		}						\
		asm("divl %2" : "=a" (__low), "=d" (__mod)	\
			: "rm" (__base), "0" (__low), "1" (__upper));	\
		asm("" : "=A" (n) : "a" (__low), "d" (__high));	\
	}							\
	__mod;							\
})
#endif
