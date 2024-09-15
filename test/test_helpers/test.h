#ifndef TEST_H
#define TEST_H

#include <stdbool.h>

#define EXPECT_EQ_INT(x, y) _TEST_ASSERT_INT((x) == (y), x, y)
#define EXPECT_NE_INT(x, y) _TEST_ASSERT_INT((x) != (y), x, y)
#define EXPECT_LT_INT(x, y) _TEST_ASSERT_INT((x) < (y), x, y)
#define EXPECT_LE_INT(x, y) _TEST_ASSERT_INT((x) <= (y), x, y)
#define EXPECT_GT_INT(x, y) _TEST_ASSERT_INT((x) > (y), x, y)
#define EXPECT_GE_INT(x, y) _TEST_ASSERT_INT((x) >= (y), x, y)

#define EXPECT_EQ_PTR(x, y) _TEST_ASSERT_PTR((x) == (y), x, y)
#define EXPECT_NE_PTR(x, y) _TEST_ASSERT_PTR((x) != (y), x, y)
#define EXPECT_LT_PTR(x, y) _TEST_ASSERT_PTR((x) < (y), x, y)
#define EXPECT_LE_PTR(x, y) _TEST_ASSERT_PTR((x) <= (y), x, y)
#define EXPECT_GT_PTR(x, y) _TEST_ASSERT_PTR((x) > (y), x, y)
#define EXPECT_GE_PTR(x, y) _TEST_ASSERT_PTR((x) >= (y), x, y)

#define EXPECT_TRUE(x) _TEST_ASSERT_TRUE(x)
#define EXPECT_FALSE(x) _TEST_ASSERT_FALSE(x)

#define _TEST_ASSERT_INT(expression, x, y) \
	_testAssertInt(#expression, expression, x, y, __FILE__, __LINE__)

#define _TEST_ASSERT_PTR(expression, x, y) \
	_testAssertPtr(#expression, expression, x, y, __FILE__, __LINE__)

#define _TEST_ASSERT_TRUE(condition) \
	_testAssertTrue(#condition, condition, __FILE__, __LINE__)

#define _TEST_ASSERT_FALSE(condition) \
	_testAssertFalse(#condition, condition, __FILE__, __LINE__)

void _testAssertTrue(const char* expression, bool is_true, const char* file,
                     int line);

void _testAssertFalse(const char* expression, bool is_true, const char* file,
                      int line);

void _testAssertInt(const char* expression, bool is_true, int x, int y,
                    const char* file, int line);

void _testAssertPtr(const char* expression, bool is_true, const void* x,
                    const void* y, const char* file, int line);

#endif
