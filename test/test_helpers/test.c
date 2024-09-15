#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void testSetGreen()
{
	if (isatty(STDOUT_FILENO)) {
		fprintf(stdout, "\e[32m");
	}
}
static void testResetStout()
{
	if (isatty(STDOUT_FILENO)) {
		fprintf(stdout, "\e[39m");
	}
}
static void testSetRed()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[31m");
	}
}
static void testResetStderr()
{
	if (isatty(STDERR_FILENO)) {
		fprintf(stderr, "\e[39m");
	}
}

void _testAssertTrue(const char* expression, bool is_true, const char* file,
                     int line)
{
	if (!is_true) {
		testSetRed();
		fprintf(stderr, "%s:%d Assertion \"%s\" [NOT PASSED]\n", file, line,
		        expression);
		testResetStderr();
		exit(1);
	}
	testSetGreen();
	printf("%s:%d Assertion \"%s\" [PASSED]\n", file, line, expression);
	testResetStout();
}

void _testAssertFalse(const char* expression, bool is_true, const char* file,
                      int line)
{
	if (is_true) {
		testSetRed();
		fprintf(stderr, "%s:%d Assertion \"%s\" [NOT PASSED]\n", file, line,
		        expression);
		testResetStderr();
		exit(1);
	}
	testSetGreen();
	printf("%s:%d Assertion \"%s\" [PASSED]\n", file, line, expression);
	testResetStout();
}

void _testAssertInt(const char* expression, bool is_true, int x, int y,
                    const char* file, int line)
{
	if (!is_true) {
		testSetRed();
		fprintf(stderr,
		        "%s:%d Assertion \"%s\" [NOT PASSED]\nGiven values: %d, %d\n",
		        file, line, expression, x, y);
		testResetStderr();
		exit(1);
	}
	testSetGreen();
	printf("%s:%d Assertion \"%s\" [PASSED]\n", file, line, expression);
	testResetStout();
}

void _testAssertPtr(const char* expression, bool is_true, const void* x,
                    const void* y, const char* file, int line)
{
	if (!is_true) {
		testSetRed();
		fprintf(stderr,
		        "%s:%d Assertion \"%s\" [NOT PASSED]\nGiven pointers: %p, %p\n",
		        file, line, expression, x, y);
		testResetStderr();
		exit(1);
	}
	testSetGreen();
	printf("%s:%d Assert: \"%s\" [PASSED]\n", file, line, expression);
	testResetStout();
}
