struct A {
	int a;
	int b;
};

double asdf = 10;

int function();

int main()
{
	int a = 10;
	int b = 100;
	a = 20;
	{
		int c = 1000;
		b = 10;
		c = a * b + 100;
		c += 1;
		b++;
	}
	int c;
	if (a > 100) {
		b = 1000;
	}

	while (c == 100) {
		b--;
		a++;
	}
	for (int i = 100; i < 1000; i++) {
		a++;
		++b;
	}
	return a * b;
}
