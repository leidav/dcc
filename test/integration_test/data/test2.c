#define min(X, Y) ((X) < (Y) ? (X) : (Y))

int function(int a, int b, int c)
{
	return min(min(a, b), c);
}
