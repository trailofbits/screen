
extern int g(int x); 

int f() {
	int x = 2;
	int y = 50;

	if (g(x)) {
		x++;
	} else {
		y+=5;
	}
	return y-x;
}
