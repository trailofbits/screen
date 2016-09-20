
extern int toto(int x);

int f () {
	int x = 0;
	int y = 0;

	while (x < 100) {
		y = toto(y);
		assert(y == 10);
		x++;	
	}

	toto (x);
	return x;
}
