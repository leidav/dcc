/*	Copyright (C) 2021 David Leiter
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

const char* fileName(const char* path);

#endif
