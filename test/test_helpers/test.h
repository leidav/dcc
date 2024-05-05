#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define EXPECT_EQ(x, y) _TEST_ASSERT((x) == (y))
#define EXPECT_NE(x, y) _TEST_ASSERT((x) != (y))
#define EXPECT_LT(x, y) _TEST_ASSERT((x) < (y))
#define EXPECT_LE(x, y) _TEST_ASSERT((x) <= (y))
#define EXPECT_GT(x, y) _TEST_ASSERT((x) > (y))
#define EXPECT_GE(x, y) _TEST_ASSERT((x) >= (y))
#define EXPECT_TRUE(x) _TEST_ASSERT(x)
#define EXPECT_FALSE(x) _TEST_ASSERT(!(x))

#define _TEST_ASSERT(x)                      \
	if (!(x)) {                              \
		_testAssert(#x, __FILE__, __LINE__); \
		exit(1);                             \
	}

inline static void _testSetRed()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[31m");
	}
}
inline static void _testResetRed()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[39m");
	}
}

inline static void _testAssert(const char* expression, const char* file,
                               int line)
{
	_testSetRed();
	fprintf(stderr, "%s:%d Assert: \"%s\" expected\n", file, line, expression);
	_testResetRed();
	exit(1);
}

#endif
