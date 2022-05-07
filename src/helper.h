#ifndef HELPER
#define HELPER

#include <stdbool.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

const static inline bool isDecimalDigit(unsigned int c)
{
	return (c - 0x30) < 10;
}
const static inline bool isHexDigit(unsigned int c)
{
	return isDecimalDigit(c) || (((c & 0xdf) - 0x41) < 6);
}
const static inline bool isOctalDigit(unsigned int c)
{
	return (c - 0x30) < 8;
}

const static inline bool isBinaryDigit(unsigned int c)
{
	return (c - 0x30) <= 1;
}

const static inline bool isAlphabetic(unsigned int c)
{
	unsigned int c2 = (c & 0xdf) - 0x41;
	return (c2 < 26) | (c2 == 30);
}

const static inline bool isAlphaNumeric(unsigned int c)
{
	return isAlphabetic(c) || isDecimalDigit(c);
}

const static inline bool isWhitespace(unsigned int c)
{
	return (c == 0x20) || ((c - 0x8) < 5);
}
#endif
