#include <stdio.h>

struct Ab {
	int a;
	int b;
};

// Comment 1
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
int a;
/***
 * Comment 2
 **/
int b;
int function(struct Ab* ab_structure, int c)
{
	return MAX(MIN(ab_structure->a, ab_structure->b), c);
}

int main(int argc, char** argv)
{
	struct Ab asdf = {3, 2};  // Comment 3

	printf("result: %d\n", function(&asdf, 1));
	return 0;
}
