#ifndef HELPER
#define HELPER

#include <stdbool.h>

const static inline bool isDecimalDigit(char c)
{
	return c >= '0' && c <= '9';
}
const static inline bool isHexDigit(char c)
{
	return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
	       (c >= '0' && c <= '9');
}
const static inline bool isOctalDigit(char c)
{
	return c >= '0' && c <= '7';
}

const static inline bool isBinaryDigit(char c)
{
	return c == '0' || c == '1';
}
const static inline bool isAlphaNumeric(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	       (c >= '0' && c <= '9') || (c == '_');
}

const static inline bool isAlphabetic(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_');
}
#endif