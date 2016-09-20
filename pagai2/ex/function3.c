
int k(int x) {
	return x+1;
}

int g(int x, int y) {

	while (y >= 0) {
		if (x < 50)
			y++;
		else {
			y--;
		}
		x++;
	}
	return x+y;
}

