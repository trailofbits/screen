
int global;

int f() {
	int x = 2;
	global = 5;
	int *p;
	p = &x;

	x += global;
	p+=10;
	p-=10;
	(*p)++;
	return x+1;
}
