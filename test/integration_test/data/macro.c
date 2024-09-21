#include <stdio.h>

#define MACRO(a, b) ((a) + (b) + (c)) + "(c))"
#define MACRO2(a, b) ((a) + (b))
int a;

#define jjj 6
#define macro(a) a + jjj
#define ASDF() 10##2
#define ASDF2(a, b) 1 + a##20 + b
#define ASDF3 #asdf
#define ASDF4
#define RECURSIVE(a) a + macro(100)
#define t(a, b) a##b

//  asdf  /
//  asdf /

int main()
{
	int rec = RECURSIVE(10);
	int line = __LINE__;
	int asdf = ASDF();
	int asdf2 = ASDF2(ASDF() + 2, 1);
	int a = t(10 + jj, j + 8);
	int v = MACRO2((10, 8), 2);
	printf("%d\n", v);
	return 0;
}
